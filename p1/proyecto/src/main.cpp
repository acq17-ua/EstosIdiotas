#include <iostream>
#include "../include/tokenizador.h"
#include <list>
#include <fstream>
#include <chrono>
using namespace std;

int main(int argc, char** argv)
{
	Tokenizador t = Tokenizador();	

	t.Tokenizar("input.txt");

	list<string>::const_iterator i;


	return 0;
}


