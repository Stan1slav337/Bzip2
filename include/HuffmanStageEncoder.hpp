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

#ifndef HUFFMAN_STAGE_ENCODER_HPP
#define HUFFMAN_STAGE_ENCODER_HPP

#include "Config.hpp"
#include "BitOutputStream.hpp"
#include "HuffmanAllocator.hpp"
#include "MoveToFront.hpp"

class HuffmanStageEncoder
{
public:
    HuffmanStageEncoder(BitOutputStream &outputStream, std::vector<uint16_t> &mtfBlock, int mtfLength, int mtfAlphabetSize, std::vector<int> &mtfSymbolFrequencies)
        : bitOutputStream(outputStream), mtfBlock(mtfBlock), mtfLength(mtfLength), mtfAlphabetSize(mtfAlphabetSize), mtfSymbolFrequencies(mtfSymbolFrequencies)
    {
        int totalTables = selectTableCount(mtfLength);
        huffmanCodeLengths.resize(totalTables, std::vector<int>(mtfAlphabetSize, 0));
        huffmanMergedCodeSymbols.resize(totalTables, std::vector<int>(mtfAlphabetSize, 0));
        selectors.resize((mtfLength + HUFFMAN_GROUP_RUN_LENGTH - 1) / HUFFMAN_GROUP_RUN_LENGTH);
    }

    void encode()
    {
        generateHuffmanOptimisationSeeds();
        for (int i = 3; i >= 0; i--)
        {
            optimiseSelectorsAndHuffmanTables(i == 0);
        }
        assignHuffmanCodeSymbols();

        writeSelectorsAndHuffmanTables();
        writeBlockData();
    }

private:
    BitOutputStream &bitOutputStream;
    std::vector<uint16_t> &mtfBlock;
    int mtfLength;
    int mtfAlphabetSize;
    std::vector<int> &mtfSymbolFrequencies;
    std::vector<std::vector<int>> huffmanCodeLengths;
    std::vector<std::vector<int>> huffmanMergedCodeSymbols;
    std::vector<uint8_t> selectors;

    static int selectTableCount(int mtfLength)
    {
        if (mtfLength >= 2400)
            return 6;
        if (mtfLength >= 1200)
            return 5;
        if (mtfLength >= 600)
            return 4;
        if (mtfLength >= 200)
            return 3;
        return 2;
    }

    void generateHuffmanCodeLengths(int alphabetSize, const std::vector<int> &symbolFrequencies, std::vector<int> &codeLengths)
    {
        std::vector<int> mergedFrequenciesAndIndices(alphabetSize);
        std::vector<int> sortedFrequencies(alphabetSize);

        for (int i = 0; i < alphabetSize; i++)
        {
            mergedFrequenciesAndIndices[i] = (symbolFrequencies[i] << 9) | i;
        }

        std::sort(mergedFrequenciesAndIndices.begin(), mergedFrequenciesAndIndices.end());

        for (int i = 0; i < alphabetSize; i++)
        {
            sortedFrequencies[i] = mergedFrequenciesAndIndices[i] >> 9;
        }

        HuffmanAllocator::allocateHuffmanCodeLengths(sortedFrequencies, HUFFMAN_ENCODE_MAXIMUM_CODE_LENGTH);

        for (int i = 0; i < alphabetSize; i++)
        {
            codeLengths[mergedFrequenciesAndIndices[i] & 0x1ff] = sortedFrequencies[i];
        }
    }

    void generateHuffmanOptimisationSeeds()
    {
        int totalTables = huffmanCodeLengths.size();
        int remainingLength = mtfLength;
        int lowCostEnd = -1;

        for (int i = 0; i < totalTables; i++)
        {
            int targetCumulativeFrequency = remainingLength / (totalTables - i);
            int lowCostStart = lowCostEnd + 1;
            int actualCumulativeFrequency = 0;

            while ((actualCumulativeFrequency < targetCumulativeFrequency) && (lowCostEnd < (mtfAlphabetSize - 1)))
            {
                actualCumulativeFrequency += mtfSymbolFrequencies[++lowCostEnd];
            }

            if ((lowCostEnd > lowCostStart) && (i != 0) && (i != (totalTables - 1)) && ((totalTables - i) % 2) == 0)
            {
                actualCumulativeFrequency -= mtfSymbolFrequencies[lowCostEnd--];
            }

            for (int j = 0; j < mtfAlphabetSize; j++)
            {
                if ((j < lowCostStart) || (j > lowCostEnd))
                {
                    huffmanCodeLengths[i][j] = HUFFMAN_HIGH_SYMBOL_COST;
                }
            }

            remainingLength -= actualCumulativeFrequency;
        }
    }

