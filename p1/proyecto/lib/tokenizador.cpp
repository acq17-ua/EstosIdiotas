#include "tokenizador.h"
#include <iostream>

#include <sys/stat.h> 		// para metadatos de archivos
#include <sys/mman.h> 		// para memory mapping
#include <fcntl.h> 			// para acceso a archivos
#include <unistd.h> 		// set size of output files

//////////
// save //
//////////

enum casosEspeciales {
						UDEAM,
						DEAM,
						EAM,
						AM,
						M
};

bool delimiters[256] = {0};

constexpr int conversion[256] = 	{
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

// URL    ->    _:/.?&-=#@
// decim  -> 	.,
// email  ->   	.-_@
// acron  -> 	.
// multiw -> 	-

// estados necesarios para expresar que caracteres son excepciones para cada caso especial
// 0: no es excepci�n
// 1: exc para solo decimal
// 2: exc para solo URL
// 3: exc para solo URL e email
// 4: exc para solo URL, email y multipalabra
// 5: exc para solo URL, decimal, email, acronimo y multipalabra
constexpr char exceptions[256] = 	{
						  0,   0,   0,   0,   0,   0,   0,   0,   0,   0, 
						  0,   0,   0,   0,   0,   0,   0,   0,   0,   0, 
						  0,   0,   0,   0,   0,   0,   0,   0,   0,   0, 
						  0,   0,   0,   0,   0,   2,   0,   0,   2,   0, 
						  0,   0,   0,   0,   1,   4,   5,   2,   0,   0, 
						  0,   0,   0,   0,   0,   0,   0,   0,   2,   0, 
						  0,   2,   0,   2,   3,   0,   0,   0,   0,   0, 
						  0,   0,   0,   0,   0,   0,   0,   0,   0,   0, 
						  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
						  0,   0,   0,   0,   0,   3,  	0, 	 0,   0,   0,
						  0,   0,   2,   0,   2,   0,   0,   0,   0,   0,
						  0,   0,   2,   0,   0,   2,   0,   0,   0,   0,
						  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
						  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
						  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
						  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
						  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
						  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
						  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
						  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
						  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
						  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
						  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
						  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
						  0,   0,   0,   0,   0,   0,   0
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
	delimiters[32] = true; // space
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
	this->delimiters = ",;:.-/+*\\ '\"{}[]()<>�!�?&#=\t@";

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
	delimiters.clear();
}

Tokenizador& Tokenizador::operator= (const Tokenizador& t)
{
	this->delimiters = t.delimiters;
	this->casosEspeciales = t.casosEspeciales;
	this->pasarAminuscSinAcentos = t.pasarAminuscSinAcentos;

	return *this;
}

void print_list(list<string>& cadena) {

	cout << "tokens for now: [";
	list<string>::const_iterator itCadena;
        for(itCadena=cadena.begin();itCadena!=cadena.end();itCadena++)
        {
                cout << (*itCadena) << ", ";
        }
        cout << "]" << endl;

}

void TokenizarCasosEspeciales( const string& str, list<string>& tokens )
{
	// True start of index, to be able to come back in case
	// we advance i under a pressumption that later becomes false
	int token_start = 0;
	int at_position = 0;
	
	// army of flags
	bool canBeUrl 				= true,
		 		start_checked 	= false,
		 canBeDecimal 			= false,
				heading_zero 	= false,
				just_dot 		= false,
		 canBeEmail 			= true,
		 		after_at 		= false,
		 canBeAcronym 			= true,
		 canBeMultiword 		= true;

	string curr_token = "";
	string aux;

	enum casosEspeciales estado;

	// LOOPING THROUGH ENTIRE STRING
	for( int i=0 ; i < str.size(); i++ ) {   	

		read_start:

		print_list(tokens);
		cout << "i: " << i << endl;

		estado = UDEAM;
		curr_token.clear();
		token_start = i;
		just_dot = after_at = heading_zero = at_position = 0;

		// TODO not that
		if( string(str, i, i+4) == "ftp:" ) 
			curr_token = string(str,i,i+4);
		else if( string(str, i, i+5) == "http:" ) 
			curr_token = string(str,i,i+5);
		else if( string(str, i, i+6) == "https:" ) 
			curr_token = string(str,i,i+6);
		else
			estado = DEAM;
 
		canBeEmail 		= (str[i] != '@');
		canBeAcronym 	= delimiters['.']; 	// thats literally it
		canBeMultiword 	= delimiters['-']; 	// thats literally it

		switch( estado ) {

			// CHECK URL: cant fuck this up
			case UDEAM:
				cout << "enter url" << endl;
				for( ; i<str.size(); i++ ) {
			
					switch( str[i] ) {

						case '_':
						case ':':
						case '/':
						case '.':
						case '?':
						case '&':
						case '-':
						case '=':
						case '#':
						case '@': 								// allowed character
							curr_token += conversion[str[i]];
							break;

						default:
							if( delimiters[str[i]] ) {   		// true delimiter, token over
								
								tokens.push_back(curr_token);
								goto end;
							}
							else  								// content 
								curr_token += conversion[str[i]];
					}
				}
		
				break;

			// CHECK DECIMAL
			case DEAM:	
				cout << "enter decimal" << endl;	
				// remove heading .,
				while( i<str.size() ) {
					//cout << "decimal i: " << i << endl;
					switch( str[i] ) {

						case '.': 
						case ',':
							if( delimiters[','] )
								canBeAcronym = false;
							i++;
							break;
						case 0: case 1: case 2: case 3: case 4:
						case 5: case 6: case 7: case 8: case 9:
							goto decimal_correct;
						default:
							estado = EAM;
							goto read_email;

					}   
				}
				decimal_correct:
				// ...except for one
				if( token_start!=i ) {
					//cout << curr_token << "-> dassit" << endl;
					curr_token += '0' + str[i-1];
					canBeEmail = after_at; 
					//cout << "dec crr_token: " << curr_token << endl;
				}

				// read rest of number
				for( ; i<str.size(); i++ ) {

					switch( str[i] ) {

						case ',':
							if( delimiters[','] )
								canBeAcronym = false;
							
						case '.':

							if( !just_dot ) { 					// single dot, we continue

								just_dot = true;
								curr_token += str[i];
							}

							else if( delimiters[str[i]] ) { 		// repeated dots, token over (at i-1)
								
								tokens.push_back(string(curr_token, 0, curr_token.size()-1));
								i--;
								goto read_start; 				// (not end bc we dont want i incremented)

							}
							else {   							// repeated dots, not delimited but not decimal
								//curr_token += conversion[str[i]];
								// let email take care of this one
								estado = EAM;
								//cout << "no, jumpin here" << endl;
								goto read_email;
							}

							break;

						case '0': case '1': case '2': case '3': case '4':
						case '5': case '6': case '7': case '8': case '9': 								
							// content
							curr_token += str[i];
							just_dot = false;
							break;
						case '@':
							after_at = true;

						default:
							if( delimiters[str[i]] ) {  		// true delimiter

								tokens.push_back(curr_token);
								goto end;

							}
							else { 								// not delimited but not decimal
								//curr_token += conversion[str[i]];
								// let email take care of this one
								estado = EAM;
								//cout << "jumpin here" << endl;
								goto read_email;
							}
					}
				}

				break;

			// CHECK EMAIL
			case EAM:
				read_email:
				cout << "enter email" << endl;	

				if( canBeEmail ) { // TODO comprobar canbeacronym still true
					// loop before @
					cout << i << " " << str.size() << endl;
					for( ; i<str.size(); i++ ) {

						//cout << "at i " << i << endl;
						switch( str[i] ) {
							case '.':  					// not an email
								goto read_acronym;
							
							case '-': 					// not an email
								
								if( delimiters['-'] ) {
									estado = M;
									goto read_multiword;
								}
								else {
									estado = AM;
									goto read_acronym;
								}
							
							case '_': 					// not an email
								if( delimiters['_'] ) { // delimited as regular token
									tokens.push_back(curr_token);
									goto end;
								}
								else {				// could still be AM
									estado = AM;
									goto read_acronym;
								}
							case '@':
								at_position = i * delimiters['@'];  // terminamos este bucle
								i++;
								goto after_at_loop;

							default:
								if( delimiters[str[i]] )  {	// not an email
									estado = AM;
									goto read_acronym;
								}
								
								else {					// filler character
									curr_token += conversion[str[i]];
									//cout << "curr token is " << curr_token << endl;
								}
						}							
					}
					// loop after @
					after_at_loop:
					for( ; i<str.size(); i++ ) {
						
						switch( str[i] ) {

							case '@':   						
								if( delimiters['@'] ) {  		// delimits
									tokens.push_back(string(curr_token, 0, curr_token.size()-1));
									goto end;
								}
								else { 							// not an email anymore, not delimited
									curr_token += '@';
									goto read_acronym;
								}
								break;

							case '-': // if delim, acron would NOT like that
							case '_': // TODO tienen que ir por enmedio de caracteres normales
								canBeAcronym = !delimiters[str[i]];

							case '.': 
								if( just_dot ) { // not an email anymore

									if( delimiters['@'] ) { // cortamos en el @
										tokens.push_back(string(curr_token, 0, at_position));
										i = at_position;
										goto end;
									}
								}
								else
									just_dot = true;


								curr_token += str[i];
								break;
							
							default:
								// check delimiter
								if( delimiters[str[i]] ) { 
									
									// arios@&
									if( at_position == i ) { // Not an email
										
										tokens.push_back(string(curr_token, 0, curr_token.size()-1));
										goto end;
									}
									// arios@iuii&
									else { 								// true delimiter
										tokens.push_back(curr_token);
										goto end;
									}
								}
								else { 									// filler character
									just_dot = false;
									curr_token += conversion[str[i]];
								}
								break;

						}

					}


				}
				break;

			// check acronym
			case AM:
				read_acronym:
				printf("enter acronym (%d)\n", i);	

				if( canBeAcronym ) {
					for( ; i<str.size(); i++ ) {

						switch( str[i] ) {
							case '.':
								if( just_dot ) { // not an acronym anymore

									tokens.push_back(string(curr_token, 0, curr_token.size()-1));
									goto end;
								}
								else {
									just_dot = true;
									curr_token += '.';
								}
								break;
							default: 		// filler character
							
								if( delimiters[str[i]] ) { // true delimiter
									
									tokens.push_back(string(curr_token, 0, curr_token.size()-just_dot));
									//i++;
									goto end;
								}
								just_dot = false;
								curr_token += conversion[str[i]];
								break;
						}
					}
				} 
				else
					goto read_multiword;
				break;

			// check multiword
			case M:
				read_multiword:
				cout << "enter multiw" << endl;	

				if( canBeMultiword ) {

					for( ; i<str.size(); i++ ) {

						switch( str[i] ) {
							case '-':
								if( just_dot ) { // not a multiword anymore

									tokens.push_back(string(curr_token, 0, curr_token.size()-1));
									goto end;
								}
								else {
									just_dot = true;
									curr_token += '-';
								}
								break;
							default: 				// filler character
								
								if( delimiters[str[i]] ) {
									tokens.push_back(curr_token);
									goto end;
								}
								just_dot = false;
								curr_token += conversion[str[i]];
								break;
						}

					}
				}
				else
					goto read_token;
				break;

			default:
				read_token:
				for( ; i<str.size(); i++ ) { 		// just read until delim or EOS
					if( delimiters[str[i]] ) {
						tokens.push_back(curr_token);
						goto end;
					}
					curr_token += conversion[str[i]];
				}
				tokens.push_back(curr_token);
					goto end;
		}


		end:
			continue;
	}
}

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
	else
		TokenizarCasosEspeciales(str, tokens);
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

