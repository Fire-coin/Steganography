#include <iostream>
#include <fstream>
#include <cstdint> // uint8_t, uint32_t
#include <cstring>
#include <zlib.h> // inflate & deflate algorithms
#include <vector> // std::vector
#include <utility> // std::pair
#include <cmath>
#include <iomanip> // for std::hex and std::setw

//TODO add option of adding password

using namespace std;

uint32_t calculate_crc(const char* type, const uint8_t* data, size_t length) {
    if (data == nullptr) {
        return 0xAE426082;
    }
    uint32_t crc = crc32(0L, Z_NULL, 0);             // Start CRC
    crc = crc32(crc, reinterpret_cast<const Bytef*>(type), 4);  // Chunk type
    crc = crc32(crc, data, length);                  // Chunk data
    return crc;
}


pair<unsigned char*, uint32_t> compress(unsigned char* data, uint32_t size) {
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
    cout << std::dec;
    cout << "Initial size: " << size << endl;
    cout << "Size left in stream: " << strm.avail_out << endl;
    cout << "Compressed size: " << size - strm.avail_out << endl;
    
    return pair<unsigned char*, uint32_t>(compressed, size - strm.avail_out);
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

struct chunk
{
    uint32_t length;
    unsigned char type[4];
    unsigned char* data;
    unsigned char crc[4];
};




class Image {
    vector<pair<unsigned char*, uint32_t>> IDAT_chunks; // Pointer to the begging of IDAT file and it's size
    vector<chunk> other_chunks;
    uint32_t IDAT_size;
    public:
        Image();
        ~Image();
        void addIDATChunk(unsigned char* data, uint32_t size);
        void addChunk(const chunk& chunk);
        const vector<pair<unsigned char*, uint32_t>>& getIDATChunks();
        const vector<chunk>& getOtherChunks();
        uint32_t getIDATSize();
};

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
    this->IDAT_chunks.emplace_back(pair<unsigned char*, uint32_t>(dataCopy, size));
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


const vector<pair<unsigned char*, uint32_t>>& Image::getIDATChunks() {
    return this->IDAT_chunks;
}

const vector<chunk>& Image::getOtherChunks() {
    return this->other_chunks;
}


uint32_t Image::getIDATSize() {
    return this->IDAT_size;
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

// Inflated size for data = (bytes_per_scanline + 1) * height
// + 1 because PNG includes filter byte at its start (1 byte per line)
// bytes_per_scanline = ceil((width * bits_per_pixel) / 8)
// bits_per_pixel = bit_depth * channels

uint32_t getImageSize(const m_data& data) {
    uint32_t bitsPerPixel = data.bitDepth * data.channels;
    uint32_t bytesPerScanline = ceil((data.width * bitsPerPixel) / 8);
    uint32_t inflatedSize = (bytesPerScanline + 1) * data.height;

    return inflatedSize;
}


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

    std::memcpy(data, filteredData, size);
    delete[] filteredData;
}


inline uint32_t getMaxMessageSize(const uint32_t rawSize, const m_data& metadata) {
    return (metadata.height * metadata.width / 8) - (4 * 8); // Only 1 bit per pixel and 4 bytes for message length
}


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
    cout << "Message length: " << messageLength << endl;

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

        string message;
        std::getline(cin, message);
        std::cout << "Message is: " << message << endl;
        std::cout << "Length of message is: " << message.length() << endl;
        uint32_t messageLength = message.length();
        uint32_t maxMessageLegth = getMaxMessageSize(inflatedSize, metadata);
        cout << "Max size for message is: " << maxMessageLegth << endl;
        std::cout << (int)message[0] << endl;
        const uint32_t MAX_MESSAGE_SIZE = getMaxMessageSize(inflatedSize, metadata);
        // TODO add check if message fits into image
        std::cout << endl;

        

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
        
        fstream output;
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
    
        // Freeing memory
        delete inflatedData;
        delete deflatedData;
    } else 
        cerr << "Could not open file\n";
    
    
    return 0;
}
