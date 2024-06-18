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

#ifndef MOVE_TO_FRONT_HPP
#define MOVE_TO_FRONT_HPP

#include <vector>
#include <algorithm>
#include <cstdint>

class MoveToFront
{
private:
    std::vector<uint8_t> mtf;

public:
    MoveToFront() : mtf(256)
    {
        for (int i = 0; i < 256; ++i)
        {
            mtf[i] = i;
        }
    }

    int valueToFront(uint8_t value)
    {
        auto pos = std::find(mtf.begin(), mtf.end(), value);
        int index = std::distance(mtf.begin(), pos);
        if (pos != mtf.begin())
        {
            mtf.erase(pos);
            mtf.insert(mtf.begin(), value);
        }
        return index;
    }

    uint8_t indexToFront(int index)
    {
        uint8_t value = mtf[index];
        if (index > 0)
        {
            std::rotate(mtf.begin(), mtf.begin() + index, mtf.begin() + index + 1);
        }
        mtf[0] = value;
        return value;
    }
};
#endif