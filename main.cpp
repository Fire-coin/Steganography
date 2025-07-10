#include <iostream>
#include <fstream>
#include <cstdint>

using namespace std;

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

        fs.close(); // Closing image file
    } else 
        cerr << "Could not open file\n";


    return 0;
}