    void optimiseSelectorsAndHuffmanTables(bool storeSelectors)
    {
        int totalTables = huffmanCodeLengths.size();
        std::vector<std::vector<int>> tableFrequencies(totalTables, std::vector<int>(mtfAlphabetSize, 0));

        int selectorIndex = 0;
        for (int groupStart = 0; groupStart < mtfLength;)
        {
            int groupEnd = std::min(groupStart + HUFFMAN_GROUP_RUN_LENGTH, mtfLength) - 1;

            std::vector<int> cost(totalTables, 0);
            for (int i = groupStart; i <= groupEnd; i++)
            {
                uint16_t value = mtfBlock[i];
                for (int j = 0; j < totalTables; j++)
                {
                    cost[j] += huffmanCodeLengths[j][value];
                }
            }

            int bestTable = 0;
            int bestCost = cost[0];
            for (int i = 1; i < totalTables; i++)
            {
                if (cost[i] < bestCost)
                {
                    bestCost = cost[i];
                    bestTable = i;
                }
            }

            for (int i = groupStart; i <= groupEnd; i++)
            {
                tableFrequencies[bestTable][mtfBlock[i]]++;
            }

            if (storeSelectors)
            {
                selectors[selectorIndex++] = bestTable;
            }

            groupStart = groupEnd + 1;
        }

        for (int i = 0; i < totalTables; i++)
        {
            generateHuffmanCodeLengths(mtfAlphabetSize, tableFrequencies[i], huffmanCodeLengths[i]);
        }
    }

    void assignHuffmanCodeSymbols()
    {
        int totalTables = huffmanCodeLengths.size();

        for (int i = 0; i < totalTables; i++)
        {
            int minimumLength = 32;
            int maximumLength = 0;
            // TODO
            for (int length : huffmanCodeLengths[i])
            {
                if (length > maximumLength)
                {
                    maximumLength = length;
                }
                if (length < minimumLength)
                {
                    minimumLength = length;
                }
            }

            int code = 0;
            for (int j = minimumLength; j <= maximumLength; j++)
            {
                for (int k = 0; k < mtfAlphabetSize; k++)
                {
                    if ((huffmanCodeLengths[i][k] & 0xff) == j)
                    {
                        huffmanMergedCodeSymbols[i][k] = (j << 24) | code;
                        code++;
                    }
                }
                code <<= 1;
            }
        }
    }

    void writeSelectorsAndHuffmanTables()
    {
        int totalTables = huffmanCodeLengths.size();
        int totalSelectors = selectors.size();

        bitOutputStream.writeBits(3, totalTables);
        bitOutputStream.writeBits(15, totalSelectors);

        MoveToFront selectorMTF;
        for (int i = 0; i < totalSelectors; i++)
        {
            bitOutputStream.writeUnary(selectorMTF.valueToFront(selectors[i]));
        }

        // Write the Huffman tables
        for (auto &tableLengths : huffmanCodeLengths)
        {
            int currentLength = tableLengths[0];
            bitOutputStream.writeBits(5, currentLength);

            for (int j = 0; j < mtfAlphabetSize; j++)
            {
                int codeLength = tableLengths[j];
                int value = (currentLength < codeLength) ? 2 : 3;
                int delta = std::abs(codeLength - currentLength);

                while (delta-- > 0)
                {
                    bitOutputStream.writeBits(2, value);
                }
                bitOutputStream.writeBoolean(false);
                currentLength = codeLength;
            }
        }
    }

    void writeBlockData()
    {
        int selectorIndex = 0;
        int mtfIndex = 0;
        while (mtfIndex < mtfLength)
        {
            int groupEnd = std::min(mtfIndex + HUFFMAN_GROUP_RUN_LENGTH, mtfLength) - 1;
            const std::vector<int> &tableMergedCodeSymbols = huffmanMergedCodeSymbols[selectors[selectorIndex++]];

            while (mtfIndex <= groupEnd)
            {
                int mergedCodeSymbol = tableMergedCodeSymbols[mtfBlock[mtfIndex++]];
                bitOutputStream.writeBits(mergedCodeSymbol >> 24, mergedCodeSymbol);
            }
        }
    }
};
#endif