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

#ifndef INPUT_STREAM_HPP
#define INPUT_STREAM_HPP

#include <istream>
#include <vector>
#include <memory>

#include "Config.hpp"
#include "CRC32.hpp"
#include "BitInputStream.hpp"
#include "BlockDecompressor.hpp"

class InputStream
{
private:
    std::istream &inputStream;
    BitInputStream bitInputStream;
    bool streamComplete = false;
    int streamBlockSize{};
    int streamCRC = 0;
    std::unique_ptr<BlockDecompressor> blockDecompressor;

public:
    InputStream(std::istream &in) : inputStream(in), bitInputStream(in)
    {
    }

    int read()
    {
        int nextByte = -1;
        if (!blockDecompressor)
        {
            initializeStream();
        }
        else
        {
            nextByte = blockDecompressor->read();
        }

        if (nextByte == -1)
        {
            if (initializeNextBlock())
            {
                nextByte = blockDecompressor->read();
            }
        }

        return nextByte;
    }

    int read(std::vector<uint8_t> &buffer, int offset, int length)
    {
        int bytesRead = -1;
        if (!blockDecompressor)
        {
            initializeStream();
        }
        else
        {
            bytesRead = blockDecompressor->read(buffer, offset, length);
        }

        if (bytesRead == -1)
        {
            if (initializeNextBlock())
            {
                bytesRead = blockDecompressor->read(buffer, offset, length);
            }
        }

        return bytesRead;
    }

    void close()
    {
        streamComplete = true;
        blockDecompressor.reset(nullptr);
    }

private:
    void initializeStream()
    {
        if (streamComplete)
        {
            return;
        }

        int marker1 = bitInputStream.readBits(16);
        int marker2 = bitInputStream.readBits(8);
        int blockSize = bitInputStream.readBits(8) - '0';

        if (marker1 != STREAM_START_MARKER_1 ||
            marker2 != STREAM_START_MARKER_2 ||
            blockSize < 1 || blockSize > 9)
        {
            throw std::runtime_error("Invalid BZip2 header");
        }

        streamBlockSize = blockSize * 100000;
    }

    bool initializeNextBlock()
    {
        if (streamComplete)
            return false;
        if (blockDecompressor)
        {
            int blockCRC = blockDecompressor->checkCRC();
            streamCRC = ((streamCRC << 1) | (static_cast<unsigned int>(streamCRC) >> 31)) ^ blockCRC;
        }

        int marker1 = bitInputStream.readBits(24);
        int marker2 = bitInputStream.readBits(24);

        if (marker1 == BLOCK_HEADER_MARKER_1 && marker2 == BLOCK_HEADER_MARKER_2)
        {
            blockDecompressor.reset(new BlockDecompressor(bitInputStream, streamBlockSize));
            return true;
        }
        else if (marker1 == STREAM_END_MARKER_1 && marker2 == STREAM_END_MARKER_2)
        {
            streamComplete = true;
            int storedCRC = bitInputStream.readInteger();
            if (storedCRC != streamCRC)
            {
                throw std::runtime_error("BZip2 stream CRC error");
            }
            return false;
        }
        else
        {
            streamComplete = true;
            throw std::runtime_error("BZip2 stream format error");
        }
    }
};

#endif