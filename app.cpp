#include <iostream>
#include <fstream>
#include <vector>
#include <cstring>

#include "include/OutputStream.hpp"
#include "include/InputStream.hpp"

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        std::cerr << "Usage: " << argv[0] << " <filename> [--dec|-d] [--keep|-k] [--size|-s <1-9>] [--check|-c]" << std::endl;
        return 1;
    }

    std::string filename = argv[1];
    std::ifstream inputFile(filename, std::ios::binary);
    if (!inputFile.is_open())
    {
        std::cerr << "Failed to open input file." << std::endl;
        return 1;
    }

    bool decompress = false;
    bool keepFile = false;
    bool checkCRC = false;
    int blockSize = 9; // Default block size

    // Parse command-line arguments
    for (int i = 2; i < argc; ++i)
    {
        if (std::strcmp(argv[i], "--dec") == 0 || std::strcmp(argv[i], "-d") == 0)
        {
            decompress = true;
        }
        else if (std::strcmp(argv[i], "--keep") == 0 || std::strcmp(argv[i], "-k") == 0)
        {
            keepFile = true;
        }
        else if ((std::strcmp(argv[i], "--size") == 0 || std::strcmp(argv[i], "-s") == 0) && i + 1 < argc)
        {
            blockSize = std::atoi(argv[++i]);
            if (blockSize < 1 || blockSize > 9)
            {
                std::cerr << "Invalid block size. Must be between 1 and 9." << std::endl;
                return 1;
            }
        }
        else if (std::strcmp(argv[i], "--check") == 0 || std::strcmp(argv[i], "-c") == 0)
        {
            checkCRC = true;
        }
    }

    if (!decompress)
    {
        std::string outputFilename = filename + ".bz2";
        std::ofstream outputFile(outputFilename, std::ios::binary);
        if (!outputFile.is_open())
        {
            std::cerr << "Failed to open output file.";
            return 1;
        }

        OutputStream bz2out(outputFile, blockSize);

        const size_t bufferSize = 1000000;
        std::vector<char> buffer(bufferSize);

        while (inputFile)
        {
            inputFile.read(buffer.data(), bufferSize);
            std::streamsize bytes_read = inputFile.gcount();
            for (int i = 0; i < bytes_read; ++i)
            {
                bz2out.write(buffer[i]);
            }
        }
        bz2out.close();

        if (!keepFile)
        {
            remove(filename.c_str());
        }

        outputFile.close();
    }
    else
    {
        if (!checkCRC)
        {
            if (filename.size() < 4 || filename.substr(filename.size() - 4) != ".bz2")
            {
                std::cerr << "Input file has wrong extension for decompression!" << std::endl;
                return 1;
            }

            std::string outputFilename = filename.substr(0, filename.size() - 4); // Remove ".bz2"
            if (!checkCRC && std::ifstream(outputFilename))
            {
                outputFilename += ".out";
                std::cerr << "Warning: Output file exists. Writing to " << outputFilename << std::endl;
            }
            std::ofstream outputFile(outputFilename, std::ios::binary);
            if (!outputFile.is_open())
            {
                std::cerr << "Failed to open output file." << std::endl;
                return 1;
            }

            InputStream bz2in(inputFile);

            int ch;
            while ((ch = bz2in.read()) != -1)
            {
                char x = ch;
                outputFile.write(&x, 1);
            }

            if (!keepFile)
            {
                remove(filename.c_str());
            }

            outputFile.close();
        }
    }

    inputFile.close();

    return 0;
}
