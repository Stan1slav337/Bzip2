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

#ifndef MTF_AND_RLE2_STAGE_ENCODER_HPP
#define MTF_AND_RLE2_STAGE_ENCODER_HPP

#include <vector>
#include <array>
#include <cstdint>

#include "Config.hpp"
#include "MoveToFront.hpp"

class MTFAndRLE2StageEncoder
{
private:
    std::vector<int> &bwtBlock;
    int bwtLength;
    std::array<bool, 256> &bwtValuesInUse;
    std::vector<uint16_t> mtfBlock;
    std::vector<int> mtfSymbolFrequencies;
    int alphabetSize{};
    int mtfLength{};

public:
    MTFAndRLE2StageEncoder(std::vector<int> &bwtBlock, int bwtLength, std::array<bool, 256> &bwtValuesInUse)
        : bwtBlock(bwtBlock), bwtLength(bwtLength), bwtValuesInUse(bwtValuesInUse), mtfBlock(bwtLength + 1, 0), mtfSymbolFrequencies(HUFFMAN_MAXIMUM_ALPHABET_SIZE, 0) {}

    std::vector<uint16_t> &getMtfBlock()
    {
        return mtfBlock;
    }

    int getMtfLength() const
    {
        return mtfLength;
    }

    int getMtfAlphabetSize() const
    {
        return alphabetSize;
    }

    std::vector<int> &getMtfSymbolFrequencies()
    {
        return mtfSymbolFrequencies;
    }

    void encode()
    {
        std::array<uint8_t, 256> huffmanSymbolMap{};
        MoveToFront symbolMTF;

        int totalUniqueValues = 0;
        for (int i = 0; i < 256; i++)
        {
            if (bwtValuesInUse[i])
            {
                huffmanSymbolMap[i] = totalUniqueValues++;
            }
        }

        int endOfBlockSymbol = totalUniqueValues + 1;
        int mtfIndex = 0;
        int repeatCount = 0;
        int totalRunAs = 0;
        int totalRunBs = 0;

        for (int i = 0; i < bwtLength; i++)
        {
            int mtfPosition = symbolMTF.valueToFront(huffmanSymbolMap[bwtBlock[i] & 0xff]);

            if (mtfPosition == 0)
            {
                repeatCount++;
            }
            else
            {
                if (repeatCount > 0)
                {
                    repeatCount--;
                    while (true)
                    {
                        if ((repeatCount & 1) == 0)
                        {
                            mtfBlock[mtfIndex++] = HUFFMAN_SYMBOL_RUNA;
                            totalRunAs++;
                        }
                        else
                        {
                            mtfBlock[mtfIndex++] = HUFFMAN_SYMBOL_RUNB;
                            totalRunBs++;
                        }

                        if (repeatCount <= 1)
                        {
                            break;
                        }
                        repeatCount = (repeatCount - 2) >> 1;
                    }
                    repeatCount = 0;
                }

                mtfBlock[mtfIndex++] = mtfPosition + 1;
                mtfSymbolFrequencies[mtfPosition + 1]++;
            }
        }

        if (repeatCount > 0)
        {
            repeatCount--;
            while (true)
            {
                if ((repeatCount & 1) == 0)
                {
                    mtfBlock[mtfIndex++] = HUFFMAN_SYMBOL_RUNA;
                    totalRunAs++;
                }
                else
                {
                    mtfBlock[mtfIndex++] = HUFFMAN_SYMBOL_RUNB;
                    totalRunBs++;
                }

                if (repeatCount <= 1)
                {
                    break;
                }
                repeatCount = (repeatCount - 2) >> 1;
            }
        }

        mtfBlock[mtfIndex] = endOfBlockSymbol;
        mtfSymbolFrequencies[endOfBlockSymbol]++;
        mtfSymbolFrequencies[HUFFMAN_SYMBOL_RUNA] += totalRunAs;
        mtfSymbolFrequencies[HUFFMAN_SYMBOL_RUNB] += totalRunBs;

        mtfLength = mtfIndex + 1;
        alphabetSize = endOfBlockSymbol + 1;
    }
};
#endif