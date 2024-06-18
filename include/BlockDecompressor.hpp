/*
 * Copyright (c) 2011 Matthew Francis
 * Copyright (c) 2024 Stanislav Brega
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#ifndef BLOCK_DECOMPRESSOR_HPP
#define BLOCK_DECOMPRESSOR_HPP

#include <vector>
#include <cstdint>
#include <iostream>

#include "Config.hpp"
#include "CRC32.hpp"
#include "BitInputStream.hpp"
#include "MoveToFront.hpp"
#include "HuffmanStageDecoder.hpp"

class BlockDecompressor
{
public:
    BlockDecompressor(BitInputStream &inputStream, int blockSize)
        : bitInputStream(inputStream),
          huffmanSymbolMap(256),
          bwtByteCounts(256),
          bwtBlock(blockSize)
    {
        blockCRC = bitInputStream.readInteger();
        blockRandomised = bitInputStream.readBoolean();
        int bwtStartPointer = bitInputStream.readBits(24);

        HuffmanStageDecoder huffmanDecoder = readHuffmanTables();
        decodeHuffmanData(huffmanDecoder);
        initialiseInverseBWT(bwtStartPointer);
    }

    int read()
    {
        while (rleRepeat < 1)
        {
            if (bwtBytesDecoded == bwtBlockLength)
            {
                return -1; // EOF
            }

            int nextByte = decodeNextBWTByte();
            if (nextByte != rleLastDecodedByte)
            {
                rleLastDecodedByte = nextByte;
                rleRepeat = 1;
                rleAccumulator = 1;
                crc.updateCRC(nextByte);
            }
            else
            {
                if (++rleAccumulator == 4)
                {
                    rleRepeat = decodeNextBWTByte() + 1;
                    rleAccumulator = 0;
                    crc.updateCRC(nextByte, rleRepeat);
                }
                else
                {
                    rleRepeat = 1;
                    crc.updateCRC(nextByte);
                }
            }
        }

        rleRepeat--;
        return rleLastDecodedByte;
    }

    int read(std::vector<uint8_t> &destination, int offset, int length)
    {
        int i;
        for (i = 0; i < length; i++, offset++)
        {
            int decoded = read();
            if (decoded == -1)
            {
                return (i == 0) ? -1 : i;
            }
            destination[offset] = decoded;
        }
        return i;
    }

    int checkCRC()
    {
        if (blockCRC != crc.getCRC())
        {
            throw std::runtime_error("BZip2 block CRC error");
        }

        return crc.getCRC();
    }

private:
    BitInputStream &bitInputStream;
    CRC32 crc{};
    int blockCRC{};
    bool blockRandomised{}; // should be always false
    int huffmanEndOfBlockSymbol{};
    std::vector<uint8_t> huffmanSymbolMap;
    std::vector<int> bwtByteCounts;
    std::vector<uint8_t> bwtBlock;
    std::vector<int> bwtMergedPointers{};
    int bwtCurrentMergedPointer;
    int bwtBlockLength = 0;
    int bwtBytesDecoded = 0;
    int rleLastDecodedByte = -1;
    int rleAccumulator = 0;
    int rleRepeat = 0;

    HuffmanStageDecoder readHuffmanTables()
    {
        std::vector<std::vector<uint8_t>> tableCodeLengths(HUFFMAN_MAXIMUM_TABLES, std::vector<uint8_t>(HUFFMAN_MAXIMUM_ALPHABET_SIZE));

        int huffmanUsedRanges = bitInputStream.readBits(16);
        int huffmanSymbolCount = 0;
        for (int i = 0; i < 16; i++)
        {
            if ((huffmanUsedRanges & ((1 << 15) >> i)) != 0)
            {
                for (int j = 0, k = i << 4; j < 16; j++, k++)
                {
                    if (bitInputStream.readBoolean())
                    {
                        huffmanSymbolMap[huffmanSymbolCount++] = k;
                    }
                }
            }
        }
        int endOfBlockSymbol = huffmanSymbolCount + 1;
        huffmanEndOfBlockSymbol = endOfBlockSymbol;

        int totalTables = bitInputStream.readBits(3);
        int totalSelectors = bitInputStream.readBits(15);
        if (totalTables < HUFFMAN_MINIMUM_TABLES || totalTables > HUFFMAN_MAXIMUM_TABLES ||
            totalSelectors < 1 || totalSelectors > HUFFMAN_MAXIMUM_SELECTORS)
        {
            throw std::runtime_error("block Huffman tables invalid");
        }

        std::vector<uint8_t> selectors(totalSelectors);
        MoveToFront tableMTF;
        for (int i = 0; i < totalSelectors; i++)
        {
            selectors[i] = tableMTF.indexToFront(bitInputStream.readUnary());
        }

        for (int table = 0; table < totalTables; table++)
        {
            int currentLength = bitInputStream.readBits(5);
            for (int j = 0; j <= endOfBlockSymbol; j++)
            {
                while (bitInputStream.readBoolean())
                {
                    currentLength += bitInputStream.readBoolean() ? -1 : 1;
                }
                tableCodeLengths[table][j] = currentLength;
            }
        }

        return HuffmanStageDecoder(bitInputStream, endOfBlockSymbol + 1, tableCodeLengths, selectors);
    }

    void decodeHuffmanData(HuffmanStageDecoder &huffmanDecoder)
    {
        int streamBlockSize = bwtBlock.size();
        MoveToFront symbolMTF;
        int repeatCount = 0;
        int repeatIncrement = 1;
        int mtfValue = 0;

        while (true)
        {
            int nextSymbol = huffmanDecoder.nextSymbol();

            if (nextSymbol == HUFFMAN_SYMBOL_RUNA)
            {
                repeatCount += repeatIncrement;
                repeatIncrement <<= 1;
            }
            else if (nextSymbol == HUFFMAN_SYMBOL_RUNB)
            {
                repeatCount += (repeatIncrement << 1);
                repeatIncrement <<= 1;
            }
            else
            {
                if (repeatCount > 0)
                {
                    if (bwtBlockLength + repeatCount > streamBlockSize)
                    {
                        throw std::runtime_error("BZip2 block exceeds declared block size");
                    }
                    uint8_t nextByte = huffmanSymbolMap[mtfValue];
                    bwtByteCounts[nextByte & 0xff] += repeatCount;
                    while (--repeatCount >= 0)
                    {
                        bwtBlock[bwtBlockLength++] = nextByte;
                    }

                    repeatCount = 0;
                    repeatIncrement = 1;
                }

                if (nextSymbol == huffmanEndOfBlockSymbol)
                    break;

                if (bwtBlockLength >= streamBlockSize)
                {
                    throw std::runtime_error("BZip2 block exceeds declared block size");
                }

                mtfValue = symbolMTF.indexToFront(nextSymbol - 1) & 0xff;
                uint8_t nextByte = huffmanSymbolMap[mtfValue];
                bwtByteCounts[nextByte & 0xff]++;
                bwtBlock[bwtBlockLength++] = nextByte;
            }
        }
    }

    void initialiseInverseBWT(int bwtStartPointer)
    {
        if (bwtStartPointer < 0 || bwtStartPointer >= bwtBlockLength)
        {
            throw std::runtime_error("BZip2 start pointer invalid");
        }

        std::vector<int> characterBase(256, 0);
        std::copy(bwtByteCounts.begin(), bwtByteCounts.begin() + 255, characterBase.begin() + 1);
        for (int i = 2; i <= 255; ++i)
        {
            characterBase[i] += characterBase[i - 1];
        }

        bwtMergedPointers.resize(bwtBlockLength);
        for (size_t i = 0; i < bwtBlockLength; i++)
        {
            int value = bwtBlock[i] & 0xff;
            bwtMergedPointers[characterBase[value]++] = (i << 8) + value;
        }

        bwtBlock.clear();
        bwtCurrentMergedPointer = bwtMergedPointers[bwtStartPointer];
    }

    int decodeNextBWTByte()
    {
        int nextDecodedByte = bwtCurrentMergedPointer & 0xff;
        bwtCurrentMergedPointer = bwtMergedPointers[bwtCurrentMergedPointer >> 8];

        if (blockRandomised)
        {
            throw std::runtime_error("BZip2 randomised blocks not implemented");
        }

        bwtBytesDecoded++;

        return nextDecodedByte;
    }
};

#endif