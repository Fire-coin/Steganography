#include <iostream>
#include <fstream>
#include <cstdint> // uint8_t, uint32_t
#include <cstring>
#include <zlib.h> // inflate & deflate algorithms
#include <vector> // std::vector
#include <utility> // std::pair


using namespace std;

// Inflated size for data = (bytes_per_scanline + 1) * height
// + 1 because PNG includes filter byte at its start (1 byte per line)
// bytes_per_scanline = ceil((width * bits_per_pixel) / 8)
// bits_per_pixel = bit_depth * channels


class Image {
    vector<pair<unsigned char*, uint32_t>> chunks; // Pointer to the begging of IDAT file and it's size
    uint32_t size;
    public:
        Image();

        void addChunk(pair<unsigned char*, uint32_t> p);
        void addChunk(unsigned char* data, uint32_t size);
        vector<pair<unsigned char*, uint32_t>> getChunks();
        uint32_t getSize();
};

Image::Image() {
    this->chunks.reserve(5);
    this->size = 0;
}

void Image::addChunk(pair<unsigned char*, uint32_t> p) {
    cout << "called 0\n";
    this->chunks.emplace_back(p);
    this->size += p.second;
}

void Image::addChunk(unsigned char* data, uint32_t size) {
    cout << "called 1\n" << "size: " << size << endl;
    this->chunks.emplace_back(pair<unsigned char*, uint32_t>(data, size));
    this->size += size;
    cout << "this->size: " << this->size << endl;
}

vector<pair<unsigned char*, uint32_t>> Image::getChunks() {
    return this->chunks;
}

uint32_t Image::getSize() {
    return this->size;
}

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


uint32_t swapEdian(uint32_t value) {
    return (((value & 0xFF000000) >> 24) |
            ((value & 0x00FF0000) >> 8) |
            ((value & 0x0000FF00) << 8) | 
            ((value & 0x000000FF) << 24));
}


/* Color in IHDR chunk specification
Color    Allowed    Interpretation
   Type    Bit Depths
   
   0       1,2,4,8,16  Each pixel is a grayscale sample.
   
   2       8,16        Each pixel is an R,G,B triple.
   
   3       1,2,4,8     Each pixel is a palette index;
                       a PLTE chunk must appear.
   
   4       8,16        Each pixel is a grayscale sample,
                       followed by an alpha sample.
   
   6       8,16        Each pixel is an R,G,B triple,
                       followed by an alpha sample.
*/

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


/* IHDR chunk structure
    Width:              4 bytes
    Height:             4 bytes
    Bit depth:          1 byte
    Color type:         1 byte
    Compression method: 1 byte
    Filter method:      1 byte
    Interlace method:   1 byte
*/

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

/* Chunk structure
    Length: 4 bytes of uint32_t
    Type: 4 bytes of (ASCII) letters
    Data: Lenght bytes 
    CRC: 4 bytes
*/

int main() {
    fstream fs;
    fs.open("5x4.png", ios::in | ios::binary); // Opening file for reading in binary mod

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
        Image encodedImage;
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
            if (strcmp(reinterpret_cast<const char*>(type), "IDAT") == 0)
                encodedImage.addChunk(data, chunkLength); // Adding data from IDAT chunk into total image

            if (strcmp(reinterpret_cast<const char*>(type), "IHDR") == 0)
                metadata = getMetadata(data, chunkLength);
            

    } while (strcmp(reinterpret_cast<const char*>(type), "IEND") != 0);
        
        cout << "width: " << metadata.width << endl;
        cout << "height: " << metadata.height << endl;
        cout << "bitDepth:" << static_cast<int>(metadata.bitDepth) << endl;
        cout << "color: " << static_cast<int>(metadata.color) << endl;
        cout << "compression: " << static_cast<int>(metadata.compression) << endl;
        cout << "filter: " << static_cast<int>(metadata.filter) << endl;
        cout << "interlance: " << static_cast<int>(metadata.interlance) << endl;
        cout << "channels: " << static_cast<int>(metadata.channels) << endl;

        auto image = encodedImage.getChunks();
        int totalSize = encodedImage.getSize();
        unsigned char encodedData[totalSize];
        for (auto i : image) {
            strcat(reinterpret_cast<char*>(encodedData), reinterpret_cast<const char*>(i.first));
        }
        for (auto i = 0; i < totalSize; ++i) {
            cout << *(encodedData + i);
        }
        cout << endl << totalSize << endl;

        fs.close(); // Closing image file
    } else 
        cerr << "Could not open file\n";

    

    return 0;
}