#include "tokenizador.h"
#include <iostream>
#include <cstdlib>

friend ostream& operator<<(ostream&, const Tokenizador&);	 

Tokenizador ()
{
	this->delimiters = ",;:.-/+*\\ '\"{}[]()<>¡!¿?&#=\t@";
	this->casosEspeciales = true;
	this->pasarAminuscSinAcentos = false;
}	

Tokenizador (const string& delimitadoresPalabra, const bool& kcasosEspeciales, const bool& minuscSinAcentos)
{
	this->delimiters = quitarRepetidos(delimitadoresPalabra);
	this->casosEspeciales = kcasosEspeciales;
	this->pasarAminuscSinAcentos = minuscSinAcentos;
}	

Tokenizador (const Tokenizador& t)
{
	this->delimiters = t->delimiters;
	this->casosEspeciales = t->casosEspeciales;
	this->pasarAminuscSinAcentos = t->pasarAminuscSinAcentos;
}

~Tokenizador () 
{
	delimiters[0] = '\0';
}

Tokenizador& operator= (const Tokenizador& t)
{
	this->delimiters = t->delimiters;
	this->casosEspeciales = t->casosEspeciales;
	this->pasarAminuscSinAcentos = t->pasarAminuscSinAcentos;
}

void Tokenizar (const string& str, list<string>& tokens) const;

bool Tokenizar (const string& i, const string& f) const; 

bool Tokenizar (const string & i) const;

// LA IMPORTANTE
bool TokenizarListaFicheros (const string& i) const; 

bool TokenizarDirectorio (const string& i) const; 

// S
void DelimitadoresPalabra(const string& nuevoDelimiters) { this->nuevoDelimiters = nuevoDelimiters; }

void AnyadirDelimitadoresPalabra(const string& nuevoDelimiters)
{
	this->delimiters += nuevoDelimiters;
	// controlar repeticiones!!
}

// G
string DelimitadoresPalabra() const { return delimiters; } 

// S 
void CasosEspeciales (const bool& nuevoCasosEspeciales) { this->casosEspeciales = nuevoCasosEspeciales; }

// G
bool CasosEspeciales () { return casosEspeciales; }

// S
void PasarAminuscSinAcentos (const bool& nuevoPasarAminuscSinAcentos) { this->pasarAminuscSinAcentos = nuevoPasarAminuscSinAcentos; }

// G
bool PasarAminuscSinAcentos () { return pasarAminuscSinAcentos; }

//////////////////////////
// AUX A PARTIR DE AQUI //
//////////////////////////

string quitarRepetidos(string& str);

