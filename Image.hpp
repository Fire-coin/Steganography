#pragma once
#ifndef IMAGE_HPP
#define IMAGE_HPP

#include <vector>
#include <utility>
#include <stdint.h>

struct chunk
{
    uint32_t length;
    unsigned char type[4];
    unsigned char* data;
    unsigned char crc[4];
};

class Image {
    std::vector<std::pair<unsigned char*, uint32_t>> IDAT_chunks; // Pointer to the begging of IDAT file and it's size
    std::vector<chunk> other_chunks;
    uint32_t IDAT_size;
    public:
        Image();
        ~Image();
        void addIDATChunk(unsigned char* data, uint32_t size);
        void addChunk(const chunk& chunk);
        const std::vector<std::pair<unsigned char*, uint32_t>>& getIDATChunks();
        const std::vector<chunk>& getOtherChunks();
        uint32_t getIDATSize();
};
#endif