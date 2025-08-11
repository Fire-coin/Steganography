#include <iostream>
#include <fstream>
#include <cstdint> // uint8_t, uint32_t
#include <cstring>
#include <vector> // std::vector
#include <utility> // std::pair
#include <iomanip> // for std::hex and std::setw
#include "Image.hpp"
#include "utils.hpp"

//TODO add option of adding password

using namespace std;


inline uint32_t getMaxMessageSize(const m_data& metadata) {
    return (metadata.height * metadata.width / 8) - (4 * 8); // Only 1 bit per pixel and 4 bytes for message length
}


int main() {
    fstream fs;
    fs.open("NewTux.png", ios::in | ios::binary); // Opening file for reading in binary mod

    if (fs) {
        unsigned char sign[8];
        // Reading signature of file
        fs.read(reinterpret_cast<char*>(sign), sizeof(sign));
        // Displaying signature of file
        for (int i = 0; i < 8; ++i)
        cout << static_cast<int>(sign[i]) << ' ';
        cout << endl;
        
        unsigned char type[5] = {0}; // Setting all characters to 0
        unsigned char crc[4];
        m_data metadata = {};
        Image image;
        do {
            chunk currentChunk{};
            uint32_t chunkLength = 0;
            
            // Reading chunk length
            fs.read(reinterpret_cast<char*>(&chunkLength), sizeof(chunkLength));
            // Reading chunk type
            fs.read(reinterpret_cast<char*>(type), 4);
            type[4] = '\0';
            memcpy(currentChunk.type, type, 4);
            
            // Swaping Edian, because png writes values as MSB is on the right side
            chunkLength = swapEdian(chunkLength);
            currentChunk.data = new unsigned char[chunkLength];
            currentChunk.length = chunkLength;

            unsigned char data[chunkLength] = {0};
            // Reading chunk data
            fs.read(reinterpret_cast<char*>(data), chunkLength);
            memcpy(currentChunk.data, data, chunkLength);
            // Reading chunk CRC
            fs.read(reinterpret_cast<char*>(crc), 4);
            memcpy(currentChunk.crc, crc, 4);

            // for (int i = 0; i < 4; ++i)
            // cout << type[i];
            // cout << endl;
            
            // cout << "Lenght: " << chunkLength  << " bytes" << endl;
            // cout << '\n';
            if (strcmp(reinterpret_cast<const char*>(type), "IDAT") == 0)
                image.addIDATChunk(data, chunkLength); // Adding data from IDAT chunk into total image
            else
                image.addChunk(currentChunk);

            if (strcmp(reinterpret_cast<const char*>(type), "IHDR") == 0)
                metadata = getMetadata(data, chunkLength);
        
        } while (strcmp(reinterpret_cast<const char*>(type), "IEND") != 0);
        fs.close();
        //TODO add specification for indexed colors
        cout << "width: " << metadata.width << endl;
        cout << "height: " << metadata.height << endl;
        cout << "bitDepth:" << static_cast<int>(metadata.bitDepth) << endl;
        cout << "color: " << static_cast<int>(metadata.color) << endl;
        cout << "compression: " << static_cast<int>(metadata.compression) << endl;
        cout << "filter: " << static_cast<int>(metadata.filter) << endl;
        cout << "interlance: " << static_cast<int>(metadata.interlance) << endl;
        cout << "channels: " << static_cast<int>(metadata.channels) << endl;

        auto IDAT_chunks = image.getIDATChunks();
        auto otherChunks = image.getOtherChunks();
        uint32_t total_IDAT_size = image.getIDATSize();
        unsigned char* compressedData = new unsigned char[total_IDAT_size];
        
        size_t offset = 0;
        for (auto& chunk : IDAT_chunks) {
            memcpy(compressedData + offset, chunk.first, chunk.second);
            offset += chunk.second;
        }
        std::cout << image.getIDATSize() << endl;
        uint32_t inflatedSize = getImageSize(metadata);
        
        unsigned char* inflatedData = decompress(compressedData, total_IDAT_size, inflatedSize);
        
        // Filtering to get raw data
        filter(inflatedData, inflatedSize, metadata, true);

        uint32_t maxMessageLegth = getMaxMessageSize(metadata);
        cout << "Max size for message is: " << maxMessageLegth << endl;
        string message;
        std::getline(cin, message);
        std::cout << "Message is: " << message << endl;
        std::cout << "Length of message is: " << message.length() << endl;
        uint32_t messageLength = message.length();
        std::cout << (int)message[0] << endl;
        // TODO add check if message fits into image
        std::cout << endl;

        fstream output;

        output.open("10x50_2.txt", ios::out);
        if (!output.is_open()) cerr << "could not open file\n";
        std::cout << endl;
        output << std::hex;
        for (size_t i = 0; i < inflatedSize; ++i) {
            if (i % (((metadata.bitDepth + 7) / 8) * metadata.width * metadata.channels + 1) == 0)
                output << "\n";
            output << std::setfill('0') << std::setw(2)
                    << (int)inflatedData[i] << " ";
            
        }
        output.close();

        // encodeMessage(inflatedData, inflatedSize, message, metadata.width, 
        //     metadata.height, metadata.channels);
        encodeMessage(inflatedData, inflatedSize, message, metadata);
        string msg = decodeMessage(inflatedData, inflatedSize, metadata);

        std::cout << "Given message: " << msg << endl;
        cout << "Size of given message: " << msg.length() << endl; 

        std::cout << endl;
        
        // Filtering to potentially get better compression
        filter(inflatedData, inflatedSize, metadata, false);
        
        auto p = compress(inflatedData, inflatedSize);

        unsigned char* deflatedData = p.first;
        uint32_t deflatedSize = p.second;
        
        // fstream output;
        output.open("NewTux2.png", ios::out | ios::binary);
        
        if (output.is_open()) {
            // Writing PNG signature into output file
            output.write(reinterpret_cast<char*>(sign), 8);
            // Writing all other chunks except IEND
            for (auto it = otherChunks.begin(); it != otherChunks.end() - 1; ++it) {
                uint32_t chunkLen = swapEdian(it->length);
                output.write(reinterpret_cast<const char*>(&chunkLen), 4);
                output.write(reinterpret_cast<const char*>(it->type), 4);
                output.write(reinterpret_cast<char*>(it->data), it->length);
                output.write(reinterpret_cast<const char*>(it->crc), 4);
            }
            uint32_t MAX_SIZE = 8192;

            size_t offset = 0;
            uint32_t chunkLength = 0;
            unsigned char* chunkData = nullptr;
            // Writing all IDAT chunks
            for (int i = 0 ; i < int(deflatedSize / MAX_SIZE) + 1; ++i) {
                if (deflatedSize < (i + 1) * MAX_SIZE) {
                    chunkLength = deflatedSize - (i * MAX_SIZE);
                } else {
                    chunkLength = MAX_SIZE;
                }
                unsigned char* chunkData = new unsigned char[chunkLength];
                memcpy(chunkData, (deflatedData + i * MAX_SIZE), chunkLength);
                uint32_t edianLength = swapEdian(chunkLength);
                // Writing length of IDAT chunk
                output.write(reinterpret_cast<char*>(&edianLength), 4);
                // Writing chunk type
                output.write("IDAT", 4);
                // Writing chunk data
                output.write(reinterpret_cast<char*>(chunkData), chunkLength);
                // Writing chunk crc
                uint32_t crc = swapEdian(calculate_crc("IDAT", chunkData, chunkLength));

                output.write(reinterpret_cast<char*>(&crc), 4);
                delete[] chunkData;
            }

            // Writing IEND chunk    
            uint32_t crc = swapEdian(calculate_crc("IEND", nullptr, 0));
            uint32_t IEND_length = 0;

            output.write(reinterpret_cast<char*>(&IEND_length), 4);
            output.write("IEND", 4);
            output.write(reinterpret_cast<char*>(&crc), 4);

        } else 
            cerr << "Could not open output file\n";
        
        output.close();

        output.open("10x50_3.txt", ios::out);
        if (!output.is_open()) cerr << "could not open file\n";
        std::cout << endl;
        output << std::hex;
        for (size_t i = 0; i < inflatedSize; ++i) {
            if (i % (((metadata.bitDepth + 7) / 8) * metadata.width * metadata.channels + 1) == 0)
                output << "\n";
            output << std::setfill('0') << std::setw(2)
                    << (int)inflatedData[i] << " ";
            
        }
        output.close();
        // Freeing memory
        delete inflatedData;
        delete deflatedData;
    } else 
        cerr << "Could not open file\n";
    
    
    return 0;
}
