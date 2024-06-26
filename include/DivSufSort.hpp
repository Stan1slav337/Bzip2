/*
 * Copyright (c) 2003-2008 Yuta Mori All Rights Reserved.
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

#ifndef DIV_SUF_SORT_HPP
#define DIV_SUF_SORT_HPP

#include <vector>
#include <cstdint>
#include <array>

class DivSufSort
{
public:
    DivSufSort(std::vector<uint8_t> &T, std::vector<int> &SA, int n) : T(T), SA(SA), n(n)
    {
    }

    int bwt()
    {
        std::vector<int> bucketA(BUCKET_A_SIZE);
        std::vector<int> bucketB(BUCKET_B_SIZE);

        if (n == 0)
        {
            return 0;
        }
        else if (n == 1)
        {
            SA[0] = T[0];
            return 0;
        }

        int m = sortTypeBstar(bucketA, bucketB);
        if (0 < m)
        {
            return constructBWT(bucketA, bucketB);
        }

        return 0;
    }

private:
    static constexpr int STACK_SIZE = 64;
    static constexpr int BUCKET_A_SIZE = 256;
    static constexpr int BUCKET_B_SIZE = 65536;
    static constexpr int SS_BLOCKSIZE = 1024;
    static constexpr int INSERTIONSORT_THRESHOLD = 8;
    static constexpr int log2table[] = {
        -1, 0, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
        5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
        6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
        6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
        7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
        7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
        7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
        7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7};

    std::vector<int> &SA;
    std::vector<uint8_t> &T;
    int n;

    void swapElements(std::vector<int> &array1, int index1, std::vector<int> &array2, int index2)
    {
        int temp = array1[index1];
        array1[index1] = array2[index2];
        array2[index2] = temp;
    }

    int ssCompare(int p1, int p2, int depth)
    {
        int U1n, U2n; // pointers within T
        int U1, U2;

        for (
            U1 = depth + SA[p1], U2 = depth + SA[p2], U1n = SA[p1 + 1] + 2, U2n = SA[p2 + 1] + 2;
            (U1 < U1n) && (U2 < U2n) && (T[U1] == T[U2]);
            ++U1, ++U2)
            ;

        return U1 < U1n ? (U2 < U2n ? (T[U1] & 0xff) - (T[U2] & 0xff) : 1)
                        : (U2 < U2n ? -1 : 0);
    }

    int ssCompareLast(int PA, int p1, int p2, int depth, int size)
    {
        int U1, U2, U1n, U2n;

        for (
            U1 = depth + SA[p1], U2 = depth + SA[p2], U1n = size, U2n = SA[(p2 + 1)] + 2;
            (U1 < U1n) && (U2 < U2n) && (T[U1] == T[U2]);
            ++U1, ++U2)
            ;

        if (U1 < U1n)
        {
            return (U2 < U2n) ? (T[U1] & 0xff) - (T[U2] & 0xff) : 1;
        }
        else if (U2 == U2n)
        {
            return 1;
        }

        for (
            U1 = U1 % size, U1n = SA[PA] + 2;
            (U1 < U1n) && (U2 < U2n) && (T[U1] == T[U2]);
            ++U1, ++U2)
            ;

        return U1 < U1n ? (U2 < U2n ? (T[U1] & 0xff) - (T[U2] & 0xff) : 1)
                        : (U2 < U2n ? -1 : 0);
    }

    void ssInsertionSort(int PA, int first, int last, int depth)
    {
        int i, j; // pointer within SA
        int t;
        int r;

        for (i = last - 2; first <= i; --i)
        {
            for (t = SA[i], j = i + 1; 0 < (r = ssCompare(PA + t, PA + SA[j], depth));)
            {
                do
                {
                    SA[j - 1] = SA[j];
                } while ((++j < last) && (SA[j] < 0));
                if (last <= j)
                {
                    break;
                }
            }
            if (r == 0)
            {
                SA[j] = ~SA[j];
            }
            SA[j - 1] = t;
        }
    }

    void ssFixdown(int Td, int PA, int sa, int i, int size)
    {
        int j, k;
        int v;
        int c, d, e;

        for (v = SA[sa + i], c = (T[Td + SA[PA + v]]) & 0xff; (j = 2 * i + 1) < size; SA[sa + i] = SA[sa + k], i = k)
        {
            d = T[Td + SA[PA + SA[sa + (k = j++)]]] & 0xff;
            if (d < (e = T[Td + SA[PA + SA[sa + j]]] & 0xff))
            {
                k = j;
                d = e;
            }
            if (d <= c)
                break;
        }
        SA[sa + i] = v;
    }

    void ssHeapSort(int Td, int PA, int sa, int size)
    {
        int i, m;
        int t;

        m = size;
        if ((size % 2) == 0)
        {
            m--;
            if ((T[Td + SA[PA + SA[sa + (m / 2)]]] & 0xff) < (T[Td + SA[PA + SA[sa + m]]] & 0xff))
            {
                swapElements(SA, sa + m, SA, sa + (m / 2));
            }
        }

        for (i = m / 2 - 1; 0 <= i; --i)
        {
            ssFixdown(Td, PA, sa, i, m);
        }

        if ((size % 2) == 0)
        {
            swapElements(SA, sa, SA, sa + m);
            ssFixdown(Td, PA, sa, 0, m);
        }

        for (i = m - 1; 0 < i; --i)
        {
            t = SA[sa];
            SA[sa] = SA[sa + i];
            ssFixdown(Td, PA, sa, 0, i);
            SA[sa + i] = t;
        }
    }

    int ssMedian3(int Td, int PA, int v1, int v2, int v3)
    {
        int T_v1 = T[Td + SA[PA + SA[v1]]] & 0xff;
        int T_v2 = T[Td + SA[PA + SA[v2]]] & 0xff;
        int T_v3 = T[Td + SA[PA + SA[v3]]] & 0xff;

        if (T_v1 > T_v2)
        {
            int temp = v1;
            v1 = v2;
            v2 = temp;
            int T_vtemp = T_v1;
            T_v1 = T_v2;
            T_v2 = T_vtemp;
        }
        if (T_v2 > T_v3)
        {
            if (T_v1 > T_v3)
            {
                return v1;
            }
            return v3;
        }
        return v2;
    }

    int ssMedian5(int Td, int PA, int v1, int v2, int v3, int v4, int v5)
    {
        int T_v1 = T[Td + SA[PA + SA[v1]]] & 0xff;
        int T_v2 = T[Td + SA[PA + SA[v2]]] & 0xff;
        int T_v3 = T[Td + SA[PA + SA[v3]]] & 0xff;
        int T_v4 = T[Td + SA[PA + SA[v4]]] & 0xff;
        int T_v5 = T[Td + SA[PA + SA[v5]]] & 0xff;
        int temp;
        int T_vtemp;

        if (T_v2 > T_v3)
        {
            temp = v2;
            v2 = v3;
            v3 = temp;
            T_vtemp = T_v2;
            T_v2 = T_v3;
            T_v3 = T_vtemp;
        }
        if (T_v4 > T_v5)
        {
            temp = v4;
            v4 = v5;
            v5 = temp;
            T_vtemp = T_v4;
            T_v4 = T_v5;
            T_v5 = T_vtemp;
        }
        if (T_v2 > T_v4)
        {
            temp = v2;
            v2 = v4;
            v4 = temp;
            T_vtemp = T_v2;
            T_v2 = T_v4;
            T_v4 = T_vtemp;
            temp = v3;
            v3 = v5;
            v5 = temp;
            T_vtemp = T_v3;
            T_v3 = T_v5;
            T_v5 = T_vtemp;
        }
        if (T_v1 > T_v3)
        {
            temp = v1;
            v1 = v3;
            v3 = temp;
            T_vtemp = T_v1;
            T_v1 = T_v3;
            T_v3 = T_vtemp;
        }
        if (T_v1 > T_v4)
        {
            temp = v1;
            v1 = v4;
            v4 = temp;
            T_vtemp = T_v1;
            T_v1 = T_v4;
            T_v4 = T_vtemp;
            temp = v3;
            v3 = v5;
            v5 = temp;
            T_vtemp = T_v3;
            T_v3 = T_v5;
            T_v5 = T_vtemp;
        }
        if (T_v3 > T_v4)
        {
            return v4;
        }
        return v3;
    }

    int ssPivot(int Td, int PA, int first, int last)
    {
        int middle;
        int t;

        t = last - first;
        middle = first + t / 2;

        if (t <= 512)
        {
            if (t <= 32)
            {
                return ssMedian3(Td, PA, first, middle, last - 1);
            }
            t >>= 2;
            return ssMedian5(Td, PA, first, first + t, middle, last - 1 - t, last - 1);
        }
        t >>= 3;
        return ssMedian3(
            Td, PA,
            ssMedian3(Td, PA, first, first + t, first + (t << 1)),
            ssMedian3(Td, PA, middle - t, middle, middle + t),
            ssMedian3(Td, PA, last - 1 - (t << 1), last - 1 - t, last - 1));
    }

    int ssLog(int n)
    {

        return ((n & 0xff00) != 0) ? 8 + log2table[(n >> 8) & 0xff]
                                   : log2table[n & 0xff];
    }

    int ssSubstringPartition(int PA, int first, int last, int depth)
    {
        int a, b;
        int t;

        for (a = first - 1, b = last;;)
        {
            for (; (++a < b) && ((SA[PA + SA[a]] + depth) >= (SA[PA + SA[a] + 1] + 1));)
            {
                SA[a] = ~SA[a];
            }
            for (; (a < --b) && ((SA[PA + SA[b]] + depth) < (SA[PA + SA[b] + 1] + 1));)
                ;
            if (b <= a)
            {
                break;
            }
            t = ~SA[b];
            SA[b] = SA[a];
            SA[a] = t;
        }
        if (first < a)
        {
            SA[first] = ~SA[first];
        }

        return a;
    }

    struct StackEntry
    {
        int a;
        int b;
        int c;
        int d;
    };

    void ssMultiKeyIntroSort(int PA, int first, int last, int depth)
    {
        std::array<StackEntry, STACK_SIZE> stack{};

        int Td = 0;
        int a = 0, b = 0, c = 0, d = 0, e = 0, f = 0;
        int s = 0, t = 0;
        int ssize;
        int limit;
        int v = 0, x = 0;

        for (ssize = 0, limit = ssLog(last - first);;)
        {
            if ((last - first) <= INSERTIONSORT_THRESHOLD)
            {
                if (1 < (last - first))
                {
                    ssInsertionSort(PA, first, last, depth);
                }
                if (ssize == 0)
                    return;
                StackEntry entry = stack[--ssize];
                first = entry.a;
                last = entry.b;
                depth = entry.c;
                limit = entry.d;
                continue;
            }

            Td = depth;
            if (limit-- == 0)
            {
                ssHeapSort(Td, PA, first, last - first);
            }
            if (limit < 0)
            {
                for (a = first + 1, v = T[Td + SA[PA + SA[first]]] & 0xff; a < last; ++a)
                {
                    if ((x = (T[Td + SA[PA + SA[a]]] & 0xff)) != v)
                    {
                        if (1 < (a - first))
                        {
                            break;
                        }
                        v = x;
                        first = a;
                    }
                }
                if ((T[Td + SA[PA + SA[first]] - 1] & 0xff) < v)
                {
                    first = ssSubstringPartition(PA, first, a, depth);
                }
                if ((a - first) <= (last - a))
                {
                    if (1 < (a - first))
                    {
                        stack[ssize++] = StackEntry{a, last, depth, -1};
                        last = a;
                        depth += 1;
                        limit = ssLog(a - first);
                    }
                    else
                    {
                        first = a;
                        limit = -1;
                    }
                }
                else
                {
                    if (1 < (last - a))
                    {
                        stack[ssize++] = StackEntry{first, a, depth + 1, ssLog(a - first)};
                        first = a;
                        limit = -1;
                    }
                    else
                    {
                        last = a;
                        depth += 1;
                        limit = ssLog(a - first);
                    }
                }
                continue;
            }

            a = ssPivot(Td, PA, first, last);
            v = T[Td + SA[PA + SA[a]]] & 0xff;
            swapElements(SA, first, SA, a);

            for (b = first; (++b < last) && ((x = (T[Td + SA[PA + SA[b]]] & 0xff)) == v);)
                ;
            if (((a = b) < last) && (x < v))
            {
                for (; (++b < last) && ((x = (T[Td + SA[PA + SA[b]]] & 0xff)) <= v);)
                {
                    if (x == v)
                    {
                        swapElements(SA, b, SA, a);
                        ++a;
                    }
                }
            }
            for (c = last; (b < --c) && ((x = (T[Td + SA[PA + SA[c]]] & 0xff)) == v);)
                ;
            if ((b < (d = c)) && (x > v))
            {
                for (; (b < --c) && ((x = (T[Td + SA[PA + SA[c]]] & 0xff)) >= v);)
                {
                    if (x == v)
                    {
                        swapElements(SA, c, SA, d);
                        --d;
                    }
                }
            }
            for (; b < c;)
            {
                swapElements(SA, b, SA, c);
                for (; (++b < c) && ((x = (T[Td + SA[PA + SA[b]]] & 0xff)) <= v);)
                {
                    if (x == v)
                    {
                        swapElements(SA, b, SA, a);
                        ++a;
                    }
                }
                for (; (b < --c) && ((x = (T[Td + SA[PA + SA[c]]] & 0xff)) >= v);)
                {
                    if (x == v)
                    {
                        swapElements(SA, c, SA, d);
                        --d;
                    }
                }
            }

            if (a <= d)
            {
                c = b - 1;

                if ((s = a - first) > (t = b - a))
                {
                    s = t;
                }
                for (e = first, f = b - s; 0 < s; --s, ++e, ++f)
                {
                    swapElements(SA, e, SA, f);
                }
                if ((s = d - c) > (t = last - d - 1))
                {
                    s = t;
                }
                for (e = b, f = last - s; 0 < s; --s, ++e, ++f)
                {
                    swapElements(SA, e, SA, f);
                }

                a = first + (b - a);
                c = last - (d - c);
                b = (v <= (T[Td + SA[PA + SA[a]] - 1] & 0xff)) ? a : ssSubstringPartition(PA, a, c, depth);

                if ((a - first) <= (last - c))
                {
                    if ((last - c) <= (c - b))
                    {
                        stack[ssize++] = StackEntry{b, c, depth + 1, ssLog(c - b)};
                        stack[ssize++] = StackEntry{c, last, depth, limit};
                        last = a;
                    }
                    else if ((a - first) <= (c - b))
                    {
                        stack[ssize++] = StackEntry{c, last, depth, limit};
                        stack[ssize++] = StackEntry{b, c, depth + 1, ssLog(c - b)};
                        last = a;
                    }
                    else
                    {
                        stack[ssize++] = StackEntry{c, last, depth, limit};
                        stack[ssize++] = StackEntry{first, a, depth, limit};
                        first = b;
                        last = c;
                        depth += 1;
                        limit = ssLog(c - b);
                    }
                }
                else
                {
                    if ((a - first) <= (c - b))
                    {
                        stack[ssize++] = StackEntry{b, c, depth + 1, ssLog(c - b)};
                        stack[ssize++] = StackEntry{first, a, depth, limit};
                        first = c;
                    }
                    else if ((last - c) <= (c - b))
                    {
                        stack[ssize++] = StackEntry{first, a, depth, limit};
                        stack[ssize++] = StackEntry{b, c, depth + 1, ssLog(c - b)};
                        first = c;
                    }
                    else
                    {
                        stack[ssize++] = StackEntry{first, a, depth, limit};
                        stack[ssize++] = StackEntry{c, last, depth, limit};
                        first = b;
                        last = c;
                        depth += 1;
                        limit = ssLog(c - b);
                    }
                }
            }
            else
            {
                limit += 1;
                if ((T[Td + SA[PA + SA[first]] - 1] & 0xff) < v)
                {
                    first = ssSubstringPartition(PA, first, last, depth);
                    limit = ssLog(last - first);
                }
                depth += 1;
            }
        }
    }

    void ssBlockSwap(std::vector<int> &array1, int first1, std::vector<int> &array2, int first2, int size)
    {
        int a, b;
        int i;
        for (i = size, a = first1, b = first2; 0 < i; --i, ++a, ++b)
        {
            swapElements(array1, a, array2, b);
        }
    }

    void ssMergeForward(int PA, std::vector<int> &buf, int bufoffset, int first, int middle, int last, int depth)
    {
        int bufend;
        int i, j, k;
        int t;
        int r;

        bufend = bufoffset + (middle - first) - 1;
        ssBlockSwap(buf, bufoffset, SA, first, middle - first);

        for (t = SA[first], i = first, j = bufoffset, k = middle;;)
        {
            r = ssCompare(PA + buf[j], PA + SA[k], depth);
            if (r < 0)
            {
                do
                {
                    SA[i++] = buf[j];
                    if (bufend <= j)
                    {
                        buf[j] = t;
                        return;
                    }
                    buf[j++] = SA[i];
                } while (buf[j] < 0);
            }
            else if (r > 0)
            {
                do
                {
                    SA[i++] = SA[k];
                    SA[k++] = SA[i];
                    if (last <= k)
                    {
                        while (j < bufend)
                        {
                            SA[i++] = buf[j];
                            buf[j++] = SA[i];
                        }
                        SA[i] = buf[j];
                        buf[j] = t;
                        return;
                    }
                } while (SA[k] < 0);
            }
            else
            {
                SA[k] = ~SA[k];
                do
                {
                    SA[i++] = buf[j];
                    if (bufend <= j)
                    {
                        buf[j] = t;
                        return;
                    }
                    buf[j++] = SA[i];
                } while (buf[j] < 0);

                do
                {
                    SA[i++] = SA[k];
                    SA[k++] = SA[i];
                    if (last <= k)
                    {
                        while (j < bufend)
                        {
                            SA[i++] = buf[j];
                            buf[j++] = SA[i];
                        }
                        SA[i] = buf[j];
                        buf[j] = t;
                        return;
                    }
                } while (SA[k] < 0);
            }
        }
    }

    void ssMergeBackward(int PA, std::vector<int> &buf, int bufoffset, int first, int middle, int last, int depth)
    {
        int p1, p2;
        int bufend;
        int i, j, k;
        int t;
        int r;
        int x;

        bufend = bufoffset + (last - middle);
        ssBlockSwap(buf, bufoffset, SA, middle, last - middle);

        x = 0;
        if (buf[bufend - 1] < 0)
        {
            x |= 1;
            p1 = PA + ~buf[bufend - 1];
        }
        else
        {
            p1 = PA + buf[bufend - 1];
        }
        if (SA[middle - 1] < 0)
        {
            x |= 2;
            p2 = PA + ~SA[middle - 1];
        }
        else
        {
            p2 = PA + SA[middle - 1];
        }
        for (t = SA[last - 1], i = last - 1, j = bufend - 1, k = middle - 1;;)
        {

            r = ssCompare(p1, p2, depth);
            if (r > 0)
            {
                if ((x & 1) != 0)
                {
                    do
                    {
                        SA[i--] = buf[j];
                        buf[j--] = SA[i];
                    } while (buf[j] < 0);
                    x ^= 1;
                }
                SA[i--] = buf[j];
                if (j <= bufoffset)
                {
                    buf[j] = t;
                    return;
                }
                buf[j--] = SA[i];

                if (buf[j] < 0)
                {
                    x |= 1;
                    p1 = PA + ~buf[j];
                }
                else
                {
                    p1 = PA + buf[j];
                }
            }
            else if (r < 0)
            {
                if ((x & 2) != 0)
                {
                    do
                    {
                        SA[i--] = SA[k];
                        SA[k--] = SA[i];
                    } while (SA[k] < 0);
                    x ^= 2;
                }
                SA[i--] = SA[k];
                SA[k--] = SA[i];
                if (k < first)
                {
                    while (bufoffset < j)
                    {
                        SA[i--] = buf[j];
                        buf[j--] = SA[i];
                    }
                    SA[i] = buf[j];
                    buf[j] = t;
                    return;
                }

                if (SA[k] < 0)
                {
                    x |= 2;
                    p2 = PA + ~SA[k];
                }
                else
                {
                    p2 = PA + SA[k];
                }
            }
            else
            {
                if ((x & 1) != 0)
                {
                    do
                    {
                        SA[i--] = buf[j];
                        buf[j--] = SA[i];
                    } while (buf[j] < 0);
                    x ^= 1;
                }
                SA[i--] = ~buf[j];
                if (j <= bufoffset)
                {
                    buf[j] = t;
                    return;
                }
                buf[j--] = SA[i];

                if ((x & 2) != 0)
                {
                    do
                    {
                        SA[i--] = SA[k];
                        SA[k--] = SA[i];
                    } while (SA[k] < 0);
                    x ^= 2;
                }
                SA[i--] = SA[k];
                SA[k--] = SA[i];
                if (k < first)
                {
                    while (bufoffset < j)
                    {
                        SA[i--] = buf[j];
                        buf[j--] = SA[i];
                    }
                    SA[i] = buf[j];
                    buf[j] = t;
                    return;
                }

                if (buf[j] < 0)
                {
                    x |= 1;
                    p1 = PA + ~buf[j];
                }
                else
                {
                    p1 = PA + buf[j];
                }
                if (SA[k] < 0)
                {
                    x |= 2;
                    p2 = PA + ~SA[k];
                }
                else
                {
                    p2 = PA + SA[k];
                }
            }
        }
    }

    int getIDX(int a)
    {
        return (0 <= a) ? a : ~a;
    }

    void ssMergeCheckEqual(int PA, int depth, int a)
    {
        if (
            (0 <= SA[a]) && (ssCompare(PA + getIDX(SA[a - 1]), PA + SA[a], depth) == 0))
        {
            SA[a] = ~SA[a];
        }
    }

    void ssMerge(int PA, int first, int middle, int last, std::vector<int> &buf, int bufoffset, int bufsize, int depth)
    {
        std::array<StackEntry, STACK_SIZE> stack;
        int i, j;
        int m, len, half;
        int ssize;
        int check, next;

        for (check = 0, ssize = 0;;)
        {
            if ((last - middle) <= bufsize)
            {
                if ((first < middle) && (middle < last))
                {
                    ssMergeBackward(PA, buf, bufoffset, first, middle, last, depth);
                }

                if ((check & 1) != 0)
                {
                    ssMergeCheckEqual(PA, depth, first);
                }
                if ((check & 2) != 0)
                {
                    ssMergeCheckEqual(PA, depth, last);
                }
                if (ssize == 0)
                    return;
                StackEntry entry = stack[--ssize];
                first = entry.a;
                middle = entry.b;
                last = entry.c;
                check = entry.d;
                continue;
            }

            if ((middle - first) <= bufsize)
            {
                if (first < middle)
                {
                    ssMergeForward(PA, buf, bufoffset, first, middle, last, depth);
                }
                if ((check & 1) != 0)
                {
                    ssMergeCheckEqual(PA, depth, first);
                }
                if ((check & 2) != 0)
                {
                    ssMergeCheckEqual(PA, depth, last);
                }
                if (ssize == 0)
                    return;
                StackEntry entry = stack[--ssize];
                first = entry.a;
                middle = entry.b;
                last = entry.c;
                check = entry.d;
                continue;
            }

            for (
                m = 0, len = std::min(middle - first, last - middle), half = len >> 1;
                0 < len;
                len = half, half >>= 1)
            {
                if (ssCompare(PA + getIDX(SA[middle + m + half]),
                              PA + getIDX(SA[middle - m - half - 1]), depth) < 0)
                {
                    m += half + 1;
                    half -= (len & 1) ^ 1;
                }
            }

            if (0 < m)
            {
                ssBlockSwap(SA, middle - m, SA, middle, m);
                i = j = middle;
                next = 0;
                if ((middle + m) < last)
                {
                    if (SA[middle + m] < 0)
                    {
                        for (; SA[i - 1] < 0; --i)
                            ;
                        SA[middle + m] = ~SA[middle + m];
                    }
                    for (j = middle; SA[j] < 0; ++j)
                        ;
                    next = 1;
                }
                if ((i - first) <= (last - j))
                {
                    stack[ssize++] = StackEntry{j, middle + m, last, (check & 2) | (next & 1)};
                    middle -= m;
                    last = i;
                    check = (check & 1);
                }
                else
                {
                    if ((i == middle) && (middle == j))
                    {
                        next <<= 1;
                    }
                    stack[ssize++] = StackEntry{first, middle - m, i, (check & 1) | (next & 2)};
                    first = j;
                    middle += m;
                    check = (check & 2) | (next & 1);
                }
            }
            else
            {
                if ((check & 1) != 0)
                {
                    ssMergeCheckEqual(PA, depth, first);
                }
                ssMergeCheckEqual(PA, depth, middle);
                if ((check & 2) != 0)
                {
                    ssMergeCheckEqual(PA, depth, last);
                }
                if (ssize == 0)
                    return;
                StackEntry entry = stack[--ssize];
                first = entry.a;
                middle = entry.b;
                last = entry.c;
                check = entry.d;
            }
        }
    }

    void subStringSort(int PA, int first, int last, std::vector<int> &buf, int bufoffset, int bufsize, int depth, bool lastsuffix, int size)
    {
        int a, b;
        int curbufoffset;
        int i, j, k;
        int curbufsize;

        if (lastsuffix)
        {
            ++first;
        }
        for (a = first, i = 0; (a + SS_BLOCKSIZE) < last; a += SS_BLOCKSIZE, ++i)
        {
            ssMultiKeyIntroSort(PA, a, a + SS_BLOCKSIZE, depth);
            std::vector<int> *curbuf = &SA;
            curbufoffset = a + SS_BLOCKSIZE;
            curbufsize = last - (a + SS_BLOCKSIZE);
            if (curbufsize <= bufsize)
            {
                curbufsize = bufsize;
                curbuf = &buf;
                curbufoffset = bufoffset;
            }
            for (b = a, k = SS_BLOCKSIZE, j = i; (j & 1) != 0; b -= k, k <<= 1, j >>= 1)
            {
                ssMerge(PA, b - k, b, b + k, *curbuf, curbufoffset, curbufsize, depth);
            }
        }

        ssMultiKeyIntroSort(PA, a, last, depth);

        for (k = SS_BLOCKSIZE; i != 0; k <<= 1, i >>= 1)
        {
            if ((i & 1) != 0)
            {
                ssMerge(PA, a - k, a, last, buf, bufoffset, bufsize, depth);
                a -= k;
            }
        }

        if (lastsuffix)
        {
            int r;
            for (
                a = first, i = SA[first - 1], r = 1;
                (a < last) && ((SA[a] < 0) || (0 < (r = ssCompareLast(PA, PA + i, PA + SA[a], depth, size))));
                ++a)
            {
                SA[a - 1] = SA[a];
            }
            if (r == 0)
            {
                SA[a] = ~SA[a];
            }
            SA[a - 1] = i;
        }
    }

    int trGetC(int ISA, int ISAd, int ISAn, int p)
    {

        return (((ISAd + p) < ISAn) ? SA[ISAd + p] : SA[ISA + ((ISAd - ISA + p) % (ISAn - ISA))]);
    }

    void trFixdown(int ISA, int ISAd, int ISAn, int sa, int i, int size)
    {
        int j, k;
        int v;
        int c, d, e;

        for (v = SA[sa + i], c = trGetC(ISA, ISAd, ISAn, v); (j = 2 * i + 1) < size; SA[sa + i] = SA[sa + k], i = k)
        {
            k = j++;
            d = trGetC(ISA, ISAd, ISAn, SA[sa + k]);
            if (d < (e = trGetC(ISA, ISAd, ISAn, SA[sa + j])))
            {
                k = j;
                d = e;
            }
            if (d <= c)
            {
                break;
            }
        }
        SA[sa + i] = v;
    }

    void trHeapSort(int ISA, int ISAd, int ISAn, int sa, int size)
    {
        int i, m;
        int t;

        m = size;
        if ((size % 2) == 0)
        {
            m--;
            if (trGetC(ISA, ISAd, ISAn, SA[sa + (m / 2)]) < trGetC(ISA, ISAd, ISAn, SA[sa + m]))
            {
                swapElements(SA, sa + m, SA, sa + (m / 2));
            }
        }

        for (i = m / 2 - 1; 0 <= i; --i)
        {
            trFixdown(ISA, ISAd, ISAn, sa, i, m);
        }

        if ((size % 2) == 0)
        {
            swapElements(SA, sa + 0, SA, sa + m);
            trFixdown(ISA, ISAd, ISAn, sa, 0, m);
        }

        for (i = m - 1; 0 < i; --i)
        {
            t = SA[sa + 0];
            SA[sa + 0] = SA[sa + i];
            trFixdown(ISA, ISAd, ISAn, sa, 0, i);
            SA[sa + i] = t;
        }
    }

    void trInsertionSort(int ISA, int ISAd, int ISAn, int first, int last)
    {
        int a, b;
        int t, r;

        for (a = first + 1; a < last; ++a)
        {
            for (t = SA[a], b = a - 1; 0 > (r = trGetC(ISA, ISAd, ISAn, t) - trGetC(ISA, ISAd, ISAn, SA[b]));)
            {
                do
                {
                    SA[b + 1] = SA[b];
                } while ((first <= --b) && (SA[b] < 0));
                if (b < first)
                {
                    break;
                }
            }
            if (r == 0)
            {
                SA[b] = ~SA[b];
            }
            SA[b + 1] = t;
        }
    }

    int trLog(int n)
    {

        return ((n & 0xffff0000) != 0) ? (((n & 0xff000000) != 0) ? 24 + log2table[(n >> 24) & 0xff] : 16 + log2table[(n >> 16) & 0xff])
                                       : (((n & 0x0000ff00) != 0) ? 8 + log2table[(n >> 8) & 0xff] : 0 + log2table[(n >> 0) & 0xff]);
    }

    int trMedian3(int ISA, int ISAd, int ISAn, int v1, int v2, int v3)
    {
        int SA_v1 = trGetC(ISA, ISAd, ISAn, SA[v1]);
        int SA_v2 = trGetC(ISA, ISAd, ISAn, SA[v2]);
        int SA_v3 = trGetC(ISA, ISAd, ISAn, SA[v3]);

        if (SA_v1 > SA_v2)
        {
            int temp = v1;
            v1 = v2;
            v2 = temp;
            int SA_vtemp = SA_v1;
            SA_v1 = SA_v2;
            SA_v2 = SA_vtemp;
        }
        if (SA_v2 > SA_v3)
        {
            if (SA_v1 > SA_v3)
            {
                return v1;
            }
            return v3;
        }

        return v2;
    }

    int trMedian5(int ISA, int ISAd, int ISAn, int v1, int v2, int v3, int v4, int v5)
    {
        int SA_v1 = trGetC(ISA, ISAd, ISAn, SA[v1]);
        int SA_v2 = trGetC(ISA, ISAd, ISAn, SA[v2]);
        int SA_v3 = trGetC(ISA, ISAd, ISAn, SA[v3]);
        int SA_v4 = trGetC(ISA, ISAd, ISAn, SA[v4]);
        int SA_v5 = trGetC(ISA, ISAd, ISAn, SA[v5]);
        int temp;
        int SA_vtemp;

        if (SA_v2 > SA_v3)
        {
            temp = v2;
            v2 = v3;
            v3 = temp;
            SA_vtemp = SA_v2;
            SA_v2 = SA_v3;
            SA_v3 = SA_vtemp;
        }
        if (SA_v4 > SA_v5)
        {
            temp = v4;
            v4 = v5;
            v5 = temp;
            SA_vtemp = SA_v4;
            SA_v4 = SA_v5;
            SA_v5 = SA_vtemp;
        }
        if (SA_v2 > SA_v4)
        {
            temp = v2;
            v2 = v4;
            v4 = temp;
            SA_vtemp = SA_v2;
            SA_v2 = SA_v4;
            SA_v4 = SA_vtemp;
            temp = v3;
            v3 = v5;
            v5 = temp;
            SA_vtemp = SA_v3;
            SA_v3 = SA_v5;
            SA_v5 = SA_vtemp;
        }
        if (SA_v1 > SA_v3)
        {
            temp = v1;
            v1 = v3;
            v3 = temp;
            SA_vtemp = SA_v1;
            SA_v1 = SA_v3;
            SA_v3 = SA_vtemp;
        }
        if (SA_v1 > SA_v4)
        {
            temp = v1;
            v1 = v4;
            v4 = temp;
            SA_vtemp = SA_v1;
            SA_v1 = SA_v4;
            SA_v4 = SA_vtemp;
            temp = v3;
            v3 = v5;
            v5 = temp;
            SA_vtemp = SA_v3;
            SA_v3 = SA_v5;
            SA_v5 = SA_vtemp;
        }
        if (SA_v3 > SA_v4)
        {
            return v4;
        }
        return v3;
    }

    int trPivot(int ISA, int ISAd, int ISAn, int first, int last)
    {
        int middle;
        int t;

        t = last - first;
        middle = first + t / 2;

        if (t <= 512)
        {
            if (t <= 32)
            {
                return trMedian3(ISA, ISAd, ISAn, first, middle, last - 1);
            }
            t >>= 2;
            return trMedian5(
                ISA, ISAd, ISAn,
                first, first + t,
                middle,
                last - 1 - t, last - 1);
        }
        t >>= 3;
        return trMedian3(
            ISA, ISAd, ISAn,
            trMedian3(ISA, ISAd, ISAn, first, first + t, first + (t << 1)),
            trMedian3(ISA, ISAd, ISAn, middle - t, middle, middle + t),
            trMedian3(ISA, ISAd, ISAn, last - 1 - (t << 1), last - 1 - t, last - 1));
    }

    void lsUpdateGroup(int ISA, int first, int last)
    {
        int a, b;
        int t;

        for (a = first; a < last; ++a)
        {
            if (0 <= SA[a])
            {
                b = a;
                do
                {
                    SA[ISA + SA[a]] = a;
                } while ((++a < last) && (0 <= SA[a]));
                SA[b] = b - a;
                if (last <= a)
                {
                    break;
                }
            }
            b = a;
            do
            {
                SA[a] = ~SA[a];
            } while (SA[++a] < 0);
            t = a;
            do
            {
                SA[ISA + SA[b]] = t;
            } while (++b <= a);
        }
    }

    void lsIntroSort(int ISA, int ISAd, int ISAn, int first, int last)
    {
        std::array<StackEntry, STACK_SIZE> stack;
        int a, b, c, d, e, f;
        int s, t;
        int limit;
        int v, x = 0;
        int ssize;

        for (ssize = 0, limit = trLog(last - first);;)
        {

            if ((last - first) <= INSERTIONSORT_THRESHOLD)
            {
                if (1 < (last - first))
                {
                    trInsertionSort(ISA, ISAd, ISAn, first, last);
                    lsUpdateGroup(ISA, first, last);
                }
                else if ((last - first) == 1)
                {
                    SA[first] = -1;
                }
                if (ssize == 0)
                    return;
                StackEntry entry = stack[--ssize];
                first = entry.a;
                last = entry.b;
                limit = entry.c;
                continue;
            }

            if (limit-- == 0)
            {
                trHeapSort(ISA, ISAd, ISAn, first, last - first);
                for (a = last - 1; first < a; a = b)
                {
                    for (
                        x = trGetC(ISA, ISAd, ISAn, SA[a]), b = a - 1;
                        (first <= b) && (trGetC(ISA, ISAd, ISAn, SA[b]) == x);
                        --b)
                    {
                        SA[b] = ~SA[b];
                    }
                }
                lsUpdateGroup(ISA, first, last);
                if (ssize == 0)
                    return;
                StackEntry entry = stack[--ssize];
                first = entry.a;
                last = entry.b;
                limit = entry.c;
                continue;
            }

            a = trPivot(ISA, ISAd, ISAn, first, last);
            swapElements(SA, first, SA, a);
            v = trGetC(ISA, ISAd, ISAn, SA[first]);

            for (b = first; (++b < last) && ((x = trGetC(ISA, ISAd, ISAn, SA[b])) == v);)
                ;
            if (((a = b) < last) && (x < v))
            {
                for (; (++b < last) && ((x = trGetC(ISA, ISAd, ISAn, SA[b])) <= v);)
                {
                    if (x == v)
                    {
                        swapElements(SA, b, SA, a);
                        ++a;
                    }
                }
            }
            for (c = last; (b < --c) && ((x = trGetC(ISA, ISAd, ISAn, SA[c])) == v);)
                ;
            if ((b < (d = c)) && (x > v))
            {
                for (; (b < --c) && ((x = trGetC(ISA, ISAd, ISAn, SA[c])) >= v);)
                {
                    if (x == v)
                    {
                        swapElements(SA, c, SA, d);
                        --d;
                    }
                }
            }
            for (; b < c;)
            {
                swapElements(SA, b, SA, c);
                for (; (++b < c) && ((x = trGetC(ISA, ISAd, ISAn, SA[b])) <= v);)
                {
                    if (x == v)
                    {
                        swapElements(SA, b, SA, a);
                        ++a;
                    }
                }
                for (; (b < --c) && ((x = trGetC(ISA, ISAd, ISAn, SA[c])) >= v);)
                {
                    if (x == v)
                    {
                        swapElements(SA, c, SA, d);
                        --d;
                    }
                }
            }

            if (a <= d)
            {
                c = b - 1;

                if ((s = a - first) > (t = b - a))
                {
                    s = t;
                }
                for (e = first, f = b - s; 0 < s; --s, ++e, ++f)
                {
                    swapElements(SA, e, SA, f);
                }
                if ((s = d - c) > (t = last - d - 1))
                {
                    s = t;
                }
                for (e = b, f = last - s; 0 < s; --s, ++e, ++f)
                {
                    swapElements(SA, e, SA, f);
                }

                a = first + (b - a);
                b = last - (d - c);

                for (c = first, v = a - 1; c < a; ++c)
                {
                    SA[ISA + SA[c]] = v;
                }
                if (b < last)
                {
                    for (c = a, v = b - 1; c < b; ++c)
                    {
                        SA[ISA + SA[c]] = v;
                    }
                }
                if ((b - a) == 1)
                {
                    SA[a] = -1;
                }

                if ((a - first) <= (last - b))
                {
                    if (first < a)
                    {
                        stack[ssize++] = StackEntry{b, last, limit, 0};
                        last = a;
                    }
                    else
                    {
                        first = b;
                    }
                }
                else
                {
                    if (b < last)
                    {
                        stack[ssize++] = StackEntry{first, a, limit, 0};
                        first = b;
                    }
                    else
                    {
                        last = a;
                    }
                }
            }
            else
            {
                if (ssize == 0)
                    return;
                StackEntry entry = stack[--ssize];
                first = entry.a;
                last = entry.b;
                limit = entry.c;
            }
        }
    }

    void lsSort(int ISA, int n, int depth)
    {
        int ISAd;
        int first, last, i;
        int t, skip;

        for (ISAd = ISA + depth; -n < SA[0]; ISAd += (ISAd - ISA))
        {
            first = 0;
            skip = 0;
            do
            {
                if ((t = SA[first]) < 0)
                {
                    first -= t;
                    skip += t;
                }
                else
                {
                    if (skip != 0)
                    {
                        SA[first + skip] = skip;
                        skip = 0;
                    }
                    last = SA[ISA + t] + 1;
                    lsIntroSort(ISA, ISAd, ISA + n, first, last);
                    first = last;
                }
            } while (first < n);
            if (skip != 0)
            {
                SA[first + skip] = skip;
            }
            if (n < (ISAd - ISA))
            {
                first = 0;
                do
                {
                    if ((t = SA[first]) < 0)
                    {
                        first -= t;
                    }
                    else
                    {
                        last = SA[ISA + t] + 1;
                        for (i = first; i < last; ++i)
                        {
                            SA[ISA + SA[i]] = i;
                        }
                        first = last;
                    }
                } while (first < n);
                break;
            }
        }
    }

    struct PartitionResult
    {
        int first;
        int last;
    };

    PartitionResult
    trPartition(int ISA, int ISAd, int ISAn, int first, int last, int v)
    {
        int a, b, c, d, e, f;
        int t, s;
        int x = 0;

        for (b = first - 1; (++b < last) && ((x = trGetC(ISA, ISAd, ISAn, SA[b])) == v);)
            ;
        if (((a = b) < last) && (x < v))
        {
            for (; (++b < last) && ((x = trGetC(ISA, ISAd, ISAn, SA[b])) <= v);)
            {
                if (x == v)
                {
                    swapElements(SA, b, SA, a);
                    ++a;
                }
            }
        }
        for (c = last; (b < --c) && ((x = trGetC(ISA, ISAd, ISAn, SA[c])) == v);)
            ;
        if ((b < (d = c)) && (x > v))
        {
            for (; (b < --c) && ((x = trGetC(ISA, ISAd, ISAn, SA[c])) >= v);)
            {
                if (x == v)
                {
                    swapElements(SA, c, SA, d);
                    --d;
                }
            }
        }
        for (; b < c;)
        {
            swapElements(SA, b, SA, c);
            for (; (++b < c) && ((x = trGetC(ISA, ISAd, ISAn, SA[b])) <= v);)
            {
                if (x == v)
                {
                    swapElements(SA, b, SA, a);
                    ++a;
                }
            }
            for (; (b < --c) && ((x = trGetC(ISA, ISAd, ISAn, SA[c])) >= v);)
            {
                if (x == v)
                {
                    swapElements(SA, c, SA, d);
                    --d;
                }
            }
        }

        if (a <= d)
        {
            c = b - 1;
            if ((s = a - first) > (t = b - a))
            {
                s = t;
            }
            for (e = first, f = b - s; 0 < s; --s, ++e, ++f)
            {
                swapElements(SA, e, SA, f);
            }
            if ((s = d - c) > (t = last - d - 1))
            {
                s = t;
            }
            for (e = b, f = last - s; 0 < s; --s, ++e, ++f)
            {
                swapElements(SA, e, SA, f);
            }
            first += (b - a);
            last -= (d - c);
        }

        return PartitionResult{first, last};
    }

    void trCopy(int ISA, int ISAn, int first, int a, int b, int last, int depth)
    {
        int c, d, e;
        int s, v;

        v = b - 1;

        for (c = first, d = a - 1; c <= d; ++c)
        {
            if ((s = SA[c] - depth) < 0)
            {
                s += ISAn - ISA;
            }
            if (SA[ISA + s] == v)
            {
                SA[++d] = s;
                SA[ISA + s] = d;
            }
        }
        for (c = last - 1, e = d + 1, d = b; e < d; --c)
        {
            if ((s = SA[c] - depth) < 0)
            {
                s += ISAn - ISA;
            }
            if (SA[ISA + s] == v)
            {
                SA[--d] = s;
                SA[ISA + s] = d;
            }
        }
    }

    struct TRBudget
    {
        int budget;
        int chance;

        bool update(int size, int n)
        {
            budget -= n;
            if (budget <= 0)
            {
                if (--chance == 0)
                {
                    return false;
                }
                budget += size;
            }
            return true;
        }
    };

    void trIntroSort(int ISA, int ISAd, int ISAn, int first, int last, TRBudget budget, int size)
    {

        std::array<StackEntry, STACK_SIZE> stack;

        int a, b, c, d, e, f;
        int s, t;
        int v, x = 0;
        int limit, next;
        int ssize;

        for (ssize = 0, limit = trLog(last - first);;)
        {
            if (limit < 0)
            {
                if (limit == -1)
                {
                    if (!budget.update(size, last - first))
                        break;
                    PartitionResult result = trPartition(ISA, ISAd - 1, ISAn, first, last, last - 1);
                    a = result.first;
                    b = result.last;
                    if ((first < a) || (b < last))
                    {
                        if (a < last)
                        {
                            for (c = first, v = a - 1; c < a; ++c)
                            {
                                SA[ISA + SA[c]] = v;
                            }
                        }
                        if (b < last)
                        {
                            for (c = a, v = b - 1; c < b; ++c)
                            {
                                SA[ISA + SA[c]] = v;
                            }
                        }

                        stack[ssize++] = StackEntry{0, a, b, 0};
                        stack[ssize++] = StackEntry{ISAd - 1, first, last, -2};
                        if ((a - first) <= (last - b))
                        {
                            if (1 < (a - first))
                            {
                                stack[ssize++] = StackEntry{ISAd, b, last, trLog(last - b)};
                                last = a;
                                limit = trLog(a - first);
                            }
                            else if (1 < (last - b))
                            {
                                first = b;
                                limit = trLog(last - b);
                            }
                            else
                            {
                                if (ssize == 0)
                                    return;
                                StackEntry entry = stack[--ssize];
                                ISAd = entry.a;
                                first = entry.b;
                                last = entry.c;
                                limit = entry.d;
                            }
                        }
                        else
                        {
                            if (1 < (last - b))
                            {
                                stack[ssize++] = StackEntry{ISAd, first, a, trLog(a - first)};
                                first = b;
                                limit = trLog(last - b);
                            }
                            else if (1 < (a - first))
                            {
                                last = a;
                                limit = trLog(a - first);
                            }
                            else
                            {
                                if (ssize == 0)
                                    return;
                                StackEntry entry = stack[--ssize];
                                ISAd = entry.a;
                                first = entry.b;
                                last = entry.c;
                                limit = entry.d;
                            }
                        }
                    }
                    else
                    {
                        for (c = first; c < last; ++c)
                        {
                            SA[ISA + SA[c]] = c;
                        }
                        if (ssize == 0)
                            return;
                        StackEntry entry = stack[--ssize];
                        ISAd = entry.a;
                        first = entry.b;
                        last = entry.c;
                        limit = entry.d;
                    }
                }
                else if (limit == -2)
                {
                    a = stack[--ssize].b;
                    b = stack[ssize].c;
                    trCopy(ISA, ISAn, first, a, b, last, ISAd - ISA);
                    if (ssize == 0)
                        return;
                    StackEntry entry = stack[--ssize];
                    ISAd = entry.a;
                    first = entry.b;
                    last = entry.c;
                    limit = entry.d;
                }
                else
                {
                    if (0 <= SA[first])
                    {
                        a = first;
                        do
                        {
                            SA[ISA + SA[a]] = a;
                        } while ((++a < last) && (0 <= SA[a]));
                        first = a;
                    }
                    if (first < last)
                    {
                        a = first;
                        do
                        {
                            SA[a] = ~SA[a];
                        } while (SA[++a] < 0);
                        next = (SA[ISA + SA[a]] != SA[ISAd + SA[a]]) ? trLog(a - first + 1) : -1;
                        if (++a < last)
                        {
                            for (b = first, v = a - 1; b < a; ++b)
                            {
                                SA[ISA + SA[b]] = v;
                            }
                        }

                        if ((a - first) <= (last - a))
                        {
                            stack[ssize++] = StackEntry{ISAd, a, last, -3};
                            ISAd += 1;
                            last = a;
                            limit = next;
                        }
                        else
                        {
                            if (1 < (last - a))
                            {
                                stack[ssize++] = StackEntry{ISAd + 1, first, a, next};
                                first = a;
                                limit = -3;
                            }
                            else
                            {
                                ISAd += 1;
                                last = a;
                                limit = next;
                            }
                        }
                    }
                    else
                    {
                        if (ssize == 0)
                            return;
                        StackEntry entry = stack[--ssize];
                        ISAd = entry.a;
                        first = entry.b;
                        last = entry.c;
                        limit = entry.d;
                    }
                }
                continue;
            }

            if ((last - first) <= INSERTIONSORT_THRESHOLD)
            {
                if (!budget.update(size, last - first))
                    break;
                trInsertionSort(ISA, ISAd, ISAn, first, last);
                limit = -3;
                continue;
            }

            if (limit-- == 0)
            {
                if (!budget.update(size, last - first))
                    break;
                trHeapSort(ISA, ISAd, ISAn, first, last - first);
                for (a = last - 1; first < a; a = b)
                {
                    for (
                        x = trGetC(ISA, ISAd, ISAn, SA[a]), b = a - 1;
                        (first <= b) && (trGetC(ISA, ISAd, ISAn, SA[b]) == x);
                        --b)
                    {
                        SA[b] = ~SA[b];
                    }
                }
                limit = -3;
                continue;
            }

            a = trPivot(ISA, ISAd, ISAn, first, last);

            swapElements(SA, first, SA, a);
            v = trGetC(ISA, ISAd, ISAn, SA[first]);
            for (b = first; (++b < last) && ((x = trGetC(ISA, ISAd, ISAn, SA[b])) == v);)
                ;
            if (((a = b) < last) && (x < v))
            {
                for (; (++b < last) && ((x = trGetC(ISA, ISAd, ISAn, SA[b])) <= v);)
                {
                    if (x == v)
                    {
                        swapElements(SA, b, SA, a);
                        ++a;
                    }
                }
            }
            for (c = last; (b < --c) && ((x = trGetC(ISA, ISAd, ISAn, SA[c])) == v);)
                ;
            if ((b < (d = c)) && (x > v))
            {
                for (; (b < --c) && ((x = trGetC(ISA, ISAd, ISAn, SA[c])) >= v);)
                {
                    if (x == v)
                    {
                        swapElements(SA, c, SA, d);
                        --d;
                    }
                }
            }
            for (; b < c;)
            {
                swapElements(SA, b, SA, c);
                for (; (++b < c) && ((x = trGetC(ISA, ISAd, ISAn, SA[b])) <= v);)
                {
                    if (x == v)
                    {
                        swapElements(SA, b, SA, a);
                        ++a;
                    }
                }
                for (; (b < --c) && ((x = trGetC(ISA, ISAd, ISAn, SA[c])) >= v);)
                {
                    if (x == v)
                    {
                        swapElements(SA, c, SA, d);
                        --d;
                    }
                }
            }

            if (a <= d)
            {
                c = b - 1;

                if ((s = a - first) > (t = b - a))
                {
                    s = t;
                }
                for (e = first, f = b - s; 0 < s; --s, ++e, ++f)
                {
                    swapElements(SA, e, SA, f);
                }
                if ((s = d - c) > (t = last - d - 1))
                {
                    s = t;
                }
                for (e = b, f = last - s; 0 < s; --s, ++e, ++f)
                {
                    swapElements(SA, e, SA, f);
                }

                a = first + (b - a);
                b = last - (d - c);
                next = (SA[ISA + SA[a]] != v) ? trLog(b - a) : -1;

                for (c = first, v = a - 1; c < a; ++c)
                {
                    SA[ISA + SA[c]] = v;
                }
                if (b < last)
                {
                    for (c = a, v = b - 1; c < b; ++c)
                    {
                        SA[ISA + SA[c]] = v;
                    }
                }

                if ((a - first) <= (last - b))
                {
                    if ((last - b) <= (b - a))
                    {
                        if (1 < (a - first))
                        {
                            stack[ssize++] = StackEntry{ISAd + 1, a, b, next};
                            stack[ssize++] = StackEntry{ISAd, b, last, limit};
                            last = a;
                        }
                        else if (1 < (last - b))
                        {
                            stack[ssize++] = StackEntry{ISAd + 1, a, b, next};
                            first = b;
                        }
                        else if (1 < (b - a))
                        {
                            ISAd += 1;
                            first = a;
                            last = b;
                            limit = next;
                        }
                        else
                        {
                            if (ssize == 0)
                                return;
                            StackEntry entry = stack[--ssize];
                            ISAd = entry.a;
                            first = entry.b;
                            last = entry.c;
                            limit = entry.d;
                        }
                    }
                    else if ((a - first) <= (b - a))
                    {
                        if (1 < (a - first))
                        {
                            stack[ssize++] = StackEntry{ISAd, b, last, limit};
                            stack[ssize++] = StackEntry{ISAd + 1, a, b, next};
                            last = a;
                        }
                        else if (1 < (b - a))
                        {
                            stack[ssize++] = StackEntry{ISAd, b, last, limit};
                            ISAd += 1;
                            first = a;
                            last = b;
                            limit = next;
                        }
                        else
                        {
                            first = b;
                        }
                    }
                    else
                    {
                        if (1 < (b - a))
                        {
                            stack[ssize++] = StackEntry{ISAd, b, last, limit};
                            stack[ssize++] = StackEntry{ISAd, first, a, limit};
                            ISAd += 1;
                            first = a;
                            last = b;
                            limit = next;
                        }
                        else
                        {
                            stack[ssize++] = StackEntry{ISAd, b, last, limit};
                            last = a;
                        }
                    }
                }
                else
                {
                    if ((a - first) <= (b - a))
                    {
                        if (1 < (last - b))
                        {
                            stack[ssize++] = StackEntry{ISAd + 1, a, b, next};
                            stack[ssize++] = StackEntry{ISAd, first, a, limit};
                            first = b;
                        }
                        else if (1 < (a - first))
                        {
                            stack[ssize++] = StackEntry{ISAd + 1, a, b, next};
                            last = a;
                        }
                        else if (1 < (b - a))
                        {
                            ISAd += 1;
                            first = a;
                            last = b;
                            limit = next;
                        }
                        else
                        {
                            stack[ssize++] = StackEntry{ISAd, first, last, limit};
                        }
                    }
                    else if ((last - b) <= (b - a))
                    {
                        if (1 < (last - b))
                        {
                            stack[ssize++] = StackEntry{ISAd, first, a, limit};
                            stack[ssize++] = StackEntry{ISAd + 1, a, b, next};
                            first = b;
                        }
                        else if (1 < (b - a))
                        {
                            stack[ssize++] = StackEntry{ISAd, first, a, limit};
                            ISAd += 1;
                            first = a;
                            last = b;
                            limit = next;
                        }
                        else
                        {
                            last = a;
                        }
                    }
                    else
                    {
                        if (1 < (b - a))
                        {
                            stack[ssize++] = StackEntry{ISAd, first, a, limit};
                            stack[ssize++] = StackEntry{ISAd, b, last, limit};
                            ISAd += 1;
                            first = a;
                            last = b;
                            limit = next;
                        }
                        else
                        {
                            stack[ssize++] = StackEntry{ISAd, first, a, limit};
                            first = b;
                        }
                    }
                }
            }
            else
            {
                if (!budget.update(size, last - first))
                    break; // BUGFIX : Added to prevent an infinite loop in the original code
                limit += 1;
                ISAd += 1;
            }
        }

        for (s = 0; s < ssize; ++s)
        {
            if (stack[s].d == -3)
            {
                lsUpdateGroup(ISA, stack[s].b, stack[s].c);
            }
        }
    }

    void trSort(int ISA, int n, int depth)
    {
        int first = 0, last;
        int t;

        if (-n < SA[0])
        {
            TRBudget budget{n, trLog(n) * 2 / 3 + 1};
            do
            {
                if ((t = SA[first]) < 0)
                {
                    first -= t;
                }
                else
                {
                    last = SA[ISA + t] + 1;
                    if (1 < (last - first))
                    {
                        trIntroSort(ISA, ISA + depth, ISA + n, first, last, budget, n);
                        if (budget.chance == 0)
                        {
                            if (0 < first)
                            {
                                SA[0] = -first;
                            }
                            lsSort(ISA, n, depth);
                            break;
                        }
                    }
                    first = last;
                }
            } while (first < n);
        }
    }

    int BUCKET_B(int c0, int c1)
    {

        return (c1 << 8) | c0;
    }

    int BUCKET_BSTAR(int c0, int c1)
    {

        return (c0 << 8) | c1;
    }

    int sortTypeBstar(std::vector<int> &bucketA, std::vector<int> &bucketB)
    {
        std::vector<int> tempbuf(256);

        int PAb, ISAb, bufoffset;
        int i, j, k, t, m, bufsize;
        int c0, c1;
        int flag;

        for (i = 1, flag = 1; i < n; ++i)
        {
            if (T[i - 1] != T[i])
            {
                if ((T[i - 1] & 0xff) > (T[i] & 0xff))
                {
                    flag = 0;
                }
                break;
            }
        }
        i = n - 1;
        m = n;

        int ti, ti1, t0;
        if (((ti = (T[i] & 0xff)) < (t0 = (T[0] & 0xff))) || ((T[i] == T[0]) && (flag != 0)))
        {
            if (flag == 0)
            {
                ++bucketB[BUCKET_BSTAR(ti, t0)];
                SA[--m] = i;
            }
            else
            {
                ++bucketB[BUCKET_B(ti, t0)];
            }
            for (--i; (0 <= i) && ((ti = (T[i] & 0xff)) <= (ti1 = (T[i + 1] & 0xff))); --i)
            {
                ++bucketB[BUCKET_B(ti, ti1)];
            }
        }

        for (; 0 <= i;)
        {
            do
            {
                ++bucketA[T[i] & 0xff];
            } while ((0 <= --i) && ((T[i] & 0xff) >= (T[i + 1] & 0xff)));
            if (0 <= i)
            {
                ++bucketB[BUCKET_BSTAR(T[i] & 0xff, T[i + 1] & 0xff)];
                SA[--m] = i;
                for (--i; (0 <= i) && ((ti = (T[i] & 0xff)) <= (ti1 = (T[i + 1] & 0xff))); --i)
                {
                    ++bucketB[BUCKET_B(ti, ti1)];
                }
            }
        }
        m = n - m;
        if (m == 0)
        {
            for (i = 0; i < n; ++i)
            {
                SA[i] = i;
            }
            return 0;
        }

        for (c0 = 0, i = -1, j = 0; c0 < 256; ++c0)
        {
            t = i + bucketA[c0];
            bucketA[c0] = i + j;
            i = t + bucketB[BUCKET_B(c0, c0)];
            for (c1 = c0 + 1; c1 < 256; ++c1)
            {
                j += bucketB[BUCKET_BSTAR(c0, c1)];
                bucketB[(c0 << 8) | c1] = j;
                i += bucketB[BUCKET_B(c0, c1)];
            }
        }

        PAb = n - m;
        ISAb = m;
        for (i = m - 2; 0 <= i; --i)
        {
            t = SA[PAb + i];
            c0 = T[t] & 0xff;
            c1 = T[t + 1] & 0xff;
            SA[--bucketB[BUCKET_BSTAR(c0, c1)]] = i;
        }
        t = SA[PAb + m - 1];
        c0 = T[t] & 0xff;
        c1 = T[t + 1] & 0xff;
        SA[--bucketB[BUCKET_BSTAR(c0, c1)]] = m - 1;

        std::vector<int> *buf = &SA;
        bufoffset = m;
        bufsize = n - (2 * m);
        if (bufsize <= 256)
        {
            buf = &tempbuf;
            bufoffset = 0;
            bufsize = 256;
        }

        for (c0 = 255, j = m; 0 < j; --c0)
        {
            for (c1 = 255; c0 < c1; j = i, --c1)
            {
                i = bucketB[BUCKET_BSTAR(c0, c1)];
                if (1 < (j - i))
                {
                    subStringSort(PAb, i, j, *buf, bufoffset, bufsize, 2, SA[i] == (m - 1), n);
                }
            }
        }

        for (i = m - 1; 0 <= i; --i)
        {
            if (0 <= SA[i])
            {
                j = i;
                do
                {
                    SA[ISAb + SA[i]] = i;
                } while ((0 <= --i) && (0 <= SA[i]));
                SA[i + 1] = i - j;
                if (i <= 0)
                {
                    break;
                }
            }
            j = i;
            do
            {
                SA[ISAb + (SA[i] = ~SA[i])] = j;
            } while (SA[--i] < 0);
            SA[ISAb + SA[i]] = j;
        }

        trSort(ISAb, m, 1);

        i = n - 1;
        j = m;
        if (((T[i] & 0xff) < (T[0] & 0xff)) || ((T[i] == T[0]) && (flag != 0)))
        {
            if (flag == 0)
            {
                SA[SA[ISAb + --j]] = i;
            }
            for (--i; (0 <= i) && ((T[i] & 0xff) <= (T[i + 1] & 0xff)); --i)
                ;
        }
        for (; 0 <= i;)
        {
            for (--i; (0 <= i) && ((T[i] & 0xff) >= (T[i + 1] & 0xff)); --i)
                ;
            if (0 <= i)
            {
                SA[SA[ISAb + --j]] = i;
                for (--i; (0 <= i) && ((T[i] & 0xff) <= (T[i + 1] & 0xff)); --i)
                    ;
            }
        }

        for (c0 = 255, i = n - 1, k = m - 1; 0 <= c0; --c0)
        {
            for (c1 = 255; c0 < c1; --c1)
            {
                t = i - bucketB[BUCKET_B(c0, c1)];
                bucketB[BUCKET_B(c0, c1)] = i + 1;

                for (i = t, j = bucketB[BUCKET_BSTAR(c0, c1)]; j <= k; --i, --k)
                {
                    SA[i] = SA[k];
                }
            }
            t = i - bucketB[BUCKET_B(c0, c0)];
            bucketB[BUCKET_B(c0, c0)] = i + 1;
            if (c0 < 255)
            {
                bucketB[BUCKET_BSTAR(c0, c0 + 1)] = t + 1;
            }
            i = bucketA[c0];
        }

        return m;
    }

    int constructBWT(std::vector<int> &bucketA, std::vector<int> &bucketB)
    {
        int i, j, t = 0;
        int s, s1;
        int c0 = 0, c1, c2 = 0;
        int orig = -1;

        for (c1 = 254; 0 <= c1; --c1)
        {
            for (
                i = bucketB[BUCKET_BSTAR(c1, c1 + 1)], j = bucketA[c1 + 1], t = 0, c2 = -1;
                i <= j;
                --j)
            {
                if (0 <= (s1 = s = SA[j]))
                {
                    if (--s < 0)
                    {
                        s = n - 1;
                    }
                    if ((c0 = (T[s] & 0xff)) <= c1)
                    {
                        SA[j] = ~s1;
                        if ((0 < s) && ((T[s - 1] & 0xff) > c0))
                        {
                            s = ~s;
                        }
                        if (c2 == c0)
                        {
                            SA[--t] = s;
                        }
                        else
                        {
                            if (0 <= c2)
                            {
                                bucketB[BUCKET_B(c2, c1)] = t;
                            }
                            SA[t = bucketB[BUCKET_B(c2 = c0, c1)] - 1] = s;
                        }
                    }
                }
                else
                {
                    SA[j] = ~s;
                }
            }
        }

        for (i = 0; i < n; ++i)
        {
            if (0 <= (s1 = s = SA[i]))
            {
                if (--s < 0)
                {
                    s = n - 1;
                }
                if ((c0 = (T[s] & 0xff)) >= (T[s + 1] & 0xff))
                {
                    if ((0 < s) && ((T[s - 1] & 0xff) < c0))
                    {
                        s = ~s;
                    }
                    if (c0 == c2)
                    {
                        SA[++t] = s;
                    }
                    else
                    {
                        if (c2 != -1) // BUGFIX: Original code can write to bucketA[-1]
                            bucketA[c2] = t;
                        SA[t = bucketA[c2 = c0] + 1] = s;
                    }
                }
            }
            else
            {
                s1 = ~s1;
            }

            if (s1 == 0)
            {
                SA[i] = T[n - 1];
                orig = i;
            }
            else
            {
                SA[i] = T[s1 - 1];
            }
        }

        return orig;
    }
};
#endif