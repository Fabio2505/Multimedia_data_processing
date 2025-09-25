#include <iostream>
#include <fstream>
#include <vector>
#include <iterator>
#include <algorithm>
#include <string>
#include <functional>
#include <exception>
#include <variant>
#include <cstring>

using namespace std;

#include "ppm.h"
#include "image.h"
#include "types.h"
#include "image_operations.h"

//16:44

bool writeP6(const std::string& sFileName, const image<vec3b>& img) {

	
	std::ofstream os(sFileName,std::ios::binary);
	if (!os) { return false; }
	
	os << "P6\n";
	int height=img.height();
	int width = img.width();

	os <<  width << ' ' << height;
	os << "\n255\n";


	os.write(reinterpret_cast<const char*>(img.data()),img.data_size()); //numero di byte non di pixel

	return true;

}

std::string extract_key(std::ifstream &is) {

	uint32_t len = 0; //vedi se modificare il tipo dopo
	std::string chiave;
	char c;
	is.get(c);
	if (c == 'i') {
		is.get(c);
		len = static_cast<int>(c);
	}
	else if (c == 'u') {

		is.get(c);
		len = static_cast<uint8_t>(c);

	}
	else if (c == 'I') {

		is.get(c);
		len |= c;
		len = len << 8;
		is.get(c);
		len |= c;
		len=static_cast<int16_t>(len);
	}

	else if (c == 'l') {

		len = len << 8;
		is.get(c);
		len |= c;
		len = len << 8;
		is.get(c);
		len |= c;
		len = len << 8;
		is.get(c);
		len |= c;
		len = len << 8;
		is.get(c);
		len |= c;
		len = static_cast<int32_t>(len);


	}

	else { std::cout << "letto identificatore non previsto!"; }

	for (int i = 0; i < len; i++) {
	
	
		is.get(c);
		chiave += c;
	
	
	}

	
	return chiave;
}



