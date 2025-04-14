#include <iostream>
#include "../include/indexadorHash.h"
using namespace std;
#include <cstring>

int main() {

	IndexadorHash ind = IndexadorHash( 	"StopwordsEspanyol.txt",
										",;:.-/+*\\ '\"{}[]()<>�!�?&#=\t@",
										false,
										true,
										"./indice",
										1,
										true);
	
	cout << ind.Indexar("listaFicheros_corto.txt") << endl;
	ind.imprimir_full();
	
	cout << ind.GuardarIndexacion() << endl;
	
	IndexadorHash ind1 = IndexadorHash( "StopwordsEspanyol.txt",
										",;:.-/+*\\ '\"{}[]()<>�!�?&#=\t@",
										false,
										true,
										"./indice",
										1,
										true);

	cout  << ind.RecuperarIndexacion("./indice") << endl;
	
	cout << "we good" << endl;


/* 	string a = "testo";//, a_read;
	size_t str_size = a.size(), str_size_read;
	char* a_read;
	const char* a2 = a.c_str();
	str_size = strlen(a2); 

	cout << "a2: " << a2 << endl;
	cout << "str_size: " << str_size << endl;

	FILE* f = fopen("suckmadick", "wb");
	fwrite(&str_size, sizeof(size_t), 1, f);
	fwrite(a.c_str(), sizeof(char), str_size, f);

	fclose(f);

	f = fopen("suckmadick", "rb");
	fread(&str_size_read, sizeof(size_t), 1, f);
	cout << "read string size: " << str_size_read << endl;
	fread(a_read, sizeof(char), str_size_read, f);
	a_read[str_size_read] ='\0';

	fclose(f);

	cout << "Initial string: '" << a << "'    Read string: '" << a_read << "'\n";
 */
	return 0;
}
