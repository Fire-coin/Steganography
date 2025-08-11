#include "utils.hpp"
#include <zlib.h>
#include <cmath>

uint32_t calculate_crc(const char* type, const uint8_t* data, size_t length) {
    if (data == nullptr) {
        return 0xAE426082;
    }
    uint32_t crc = crc32(0L, Z_NULL, 0);             // Start CRC
    crc = crc32(crc, reinterpret_cast<const Bytef*>(type), 4);  // Chunk type
    crc = crc32(crc, data, length);                  // Chunk data
    return crc;
}


uint32_t swapEdian(uint32_t value) {
    return (((value & 0xFF000000) >> 24) |
            ((value & 0x00FF0000) >> 8) |
            ((value & 0x0000FF00) << 8) | 
            ((value & 0x000000FF) << 24));
}


uint8_t getChannels(uint8_t color) {
    switch (color) {
        case 0: // Grayscale
            return 1;
            break;
        case 2: // Truecolor (RGB)
            return 3;
            break;
        case 3: // Indexed-color
            return 1;
            break;
        case 4: // Grayscale + Alpha
            return 2;
            break;
        case 6: // Truecolor + Alpha (RGBA)
            return 4;
            break;
        default:
            return -1;
            break;
    }
}


m_data getMetadata(const unsigned char* data, uint8_t size) {
    m_data metadata = {0};
    unsigned char width[4] = {0};
    unsigned char height[4] = {0};
    for (int i = 0; i < 4; ++i) {
        width[i] = data[i];
        height[i] = data[i + 4];
    }
    metadata.width = swapEdian(*reinterpret_cast<uint32_t*>(width));
    metadata.height = swapEdian(*reinterpret_cast<uint32_t*>(height));
    metadata.bitDepth = data[8];
    metadata.color = data[9];
    metadata.compression = data[10];
    metadata.filter = data[11];
    metadata.interlance = data[12];
    metadata.channels = getChannels(metadata.color);
    return metadata;
}


uint32_t getImageSize(const m_data& data) {
    uint32_t bitsPerPixel = data.bitDepth * data.channels;
    uint32_t bytesPerScanline = ceil((data.width * bitsPerPixel) / 8);
    uint32_t inflatedSize = (bytesPerScanline + 1) * data.height;

    return inflatedSize;
}