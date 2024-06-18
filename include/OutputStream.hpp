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

#ifndef OUTPUT_STREAM_HPP
#define OUTPUT_STREAM_HPP

#include <ostream>
#include <memory>
#include <bitset>

#include "BitOutputStream.hpp"
#include "BlockCompressor.hpp"

class OutputStream
{
private:
    std::ostream &outputStream;
    bool streamFinished = false;
    int streamBlockSize;
    int streamCRC = 0;
    BitOutputStream blockOutputStream{};
    std::unique_ptr<BlockCompressor> blockCompressor{};

public:
    OutputStream(std::ostream &out, int blockSizeMultiplier) : outputStream(out), streamBlockSize(blockSizeMultiplier * 100000)
    {
        if (blockSizeMultiplier < 1 || blockSizeMultiplier > 9)
        {
            throw std::invalid_argument("Invalid block size");
        }
        // Initial block has stream start info
        blockOutputStream.writeBits(16, STREAM_START_MARKER_1);
        blockOutputStream.writeBits(8, STREAM_START_MARKER_2);
        blockOutputStream.writeBits(8, '0' + blockSizeMultiplier);
        initialiseNextBlock();
    }

    void write(int value)
    {
        if (streamFinished)
        {
            throw std::runtime_error("Write beyond end of stream");
        }
        if (!blockCompressor->write(value & 0xff))
        {
            closeBlock();
            initialiseNextBlock();
            blockCompressor->write(value & 0xff);
        }
    }

    void write(const std::vector<char> &data, int offset, int length)
    {
        if (streamFinished)
        {
            throw std::runtime_error("Write beyond end of stream");
        }

        int bytesWritten = 0;
        while (length > 0)
        {
            if ((bytesWritten = blockCompressor->write(data, offset, length)) < length)
            {
                closeBlock();
                initialiseNextBlock();
            }
            offset += bytesWritten;
            length -= bytesWritten;
        }
    }

    void close()
    {
        if (!streamFinished)
        {
            streamFinished = true;
            closeBlock();
            blockOutputStream.writeBits(24, STREAM_END_MARKER_1);
            blockOutputStream.writeBits(24, STREAM_END_MARKER_2);
            blockOutputStream.writeInteger(streamCRC);
            blockOutputStream.padding();
            blockOutputStream.writeFileBytes(outputStream);
            outputStream.flush();
        }
    }

private:
    void initialiseNextBlock()
    {
        blockCompressor.reset(new BlockCompressor(blockOutputStream, streamBlockSize));
    }

    void closeBlock()
    {
        if (!blockCompressor->isEmpty())
        {
            blockCompressor->close();
            blockOutputStream.writeFileBytes(outputStream);
            int blockCRC = blockCompressor->getCRC();
            streamCRC = ((streamCRC << 1) | (static_cast<unsigned int>(streamCRC) >> 31)) ^ blockCRC;
        }
    }
};
#endif