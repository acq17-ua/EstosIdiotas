#include "tokenizador.h"
#include <iostream>
// #include <cstdlib> 		// for list

#include <fstream> 
#include <sys/stat.h> 		// to check smthng is dir or not
#include <dirent.h> 		// to read dir
#include <bits/stdc++.h> 	// for vector
#include <queue> 			// to read folders recursively

/////////
// AUX //
/////////

string quitarRepetidos(string str)
{
	return "";
}

bool leerArbolCarpetas(string root, vector<string>& files )
{
	queue<string> folders;
	DIR *dirp;
	struct dirent *dent;
	struct stat fileInfo;

	string my_path = "";

	// root folder
	folders.push(root);

	// read folders as they are added
	while(!folders.empty()) {

		dirp = opendir(folders.front().c_str());

		// "files/"
		my_path = folders.front() + "/";

		folders.pop();
		
		// iterate each file/folder inside
		for( ; (dent = readdir(dirp)) ; ) {

				// this is -1 when the file doesn't exist or otherwise fails
				// shouldn't be needed but if something fails try this

			string filename = my_path + dent->d_name;

			if( filename == my_path+"." or filename == my_path+".." ) 
				continue;

			// if correct reading
			if(lstat(filename.c_str(), &fileInfo) == 0) {

				// is a folder
				if( S_ISDIR(fileInfo.st_mode)) 
					folders.push(filename);  	
				
				// we assume it's a file then
				else {
					files.push_back(filename); 
					// here is where you would read it TODO
				}
			}
		}
		closedir(dirp);
	}
	return true;
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
	ifstream inputFiles;
	vector<string> fileList;
	string file;

	inputFiles.open(i);
	if( !inputFiles ) {
		cerr << "El archivo " << i << " no existe o no es accesible\n";
		return false;
	}

	while(getline(inputFiles, file))
		fileList.push_back(file);

	for(int i=0; i<fileList.size(); i++)
		this->Tokenizar(fileList[i], fileList[i]+".tk");

	return true;
} 

// esto funciona pero jo-der debe ser super lento
bool Tokenizador::TokenizarDirectorio (const string& i) const
{
	queue<string> folders; // esto podría cambiarlo por un array que redimensiono de 15 en 15 cuando veo que hace falta
	DIR *dirp;
	struct dirent *dent;
	struct stat fileInfo;
	ifstream inputFile;

	string my_path = "";

	// root folder
	folders.push(i);

	// read folders as they are added
	while(!folders.empty()) {

		dirp = opendir(folders.front().c_str());

		// "files/"
		my_path = folders.front() + "/";

		folders.pop();
		
		// iterate each file/folder inside
		for( ; (dent = readdir(dirp)) ; ) {

			string filename = my_path + dent->d_name;
			
			if( (string)dent->d_name=="." or (string)dent->d_name==".." ) 
				continue;

			// if correct reading
			if(lstat(filename.c_str(), &fileInfo) == 0) {

				// is a folder
				if( S_ISDIR(fileInfo.st_mode)) 
					folders.push(filename);  	
				
				// we assume it's a file then
				else {
					if(!this->Tokenizar(filename))
						return false;
				}
			}
		}
		//closedir(dirp);
	}
	return true;
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

