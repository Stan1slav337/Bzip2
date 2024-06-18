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

#ifndef BLOCK_COMPRESSOR_HPP
#define BLOCK_COMPRESSOR_HPP

#include <vector>
#include <array>
#include <cstdint>
#include <algorithm>

#include "Config.hpp"
#include "CRC32.hpp"
#include "BitOutputStream.hpp"
#include "DivSufSort.hpp"
#include "MTFAndRLE2StageEncoder.hpp"
#include "HuffmanStageEncoder.hpp"

class BlockCompressor
{
private:
    BitOutputStream &bitOutputStream;
    CRC32 crc{};
    int blockLength = 0;
    int blockLengthLimit;
    std::array<bool, 256> blockValuesPresent{};
    std::vector<uint8_t> block;
    std::vector<int> bwtBlock;

    int rleCurrentValue = -1;
    int rleLength = 0;

public:
    BlockCompressor(BitOutputStream &outputStream, int blockSize) : bitOutputStream(outputStream),
                                                                    block(blockSize + 1), // plus one to allow for the BWT wraparound
                                                                    bwtBlock(blockSize + 1),
                                                                    blockLengthLimit(blockSize - 6)
    {
    }

    bool isEmpty()
    {
        return blockLength == 0 && rleLength == 0;
    }

    int getCRC() const
    {
        return crc.getCRC();
    }

    bool write(int value)
    {
        if (blockLength > blockLengthLimit)
        {
            return false;
        }

        if (rleLength == 0)
        {
            rleCurrentValue = value;
            rleLength = 1;
        }
        else if (rleCurrentValue == value)
        {
            if (++rleLength > 254)
            {
                writeRun(rleCurrentValue, 255);
                rleLength = 0;
            }
        }
        else
        {
            writeRun(rleCurrentValue, rleLength);
            rleCurrentValue = value;
            rleLength = 1;
        }
        return true;
    }

    int write(const std::vector<char> &data, int offset, int length)
    {
        int written = 0;
        while (length-- > 0)
        {
            if (!write(data[offset++]))
            {
                break;
            }
            ++written;
        }
        return written;
    }

    void close()
    {
        if (rleLength > 0)
        {
            writeRun(rleCurrentValue & 0xff, rleLength);
        }

        block[blockLength] = block[0];

        DivSufSort divSufSort = DivSufSort(block, bwtBlock, blockLength);
        int bwtStartPointer = divSufSort.bwt();

        bitOutputStream.writeBits(24, BLOCK_HEADER_MARKER_1);
        bitOutputStream.writeBits(24, BLOCK_HEADER_MARKER_2);
        bitOutputStream.writeInteger(crc.getCRC());
        bitOutputStream.writeBoolean(false); // Randomised block flag
        bitOutputStream.writeBits(24, bwtStartPointer);

        writeSymbolMap();

        MTFAndRLE2StageEncoder mtfEncoder(bwtBlock, blockLength, blockValuesPresent);
        mtfEncoder.encode();

        HuffmanStageEncoder huffmanEncoder(bitOutputStream, mtfEncoder.getMtfBlock(), mtfEncoder.getMtfLength(), mtfEncoder.getMtfAlphabetSize(), mtfEncoder.getMtfSymbolFrequencies());
        huffmanEncoder.encode();
    }

private:
    void writeSymbolMap()
    {
        std::array<bool, 16> condensedInUse{};
        for (int i = 0; i < 16; ++i)
        {
            for (int j = 0, k = i << 4; j < 16; ++j, ++k)
            {
                if (blockValuesPresent[k])
                {
                    condensedInUse[i] = true;
                }
            }
        }

        for (int i = 0; i < 16; ++i)
        {
            bitOutputStream.writeBoolean(condensedInUse[i]);
        }

        for (int i = 0; i < 16; ++i)
        {
            if (condensedInUse[i])
            {
                for (int j = 0, k = i * 16; j < 16; ++j, ++k)
                {
                    bitOutputStream.writeBoolean(blockValuesPresent[k]);
                }
            }
        }
    }

    void writeRun(int value, int runLength)
    {
        blockValuesPresent[value] = true;
        crc.updateCRC(value, runLength);
        block[blockLength++] = static_cast<uint8_t>(value);
        if (runLength > 1)
        {
            block[blockLength++] = static_cast<uint8_t>(value);
            if (runLength > 2)
            {
                block[blockLength++] = static_cast<uint8_t>(value);
                if (runLength > 3)
                {
                    runLength -= 4;
                    blockValuesPresent[runLength] = true;
                    block[blockLength++] = static_cast<uint8_t>(value);
                    block[blockLength++] = uint8_t(runLength);
                }
            }
        }
    }
};
#endif