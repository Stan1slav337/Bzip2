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

#ifndef HUFFMAN_STAGE_DECODER_HPP
#define HUFFMAN_STAGE_DECODER_HPP

#include <vector>
#include <cstdint>

#include "Config.hpp"
#include "BitInputStream.hpp"

class HuffmanStageDecoder
{
public:
    HuffmanStageDecoder(BitInputStream &inputStream, int alphabetSize, const std::vector<std::vector<uint8_t>> &tableCodeLengths, std::vector<uint8_t> sel)
        : bitInputStream(inputStream),
          selectors(sel),
          minimumLengths(HUFFMAN_MAXIMUM_TABLES),
          codeBases(HUFFMAN_MAXIMUM_TABLES, std::vector<int>(HUFFMAN_DECODE_MAXIMUM_CODE_LENGTH + 2)),
          codeLimits(HUFFMAN_MAXIMUM_TABLES, std::vector<int>(HUFFMAN_DECODE_MAXIMUM_CODE_LENGTH + 1)),
          codeSymbols(HUFFMAN_MAXIMUM_TABLES, std::vector<int>(HUFFMAN_MAXIMUM_ALPHABET_SIZE)),
          currentTable(selectors[0])
    {
        createHuffmanDecodingTables(alphabetSize, tableCodeLengths);
    }

    int nextSymbol()
    {
        if (++groupPosition % HUFFMAN_GROUP_RUN_LENGTH == 0)
        {
            groupIndex++;
            if (groupIndex == selectors.size())
            {
                throw std::runtime_error("Error decoding  block");
            }
            currentTable = selectors[groupIndex] & 0xff;
        }
        int codeLength = minimumLengths[currentTable];
        int codeBits = bitInputStream.readBits(codeLength);

        while (codeLength <= HUFFMAN_DECODE_MAXIMUM_CODE_LENGTH)
        {
            if (codeBits <= codeLimits[currentTable][codeLength])
            {
                return codeSymbols[currentTable][codeBits - codeBases[currentTable][codeLength]];
            }
            codeBits = (codeBits << 1) | bitInputStream.readBits(1);
            codeLength++;
        }

        throw std::runtime_error("Error decoding  block");
    }

private:
    BitInputStream &bitInputStream;
    std::vector<uint8_t> selectors;
    std::vector<int> minimumLengths;
    std::vector<std::vector<int>> codeBases;
    std::vector<std::vector<int>> codeLimits;
    std::vector<std::vector<int>> codeSymbols;
    int currentTable;
    int groupIndex = -1;
    int groupPosition = -1;

    void createHuffmanDecodingTables(int alphabetSize, const std::vector<std::vector<uint8_t>> &tableCodeLengths)
    {
        for (size_t table = 0; table < tableCodeLengths.size(); table++)
        {
            std::vector<int> &bases = codeBases[table];
            std::vector<int> &limits = codeLimits[table];
            std::vector<int> &symbols = codeSymbols[table];
            const std::vector<uint8_t> &lengths = tableCodeLengths[table];

            int minimumLength = HUFFMAN_DECODE_MAXIMUM_CODE_LENGTH;
            int maximumLength = 0;

            for (int i = 0; i < alphabetSize; i++)
            {
                maximumLength = std::max(static_cast<int>(lengths[i]), maximumLength);
                minimumLength = std::min(static_cast<int>(lengths[i]), minimumLength);
            }
            minimumLengths[table] = minimumLength;

            for (int i = 0; i < alphabetSize; i++)
            {
                bases[lengths[i] + 1]++;
            }
            for (size_t i = 1; i < HUFFMAN_DECODE_MAXIMUM_CODE_LENGTH + 2; i++)
            {
                bases[i] += bases[i - 1];
            }

            int code = 0;
            for (int i = minimumLength; i <= maximumLength; i++)
            {
                int base = code;
                code += bases[i + 1] - bases[i];
                bases[i] = base - bases[i];
                limits[i] = code - 1;
                code <<= 1;
            }

            int codeIndex = 0;
            for (int bitLength = minimumLength; bitLength <= maximumLength; bitLength++)
            {
                for (int symbol = 0; symbol < alphabetSize; symbol++)
                {
                    if (lengths[symbol] == bitLength)
                    {
                        symbols[codeIndex++] = symbol;
                    }
                }
            }
        }
    }
};
#endif