double extract_value(std::ifstream& is) {
	char c;
	is.get(c);

	if (c == 'i') {
		is.get(c);
		return static_cast<double>(static_cast<int8_t>(c));
	}
	else if (c == 'u') {
		is.get(c);
		return static_cast<double>(static_cast<uint8_t>(c));
	}
	else if (c == 'I') {
		uint16_t value = 0;
		is.get(c);
		value |= static_cast<uint8_t>(c);
		value <<= 8;
		is.get(c);
		value |= static_cast<uint8_t>(c);
		return static_cast<double>(static_cast<int16_t>(value));
	}
	else if (c == 'l') {
		int32_t value = 0;
		for (int i = 0; i < 4; i++) {
			value <<= 8;
			is.get(c);
			value |= static_cast<uint8_t>(c);
		}
		return static_cast<double>(value);
	}
	else if (c == 'd') {
		uint32_t value = 0;
		for (int i = 0; i < 4; i++) {
			value <<= 8;
			is.get(c);
			value |= static_cast<uint8_t>(c);
		}
		float f;
		std::memcpy(&f, &value, sizeof(float));
		return static_cast<double>(f);
	}
	else if (c == 'D') {
		uint64_t value = 0;
		for (int i = 0; i < 8; i++) {
			value <<= 8;
			is.get(c);
			value |= static_cast<uint8_t>(c);
		}
		double d;
		std::memcpy(&d, &value, sizeof(double));
		return d;
	}

	else if (c == '$') {
		return -1;

	}

	else {
		throw std::runtime_error("letto identificatore non previsto!");
			return 0;

	}
}


	int convert(const string & sInput, const string & sOutput){

	std::ifstream is(sInput,ios::binary);
	std::string key;
	std::vector<uint8_t> background;
	
	char c;
	is.get(c);
	if ( c == '{') {
		std::cout << "\n'{'\n";
	}
	key = extract_key(is); //canvas
	std::cout << key << "\n";
	is.get(c); // {
	key = extract_key(is); // width
	std::cout << key << "\n";
	double width = extract_value(is);  // RICORDA CHE STAI LEGGENDO DEI DOUBLE, poi vanno modificati ad uint8
    std::cout << width << "\n";
	key = extract_key(is); // height
	std::cout << key << "\n";
	double height = extract_value(is);  // RICORDA CHE STAI LEGGENDO DEI DOUBLE, poi vanno modificati ad uint8
	std::cout << height << "\n";

	key = extract_key(is); // height
	std::cout << key << "\n";
	is.get(c);


	int len = 0;
	is.get(c);
	if (c == '$') {
		is.get(c);
		if (c != 'U') {
			throw std::runtime_error("Tipo non uint8 nell'array ottimizzato");
		}

		is.get(c);
		if (c != '#') {
			throw std::runtime_error("Atteso '#' dopo type marker");
		}

		is.get(c);
		if (c == 'i') {
			is.get(c);
			len = static_cast<int8_t>(c);
		}
		else if (c == 'u') {
			is.get(c);
			len = static_cast<uint8_t>(c);
		}
		else {
			throw std::runtime_error("Marker lunghezza non gestito");
		}

		for (int i = 0; i < len; i++) {
			is.get(c);
			background.push_back(static_cast<uint8_t>(c));
		}
	}
	else {
		throw std::runtime_error("Atteso '$' per array ottimizzato");
	} 
	for (uint8_t elem : background) {
	
		std::cout << static_cast<int>(elem)<< " ";

	}

	std::cout << "\n";
	is.get(c); // }

	key = extract_key(is); // elements
	std::cout << key << "\n";
	 
	is.get(c); // { ___________ora tocca far stampare image 
	int n_graffe = 1;
	while (true) {
		key = extract_key(is);
		if (key == "image") { 
			break; }
		else {
			is.get(c);
			while (c != '}' || n_graffe > 0) {
			
				is.get(c);
				if (c == '{') {

					n_graffe++;
				
				}
				if (c == '}') { n_graffe--; }
			
			}
		
		
		}
	
	}
	std::cout << key << "\n"; // "image"
	is.get(c); // {
	key = extract_key(is);
	std::cout << key << "=";
	auto x = extract_value(is);
	std::cout << x << "\n";
	key = extract_key(is);
	std::cout << key << "=";
	auto y = extract_value(is);
	std::cout << y << "\n";
	key = extract_key(is);
	std::cout << key << "=";
	auto image_w = extract_value(is);
	std::cout << image_w << "\n";
	key = extract_key(is);
	std::cout << key << "=";
	auto image_h = extract_value(is);
	std::cout << image_h << "\n";
	key = extract_key(is);
	std::cout << key << "=";


	//nuovo per data  fai debug
	len = 0;
	std::vector<uint8_t> data;
	is.get(c); // {
	is.get(c);
	if (c == '$') {
		is.get(c);
		if (c != 'U') {
			throw std::runtime_error("Tipo non uint8 nell'array ottimizzato");
		}

		is.get(c);
		if (c != '#') {
			throw std::runtime_error("Atteso '#' dopo type marker");
		}

		is.get(c);
		if (c == 'i') {
			is.get(c);
			len = static_cast<int8_t>(c);
		}
		else if (c == 'u') {
			is.get(c);
			len = static_cast<uint8_t>(c);
		}
		else if (c == 'I') {
			uint16_t value = 0;
			is.get(c);
			value |= static_cast<uint8_t>(c);
			value <<= 8;
			is.get(c);
			value |= static_cast<uint8_t>(c);
			len = value;
		}
		else if (c == 'l') {
			int32_t value = 0;
			for (int i = 0; i < 4; i++) {
				value <<= 8;
				is.get(c);
				value |= static_cast<uint8_t>(c);
			}
			len=value;
		}
		else {
			throw std::runtime_error("Marker lunghezza non gestito");
		}

		for (int i = 0; i < len; i++) {
			is.get(c);
			data.push_back(static_cast<uint8_t>(c));
		}
	}
	else {
		throw std::runtime_error("Atteso '$' per array ottimizzato");
	}
	for (uint8_t elem : data) {

		std::cout << static_cast<int>(elem) << " ";

	}

	std::cout << "\n";

	if (data.size() != 0) {
		is.close();
	}
	width = static_cast<int>(width);
	height = static_cast<int>(height);

	vec3b bg(background[0], background[1], background[2]);

	image<vec3b> img(width, height);

	//QUI STAI METTENDO IL BG, POI DEVI INSERIRE LE FIGURE


	for (unsigned i = 0; i < img.height(); i++) {
	
		for (unsigned j = 0; j < img.width(); j++) {
		
			img(j,i) = bg;
		
		
		}
	}

	//trasforiamo data da array a vec3b
	std::vector<vec3b> data3c;

	for (int i = 0; i < data.size(); i+=3) {

		vec3b pixel (data[i], data[i + 1], data[i + 2]);
		data3c.push_back(pixel);
		
	
	}


	image_w = static_cast<int>(image_w);
	image_h = static_cast<int>(image_h);
	image<vec3b> data_img(image_w, image_h);

	//riempi data_img

	for (unsigned i = 0; i < image_h; i++) {
	
		for (unsigned j = 0; j < image_w; j++) {
		
			data_img(j,i) = data3c[i*image_w +j];
		
		
		}
	}
	

	paste(img, data_img, x, y);



	
    

	// Dal file UBJ devo estrarre le informazioni sulle immagini da incollare su img 

	// Output in formato PPM
	
	writeP6(sOutput,img);




	//if (!writeP6(sOutput, img))
		//return EXIT_FAILURE;

	return EXIT_SUCCESS;
}


int main(int argc, char *argv[]) {

	if (argc != 3) {

		std::cout << "errore riga di comando";
		return EXIT_FAILURE;
	
	}



	string sInput = argv[1];
	string sOutput = argv[2];

	return convert(sInput, sOutput);
}
