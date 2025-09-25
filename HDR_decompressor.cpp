// HDR.cpp : Questo file contiene la funzione 'main', in cui inizia e termina l'esecuzione del programma.
//

#include <iostream>
#include <vector>
#include <array>
#include <string>
#include <fstream>
#include <sstream>
#include <cassert>
#include <cmath>
#include <algorithm>


using encoded_px = std::array<uint8_t, 4>; //vedi se modificare
using decoded_px = std::array<float, 3>; //vedi se modificare
using pixel = std::array<uint8_t, 3>;


template <typename T>
struct mat {

private:
    int Rows;
    int Cols;
    std::vector<T> data_;

public:

    mat(int Y, int X) : Rows(Y), Cols(X) { data_.resize(Rows * Cols); } //vedi se è coerente con la struttura dell'immagine!!

    int cols() { return Cols; }
    int rows() { return Rows; }

    const int cols() const { return Cols; }
    const int rows() const { return Rows; }

    std::vector<T>& data() { return data_; }
    const std::vector<T>& data() const { return data_; }

    T& operator() (int i, int j) { 
        assert(i >= 0 && i < Rows && j >= 0 && j < Cols); 
        return data_[i * Cols + j]; }
    const T& operator() (int i, int j) const{ 
        assert(i >= 0 && i < Rows && j >= 0 && j < Cols); 
        return data_[i * Cols + j]; }



    void resize(int new_y, int new_x) {
    
    
        Rows = new_y;
        Cols = new_x;
        data_.resize(new_y * new_x); //vedi se è coerente con la struttura dell'immagine!!

    }


};

void write_pam(std::ofstream& os,mat<pixel>& I) {

    os << "P7\n" << "WIDTH " << I.cols() << '\n' << "HEIGHT " << I.rows() << '\n' << "DEPTH 3" << "\n" << "MAXVAL 255\n" << "TUPLETYPE RGB" << "\n" << "ENDHDR\n\n";

    for (int i = 0; i < I.rows(); i++) {
        for (int j = 0; j < I.cols(); j++) {
            for (int c = 0; c < 3; c++) {

                os.put(I(i, j)[c]);

            }
        }
    }      
}

uint16_t scanline(std::ifstream& is) {

    char c;
    is.get(c);
    if (c != 0x02) {
        std::cerr << "Errore lettura scanline";
        return 0;
    }
    is.get(c);
    if (c != 0x02) {
        std::cerr << "Errore lettura scanline";
        return 0;
    }

    uint8_t byte[2];
    is.read(reinterpret_cast<char*>(byte), 2);
    uint16_t num = static_cast<uint16_t>(byte[0]) << 8 | byte[1];


    return num;
    
    }



std::vector<uint8_t> get_row(std::ifstream& is, int len) {

    
    std::vector<uint8_t> riga;
    int num_out = 0;
    while (num_out < len)
    {
        
        int c = is.get();
        if (c == EOF) break;
        uint8_t byte;
        if (c <= 127) {
        
            for (int i = 0; i < c; i++) {
            
                is.read(reinterpret_cast<char*>(&byte), 1);
                riga.push_back(byte);
                num_out++;


            }
        }
        else {

            is.read(reinterpret_cast<char*>(&byte), 1);
        
            for (int i = 0; i < (c-128); i++) {
            
                
            
                riga.push_back(byte);
                num_out++;
            
            }
        
        }
    }
    
    return riga;

}




int main(int argc, char*argv[]){

    if (argc != 3) {
        std::cerr << "errore linea di comando";
        return EXIT_FAILURE;
    }

    std::ifstream is(argv[1], std::ios::binary);
    if (!is) {
    
        std::cout << "input file non letto correttamente";
    }
    std::string line;

    getline(is, line);
    while (line.find("#?RADIANCE") == std::string::npos) { //esco dal ciclo quando c'è radiance
    //oppure while(getline(is.line) && line != "#?RADIANCE"){}
        getline(is, line);
    }
    while (getline(is, line) && line.find("FORMAT") == std::string::npos) {}
    if (line.find("32-bit_rle_rgbe")!=std::string::npos) { std::cout << "formato trovato" << std::endl; }
    else { std::cout << "formato non trovato"; }

    while (getline(is, line) && line.find("") == std::string::npos) {}

    if (line == "") { std::cout << "fine header" << std::endl; }

    getline(is,line);

    std::string mark;
    int Y; //N rows
    int X; //N cols
    std::istringstream iss(line);
    iss >> mark >> Y >> mark >> X;

    std::cout << Y << std::endl << X << std::endl;

    
 
    mat<encoded_px> I(Y, X);

    

    for (int i = 0; i < Y; i++) {
        int len = scanline(is);
        for (int c = 0; c < 4; c++) {
            std::vector<uint8_t> raw_riga = get_row(is, len);
            for (int j = 0; j < len; j++) {

                I(i, j)[c] = raw_riga[j];

            }
        }
    }
    

    mat<decoded_px> Iout(Y, X);
    float min = 1000000;
    float max = 0;

    for (int i = 0; i < Y; i++) {
        for (int j = 0; j < X; j++) {
            for (int c = 0; c < 3; c++) {
            

                Iout(i, j)[c] = ((I(i, j)[c]+0.5)/256)*pow(2,I(i,j)[3] - 128);
            
                if (Iout(i, j)[c] < min) {
                    min = Iout(i, j)[c];
                }

                if (Iout(i, j)[c] > max) {
                max= Iout(i, j)[c];
                }

            }
        
        
        }
    }

    mat<pixel> Idef(Y, X);

    for (int i = 0; i < Y; i++) {
        for (int j = 0; j < X; j++) {
            for (int c = 0; c < 3; c++) {


                Idef(i, j)[c] = static_cast<uint8_t>(255*pow((Iout(i, j)[c] - min) / (max - min),0.45));

                //int ival = static_cast<int>(std::lround(Idef(i, j)[c] * 255.0f));
               // Idef(i, j)[c] = static_cast<uint8_t>(std::clamp(ival, 0, 255));
            }


        }
    }



    std::ofstream os(argv[2],std::ios::binary);
    write_pam(os,Idef);

}


