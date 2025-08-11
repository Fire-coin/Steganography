#include "utils.hpp"
#include <zlib.h>
#include <iostream>

std::pair<unsigned char*, uint32_t> compress(unsigned char* data, uint32_t size) {
    z_stream strm{};
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;

    strm.next_in = data;
    strm.avail_in = size;
    strm.avail_out = size;
    unsigned char* compressed = new unsigned char[size];
    strm.next_out = compressed;

    deflateInit(&strm, Z_BEST_COMPRESSION);
    deflate(&strm, Z_FINISH); // TODO handle if compressed size will be larger than size
    deflateEnd(&strm);
    std::cout << "Initial size: " << size << std::endl;
    std::cout << "Size left in stream: " << strm.avail_out << std::endl;
    std::cout << "Compressed size: " << size - strm.avail_out << std::endl;
    
    return std::pair<unsigned char*, uint32_t>(compressed, size - strm.avail_out);
}


unsigned char* decompress(unsigned char* data, uint32_t size, uint32_t expectedSize) {
    z_stream strm{};
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;

    strm.next_in = data;
    strm.avail_in = size;
    strm.avail_out = expectedSize;
    unsigned char* decompressed = new unsigned char[expectedSize];
    strm.next_out = decompressed;   
    inflateInit(&strm);
    inflate(&strm, Z_FINISH);
    inflateEnd(&strm);

    return decompressed;
}