#include "tokenizador.h"
#include <iostream>

#include <sys/stat.h> 		// para metadatos de archivos
#include <sys/mman.h> 		// para memory mapping
#include <fcntl.h> 			// para acceso a archivos
#include <unistd.h>

//////////
// save //
//////////

bool delimiters[256] = {0};
const int conversion[256] = 	{
						  0,   1,   2,   3,   4,   5,   6,   7,   8,   9, 
						 10,  11,  12,  13,  14,  15,  16,  17,  18,  19, 
						 20,  21,  22,  23,  24,  25,  26,  27,  28,  29, 
						 30,  31,  32,  33,  34,  35,  36,  37,  38,  39, 
						 40,  41,  42,  43,  44,  45,  46,  47,  48,  49, 
						 50,  51,  52,  53,  54,  55,  56,  57,  58,  59, 
						 60,  61,  62,  63,  64,  97,  98,  99, 100, 101,   // letras mayusculas
						102, 103, 104, 105, 106, 107, 108, 109, 110, 111,   //
						112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 	//
						122,  91,  92,  93,  94,  95,  96,  97,  98,  99,   // letras minusculas
						100, 101, 102, 103, 104, 105, 106, 107, 108, 109,   //
						110, 111, 112, 113, 114, 115, 116, 117, 118, 119, 	//
						120, 121, 122, 123, 124, 125, 126, 127, 128, 129, 	//
						130, 131, 132, 133, 134, 135, 136, 137, 138, 139, 
						140, 141, 142, 143, 144, 145, 146, 147, 148, 149, 
						150, 151, 152, 153, 154, 155, 156, 157, 158, 159, 
						160, 161, 162, 163, 164, 165, 166, 167, 168, 169, 
						170, 171, 172, 173, 174, 175, 176, 177, 178, 179, 
						180, 181, 182, 183, 184, 185, 186, 187, 188, 189, 
						190, 191, 192,  97, 194, 195, 196, 197, 198, 199, 	// acentos 
						200, 101, 202, 203, 204, 105, 206, 207, 208, 209,   //
						210, 111, 212, 213, 214, 215, 216, 217, 117, 219, 
						220, 221, 222, 223, 224, 225, 226, 227, 228, 229, 
						230, 231, 232, 233, 234, 235, 236, 237, 238, 239, 
						240, 241, 242, 243, 244, 245, 246, 247, 248, 249, 
						250, 251, 252, 253, 254, 255 
						};


