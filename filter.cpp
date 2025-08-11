#include "utils.hpp"
#include <memory.h>

void filter(uint8_t* data, const uint32_t size, const m_data& metadata, bool decode) {
    uint32_t bpScanline = size / metadata.height; // scanline size including filter byte
    int bpp = (metadata.bitDepth * metadata.channels + 7) / 8; // bytes per pixel
    int factor = decode ? 1 : -1;

    uint8_t* filteredData = new uint8_t[size];

    for (uint32_t line = 0; line < metadata.height; ++line) {
        uint32_t lineStart = line * bpScanline;
        int filterType = data[lineStart];

        // Copy filter type
        filteredData[lineStart] = filterType;

        for (uint32_t i = 1; i < bpScanline; ++i) {
            uint32_t byteIndex = lineStart + i;

            int left = 0, up = 0, upLeft = 0;

            if (i > bpp) {
                if (decode)
                    left = filteredData[byteIndex - bpp]; // reconstructed previous pixel
                else
                    left = data[byteIndex - bpp]; // original
            }

            if (line > 0) {
                if (decode)
                    up = filteredData[byteIndex - bpScanline]; // reconstructed previous row
                else
                    up = data[byteIndex - bpScanline]; // original
            }

            if (line > 0 && i > bpp) {
                if (decode)
                    upLeft = filteredData[byteIndex - bpScanline - bpp]; // reconstructed prev. row and prev. pixel
                else
                    upLeft = data[byteIndex - bpScanline - bpp];
            }

            int predictor = 0;

            switch (filterType) {
                case 0: // None
                    predictor = 0;
                    break;
                case 1: // Sub
                    predictor = left;
                    break;
                case 2: // Up
                    predictor = up;
                    break;
                case 3: // Average
                    predictor = (left + up) / 2;
                    break;
                case 4: // Paeth
                {
                    int p = left + up - upLeft;
                    int pa = std::abs(p - left);
                    int pb = std::abs(p - up);
                    int pc = std::abs(p - upLeft);

                    if (pa <= pb && pa <= pc) predictor = left;
                    else if (pb <= pc) predictor = up;
                    else predictor = upLeft;
                    break;
                }
            }

            int val = (int)data[byteIndex] - factor * predictor;
            filteredData[byteIndex] = (uint8_t)(val & 0xFF);
        }
    }

    memcpy(data, filteredData, size);
    delete[] filteredData;
}