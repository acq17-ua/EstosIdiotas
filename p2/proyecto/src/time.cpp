#include <iostream>
#include<ctime>

#include "tokenizador.h"

using namespace std;

int main()
{
    std::clock_t start = std::clock();
    Tokenizador a("\t ,;:.-+/*_`'{}[]()!?&#\"\\<>", true, true);
    a.TokenizarDirectorio("./corpus"); // TODO EL CORPUS
    cout << "Ha tardado " << ((float)(std::clock()-start))/CLOCKS_PER_SEC << " segundos" << endl;
	
	//Tokenizador a("\t ,;:.-+/*_`'{}[]()!?&#\"\\<>", true, true);
	//a.Tokenizar("aas-100-mg-comprimidos-20-comprimidos.txt");
	
	
	return 0;
}
