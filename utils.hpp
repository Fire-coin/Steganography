#pragma once
#ifndef UTILS_HPP
#define UTILS_HPP

#include <stdint.h> // uint8_t, uint32_t
#include <utility> // std::pair
#include <string>

// Meta data of image
struct m_data {
    uint32_t width;
    uint32_t height;
    uint8_t bitDepth;
    uint8_t color;
    uint8_t channels;
    uint8_t compression;
    uint8_t filter;
    uint8_t interlance;
};

/*Calculates crc of chunk*/
uint32_t calculate_crc(const char* type, const uint8_t* data, size_t length);

/*Performs deflate compression; 
returns pointer to the data and deflated size*/
std::pair<uint8_t*, uint32_t> compress(uint8_t* data, uint32_t size);

/*Performs inflate decompression*/
uint8_t* decompress(uint8_t* data, uint32_t size, uint32_t expectedSize);

/*Swaps edian of given value*/
uint32_t swapEdian(uint32_t value);

/* Returns number of channels based on PNG specifications: 
Color in IHDR chunk specification
Color    Allowed    Interpretation
   Type  :  Bit Depths
   
   0 :      1,2,4,8,16 => Each pixel is a grayscale sample.
   
   2 :      8,16       => Each pixel is an R,G,B triple.
   
   3 :      1,2,4,8    => Each pixel is a palette index;
                       a PLTE chunk must appear.
   
   4 :      8,16       => Each pixel is a grayscale sample,
                       followed by an alpha sample.
   
   6 :      8,16       => Each pixel is an R,G,B triple,
                       followed by an alpha sample.
*/
uint8_t getChannels(uint8_t color);

/* Returns metadata of image
IHDR chunk structure
    Width:              4 bytes
    Height:             4 bytes
    Bit depth:          1 byte
    Color type:         1 byte
    Compression method: 1 byte
    Filter method:      1 byte
    Interlace method:   1 byte
*/
m_data getMetadata(const uint8_t* data, uint8_t size);

/*Returns the number of bytes of raw data of image*/
uint32_t getImageSize(const m_data& data);

/*Applies filter or reconstruction algorithm based on decode*/
void filter(uint8_t* data, const uint32_t size, const m_data& metadata, bool decode);

/*Encodes message into image color channels*/
void encodeMessage(uint8_t* rawData, const uint32_t rawSize, std::string message, const m_data& metadata);

/*Decodes message from image color channels*/
std::string decodeMessage(const uint8_t* rawData, const uint32_t rawSize, const m_data& metadata);

#endif