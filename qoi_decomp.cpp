#include <cstdint>
#include <iostream>
#include <fstream>
#include <vector>
#include <array>
#include <string>

template<typename T>
struct mat {
    size_t rows_, cols_;
    std::vector<T> data_;

    mat(size_t rows, size_t cols) : rows_(rows), cols_(cols), data_(rows*cols) {}

    const T& operator[](size_t i) const { return data_[i]; }
    T& operator[](size_t i) { return data_[i]; }

    size_t size() const { return rows_ * cols_; }
    size_t rows() const { return rows_; }
    size_t cols() const { return cols_; }

    const char* rawdata() const {
        return reinterpret_cast<const char*>(data_.data());
    }
    size_t rawsize() const { return size() * sizeof(T); }
};




int main(int argc, char *argv[]){
    // TODO: Manage the command line  

    if (argc != 3) {
    
        std::cout << "Errore linea di comando";
        return EXIT_FAILURE;
    }


    // TODO: Lettura dell'header e delle dimensioni dell'immagine 


    std::ifstream is(argv[1],std::ios::binary);
    std::string magic(4,'\0');
    uint32_t H=0;
    uint32_t W=0;
    uint8_t channels, colorspace;
    uint8_t diff_green=0;


    is.read(&magic[0], 4);
    if (magic.substr(0,4)=="qoif") {
    
        std::cout << "magic letto correttamente";
    }


    /*  CARICA BIG ENDIAN
    is.read(reinterpret_cast<char*>(&W), 4);
    std::cout << W;
    */
    char c;
    
    for (int i = 0; i < 4; i++) {

        W=W << 8;
        is.get(c);
        W |= static_cast<uint8_t>(c);

    }

    for (int i = 0; i < 4; i++) {

        H = H << 8;
        is.get(c);
        H |= static_cast<uint8_t>(c);

    }

    std::cout << '\n' << W;
    std::cout << '\n' << H;

    is.get(c);
    channels = static_cast<uint8_t>(c);

    is.get(c);
    colorspace = static_cast<uint8_t>(c);


    uint8_t r=0, g=0, b = 0;
    uint8_t a = 255;
    


    using rgba = std::array<uint8_t, 4>; // Potete modificare questa definizione!
    mat<rgba> img(H, W); // TODO: Dovete mettere le dimensioni corrette!
    std::vector<rgba> seen(64, { 0,0,0,0 }); //dimensione 64 inizializzati a 0

    
    img[0] = {r,g,b,a};
    // se matcha la poszione scrivo come index_position = (r * 3 + g * 5 + b * 7 + a * 11) % 64, in decoding questo valore si riferisce alla poszione da prendere in seen

    //The 8-bit tags have precedence over the 2-bit tags. A decoder must check for the presence of an 8-bit tag first.

    //The byte stream's end is marked with 7 0x00 bytes followed by a single 0x01 byte.



    int len = 0;
    uint32_t i = 0;
    uint8_t index_position=0;
    uint8_t index;
    uint8_t dr = 0, dg = 0, db = 0;
    uint8_t run_l = 0;
    uint8_t dr_dg=0, db_dg=0;
    uint32_t bookmark = 0;
    bool run = false;


    while (is.get(c) && (i < H * W)) {

        if (c == (char)0xFF) {
            // QOI_OP_RGBA
            is.get(c); r = c;
            is.get(c); g = c;
            is.get(c); b = c;
            is.get(c); a = c;
            img[i] = { r, g, b, a };

            // ← qui sposti il !run:
            index_position = (r * 3 + g * 5 + b * 7 + a * 11) % 64;
            seen[index_position] = img[i];
            i++;
        }
        else if (c == (char)0xFE) {
            // QOI_OP_RGB
            is.get(c); r = c;
            is.get(c); g = c;
            is.get(c); b = c;
            img[i] = { r, g, b, a };

            index_position = (r * 3 + g * 5 + b * 7 + a * 11) % 64;
            seen[index_position] = img[i];
            i++;
        }
        else {
            uint8_t mark = (static_cast<uint8_t>(c) >> 6) & 0b11;

            if (mark == 0b11) {
                // QOI_OP_RUN
                uint8_t run_len = (static_cast<uint8_t>(c) & 0x3F) + 1;
                uint32_t bookmark = i - 1;
                for (int x = 0; x < run_len; x++) {
                    if (i >= img.size()) {
                        std::cerr << "Index overflow: i=" << i << " >= " << img.size() << "\n";
                        std::exit(1);
                    }
                    img[i++] = img[bookmark];
                }
                
            }
            else if (mark == 0b10) {
                // QOI_OP_LUMA
                uint8_t dg = static_cast<uint8_t>(c) & 0x3F;
                is.get(c);
                uint8_t dr_dg = (static_cast<uint8_t>(c) >> 4) & 0x0F;
                uint8_t db_dg = static_cast<uint8_t>(c) & 0x0F;

                int diffG = static_cast<int>(dg) - 32;
                r = static_cast<uint8_t>(r + diffG + (dr_dg - 8));
                g = static_cast<uint8_t>(g + diffG);
                b = static_cast<uint8_t>(b + diffG + (db_dg - 8));

                img[i] = { r, g, b, a };

                index_position = (r * 3 + g * 5 + b * 7 + a * 11) % 64;
                seen[index_position] = img[i];
                i++;
            }
            else if (mark == 0b00) {
                // QOI_OP_INDEX
                uint8_t idx = static_cast<uint8_t>(c) & 0x3F;
                img[i] = seen[idx];

                
                r = img[i][0];
                g = img[i][1];
                b = img[i][2];
                a = img[i][3];

                index_position = (r * 3 + g * 5 + b * 7 + a * 11) % 64;
                seen[index_position] = img[i];
                i++;
            }
            else {  // mark == 0b01
                // QOI_OP_DIFF
                uint8_t dr = (static_cast<uint8_t>(c) >> 4) & 0x03;
                uint8_t dg = (static_cast<uint8_t>(c) >> 2) & 0x03;
                uint8_t db = static_cast<uint8_t>(c) & 0x03;

                r = static_cast<uint8_t>(r + (dr - 2));
                g = static_cast<uint8_t>(g + (dg - 2));
                b = static_cast<uint8_t>(b + (db - 2));

                img[i] = { r, g, b, a };

                index_position = (r * 3 + g * 5 + b * 7 + a * 11) % 64;
                seen[index_position] = img[i];
                i++;
            }
        }
    }

 
    



    

    // Questo è il formato di output PAM. È già pronto così, ma potete modificarlo se volete
    std::ofstream os(argv[2], std::ios::binary); // Questo utilizza il secondo parametro della linea di comando!
    os << "P7\nWIDTH " << img.cols() << "\nHEIGHT " << img.rows() <<
        "\nDEPTH 4\nMAXVAL 255\nTUPLTYPE RGB_ALPHA\nENDHDR\n";
    os.write(img.rawdata(), img.rawsize());

    return EXIT_SUCCESS;
}


/*    int payload_length = 0;
int len = 0;

while (is.get(c)) {
    payload_length++;

    if (c == 0x00 && len != 7) {
        // primo–sesto zero
        len++;
    }
    else if (c == 0x00 && len == 7) {
        // settimo zero: controllo subito il byte successivo
        if (is.get(c)) {
            if (c == 0x01) {
                break;           // marker finale: NON conto questo 0x01
            }
            else {
                payload_length++;  // byte normale, lo conto
                len = 0;          // resetto il contatore di zeri
            }
        }
        else {
            break;  // EOF prematuro
        }
    }
    else {
        len = 0;  // qualsiasi altro byte azzera il counter
    }
}

std::cout << '\n' << payload_length << '\n';*/