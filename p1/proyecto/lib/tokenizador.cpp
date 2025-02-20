#include "tokenizador.h"
#include <iostream>
#include <cstdlib>
#include <string.h>
#include <fstream>

/////////
// AUX //
/////////

string quitarRepetidos(string str)
{
	return "";
}

///////////
// RESTO //
///////////

ostream& operator<<(ostream& os, const Tokenizador& t)
{
	os << " DELIMITADORES: " << t.delimiters 
		<< " TRATA CASOS ESPECIALES: " << t.casosEspeciales 
		<< " PASAR A MINUSCULAS Y SIN ACENTOS: " << t.pasarAminuscSinAcentos;
	return os;
}	 

Tokenizador::Tokenizador ()
{
	this->delimiters = ",;:.-/+*\\ '\"{}[]()<>¡!¿?&#=\t@";
	this->casosEspeciales = true;
	this->pasarAminuscSinAcentos = false;
}	

Tokenizador::Tokenizador(const string& delimitadoresPalabra, const bool& kcasosEspeciales, const bool& minuscSinAcentos)
{
	this->delimiters = quitarRepetidos(delimitadoresPalabra);
	this->casosEspeciales = kcasosEspeciales;
	this->pasarAminuscSinAcentos = minuscSinAcentos;
}	

Tokenizador::Tokenizador (const Tokenizador& t)
{
	this->delimiters = t.delimiters;
	this->casosEspeciales = t.casosEspeciales;
	this->pasarAminuscSinAcentos = t.pasarAminuscSinAcentos;
}

Tokenizador::~Tokenizador () 
{
	delimiters[0] = '\0';
}

Tokenizador& Tokenizador::operator= (const Tokenizador& t)
{
	this->delimiters = t.delimiters;
	this->casosEspeciales = t.casosEspeciales;
	this->pasarAminuscSinAcentos = t.pasarAminuscSinAcentos;

	return *this;
}

void Tokenizador::Tokenizar (const string& str, list<string>& tokens) const
{
	string::size_type lastPos = str.find_first_not_of(this->delimiters, 0);
	string::size_type pos = str.find_first_of(this->delimiters, lastPos);

	while( string::npos != pos && string::npos != lastPos ) {

		tokens.push_back(str.substr(lastPos, pos-lastPos));
		lastPos = str.find_first_not_of(delimiters, pos);
		pos = str.find_first_of(delimiters, lastPos);
	}
}

bool Tokenizador::Tokenizar (const string& i, const string& f) const
{
	ifstream input;
	ofstream output;
	string contents;

	input.open(i);
	
	if( !input ) {
		cerr << "El archivo " << i << " no existe o no es accesible\n";
		return false;
	}

	getline(input, contents);


	list<string> tokens;
	this->Tokenizar(contents, tokens);
	
	output.open(f);

	if( !output ) {
		cerr << "El archivo " << i << " no existe o no es accesible\n";
		return false;
	}
	
	list<string>::const_iterator it;
	for (it=tokens.begin(); it!=tokens.end(); ++it)
		output << it->c_str() << endl;

	return true;
}

bool Tokenizador::Tokenizar (const string & i) const 
{
	ifstream input;
	ofstream output;
	string contents;

	input.open(i);
	
	if( !input ) {
		cerr << "El archivo " << i << " no existe o no es accesible\n";
		return false;
	}

	getline(input, contents);
	cout << "contents: " <<  contents << endl;

	list<string> tokens;
	this->Tokenizar(contents, tokens);
	
	output.open(i+".tk");

	if( !output ) {
		cerr << "El archivo " << i << " no existe o no es accesible\n";
		return false;
	}
	
	list<string>::const_iterator it;
	for (it=tokens.begin(); it!=tokens.end(); ++it)
		output << it->c_str() << endl;

	return true;
}

// LA IMPORTANTE
bool Tokenizador::TokenizarListaFicheros (const string& i) const 
{
	return false;
} 

bool Tokenizador::TokenizarDirectorio (const string& i) const
{
	return false;
}

// S
void Tokenizador::DelimitadoresPalabra(const string& nuevoDelimiters) { this->delimiters = nuevoDelimiters; }

void Tokenizador::AnyadirDelimitadoresPalabra(const string& nuevoDelimiters)
{
	this->delimiters += nuevoDelimiters;
	// controlar repeticiones!!
}

// G
string Tokenizador::DelimitadoresPalabra() const { return delimiters; } 

// S 
void Tokenizador::CasosEspeciales (const bool& nuevoCasosEspeciales) { this->casosEspeciales = nuevoCasosEspeciales; }

// G
bool Tokenizador::CasosEspeciales () { return casosEspeciales; }

// S
void Tokenizador::PasarAminuscSinAcentos (const bool& nuevoPasarAminuscSinAcentos) { this->pasarAminuscSinAcentos = nuevoPasarAminuscSinAcentos; }

// G
bool Tokenizador::PasarAminuscSinAcentos () { return pasarAminuscSinAcentos; }

