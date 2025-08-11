#include "utils.hpp"
#include <iostream>

// #include <cstdint>
// #include <cstring>
// #include <string>
// #include <stdexcept>
//
// Encodes `message` into raw PNG scanline data (rawData), which must contain
// filter bytes (one leading byte per scanline). Works only for 8-bit samples.
//
// Parameters:
// - rawData: pointer to the uncompressed image byte stream as used inside IDAT
//            (i.e. each scanline begins with a filter byte).
// - rawSize: length in bytes of rawData.
// - message: message to encode (any std::string content).
// - width:   image width in pixels.
// - height:  image height in pixels.
// - channels: number of channels per pixel (e.g., 3 for RGB, 4 for RGBA, 1 for gray).
// - bitDepth: bits per sample (this function only supports 8).
//
// Notes:
// - The function writes a 4-byte (little-endian) length prefix followed by message bytes.
// - It sets the LSB of the RED sample byte for each pixel (channel 0).
// - If there isn't enough room, the function throws std::runtime_error.
// void encodeMessage(uint8_t* rawData,
//                    uint32_t rawSize,
//                    const std::string& message,
//                    uint32_t width,
//                    uint32_t height,
//                    uint8_t channels)
// {
//     if (!rawData) throw std::invalid_argument("rawData is null");
//     if (channels < 1) throw std::invalid_argument("channels must be >= 1");
//
//     const uint32_t bytesPerPixel = channels;
//     const uint32_t bytesPerScanline = width * bytesPerPixel;
//     const uint32_t scanlineTotal = bytesPerScanline + 1;
//
//     uint32_t msgLen = static_cast<uint32_t>(message.size());
//     std::string payload(4 + msgLen, '\0');
//     payload[0] = msgLen & 0xFF;
//     payload[1] = (msgLen >> 8) & 0xFF;
//     payload[2] = (msgLen >> 16) & 0xFF;
//     payload[3] = (msgLen >> 24) & 0xFF;
//     if (msgLen) memcpy(&payload[4], message.data(), msgLen);
//
//     uint64_t pixelsNeeded = static_cast<uint64_t>(payload.size()) * 8;
//     uint64_t pixelsAvailable = static_cast<uint64_t>(width) * height;
//     if (pixelsNeeded > pixelsAvailable)
//         throw std::runtime_error("Not enough pixel capacity");
//
//     uint64_t requiredRaw = static_cast<uint64_t>(scanlineTotal) * height;
//     if (rawSize < requiredRaw)
//         throw std::runtime_error("rawSize too small");
//
//     uint64_t payloadIndex = 0;
//     uint8_t bitMask = 0x80;
//
//     for (uint32_t row = 0; row < height; ++row) {
//         uint64_t scanlineStart = row * scanlineTotal;
//         if (scanlineStart + scanlineTotal > rawSize)
//             throw std::runtime_error("Scanline start out of bounds");
//
//         uint8_t filter = rawData[scanlineStart];
//         if (filter > 4) {
//             std::cerr << "Warning: invalid filter (" << unsigned(filter)
//                       << ") at row " << row << "\n";
//         }
//
//         uint64_t pixelBase = scanlineStart + 1;
//         for (uint32_t px = 0; px < width; ++px) {
//             uint64_t pos = pixelBase + px * bytesPerPixel;
//             if (pos >= rawSize) throw std::runtime_error("Pixel pos out of bounds");
//
//             uint8_t curBit = (payload[payloadIndex] & bitMask) ? 1u : 0u;
//             rawData[pos] = (rawData[pos] & 0xFE) | curBit;
//
//             if (bitMask == 0x01) {
//                 bitMask = 0x80;
//                 ++payloadIndex;
//                 if (payloadIndex >= payload.size()) return;
//             } else {
//                 bitMask >>= 1;
//             }
//         }
//     }
//
//     throw std::runtime_error("Payload not fully embedded");
// }

void encodeMessage(uint8_t* rawData, const uint32_t rawSize, std::string message, const m_data& metadata) {
    uint32_t bpScanline = rawSize / metadata.height - 1; // scanline size without filter byte
    uint32_t bpp = (metadata.bitDepth * metadata.channels + 7) / 8; // bytes per pixel
    uint32_t bitOffset = ((metadata.bitDepth + 7) / 8) - 1; // LSB byte offset in channel
    uint32_t pixelsPerScanline = bpScanline / bpp;

    uint32_t messageLength = message.length();
    message = std::string(reinterpret_cast<char*>(&messageLength), 4) + message; // prefix length
    uint32_t offset = 0;
    uint32_t pixelIndex = 0;
    uint32_t scanlineStart = 0;

    for (uint32_t c = 0; c < message.length(); c++) {
        uint8_t curChar = message[c];
        for (uint8_t i = 0; i < 8; ++i) {
            if (pixelIndex >= pixelsPerScanline) {
                offset++;
                pixelIndex = 0;
                scanlineStart = offset * (bpScanline + 1);
            }
            uint32_t byteIndex = scanlineStart + pixelIndex * bpp + bitOffset + 1;
            uint8_t curBit = (curChar & 0x80) ? 1 : 0;
            rawData[byteIndex] = (rawData[byteIndex] & 0xFE) | curBit;
            curChar <<= 1;
            pixelIndex++;
        }
    }
}

std::string decodeMessage(const uint8_t* rawData, const uint32_t rawSize, const m_data& metadata) {
    std::string output;
    uint8_t messageLengthStr[4] = {0};

    uint32_t bpScanline = rawSize / metadata.height - 1; // scanline size without filter byte
    uint32_t bpp = (metadata.bitDepth * metadata.channels + 7) / 8; // bytes per pixel
    uint32_t bitOffset = ((metadata.bitDepth + 7) / 8) - 1;
    uint32_t pixelsPerScanline = bpScanline / bpp;

    uint32_t offset = 0;
    uint32_t pixelIndex = 0;
    uint32_t scanlineStart = 0;

    // Read message length (4 bytes)
    for (uint32_t num = 0; num < 4; ++num) {
        uint8_t byteVal = 0;
        for (uint32_t i = 0; i < 8; ++i) {
            if (pixelIndex >= pixelsPerScanline) {
                offset++;
                pixelIndex = 0;
                scanlineStart = offset * (bpScanline + 1);
            }
            uint32_t byteIndex = scanlineStart + pixelIndex * bpp + bitOffset + 1;
            uint8_t curBit = rawData[byteIndex] & 1;
            byteVal = (byteVal << 1) | curBit;
            pixelIndex++;
        }
        messageLengthStr[num] = byteVal;
    }
    uint32_t messageLength = 0;
    memcpy(&messageLength, messageLengthStr, sizeof(messageLength));
    std::cout << "Message length: " << messageLength << std::endl;

    for (uint32_t i = 0; i < messageLength; ++i) {
        uint8_t c = 0;
        for (uint8_t j = 0; j < 8; ++j) {
            if (pixelIndex >= pixelsPerScanline) {
                offset++;
                pixelIndex = 0;
                scanlineStart = offset * (bpScanline + 1);
            }
            uint32_t byteIndex = scanlineStart + pixelIndex * bpp + bitOffset + 1;
            uint8_t curBit = rawData[byteIndex] & 1;
            c = (c << 1) | curBit;
            pixelIndex++;
        }
        output += c;
    }
    return output;
}
