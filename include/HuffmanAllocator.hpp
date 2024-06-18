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

#ifndef HUFFMAN_ALLOCATOR_HPP
#define HUFFMAN_ALLOCATOR_HPP

#include <vector>
#include <cmath>
#include <algorithm>
#include <cstdint>

class HuffmanAllocator
{
public:
    static int first(const std::vector<int> &array, int i, int nodesToMove)
    {
        const int length = array.size();
        const int limit = i;
        int k = length - 2;

        while ((i >= nodesToMove) && ((array[i] % length) > limit))
        {
            k = i;
            i -= (limit - i + 1);
        }
        i = std::max(nodesToMove - 1, i);

        while (k > (i + 1))
        {
            int temp = (i + k) >> 1;
            if ((array[temp] % length) > limit)
            {
                k = temp;
            }
            else
            {
                i = temp;
            }
        }

        return k;
    }

    static void setExtendedParentPointers(std::vector<int> &array)
    {
        int length = array.size();

        array[0] += array[1];

        for (int headNode = 0, tailNode = 1, topNode = 2; tailNode < (length - 1); tailNode++)
        {
            int temp;
            if ((topNode >= length) || (array[headNode] < array[topNode]))
            {
                temp = array[headNode];
                array[headNode++] = tailNode;
            }
            else
            {
                temp = array[topNode++];
            }

            if ((topNode >= length) || ((headNode < tailNode) && (array[headNode] < array[topNode])))
            {
                temp += array[headNode];
                array[headNode++] = tailNode + length;
            }
            else
            {
                temp += array[topNode++];
            }

            array[tailNode] = temp;
        }
    }

    static int findNodesToRelocate(const std::vector<int> &array, int maximumLength)
    {
        int currentNode = array.size() - 2;
        for (int currentDepth = 1; (currentDepth < (maximumLength - 1)) && (currentNode > 1); currentDepth++)
        {
            currentNode = first(array, currentNode - 1, 0);
        }

        return currentNode;
    }

    static void allocateNodeLengths(std::vector<int> &array)
    {
        int firstNode = array.size() - 2;
        int nextNode = array.size() - 1;

        for (int currentDepth = 1, availableNodes = 2; availableNodes > 0; currentDepth++)
        {
            int lastNode = firstNode;
            firstNode = first(array, lastNode - 1, 0);

            for (int i = availableNodes - (lastNode - firstNode); i > 0; i--)
            {
                array[nextNode--] = currentDepth;
            }

            availableNodes = (lastNode - firstNode) << 1;
        }
    }

    static void allocateNodeLengthsWithRelocation(std::vector<int> &array, int nodesToMove, int insertDepth)
    {
        int firstNode = array.size() - 2;
        int nextNode = array.size() - 1;
        int currentDepth = (insertDepth == 1) ? 2 : 1;
        int nodesLeftToMove = (insertDepth == 1) ? nodesToMove - 2 : nodesToMove;

        for (int availableNodes = currentDepth << 1; availableNodes > 0; currentDepth++)
        {
            int lastNode = firstNode;
            firstNode = (firstNode <= nodesToMove) ? firstNode : first(array, lastNode - 1, nodesToMove);

            int offset = 0;
            if (currentDepth >= insertDepth)
            {
                offset = std::min(nodesLeftToMove, 1 << (currentDepth - insertDepth));
            }
            else if (currentDepth == (insertDepth - 1))
            {
                offset = 1;
                if (array[firstNode] == lastNode)
                {
                    firstNode++;
                }
            }

            for (int i = availableNodes - (lastNode - firstNode + offset); i > 0; i--)
            {
                array[nextNode--] = currentDepth;
            }

            nodesLeftToMove -= offset;
            availableNodes = (lastNode - firstNode + offset) << 1;
        }
    }

    static void allocateHuffmanCodeLengths(std::vector<int> &array, int maximumLength)
    {
        switch (array.size())
        {
        case 2:
            array[1] = 1;
        case 1:
            array[0] = 1;
            return;
        }

        setExtendedParentPointers(array);
        int nodesToRelocate = findNodesToRelocate(array, maximumLength);

        if ((array[0] % array.size()) >= nodesToRelocate)
        {
            allocateNodeLengths(array);
        }
        else
        {
            int insertDepth = maximumLength - SignificantBits(nodesToRelocate - 1);
            allocateNodeLengthsWithRelocation(array, nodesToRelocate, insertDepth);
        }
    }

private:
    HuffmanAllocator() {} // Non-instantiable

    static int SignificantBits(int x)
    {
        int n;
        for (n = 0; x > 0; n++)
        {
            x >>= 1;
        }
        return n;
    }
};
#endif