#include "Image.hpp"
#include <memory.h>

Image::Image() {
    this->IDAT_chunks.reserve(10);
    this->other_chunks.reserve(5);
    this->IDAT_size = 0;
}

Image::~Image() {
    for (auto& chunk : this->IDAT_chunks) {
        delete[] chunk.first;
    }

    for (auto& chunk : this->other_chunks) {
        delete[] chunk.data;
    }
}

void Image::addIDATChunk(unsigned char* data, uint32_t size) {
    unsigned char* dataCopy = new unsigned char[size];
    memcpy(dataCopy, data, size); 
    this->IDAT_chunks.emplace_back(std::pair<unsigned char*, uint32_t>(dataCopy, size));
    this->IDAT_size += size;
}

void Image::addChunk(const chunk& c) { 
    chunk tempChunk{};

    tempChunk.length = c.length;
    memcpy(tempChunk.type, c.type, 4);
    memcpy(tempChunk.crc, c.crc, 4);
    tempChunk.data = new uint8_t[c.length];
    memcpy(tempChunk.data, c.data, c.length);

    this->other_chunks.emplace_back(tempChunk);
}


const std::vector<std::pair<unsigned char*, uint32_t>>& Image::getIDATChunks() {
    return this->IDAT_chunks;
}

const std::vector<chunk>& Image::getOtherChunks() {
    return this->other_chunks;
}


uint32_t Image::getIDATSize() {
    return this->IDAT_size;
}