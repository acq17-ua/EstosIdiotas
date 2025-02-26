#include "tokenizador.h"
#include <iostream>

#include <fstream> 	// este compa ya está muerto
#include <sys/stat.h> 		// para metadatos de archivos
#include <sys/mman.h> 		// para memory mapping
#include <fcntl.h> 			// para acceso a archivos
#include <unordered_map>  	// para quitar repetidos de string


/////////
// AUX //
/////////
string quitar_repetidos(string s)
{
    unordered_map<char, int> exists;
	int n = s.size();
 
    string ans = "";
    for (int i = 0; i < n; i++) {
        if (exists.find(s[i]) == exists.end()) {
            ans.push_back(s[i]);
            exists[s[i]]++;
        }
    }
    return ans;
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
	this->delimiters = quitar_repetidos(delimitadoresPalabra);
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

bool http_checker(string& str, int start, int end) {

	if( (end-start) >= 4 ) {
		if (str[start]=='f' && str[start+1]=='t' && str[start+2]=='p' && str[start+3]==':')
			return true;

		if (str[start]=='h' && str[start+1]=='t' && str[start+2]=='t' && str[start+3]=='p') 
			return ((str.length()==5 && str[start+4]==':' ) || (str[start+4]=='s' && str[start+5]==':'));
	}
	return false;
}

void Tokenizador::Tokenizar (const string& str, list<string>& tokens) const
{
	tokens.clear();

	if( this->casosEspeciales ) {
		string::size_type lastPos = str.find_first_not_of(this->delimiters, 0);
		string::size_type pos = str.find_first_of(this->delimiters, lastPos);

		while( string::npos != pos && string::npos != lastPos ) {

			tokens.push_back(str.substr(lastPos, pos-lastPos));
			lastPos = str.find_first_not_of(delimiters, pos);
			pos = str.find_first_of(delimiters, lastPos);
		}
	}
	else {   // control de casos especiales

		int length = str.length();
		string token = "",
			   aux = "";
		int token_start = 0, 
			token_end = 0;
		
		bool been_letter = false,
			 canbe_url = true;


		for( int i=0; i<length; i++ ) {

			// url, email, acronimo, token
			if( isalpha(str[i]) ) {

				been_letter = true;

			}
			// url o delim
			if( str[i] == ':' ) {

				if(canbe_url) {
					//canbe_url = http_checker(str, token_start, i);
					

				}

			}


		}

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
bool Tokenizador::TokenizarListaFicheros (const string& input) const 
{
	string file;
	struct stat fileInfo, child_fileInfo;
	int fd;
	char *map;
	int prev_it = 0;

	fd = open(input.c_str(), O_RDONLY);
	
	if( lstat(input.c_str(), &fileInfo) == 0 ) {
		
		if(fd == -1) {
			cerr << "Failed to read file\n";
			return false;
		}
		map = (char*)mmap(0, fileInfo.st_size, PROT_READ, MAP_SHARED, fd, 0);
		
		if( map == 	MAP_FAILED ) {
			cerr << "Failed to read file\n";
			return false;
		}

		// we're in the clear
		for (int it = 0; it <=fileInfo.st_size; ++it) {

			if( map[it] == '\n' ) {
				
				file = string(map+prev_it, map+it);
				prev_it = it+1;

				if( lstat(file.c_str(), &child_fileInfo) == 0 ) {

					if( S_ISDIR(child_fileInfo.st_mode) ) 
						this->TokenizarDirectorio(file);
					else 
						this->Tokenizar(file, file+".tk");
				}
				else 
					return false;
			}
		}	
		return true;
	}
	return false;
} 

bool Tokenizador::TokenizarDirectorio (const string& i) const
{
	system(("find " + i + " ! -type d ! -name '*.tk' > ./tokenizar_directorio_res.txt").c_str());
	bool result = TokenizarListaFicheros("tokenizar_directorio_res.txt");
	system("rm ./tokenizar_directorio_res.txt");
	return result;
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