/////////
// AUX //
/////////
string procesar_delimitadores(string s)
{
	int n = s.size();
    string ans = "";
 
	for (int i = 0; i < n; i++) {
		
		if( !delimiters[s[i]] ) {
			delimiters[s[i]] = true;
			ans += s[i];
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

	for( auto c : this->delimiters )
		delimiters[c] = true;

	this->casosEspeciales = true;
	this->pasarAminuscSinAcentos = false;
}	

Tokenizador::Tokenizador(const string& delimitadoresPalabra, const bool& kcasosEspeciales, const bool& minuscSinAcentos)
{
	this->delimiters = procesar_delimitadores(delimitadoresPalabra);
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

// TODO
void Tokenizador::Tokenizar (const string& str, list<string>& tokens) const
{
	tokens.clear();

	if( !this->casosEspeciales ) {
		string::size_type lastPos = str.find_first_not_of(this->delimiters, 0);
		string::size_type pos = str.find_first_of(this->delimiters, lastPos);

		while( string::npos != pos && string::npos != lastPos ) {

			tokens.push_back(str.substr(lastPos, pos-lastPos));
			lastPos = str.find_first_not_of(delimiters, pos);
			pos = str.find_first_of(delimiters, lastPos);
		}
	}
	else {   // control de casos especiales

		bool canBeUrl 		= true,
			 canBeDecimal 	= true,
			 canBeEmail 	= true,
			 canBeAcronym 	= true,
			 canBeMultiword = true;

		int length = str.length();
		int i = 0, 
			token_start = 0;

		while( i < length ) {

			start_reading_token:

			// quick checks
			if( str[0] == ',' ) {
				canBeAcronym = false;
				canBeUrl = false;
				goto decimales;
			}
			else if( str[0] == '.' || (str[0] >= 48 str[0] <= 57 )  ) { // . or is number
				canBeUrl = false;
				goto decimales;
			}

			/****************/
			/***** URL ******/
			/****************/

			// start checker
			if( length >= 5 ) {
					
				i = 3 * ( 	conversion[str[0]] == 'h' &&
							conversion[str[1]] == 't' &&
							conversion[str[2]] == 't' &&
							conversion[str[3]] == 'p');

				if( i ) {
					// situate i at end of http:
					i += 2 * ( i==3 && conversion[str[4]]==':' );

					// situate i at the end of https:
					i += 3 * ( length >= 6 && conversion[str[4]]=='s' && conversion[str[5]]==':'); 
				}
			}

			if( !i && length >= 4 ) {   // then its not http: or https:

				canBeUrl = ( 	conversion[str[0]] == 'f' &&
								conversion[str[1]] == 't' &&
								conversion[str[2]] == 'p' &&
								conversion[str[3]] == ':');
				i = canBeUrl*5;

			}
			else
				canBeUrl = false;

			// get the rest
			if( canBeUrl ) {
				
				for( i ; i<length; i++ ) {
					if( delimiters[str[i]] /*y no es uno de los caracteres reservados*/ ) {
						tokens.push_back(string(token_start, i-1));
						token_start = i+1;
						goto start_reading_token;
					}
				}
			}

			/****************/
			/** decimales ***/
			/****************/
			decimales:
			
			bool add_zero = false, just_dot = false;

			if( str[i] == '.' ) {
				i++;
				add_zero = true;
			}
			if( str[i] == ',' ) {
				i++;
				add_zero true;
				canBeAcronym = false;
			}
			for( i ; i<length; i++ ) {
				
				if( just_dot && (str[i]=='.') ) {   // no longer a decimal or an acronym

					canBeDecimal = canBeAcronym = false;

					// skip dots if they're delimiters? i dont fucking know dude

				}
				just_dot = (str[i]=='.');
				canBeAcronym *= !(str[i]==',');
				


				if( delimiters[str[i]] ) {
					if( add_zero )
						tokens.push_back("0" + string(token_start, i-1));
					else
						tokens.push_back(string(token_start, i-1));
					token_start = i+1;
					goto start_reading_token;
				}
			}


		}
	}
}

bool Tokenizador::Tokenizar (const string& input, const string& output) const
{
	struct stat fileInfo;
	char* map;
	int fd, i=0;
	list<string> tokens;
	size_t fileSize;

	//////////
	// READ //

	fd = open(input.c_str(), O_RDONLY);

	if( lstat(input.c_str(), &fileInfo) == 0 ) {
		
		if(fd == -1) {
			cerr << "Failed to read file\n";
			return false;
		}
		
		fileSize = fileInfo.st_size;
		map = (char*)mmap(0, fileSize, PROT_READ, MAP_SHARED, fd, 0);
		
		if( map == 	MAP_FAILED ) {
			cerr << "Failed to read file\n";
			return false;
		}
		// get just one line
		for( i=0; i<fileInfo.st_size && map[i]!='\n'; i++); 
		
		fileSize = i * sizeof(char); // cuz it only reads one line but there may be more

		this->Tokenizar(string(map, map+i), tokens);
	}
	else return false;

	///////////
	// WRITE //

	fd = open(output.c_str(), O_RDWR | O_CREAT | O_TRUNC, (mode_t)0600);

	// set file size (will fail catastrophically if you remove this)
	ftruncate(fd, fileSize);

	if( lstat(output.c_str(), &fileInfo) == 0 ) {
		
		if(fd == -1) {
			cerr << "Failed to write into .tk file\n";
			return false;
		}
		map = (char*)mmap(0, fileSize, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
		
		if( map == 	MAP_FAILED ) {
			cerr << "Failed to write into .tk file\n";
			return false;
		}

		// copy tokens list into file
		for( auto const& it : tokens ) {

			for( i=0; i<it.size(); i++ )
				map[i] = it[i];

			map[i] = '\n';

		}
		return true;
	}
	else return false;
}

bool Tokenizador::Tokenizar (const string & input) const 
{
	struct stat fileInfo;
	char* map;
	int fd, i=0;
	list<string> tokens;
	const string output = input+".tk";
	size_t fileSize;

	//////////
	// READ //

	fd = open(input.c_str(), O_RDONLY);

	if( lstat(input.c_str(), &fileInfo) == 0 ) {
		
		if(fd == -1) {
			cerr << "Failed to read file\n";
			return false;
		}
		
		fileSize = fileInfo.st_size;
		map = (char*)mmap(0, fileSize, PROT_READ, MAP_SHARED, fd, 0);
		
		if( map == 	MAP_FAILED ) {
			cerr << "Failed to read file\n";
			return false;
		}
		// get just one line
		for( i=0; i<fileInfo.st_size && map[i]!='\n'; i++); 
		
		fileSize = i * sizeof(char); // cuz it only reads one line but there may be more

		this->Tokenizar(string(map, map+i), tokens);
	}
	else return false;

	///////////
	// WRITE //

	fd = open(output.c_str(), O_RDWR | O_CREAT | O_TRUNC, (mode_t)0600);

	// set file size (will fail catastrophically if you remove this)
	ftruncate(fd, fileSize);

	if( lstat(output.c_str(), &fileInfo) == 0 ) {
		
		if(fd == -1) {
			cerr << "Failed to write into .tk file\n";
			return false;
		}
		map = (char*)mmap(0, fileSize, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
		
		if( map == 	MAP_FAILED ) {
			cerr << "Failed to write into .tk file\n";
			return false;
		}

		// copy tokens list into file
		for( auto const& it : tokens ) {

			for( i=0; i<it.size(); i++ )
				map[i] = it[i];

			map[i] = '\n';

		}
		return true;
	}
	else return false;
}

// LA IMPORTANTE
bool Tokenizador::TokenizarListaFicheros (const string& input) const 
{
	string file;
	struct stat fileInfo;
	int fd;
	char* map;
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

				if( lstat(file.c_str(), &fileInfo) == 0 ) {

					if( S_ISDIR(fileInfo.st_mode) ) 
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

