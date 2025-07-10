#include <iostream>
#include <fstream>
#include <cstdint>
#include <cstring>
#include <zlib.h>

using namespace std;

// Meta data of image
struct m_data {
    uint8_t channels;
    uint8_t bitDepth;
    uint32_t width;
    uint32_t height;
    uint8_t color;  
};


uint32_t swapEdian(uint32_t value) {
    return (((value & 0xFF000000) >> 24) |
            ((value & 0x00FF0000) >> 8) |
            ((value & 0x0000FF00) << 8) | 
            ((value & 0x000000FF) << 24));
}


/* Chunk structure
    Length: 4 bytes of uint32_t
    Type: 4 bytes of (ASCII) letters
    Data: Lenght bytes 
    CRC: 4 bytes
*/

int main() {
    fstream fs;
    fs.open("5x4.png", ios::in | ios::out | ios::binary); // Opening file for writing and reading in binary mod

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

        do {
            uint32_t chunkLength = 0;

            // Reading chunk length
            fs.read(reinterpret_cast<char*>(&chunkLength), sizeof(chunkLength));
            // Reading chunk type
            fs.read(reinterpret_cast<char*>(type), 4);
            type[4] = '\0';

            // Swaping Edian, because png writes values as MSB is on the right side
            chunkLength = swapEdian(chunkLength);

            unsigned char data[chunkLength] = {0};
            // Reading chunk data
            fs.read(reinterpret_cast<char*>(data), chunkLength);
            // Reading chunk CRC
            fs.read(reinterpret_cast<char*>(crc), 4);

            for (int i = 0; i < 4; ++i)
            cout << type[i];
            cout << endl;
            
            cout << "Lenght: " << chunkLength  << " bytes" << endl;
            cout << '\n';
    } while (strcmp(reinterpret_cast<const char*>(type), "IEND") != 0);
        
        fs.close(); // Closing image file
    } else 
        cerr << "Could not open file\n";


    return 0;
}