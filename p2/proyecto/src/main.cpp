#include <iostream>
#include "../include/indexadorHash.h"
using namespace std;

int main() {

	IndexadorHash ind = IndexadorHash( 	"StopwordsEspanyol.txt",
										",;:.-/+*\\ '\"{}[]()<>�!�?&#=\t@",
										false,
										true,
										"indice",
										1,
										true);
	
	cout << ind.Indexar("listaFicheros_corto.txt") << endl;
	

	return 0;
}
