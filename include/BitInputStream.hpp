/*
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


#ifndef BIT_INPUT_STREAM_HPP
#define BIT_INPUT_STREAM_HPP

#include <istream>
#include <stdexcept>

class BitInputStream
{
private:
    std::istream &inputStream;
    int bitBuffer = 0;
    int bitCount = 0;

public:
    explicit BitInputStream(std::istream &inStream) : inputStream(inStream) {}

    bool readBoolean()
    {
        if (bitCount == 0)
        {
            char byteRead;
            if (!inputStream.get(byteRead))
            {
                throw std::runtime_error("Insufficient data");
            }
            bitBuffer = (bitBuffer << 8) | static_cast<int>(byteRead);
            bitCount = 8;
        }

        --bitCount;
        return (bitBuffer & (1 << bitCount)) != 0;
    }

    int readUnary()
    {
        int unaryCount = 0;
        while (readBoolean())
        {
            ++unaryCount;
        }
        return unaryCount;
    }

    int readBits(int count)
    {
        int bits = 0;
        for (int i = 0; i < count; ++i)
        {
            bits <<= 1;
            if (readBoolean())
            {
                bits |= 1;
            }
        }
        return bits;
    }

    int readInteger()
    {
        return (readBits(16) << 16) | readBits(16);
    }
};
#endif