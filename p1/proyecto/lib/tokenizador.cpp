#include "tokenizador.h"
#include <iostream>

#include <sys/stat.h> 		// para metadatos de archivos
#include <sys/mman.h> 		// para memory mapping
#include <fcntl.h> 			// para acceso a archivos
#include <unistd.h> 		// set size of output files
#include <cstring> 			// clear array with memset
#include <string> 			// find_first_not_of

const int PAGE_SIZE = sysconf(_SC_PAGE_SIZE);

/*
enum casosEspeciales {
						U,
						UDEAM,
						DEAM,
						EAM,
						EA,
						AM,
						AO, 		// puede ser acronimo pero no multipalabra
						M,
						X
};

// . , - @ son muy condicionantes
// quiero ahorrarme ifs
// Esto indica cuales son this->delimitadores
enum delimitadoresEspeciales {
								O,  		// U 	   	  	Y	
								P,        	// UA 			Y
								C, 			// U 			-
								G, 			// UM			Y
								A, 			// UE 			Y
								PC, 		// UDA 			Y
								PG, 		// UMA 			Y
								PA, 		// UAE			Y
								CG, 		// UM 			Y
								CA, 		// UE			Y
								GA, 		// UME 			Y
								PCG, 		// UDAM 		Y
								PCA, 		// UDAE 		Y
								PGA, 		// UMAE			Y
								CGA, 		// UME 			-
								PCGA 		// UDEAM 		Y
};
*/

//bool delimitadores[256] = {0};

constexpr unsigned char conversion[256] = 	{
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
						190, 191, 192,  97, 194, 195, 196, 197, 198, 199, 	// acentos mayus
						200, 101, 202, 203, 204, 105, 206, 207, 208, 241,   // Ñ
						210, 111, 212, 213, 214, 215, 216, 217, 117, 219, 
						220, 221, 222, 223, 224, 97, 226, 227, 228, 229,   	// acentos minus
						230, 231, 232, 101, 234, 235, 236, 105, 238, 239,   //
						240, 241, 242, 111, 244, 245, 246, 247, 248, 249,   //
						117, 251, 252, 253, 254, 255  						//
						};

// URL    ->    _:/.?&-=#@
// decim  -> 	.,
// email  ->   	.-_@
// acron  -> 	.
// multiw -> 	-

// estados necesarios para expresar que caracteres son excepciones para cada caso especial
// 0: no es excepci?n
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
string Tokenizador::procesar_delimitadores(string s) 
{
	int n = s.size();
    string delims = "";
 
	for( unsigned char c : this->delimiters ) 
		this->delimitadores[c] = false;

	for( unsigned char c : s ) 

		if( !this->delimitadores[c] ) {
			this->delimitadores[c] = true;
			delims += c;
		}
	
	if( this->casosEspeciales ) {
		this->delimitadores[(unsigned char)' '] = true;
		this->delimitadores[(unsigned char)'\0'] = true;
		this->delimitadores[(unsigned char)'\n'] = true;
	}
    return delims;
}

///////////
// RESTO //
///////////

ostream& operator<<(ostream& os, const Tokenizador& t)
{
	os 	<< "DELIMITADORES: " << t.delimiters 
		<< " TRATA CASOS ESPECIALES: " << t.casosEspeciales 
		<< " PASAR A MINUSCULAS Y SIN ACENTOS: " << t.pasarAminuscSinAcentos;
	return os;
}	 

Tokenizador::Tokenizador ()
{
	this->delimiters = this->procesar_delimitadores(",;:.-/+*\\ '\"{}[]()<>¡!¿?&#=\t@");
	this->casosEspeciales = true;
	this->pasarAminuscSinAcentos = false;
}	

Tokenizador::Tokenizador(const string& delimitadoresPalabra, const bool& kcasosEspeciales, const bool& minuscSinAcentos)
{
	this->casosEspeciales = kcasosEspeciales;
	this->pasarAminuscSinAcentos = minuscSinAcentos;
	this->delimiters = procesar_delimitadores(delimitadoresPalabra);
}	

Tokenizador::Tokenizador (const Tokenizador& t)
{
	this->delimiters = t.delimiters;
	this->casosEspeciales = t.casosEspeciales;
	this->pasarAminuscSinAcentos = t.pasarAminuscSinAcentos;
}

Tokenizador::~Tokenizador () 
{
	this->delimiters.clear();
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

bool Tokenizador::Tokenizar_ftp( const string& str, list<string>& tokens, int& i, string& curr_token ) const {

	
	int token_start = i;

	if( (str.size()-i)>=4 && ((str[i+1]=='t') & (str[i+2]=='p' & str[i+3]==':')) ) { 		// ftp:
		curr_token = "ftp:";
		i += 4;
	}
	else
		return false;

	if( (this->delimitadores[str[i]] & !exceptions[str[i]]>=2) || i==str.size() ) {  // http: and thats it
		curr_token.resize(curr_token.size()-1);
		i--;
		return false;
	}  

	char c;

	for( ; i<str.size(); i++ ) {
		
		if( this->pasarAminuscSinAcentos )
			c = conversion[(unsigned char)str[i]];
		else
			c = str[i];

		switch(c) {

			case '_':
			case ':':
			case '/':
			case '.':
			case '?':
			case '&':
			case '-':
			case '=':
			case '#':
			case '@':
				curr_token += c;
				break;
			default:
				if( !this->delimitadores[c] ) {
					curr_token += c;
				}
				else {
					tokens.push_back(curr_token);
					return true;
				}
		}
	}
	// si has llegado aqui, estas al final del string
	tokens.push_back(curr_token);
	return true;
}

bool Tokenizador::Tokenizar_http( const string& str, list<string>& tokens, int& i, string& curr_token ) const {

	int token_start = i;

	if( (str.size()-i)>=5 && ((str[i+1]=='t') & (str[i+2]=='t') & (str[i+3]=='p')) ) {

		if( str[i+4]==':' ) {   
			curr_token = "http:";												// http:
			i += 5;
		}
		else if( ( str.size()-i )>=6 && (str[i+4]=='s') & (str[i+5]==':') ) { 	// https:
			curr_token = "https:";
			i += 6;
		}
	}
	else return false;

	//cout << "in url is " << curr_token << endl;

	if( (this->delimitadores[str[i]] & !exceptions[str[i]]>=2) || i==str.size() ) {  // http: and thats it
		curr_token.resize(curr_token.size()-1);
		i--;
		return false;
	}  

	unsigned char c;

	for( ; i<str.size(); i++ ) {
		
		if( this->pasarAminuscSinAcentos )
			c = conversion[(unsigned char)str[i]];
		else
			c = str[i];

		switch(c) {

			case '_':
			case ':':
			case '/':
			case '.':
			case '?':
			case '&':
			case '-':
			case '=':
			case '#':
			case '@':
				curr_token += c;
				break;
			default:
				if( !this->delimitadores[c] ) {
					curr_token += c;
				}
				else {
					tokens.push_back(curr_token);
					return true;
				}
		}
	}
	// si has llegado aqui, estas al final del string
	tokens.push_back(curr_token);
	return true;
}

bool Tokenizador::Tokenizar_decimal( const string& str, list<string>& tokens, int& i, string& curr_token, char heading_zero ) const {

	unsigned char c;
	bool just_dot=false;
	int token_start=i;

	// hay heading .,
	if( heading_zero )
	{
		curr_token = "0";
		curr_token += heading_zero;
	}
	
	//cout << "decimal correcto: " << curr_token << endl;

	// 123.123,123.123,123.123,123.a  <== el caso con el que tuve pesadillas anoche
	// TODO esperar a los tests
	
	// Leer resto del número
	/// TODO Si te encuentras una coma intermedia, hay que guardar su posición para cortar ahí en caso de que el decimal falle
	for( ; i<str.size(); i++ ) {

		c = (unsigned char)str[i];

		switch(c) {

			case ',':  // TODO
			case '.':
				if( just_dot ) {   // its a normal token
					tokens.push_back(string(curr_token, 0, curr_token.size()-1));
					i--;
					return true;
				}
				just_dot = true;
				curr_token += c;
				continue;
			
			case '0': case '1': case '2': case '3': case '4':
			case '5': case '6': case '7': case '8': case '9':
				just_dot = false;
				curr_token += c;
				continue;

			default:
				if( this->delimitadores[c] ) {  // 123,23- ó 123,23& Delimit right here
					
					if( just_dot ) { // 123.&

						tokens.push_back( string(curr_token, 0, curr_token.size()-1) );
						i++;
						return true;
					}

					i++;
					tokens.push_back(curr_token);

					if( str[i]=='%' || str[i] == '$' ) {
						curr_token.clear();
						curr_token += str[i++];
						tokens.push_back(curr_token);
					}

					return true;
				}
				else {  // 123,3a 123.34,123a ABORT, nunca fue un decimal. Es un acronimo que se cortaba en la coma.

					//cout << "holy shit is it you" << endl;
					i=token_start;
					curr_token.clear();
					return false;   // provisional

				}
		}
	}
	if( !just_dot ) 
		tokens.push_back(curr_token);
	else  // sda@a.a__
		tokens.push_back(string(curr_token, 0, curr_token.size()-1));
	
	return true;
}

// si te encuentras un delimitador:
// 										. -> AO desde el principio era un acronimo
// 										- -> MO desde el principio era una multipalabra
// 										_ -> O, cortar
// 								otherwise -> O, cortar
 // si no es un delimitador,
//  									no reaction
// DESPUES DE @, si ya has encontrado al menos un caracter, you cant fuck this up.
// si te encuentras un delimitador:
// 						 			  .-_ -> allowed solo si van RODEADOS de caracteres normales.
// 											

bool Tokenizador::Tokenizar_email_O( const string& str, list<string>& tokens, int& i, string& curr_token) const {
	
	int at_position;
	int token_start = i;
	unsigned char c;

	for( ; i<str.size(); i++ ) {
	
		if( this->pasarAminuscSinAcentos )
			c = conversion[(unsigned char)str[i]];
		else
			c = (unsigned char)str[i];

		switch( c ) {

			case '@':
				at_position = i-token_start;  // relativo a curr_token
				i++;
				goto after_at;

			default:
				if( this->delimitadores[c] ) {  // not email. Over right here.

					tokens.push_back(curr_token);
					i++;
					return true;
				}
				else 						// content
					curr_token += c;
		}
	}

	// has llegado al final del string y no era un email, era un token sin mas.
	tokens.push_back(curr_token);
	return true;

	after_at:

	if( this->delimitadores[(unsigned char)str[i]] ) {   // sdasda@{.-_@ or other delimiter} Not an email anymore, delimited at previous @

		tokens.push_back(string(curr_token, 0, curr_token.size()-1));
		i++;
		return true;
	}

	bool just_dot = false;

	// read rest of email
	for( ; i<str.size(); i++ ) {

		c = (unsigned char)str[i];
		switch( c ) {

			case '_':
				if( !this->delimitadores[c] ) {
					curr_token += c;
					just_dot = false;
				}
				
				else if( !just_dot ) {
					just_dot = true;
					curr_token += c;
				}
				else { 		// sda@a__ | sad@a_a_a__a  gets delimited at ..

					tokens.push_back(string(curr_token, 0, curr_token.size()-1));
					i++; // get placed after this->delimitadores
					return true;
				}
				break;
			
			case '@':   // bro is NOT an email. Cortamos el token en el @ y del resto se encarga otro
				tokens.push_back(string(curr_token, 0, at_position));
				i = token_start + at_position + 1; // ponernos justo después del @
				return true;

			default:
				if( !this->delimitadores[c] ) {  	// content
					curr_token += c;
					just_dot = false;
				}
				
				else { 					// terminar
					if( !just_dot ) 
						tokens.push_back(curr_token);
				
					else  // quitar un delimitador especial puesto al final()
						tokens.push_back(string(curr_token, 0, curr_token.size()-1));
					
					i++;
					return true;
				}
		}
	}
	// si has llegado aqui, estas al final del string

	// sda@a.a.a.
	if( !just_dot ) 
		tokens.push_back(curr_token);
	else  // sda@a.a__
		tokens.push_back(string(curr_token, 0, curr_token.size()-1));
	
	return true;
}

bool Tokenizador::Tokenizar_email_A( const string& str, list<string>& tokens, int& i, string& curr_token, bool& canBeAcronym ) const {
	
	int at_position;
	int token_start = i;
	unsigned char c;

	i = str.find_first_not_of(this->delimiters, i);
	if( i==str.npos ) return false;

	for( ; i<str.size(); i++ ) {
	
		if( this->pasarAminuscSinAcentos )
			c = conversion[(unsigned char)str[i]];
		else
			c = (unsigned char)str[i];

		//printf("email %c\n", c);

		switch( c ) {

			case '.':  // no era un email, aún podría ser un acrónimo.  Acronimo tendrá que comprobar si curr_token está vacío o no.
				curr_token.append(1, c);
				//cout << "boutta return " << curr_token << endl;
				canBeAcronym = true;
				return false;

			case '@':
				at_position = i-token_start;  // relativo a curr_token
				//cout << "found first @ at " << at_position << " relative to currtoken: " << curr_token << endl;
				curr_token += '@';
				i++;
				goto after_at;

			default:
				if( this->delimitadores[c] ) {  // not email. Over right here.

					tokens.push_back(curr_token);
					i++;
					return true;
				}
				else 						// content
					curr_token += c;
		}
	}

	// has llegado al final del string y no era un email, era un token sin mas.
	tokens.push_back(curr_token);
	return true;

	after_at:

	//cout << "curr token is " << curr_token << " after at --" << i << " -- " << str[i]  << endl;

	if( this->delimitadores[str[i]] ) {   // sdasda@{.-_@ or other delimiter} Not an email anymore, delimited at previous @

		tokens.push_back(string(curr_token, 0, curr_token.size()-1));
		i++;
		return true;
	}

	bool just_dot = false;

	// read rest of email
	for( ; i<str.size(); i++ ) {

		c = str[i];
		switch( c ) {

			case '.':
				if( !just_dot ) {
					just_dot = true;
					curr_token += c;
				}
				else { 		// sda@a__ | sad@a_a_a__a  gets delimited at ..

					tokens.push_back(string(curr_token, 0, curr_token.size()-1));
					i-=2; // get placed before dots
					return true;
				}
				break;
			case '_':
				if( !this->delimitadores[c] )  {
					curr_token += c;
					just_dot = false;
				}
				
				else if( !just_dot ) {
					just_dot = true;
					curr_token += c;
				}
				else { 		// sda@a__ | sad@a_a_a__a  gets delimited at ..

					tokens.push_back(string(curr_token, 0, curr_token.size()-1));
					i++; // get placed after this->delimitadores
					return true;
				}
				break;
			
			case '@':   // bro is NOT an email. Cortamos el token en el @ y del resto se encarga otro
				//cout << "found second @ at " << i << " cutting token short at at_position " << at_position << endl;
				tokens.push_back(string(curr_token, 0, at_position));
				i = token_start + at_position + 1; // ponernos justo después del @
				return true;

			default:
				if( !this->delimitadores[c] )  { 	// content
					curr_token += c;
					just_dot = false;
				}
				
				else { 					// terminar
					if( !just_dot ) 
						tokens.push_back(curr_token);
				
					else  // quitar un delimitador especial puesto al final()
						tokens.push_back(string(curr_token, 0, curr_token.size()-1));
					
					i++;
					return true;
				}
		}
	}
	// si has llegado aqui, estas al final del string

	// sda@a.a.a.
	if( !just_dot ) 
		tokens.push_back(curr_token);
	
	else  // sda@a.a..
		tokens.push_back(string(curr_token, 0, curr_token.size()-1));
	return true;
}

bool Tokenizador::Tokenizar_email_M( const string& str, list<string>& tokens, int& i, string& curr_token, bool& canBeMultiword ) const {
	
	int at_position;
	int token_start = i;
	unsigned char c;

	for( ; i<str.size(); i++ ) {
	
		if( this->pasarAminuscSinAcentos )
			c = conversion[(unsigned char)str[i]];
		else
			c = str[i];

		switch( c ) {

			case '-':  // no era un email, aún podría ser un multiword.  Multiword tendrá que comprobar si curr_token está vacío o no.
				curr_token += c;
				canBeMultiword = true;
				return false;

			case '@':
				at_position = i-token_start;  // relativo a curr_token
				i++;
				goto after_at;

			default:
				if( this->delimitadores[c] ) {  // not email. Over right here.

					tokens.push_back(curr_token);
					i++;
					return true;
				}
				else 						// content
					curr_token += c;
		}
	}

	// has llegado al final del string y no era un email, era un token sin mas.
	tokens.push_back(curr_token);
	return true;

	after_at:

	if( this->delimitadores[(unsigned char)str[i]] ) {   // sdasda@{.-_@ or other delimiter} Not an email anymore, delimited at previous @

		tokens.push_back(string(curr_token, 0, curr_token.size()-1));
		i++;
		return true;
	}

	bool just_dot = false;

	// read rest of email
	for( ; i<str.size(); i++ ) {

		c = (unsigned char)str[i];
		switch( c ) {

			case '-':
				if( !just_dot ) {
					just_dot = true;
					curr_token += c;
				}
				else { 		// sda@a__ | sad@a_a_a__a  gets delimited at ..

					tokens.push_back(string(curr_token, 0, curr_token.size()-1));
					i++; // get placed before dashes
					return true;
				}
				break;
			case '_':
				if( !this->delimitadores[c] ) {
					curr_token += c;
					just_dot = false;
				}
				
				else if( !just_dot ) {
					just_dot = true;
					curr_token += c;
				}
				else { 		// sda@a__ | sad@a_a_a__a  gets delimited at ..

					tokens.push_back(string(curr_token, 0, curr_token.size()-1));
					i++; // get placed after this->delimitadores
					return true;
				}
				break;
			
			case '@':   // bro is NOT an email. Cortamos el token en el @ y del resto se encarga otro
				tokens.push_back(string(curr_token, 0, at_position));
				i = token_start + at_position + 1; // ponernos justo después del @
				return true;

			default:
				if( !this->delimitadores[c] ) {  	// content
					curr_token += c;
					just_dot = false;
				}
				
				else { 					// terminar
					
					if( !just_dot ) 
						tokens.push_back(curr_token);
					
					else  // quitar un delimitador especial puesto al final()
						tokens.push_back(string(curr_token, 0, curr_token.size()-1));
					
					i++;
					return true;
				}
		}
	}
	// si has llegado aqui, estas al final del string

	// sda@a.a.a.
	if( !just_dot ) 
		tokens.push_back(curr_token);
	
	else  // sda@a.a{--,__}
		tokens.push_back(string(curr_token, 0, curr_token.size()-1));
	
	return true;
}

bool Tokenizador::Tokenizar_email_AM( const string& str, list<string>& tokens, int& i, string& curr_token, bool& canBeAcronym, bool& canBeMultiword ) const {
	
	int at_position;
	int token_start = i;
	unsigned char c;

	i = str.find_first_not_of(this->delimiters,i);
	if( i==str.npos ) return false;

	for( ; i<str.size(); i++ ) {
	
		if( this->pasarAminuscSinAcentos )
			c = conversion[(unsigned char)str[i]];
		else
			c = (unsigned char)str[i];

		switch( c ) {

			case '.': 	// no era un email, aún podría ser un acronimo.  Acronimo tendrá que comprobar si curr_token está vacío o no.
				curr_token += c;
				canBeAcronym = true;
				return false;
			case '-':  	// no era un email, aún podría ser un multiword.  Multiword tendrá que comprobar si curr_token está vacío o no.
				curr_token += c;
				canBeMultiword = true;
				return false;

			case '@':
				at_position = i-token_start;  // relativo a curr_token
				i++;
				goto after_at;

			default:
				if( this->delimitadores[c] ) {  // not email. Over right here.

					tokens.push_back(curr_token);
					i++;
					return true;
				}
				else 						// content
					curr_token += c;
		}
	}

	// has llegado al final del string y no era un email, era un token sin mas.
	tokens.push_back(curr_token);
	return true;

	after_at:

	if( this->delimitadores[str[i]] ) {   // sdasda@{.-_@ or other delimiter} Not an email anymore, delimited at previous @

		tokens.push_back(string(curr_token, 0, curr_token.size()-1));
		i++;
		return true;
	}

	bool just_dot = false;

	// read rest of email
	for( ; i<str.size(); i++ ) {

		c = (unsigned char)str[i];
		switch( c ) {

			case '.':
			case '-':
				if( !just_dot ) {
					just_dot = true;
					curr_token += c;
				}
				else { 		// sda@a__ | sad@a_a_a__a  gets delimited at ..

					tokens.push_back(string(curr_token, 0, curr_token.size()-1));
					i++; // get placed before dashes
					return true;
				}
				break;
			case '_':
				if( !this->delimitadores[c] ) {
					curr_token += c;
					just_dot = false;
				}
				
				else if( !just_dot ) {
					just_dot = true;
					curr_token += c;
				}
				else { 		// sda@a__ | sad@a_a_a__a  gets delimited at ..

					tokens.push_back(string(curr_token, 0, curr_token.size()-1));
					i++; // get placed after this->delimitadores
					return true;
				}
				break;
			
			case '@':   // bro is NOT an email. Cortamos el token en el @ y del resto se encarga otro
				tokens.push_back(string(curr_token, 0, at_position));
				i = token_start + at_position + 1; // ponernos justo después del @
				return true;

			default:
				if( !this->delimitadores[c] ) {  	// content
					curr_token += c;
					just_dot = false;
				}
				
				else { 					// terminar
					
					if( !just_dot ) 
						tokens.push_back(curr_token);
					
					else  // quitar un delimitador especial puesto al final()
						tokens.push_back(string(curr_token, 0, curr_token.size()-1));
					
					i++;
					return true;
				}
		}
	}
	// si has llegado aqui, estas al final del string

	// sda@a.a.a.
	if( !just_dot ) 
		tokens.push_back(curr_token);
	
	else  // sda@a.a{--,__}
		tokens.push_back(string(curr_token, 0, curr_token.size()-1));
	
	return true;
}

bool Tokenizador::Tokenizar_acronimo( const string& str, list<string>& tokens, int&i, string& curr_token ) const {

	int token_start = i;
	bool just_dot = false;

	unsigned char c;
	just_dot = !curr_token.empty();
	i+=just_dot;
	//cout << "dentro con i " << i << endl; 
	// de quitar puntos del principio se ha encargado decimal

	// si estamos aqui, es porque sabemos 100%, por lo que sea, que no es un email
	for( ; i<str.size(); i++ ) {

		if( this->pasarAminuscSinAcentos )
			c = conversion[(unsigned char)str[i]];
		else
			c = (unsigned char)str[i];

		switch( c ) {

			case '.':
				if( just_dot ) {  // no longer an acronym. Previous dot delimits   a.c..5bc

					tokens.push_back(string(curr_token, 0, curr_token.size()-1));
					//i--;
					return true;
				}
				else {
					just_dot = true;
					curr_token += '.';
					//i++;
				}
				break;

			case '-':   // not an acronym, could be multiword
				if( this->delimitadores[(unsigned char)'-'] ) 
					return false;

			default:
				if( this->delimitadores[c] ) { 		
					
					if( just_dot ) {
						tokens.push_back(string(curr_token, 0, curr_token.size()-1));
						i++;
						return true;
					}
					tokens.push_back(curr_token);

					i++;
					return true;
				}
				else {
					just_dot = false;
					curr_token += c;
				}
		}
	}
	// si has llegado aqui, estas al final del string
	tokens.push_back(curr_token);
	return true;
}

bool Tokenizador::Tokenizar_multipalabra( const string& str, list<string>& tokens, int&i, string& curr_token ) const {

	bool just_dash=false;
	unsigned char c;
	
	just_dash = !curr_token.empty();
	i += just_dash;

	if( curr_token.empty() ) { 				// hay que quitar guiones del principio (si los hay)

		for( ; i<str.size(); i++ ) {

			if( (unsigned char)str[i]!='-' ) {
				if( this->delimitadores[(unsigned char)str[i]] )     // ----& 
						return false;
				else
					break;
			}
		}
	}
	for( ; i<str.size(); i++ ) {

		if( this->pasarAminuscSinAcentos )
			c = conversion[(unsigned char)str[i]];
		else
			c = (unsigned char)str[i];

		if( c!='-' ) {

			if( this->delimitadores[c] ) {
				tokens.push_back(curr_token);
				return true;
			}
			just_dash = false; 				// content character
			curr_token += c;
		}
		else {
			if( just_dash ) { 				// not a multiword anymore, delimited at previous 

				tokens.push_back(string(curr_token, 0, curr_token.size()-1));
				i++;
				return true;
			}
			else {
				just_dash = true;
				curr_token += (unsigned char)'-';
			}
		}
	}
	// si has llegado aqui, estas al final del string
	tokens.push_back(curr_token);
	return true;
}

void Tokenizador::Tokenizar_token_normal( const string& str, list<string>& tokens, int& i, string& curr_token ) const {
	
	//printf("entrando a token normal\n");

	unsigned char c;

	for( ; i<str.size(); i++ ) {


		if( this->pasarAminuscSinAcentos )
		c = conversion[(unsigned char)str[i]];
		else
		c = (unsigned char)str[i];

		if( this->delimitadores[c] ) {
			tokens.push_back(curr_token);
			i++;
			return;
		}
		else
			curr_token += c;
	}
	// si has llegado aqui, estas al final del string
	tokens.push_back(curr_token);
	return;
}

bool Tokenizador::Tokenizar_ftp( const unsigned char* input, const size_t& input_size, list<string>& tokens, int& i, string& curr_token ) const {

	
	int token_start = i;

	if( (input_size-i)>=4 && ((input[i+1]=='t') & (input[i+2]=='p' & input[i+3]==':')) ) { 		// ftp:
		curr_token = "ftp:";
		i += 4;
	}
	else
		return false;

	if( (this->delimitadores[input[i]] & !exceptions[input[i]]>=2) || i==input_size ) {  // http: and thats it
		curr_token.resize(curr_token.size()-1);
		i--;
		return false;
	}  

	char c;

	for( ; i<input_size; i++ ) {
		
		if( this->pasarAminuscSinAcentos )
			c = conversion[(unsigned char)input[i]];
		else
			c = input[i];

		switch(c) {

			case '_':
			case ':':
			case '/':
			case '.':
			case '?':
			case '&':
			case '-':
			case '=':
			case '#':
			case '@':
				curr_token += c;
				break;
			default:
				if( !this->delimitadores[c] ) {
					curr_token += c;
				}
				else {
					tokens.push_back(curr_token);
					return true;
				}
		}
	}
	// si has llegado aqui, estas al final del string
	tokens.push_back(curr_token);
	return true;
}

bool Tokenizador::Tokenizar_http( const unsigned char* input, const size_t& input_size, list<string>& tokens, int& i, string& curr_token ) const {

	int token_start = i;

	if( (input_size-i)>=5 && ((input[i+1]=='t') & (input[i+2]=='t') & (input[i+3]=='p')) ) {

		if( input[i+4]==':' ) {   
			curr_token = "http:";												// http:
			i += 5;
		}
		else if( ( input_size-i )>=6 && (input[i+4]=='s') & (input[i+5]==':') ) { 	// https:
			curr_token = "https:";
			i += 6;
		}
	}
	else return false;

	//cout << "in url is " << curr_token << endl;

	if( (this->delimitadores[input[i]] & !exceptions[input[i]]>=2) || i==input_size ) {  // http: and thats it
		curr_token.resize(curr_token.size()-1);
		i--;
		return false;
	}  

	unsigned char c;

	for( ; i<input_size; i++ ) {
		
		if( this->pasarAminuscSinAcentos )
			c = conversion[(unsigned char)input[i]];
		else
			c = input[i];

		switch(c) {

			case '_':
			case ':':
			case '/':
			case '.':
			case '?':
			case '&':
			case '-':
			case '=':
			case '#':
			case '@':
				curr_token += c;
				break;
			default:
				if( !this->delimitadores[c] ) {
					curr_token += c;
				}
				else {
					tokens.push_back(curr_token);
					return true;
				}
		}
	}
	// si has llegado aqui, estas al final del string
	tokens.push_back(curr_token);
	return true;
}

bool Tokenizador::Tokenizar_decimal( const unsigned char* input, const size_t& input_size, list<string>& tokens, int& i, string& curr_token, char heading_zero ) const {

	unsigned char c;
	bool just_dot=false;
	int token_start=i;

	// hay heading .,
	if( heading_zero )
	{
		curr_token = "0";
		curr_token += heading_zero;
	}
	
	//cout << "decimal correcto: " << curr_token << endl;

	// 123.123,123.123,123.123,123.a  <== el caso con el que tuve pesadillas anoche
	// TODO esperar a los tests
	
	// Leer resto del número
	/// TODO Si te encuentras una coma intermedia, hay que guardar su posición para cortar ahí en caso de que el decimal falle
	for( ; i<input_size; i++ ) {

		c = (unsigned char)input[i];

		switch(c) {

			case ',':  // TODO
			case '.':
				if( just_dot ) {   // its a normal token
					tokens.push_back(string(curr_token, 0, curr_token.size()-1));
					i--;
					return true;
				}
				just_dot = true;
				curr_token += c;
				continue;
			
			case '0': case '1': case '2': case '3': case '4':
			case '5': case '6': case '7': case '8': case '9':
				just_dot = false;
				curr_token += c;
				continue;

			default:
				if( this->delimitadores[c] ) {  // 123,23- ó 123,23& Delimit right here
					
					if( just_dot ) { // 123.&

						tokens.push_back( string(curr_token, 0, curr_token.size()-1) );
						i++;
						return true;
					}

					i++;
					tokens.push_back(curr_token);

					if( input[i]=='%' || input[i] == '$' ) {
						curr_token.clear();
						curr_token += input[i++];
						tokens.push_back(curr_token);
					}

					return true;
				}
				else {  // 123,3a 123.34,123a ABORT, nunca fue un decimal. Es un acronimo que se cortaba en la coma.

					//cout << "holy shit is it you" << endl;
					i=token_start;
					curr_token.clear();
					return false;   // provisional

				}
		}
	}
	if( !just_dot ) 
		tokens.push_back(curr_token);
	else  // sda@a.a__
		tokens.push_back(string(curr_token, 0, curr_token.size()-1));
	
	return true;
}

// si te encuentras un delimitador:
// 										. -> AO desde el principio era un acronimo
// 										- -> MO desde el principio era una multipalabra
// 										_ -> O, cortar
// 								otherwise -> O, cortar
 // si no es un delimitador,
//  									no reaction
// DESPUES DE @, si ya has encontrado al menos un caracter, you cant fuck this up.
// si te encuentras un delimitador:
// 						 			  .-_ -> allowed solo si van RODEADOS de caracteres normales.
// 											

bool Tokenizador::Tokenizar_email_O( const unsigned char* input, const size_t& input_size, list<string>& tokens, int& i, string& curr_token) const {
	
	int at_position;
	int token_start = i;
	unsigned char c;

	for( ; i<input_size; i++ ) {
	
		if( this->pasarAminuscSinAcentos )
			c = conversion[(unsigned char)input[i]];
		else
			c = (unsigned char)input[i];

		switch( c ) {

			case '@':
				at_position = i-token_start;  // relativo a curr_token
				i++;
				goto after_at;

			default:
				if( this->delimitadores[c] ) {  // not email. Over right here.

					tokens.push_back(curr_token);
					i++;
					return true;
				}
				else 						// content
					curr_token += c;
		}
	}

	// has llegado al final del string y no era un email, era un token sin mas.
	tokens.push_back(curr_token);
	return true;

	after_at:

	if( this->delimitadores[(unsigned char)input[i]] ) {   // sdasda@{.-_@ or other delimiter} Not an email anymore, delimited at previous @

		tokens.push_back(string(curr_token, 0, curr_token.size()-1));
		i++;
		return true;
	}

	bool just_dot = false;

	// read rest of email
	for( ; i<input_size; i++ ) {

		c = (unsigned char)input[i];
		switch( c ) {

			case '_':
				if( !this->delimitadores[c] ) {
					curr_token += c;
					just_dot = false;
				}
				
				else if( !just_dot ) {
					just_dot = true;
					curr_token += c;
				}
				else { 		// sda@a__ | sad@a_a_a__a  gets delimited at ..

					tokens.push_back(string(curr_token, 0, curr_token.size()-1));
					i++; // get placed after this->delimitadores
					return true;
				}
				break;
			
			case '@':   // bro is NOT an email. Cortamos el token en el @ y del resto se encarga otro
				tokens.push_back(string(curr_token, 0, at_position));
				i = token_start + at_position + 1; // ponernos justo después del @
				return true;

			default:
				if( !this->delimitadores[c] ) {  	// content
					curr_token += c;
					just_dot = false;
				}
				
				else { 					// terminar
					if( !just_dot ) 
						tokens.push_back(curr_token);
				
					else  // quitar un delimitador especial puesto al final()
						tokens.push_back(string(curr_token, 0, curr_token.size()-1));
					
					i++;
					return true;
				}
		}
	}
	// si has llegado aqui, estas al final del string

	// sda@a.a.a.
	if( !just_dot ) 
		tokens.push_back(curr_token);
	else  // sda@a.a__
		tokens.push_back(string(curr_token, 0, curr_token.size()-1));
	
	return true;
}

bool Tokenizador::Tokenizar_email_A( const unsigned char* input, const size_t& input_size, list<string>& tokens, int& i, string& curr_token, bool& canBeAcronym ) const {
	
	int at_position;
	int token_start = i;
	unsigned char c;

	//i = str.find_first_not_of(this->delimiters, i);
	//if( i==str.npos ) return false;

	for( ; i<input_size; i++ ) {
	
		if( this->pasarAminuscSinAcentos )
			c = conversion[(unsigned char)input[i]];
		else
			c = (unsigned char)input[i];

		//printf("email %c\n", c);

		switch( c ) {

			case '.':  // no era un email, aún podría ser un acrónimo.  Acronimo tendrá que comprobar si curr_token está vacío o no.
				curr_token.append(1, c);
				canBeAcronym = true;
				return false;

			case '@':
				at_position = i-token_start;  // relativo a curr_token
				curr_token += '@';
				i++;
				goto after_at;

			default:
				if( this->delimitadores[c] ) {  // not email. Over right here.

					tokens.push_back(curr_token);
					i++;
					return true;
				}
				else 						// content
					curr_token += c;
		}
	}

	// has llegado al final del string y no era un email, era un token sin mas.
	tokens.push_back(curr_token);
	return true;

	after_at:

	//cout << "curr token is " << curr_token << " after at --" << i << " -- " << input[i]  << endl;

	if( this->delimitadores[input[i]] ) {   // sdasda@{.-_@ or other delimiter} Not an email anymore, delimited at previous @

		tokens.push_back(string(curr_token, 0, curr_token.size()-1));
		i++;
		return true;
	}

	bool just_dot = false;

	// read rest of email
	for( ; i<input_size; i++ ) {

		if( this->pasarAminuscSinAcentos )
			c = conversion[(unsigned char)input[i]];
		else
			c = (unsigned char)input[i];

		switch( c ) {

			case '.':
				if( !just_dot ) {
					just_dot = true;
					curr_token += c;
				}
				else { 		// sda@a__ | sad@a_a_a__a  gets delimited at ..

					tokens.push_back(string(curr_token, 0, curr_token.size()-1));
					i-=2; // get placed before dots
					return true;
				}
				break;
			case '_':
				if( !this->delimitadores[c] )  {
					curr_token += c;
					just_dot = false;
				}
				
				else if( !just_dot ) {
					just_dot = true;
					curr_token += c;
				}
				else { 		// sda@a__ | sad@a_a_a__a  gets delimited at ..

					tokens.push_back(string(curr_token, 0, curr_token.size()-1));
					i++; // get placed after this->delimitadores
					return true;
				}
				break;
			
			case '@':   // bro is NOT an email. Cortamos el token en el @ y del resto se encarga otro
				tokens.push_back(string(curr_token, 0, at_position));
				i = token_start + at_position + 1; // ponernos justo después del @
				return true;

			default:
				if( !this->delimitadores[c] )  { 	// content
					curr_token += c;
					just_dot = false;
				}
				
				else { 					// terminar
					if( !just_dot ) 
						tokens.push_back(curr_token);
				
					else  // quitar un delimitador especial puesto al final()
						tokens.push_back(string(curr_token, 0, curr_token.size()-1));
					
					i++;
					return true;
				}
		}
	}
	// si has llegado aqui, estas al final del string

	// sda@a.a.a.
	if( !just_dot ) 
		tokens.push_back(curr_token);
	
	else  // sda@a.a..
		tokens.push_back(string(curr_token, 0, curr_token.size()-1));
	return true;
}

bool Tokenizador::Tokenizar_email_M( const unsigned char* input, const size_t& input_size, list<string>& tokens, int& i, string& curr_token, bool& canBeMultiword ) const {
	
	int at_position;
	int token_start = i;
	unsigned char c;

	for( ; i<input_size; i++ ) {
	
		if( this->pasarAminuscSinAcentos )
			c = conversion[(unsigned char)input[i]];
		else
			c = input[i];

		switch( c ) {

			case '-':  // no era un email, aún podría ser un multiword.  Multiword tendrá que comprobar si curr_token está vacío o no.
				curr_token += c;
				canBeMultiword = true;
				return false;

			case '@':
				at_position = i-token_start;  // relativo a curr_token
				i++;
				goto after_at;

			default:
				if( this->delimitadores[c] ) {  // not email. Over right here.

					tokens.push_back(curr_token);
					i++;
					return true;
				}
				else 						// content
					curr_token += c;
		}
	}

	// has llegado al final del string y no era un email, era un token sin mas.
	tokens.push_back(curr_token);
	return true;

	after_at:

	if( this->delimitadores[(unsigned char)input[i]] ) {   // sdasda@{.-_@ or other delimiter} Not an email anymore, delimited at previous @

		tokens.push_back(string(curr_token, 0, curr_token.size()-1));
		i++;
		return true;
	}

	bool just_dot = false;

	// read rest of email
	for( ; i<input_size; i++ ) {

		c = (unsigned char)input[i];
		switch( c ) {

			case '-':
				if( !just_dot ) {
					just_dot = true;
					curr_token += c;
				}
				else { 		// sda@a__ | sad@a_a_a__a  gets delimited at ..

					tokens.push_back(string(curr_token, 0, curr_token.size()-1));
					i++; // get placed before dashes
					return true;
				}
				break;
			case '_':
				if( !this->delimitadores[c] ) {
					curr_token += c;
					just_dot = false;
				}
				
				else if( !just_dot ) {
					just_dot = true;
					curr_token += c;
				}
				else { 		// sda@a__ | sad@a_a_a__a  gets delimited at ..

					tokens.push_back(string(curr_token, 0, curr_token.size()-1));
					i++; // get placed after this->delimitadores
					return true;
				}
				break;
			
			case '@':   // bro is NOT an email. Cortamos el token en el @ y del resto se encarga otro
				tokens.push_back(string(curr_token, 0, at_position));
				i = token_start + at_position + 1; // ponernos justo después del @
				return true;

			default:
				if( !this->delimitadores[c] ) {  	// content
					curr_token += c;
					just_dot = false;
				}
				
				else { 					// terminar
					
					if( !just_dot ) 
						tokens.push_back(curr_token);
					
					else  // quitar un delimitador especial puesto al final()
						tokens.push_back(string(curr_token, 0, curr_token.size()-1));
					
					i++;
					return true;
				}
		}
	}
	// si has llegado aqui, estas al final del string

	// sda@a.a.a.
	if( !just_dot ) 
		tokens.push_back(curr_token);
	
	else  // sda@a.a{--,__}
		tokens.push_back(string(curr_token, 0, curr_token.size()-1));
	
	return true;
}

bool Tokenizador::Tokenizar_email_AM( const unsigned char* input, const size_t& input_size, list<string>& tokens, int& i, string& curr_token, bool& canBeAcronym, bool& canBeMultiword ) const {
	
	int at_position;
	int token_start = i;
	unsigned char c;

	//i = str.find_first_not_of(this->delimiters,i);
	//if( i==str.npos ) return false;

	for( ; i<input_size; i++ ) {
	
		if( this->pasarAminuscSinAcentos )
			c = conversion[(unsigned char)input[i]];
		else
			c = (unsigned char)input[i];

		switch( c ) {

			case '.': 	// no era un email, aún podría ser un acronimo.  Acronimo tendrá que comprobar si curr_token está vacío o no.
				curr_token += c;
				canBeAcronym = true;
				return false;
			case '-':  	// no era un email, aún podría ser un multiword.  Multiword tendrá que comprobar si curr_token está vacío o no.
				curr_token += c;
				canBeMultiword = true;
				return false;

			case '@':
				at_position = i-token_start;  // relativo a curr_token
				i++;
				goto after_at;

			default:
				if( this->delimitadores[c] ) {  // not email. Over right here.

					tokens.push_back(curr_token);
					i++;
					return true;
				}
				else 						// content
					curr_token += c;
		}
	}

	// has llegado al final del string y no era un email, era un token sin mas.
	tokens.push_back(curr_token);
	return true;

	after_at:

	if( this->delimitadores[input[i]] ) {   // sdasda@{.-_@ or other delimiter} Not an email anymore, delimited at previous @

		tokens.push_back(string(curr_token, 0, curr_token.size()-1));
		i++;
		return true;
	}

	bool just_dot = false;

	// read rest of email
	for( ; i<input_size; i++ ) {

		c = (unsigned char)input[i];
		switch( c ) {

			case '.':
			case '-':
				if( !just_dot ) {
					just_dot = true;
					curr_token += c;
				}
				else { 		// sda@a__ | sad@a_a_a__a  gets delimited at ..

					tokens.push_back(string(curr_token, 0, curr_token.size()-1));
					i++; // get placed before dashes
					return true;
				}
				break;
			case '_':
				if( !this->delimitadores[c] ) {
					curr_token += c;
					just_dot = false;
				}
				
				else if( !just_dot ) {
					just_dot = true;
					curr_token += c;
				}
				else { 		// sda@a__ | sad@a_a_a__a  gets delimited at ..

					tokens.push_back(string(curr_token, 0, curr_token.size()-1));
					i++; // get placed after this->delimitadores
					return true;
				}
				break;
			
			case '@':   // bro is NOT an email. Cortamos el token en el @ y del resto se encarga otro
				tokens.push_back(string(curr_token, 0, at_position));
				i = token_start + at_position + 1; // ponernos justo después del @
				return true;

			default:
				if( !this->delimitadores[c] ) {  	// content
					curr_token += c;
					just_dot = false;
				}
				
				else { 					// terminar
					
					if( !just_dot ) 
						tokens.push_back(curr_token);
					
					else  // quitar un delimitador especial puesto al final()
						tokens.push_back(string(curr_token, 0, curr_token.size()-1));
					
					i++;
					return true;
				}
		}
	}
	// si has llegado aqui, estas al final del string

	// sda@a.a.a.
	if( !just_dot ) 
		tokens.push_back(curr_token);
	
	else  // sda@a.a{--,__}
		tokens.push_back(string(curr_token, 0, curr_token.size()-1));
	
	return true;
}

bool Tokenizador::Tokenizar_acronimo( const unsigned char* input, const size_t& input_size, list<string>& tokens, int&i, string& curr_token ) const {

	int token_start = i;
	bool just_dot = false;

	unsigned char c;
	just_dot = !curr_token.empty();
	i+=just_dot;
	//cout << "dentro con i " << i << endl; 
	// de quitar puntos del principio se ha encargado decimal

	// si estamos aqui, es porque sabemos 100%, por lo que sea, que no es un email
	for( ; i<input_size; i++ ) {

		if( this->pasarAminuscSinAcentos )
			c = conversion[(unsigned char)input[i]];
		else
			c = (unsigned char)input[i];

		switch( c ) {

			case '.':
				if( just_dot ) {  // no longer an acronym. Previous dot delimits   a.c..5bc

					tokens.push_back(string(curr_token, 0, curr_token.size()-1));
					//i--;
					return true;
				}
				else {
					just_dot = true;
					curr_token += '.';
					//i++;
				}
				break;

			case '-':   // not an acronym, could be multiword
				if( this->delimitadores[(unsigned char)'-'] ) 
					return false;

			default:
				if( this->delimitadores[c] ) { 		
					
					if( just_dot ) {
						tokens.push_back(string(curr_token, 0, curr_token.size()-1));
						i++;
						return true;
					}
					tokens.push_back(curr_token);

					i++;
					return true;
				}
				else {
					just_dot = false;
					curr_token += c;
				}
		}
	}
	// si has llegado aqui, estas al final del string
	tokens.push_back(curr_token);
	return true;
}

bool Tokenizador::Tokenizar_multipalabra( const unsigned char* input, const size_t& input_size, list<string>& tokens, int&i, string& curr_token ) const {

	bool just_dash=false;
	unsigned char c;
	
	just_dash = !curr_token.empty();
	i += just_dash;

	if( curr_token.empty() ) { 				// hay que quitar guiones del principio (si los hay)

		for( ; i<input_size; i++ ) {

			if( (unsigned char)input[i]!='-' ) {
				if( this->delimitadores[(unsigned char)input[i]] )     // ----& 
						return false;
				else
					break;
			}
		}
	}
	for( ; i<input_size; i++ ) {

		if( this->pasarAminuscSinAcentos )
			c = conversion[(unsigned char)input[i]];
		else
			c = (unsigned char)input[i];

		if( c!='-' ) {

			if( this->delimitadores[c] ) {
				tokens.push_back(curr_token);
				return true;
			}
			just_dash = false; 				// content character
			curr_token += c;
		}
		else {
			if( just_dash ) { 				// not a multiword anymore, delimited at previous 

				tokens.push_back(string(curr_token, 0, curr_token.size()-1));
				i++;
				return true;
			}
			else {
				just_dash = true;
				curr_token += (unsigned char)'-';
			}
		}
	}
	// si has llegado aqui, estas al final del string
	tokens.push_back(curr_token);
	return true;
}

void Tokenizador::Tokenizar_token_normal( const unsigned char* input, const size_t& input_size, list<string>& tokens, int& i, string& curr_token ) const {
	
	//printf("entrando a token normal\n");

	unsigned char c;

	for( ; i<input_size; i++ ) {


		if( this->pasarAminuscSinAcentos )
		c = conversion[(unsigned char)input[i]];
		else
		c = (unsigned char)input[i];

		if( this->delimitadores[c] ) {
			tokens.push_back(curr_token);
			i++;
			return;
		}
		else
			curr_token += c;
	}
	// si has llegado aqui, estas al final del string
	tokens.push_back(curr_token);
	return;
}


// army of methods para ahorrarme ifs. Despues de la _ indica cuales son this->delimitadores
void Tokenizador::TokenizarCasosEspeciales_U( const string& str, list<string>& tokens ) const {

	// URL 			Y
	// Decimal 		N 
	// Email 		N
	// Acronimo 	N 		
	// Multiw 		N

	int i=0;
	unsigned char c;
	string curr_token;

	//printf("tokenizando casos especiales U\n");

	while( i<str.size() ) {

		curr_token.clear();
		
		if( this->pasarAminuscSinAcentos )
			c = conversion[(unsigned char)str[i]];
		else
			c = (unsigned char)str[i];

		switch(c) {

			case 'h':
				if (Tokenizar_http(str,tokens, i, curr_token))
					continue;
				else
					goto token;
			case 'f':
				if (Tokenizar_ftp(str,tokens, i, curr_token))
					continue;
				else
					goto token;
				
			default:
				if( this->delimitadores[c] ) {  // es un delimitador indiscutible
					i++;
					continue;
				}
				token:
				Tokenizar_token_normal(str, tokens, i, curr_token);
		}
	}
}

void Tokenizador::TokenizarCasosEspeciales_UA( const string& str, list<string>& tokens ) const {

	// URL 			Y
	// Decimal 		N (.,)
	// Email 		N
	// Acronimo 	Y 		
	// Multiw 		N

	//cout << "estado correcto" << endl;	

	int i=0;
	unsigned char c;
	string curr_token;
	
	while( i<str.size() ) {

		curr_token.clear();
		
		if( this->pasarAminuscSinAcentos )
			c = conversion[(unsigned char)str[i]];
		else
			c = (unsigned char)str[i];

		switch(c) {

			case 'h':
				if (Tokenizar_http(str,tokens, i, curr_token))
					continue;
				else
					goto token;
			case 'f':
				if (Tokenizar_ftp(str,tokens, i, curr_token))
					continue;
				else
					goto token;
			
			default:

				if( this->delimitadores[c] ) {  // es un delimitador indiscutible
					i++;
					continue;
				}
				
				token:

				if( Tokenizar_acronimo(str, tokens, i, curr_token) )
					continue;

				Tokenizar_token_normal(str, tokens, i, curr_token);
				//continue;
		}
	}
}

void Tokenizador::TokenizarCasosEspeciales_UE( const string& str, list<string>& tokens ) const {

	// URL 			Y
	// Decimal 		N
	// Email 		Y
	// Acronimo 	N		
	// Multiw 		N

	int i=0;
	unsigned char c;
	string curr_token;

	while( i<str.size() ) {

		curr_token.clear();
		if( this->pasarAminuscSinAcentos )
			c = conversion[(unsigned char)str[i]];
		else 
			c = (unsigned char)str[i];
		
		switch(c) {

			case 'h':
				if (Tokenizar_http(str,tokens, i, curr_token))
					continue;
				else
					goto token;
			case 'f':
				if (Tokenizar_ftp(str,tokens, i, curr_token))
					continue;
				else
					goto token;
			
			case '@':
				i++;
				continue;

			default:
			
				if( this->delimitadores[c]  ) {  // es un delimitador indiscutible
					i++;
					continue;
				}
				
				token:

				if( Tokenizar_email_O(str, tokens, i, curr_token) )
					continue;

				Tokenizar_token_normal(str, tokens, i, curr_token);
		}
	}
}

void Tokenizador::TokenizarCasosEspeciales_UME( const string& str, list<string>& tokens ) const {

	// URL 			Y
	// Decimal 		N
	// Email 		Y
	// Acronimo 	N		
	// Multiw 		N

	int i=0;
	unsigned char c;
	string curr_token;
	bool canBeMultiw;

	while( i<str.size() ) {

		canBeMultiw = false;
		curr_token.clear();

		if( this->pasarAminuscSinAcentos )
			c = conversion[(unsigned char)str[i]];
		else 
			c = (unsigned char)str[i];

		switch(c) {

			case 'h':
				if (Tokenizar_http(str,tokens, i, curr_token))
					continue;
				else
					goto token;
			case 'f':
				if (Tokenizar_ftp(str,tokens, i, curr_token))
					continue;
				else
					goto token;

			case '@':
				i++;
				continue;

			default:

				if( this->delimitadores[c] ) {  // es un delimitador indiscutible
					i++;
					continue;
				}

				token:

				if( Tokenizar_email_M(str, tokens, i, curr_token, canBeMultiw) ) // te lo ha empezado 100%, si hay un guion entonces se comprueba multiw
					continue;

				if( canBeMultiw && Tokenizar_multipalabra(str, tokens, i, curr_token) )
					continue;

				Tokenizar_token_normal(str, tokens, i, curr_token);
		}
	}
}

void Tokenizador::TokenizarCasosEspeciales_UDA( const string& str, list<string>& tokens ) const {

	// URL 			Y
	// Decimal 		Y
	// Email 		N
	// Acronimo 	Y, N al encontrar una coma 		
	// Multiw 		N

	int i=0;
	unsigned char c;
	string curr_token;
	bool canBeAcronym;
	char heading_dot;
	
	while( i<str.size() ) {
		
		curr_token.clear();

		if( this->pasarAminuscSinAcentos )
			c = conversion[(unsigned char)str[i]];
		else 
			c = (unsigned char)str[i];
		
		switch(c) {

			case 'h':
				heading_dot = 0;
				if (Tokenizar_http(str,tokens, i, curr_token))
					continue;
				else
					goto token;
			
			case 'f':
				heading_dot = 0;
				if (Tokenizar_ftp(str,tokens, i, curr_token))
					continue;
				else
					goto token;
			
			case ',':
			case '.':
				heading_dot = c;
				i++;
				break;

			case '0': case '1': case '2': case '3': case '4': // ,123a  la coma se tiene que descartar igual
			case '5': case '6': case '7': case '8': case '9': 
				
				if( Tokenizar_decimal( str, tokens, i, curr_token, heading_dot ) ) // quitará la coma inicial si falla
					continue;
				
			default:
				heading_dot = 0;
				if( this->delimitadores[c] ) {  // es un delimitador indiscutible
					i++;
					continue;
				}
				token:
				if( Tokenizar_acronimo(str, tokens, i, curr_token) ) 
					continue;

				Tokenizar_token_normal(str, tokens, i, curr_token);
		}
	}
}

void Tokenizador::TokenizarCasosEspeciales_UMA( const string& str, list<string>& tokens ) const {

	// URL 			Y
	// Decimal 		N (.,)
	// Email 		N
	// Acronimo 	Y 		
	// Multiw 		N

	int i=0;
	unsigned char c;
	string curr_token;
	
	while( i<str.size() ) {

		curr_token.clear();
		
		if( this->pasarAminuscSinAcentos )
			c = conversion[(unsigned char)str[i]];
		else 
			c = (unsigned char)str[i];

		switch(c) {

			case 'h':
				if (Tokenizar_http(str,tokens, i, curr_token))
					continue;
				else
					goto token;
			case 'f':
				if (Tokenizar_ftp(str,tokens, i, curr_token))
					continue;
				else
					goto token;
			
			default:

				if( this->delimitadores[c] ) {  // es un delimitador indiscutible
					i++;
					continue;
				}

				token:

				if( Tokenizar_acronimo(str, tokens, i, curr_token) )
					continue;

				if( Tokenizar_multipalabra(str, tokens, i, curr_token) )
					continue;

				Tokenizar_token_normal(str, tokens, i, curr_token);
				//continue;
		}
	}
}

void Tokenizador::TokenizarCasosEspeciales_UAE( const string& str, list<string>& tokens ) const {

	// URL 			Y
	// Decimal 		N
	// Email 		Y
	// Acronimo 	Y 		
	// Multiw 		N

	//cout << "entrando en UAE" << endl;

	int i=0;
	unsigned char c;
	string curr_token;
	bool canBeAcronym;

	while( i<str.size() ) {

		curr_token.clear();
		canBeAcronym = false;

		if( this->pasarAminuscSinAcentos )
			c = conversion[(unsigned char)str[i]];
		else 
			c = (unsigned char)str[i];

		switch(c) {

			case 'h':
				if (Tokenizar_http(str,tokens, i, curr_token))
					continue;
				else
					goto token;
			case 'f':
				if (Tokenizar_ftp(str,tokens, i, curr_token))
					continue;
				else
					goto token;
			
			case '@':
				i++;
				continue;

			default:
				//printf("intentando default\n");

				if( this->delimitadores[c] ) {  // es un delimitador de los normales
					i++;
					continue;
				}
				//printf("intentando email %d\n", i);

				token:

				if( Tokenizar_email_A(str, tokens, i, curr_token, canBeAcronym) )
					continue;
					//printf("intentando acronimo (%d), (%s)\n", canBeAcronym, curr_token);
				
				//cout << "intentando acronimo (" << curr_token << ")" << endl;

				if ( canBeAcronym && Tokenizar_acronimo(str, tokens, i, curr_token) ) {
					//cout << "i al salir: " << i << endl;
					continue;
				}
					
				//printf("intentando normal\n");

				Tokenizar_token_normal(str, tokens, i, curr_token);
		}
	}
}

void Tokenizador::TokenizarCasosEspeciales_UDAE( const string& str, list<string>& tokens ) const {

	// URL 			Y
	// Decimal 		Y
	// Email 		N
	// Acronimo 	Y		
	// Multiw 		N

	int i=0;
	unsigned char c;
	string curr_token;
	bool canBeAcronym;
	char heading_dot;
	
	while( i<str.size() ) {
		
		curr_token.clear();
		canBeAcronym = true;

		if( this->pasarAminuscSinAcentos )
			c = conversion[(unsigned char)str[i]];
		else 
			c = (unsigned char)str[i];
		
		//cout << "vamos por " << c << endl;	

		switch(c) {

			case 'h':
				heading_dot = 0;
				if (Tokenizar_http(str,tokens, i, curr_token))
					continue;
				else
					goto token;
			
			case 'f':
				heading_dot = 0;
				if (Tokenizar_ftp(str,tokens, i, curr_token))
					continue;
				else
					goto token;
			
			case '@':
				i++;
				heading_dot = 0;
				continue;

			case ',':
			case '.':
				heading_dot = c;
				i++;
				break;

			case '0': case '1': case '2': case '3': case '4': // ,123a  la coma se tiene que descartar igual
			case '5': case '6': case '7': case '8': case '9': 
				
				if( Tokenizar_decimal( str, tokens, i, curr_token, heading_dot ) ) { // quitará la coma inicial si falla
					//cout << "pushed element " << tokens.back() << endl;
					continue;
				}
				
			default:
				heading_dot = 0;
				//cout << "es otra cosa " << this->delimitadores[c] << " - " << (int)exceptions[c] << endl;
				if( this->delimitadores[c] ) {  // es un delimitador indiscutible
					i++;
					//cout << "delimitador saltado" << endl;
					continue;
				}
				
				token:
				
				if( Tokenizar_email_A(str, tokens, i, curr_token, canBeAcronym) )
					continue;

				if( canBeAcronym && Tokenizar_acronimo(str, tokens, i, curr_token) ) 
					continue;

				Tokenizar_token_normal(str, tokens, i, curr_token);
		}
	}
}

void Tokenizador::TokenizarCasosEspeciales_UM( const string& str, list<string>& tokens ) const {

	// URL 			Y
	// Decimal 		N
	// Email 		N
	// Acronimo 	N		
	// Multiw 		Y

	//cout << "is space a delimiter: " << this->delimitadores[' '] << endl;

	int i=0;
	unsigned char c;
	string curr_token;

	while( i<str.size() ) {

		curr_token.clear();
		if( this->pasarAminuscSinAcentos )
			c = conversion[(unsigned char)str[i]];
		else 
			c = (unsigned char)str[i];
		
		switch(c) {

			case 'h':
				if (Tokenizar_http(str,tokens, i, curr_token))
					continue;
				else
					goto token;
			case 'f':
				if (Tokenizar_ftp(str,tokens, i, curr_token))
					continue;
				else
					goto token;
				
			default:
				if( this->delimitadores[c] ) {  // es un delimitador indiscutible
					i++;
					continue;
				}

				token:

				if( Tokenizar_multipalabra(str, tokens, i, curr_token) )
					continue;

				Tokenizar_token_normal(str, tokens, i, curr_token);
		}
	}
}

void Tokenizador::TokenizarCasosEspeciales_UDAM( const string& str, list<string>& tokens ) const {

	// URL 			Y
	// Decimal 		Y
	// Email 		N
	// Acronimo 	Y		
	// Multiw 		N

	int i=0;
	unsigned char c;
	string curr_token;
	char heading_dot;
	
	while( i<str.size() ) {
		
		curr_token.clear();

		if( this->pasarAminuscSinAcentos )
			c = conversion[(unsigned char)str[i]];
		else 
			c = (unsigned char)str[i];
		
		switch(c) {

			case 'h':
				heading_dot = 0;
				if (Tokenizar_http(str,tokens, i, curr_token))
					continue;
				else
					goto token;
			case 'f':
				heading_dot = 0;
				if (Tokenizar_ftp(str,tokens, i, curr_token))
					continue;
				else
					goto token;
			
			case ',':
			case '.':
				heading_dot = c;
				i++;
				break;	
			case '0': case '1': case '2': case '3': case '4': // ,123a  la coma se tiene que descartar igual
			case '5': case '6': case '7': case '8': case '9': 
				
				if( Tokenizar_decimal( str, tokens, i, curr_token, heading_dot ) ) // quitará la coma inicial si falla
					continue;
				
			default:
				heading_dot = 0;
				if( this->delimitadores[c] ) {  // es un delimitador indiscutible
					i++;
					continue;
				}
				
				token:

				if( Tokenizar_acronimo(str, tokens, i, curr_token) ) 
					continue;

				if( Tokenizar_multipalabra(str, tokens, i, curr_token) )
					continue;

				Tokenizar_token_normal(str, tokens, i, curr_token);
		}
	}
}

void Tokenizador::TokenizarCasosEspeciales_UMAE( const string& str, list<string>& tokens ) const  {

	// URL 			Y
	// Decimal 		N
	// Email 		Y
	// Acronimo 	Y 		
	// Multiw 		N

	int i=0;
	unsigned char c;
	string curr_token;
	bool canBeAcronym, canBeMultiword;

	while( i<str.size() ) {

		canBeAcronym = canBeMultiword = false;
		curr_token.clear();
		if( this->pasarAminuscSinAcentos )
			c = conversion[(unsigned char)str[i]];
		else 
			c = (unsigned char)str[i];
		switch(c) {

			case 'h':
				if (Tokenizar_http(str,tokens, i, curr_token))
					continue;
				else
					goto token;
			case 'f':
				if (Tokenizar_ftp(str,tokens, i, curr_token))
					continue;
				else
					goto token;
			
			case '@':
				i++;
				continue;

			default:
				if( this->delimitadores[c] ) {  // es un delimitador de los normales
					i++;
					continue;
				}

				token:

				if( Tokenizar_email_AM(str, tokens, i, curr_token, canBeAcronym, canBeMultiword) )
					continue;
				
				if ( canBeAcronym && Tokenizar_acronimo(str, tokens, i, curr_token))
					continue;

				if ( canBeMultiword && Tokenizar_multipalabra(str, tokens, i, curr_token))
					continue;

				Tokenizar_token_normal(str, tokens, i, curr_token);
		}
	}

}

void Tokenizador::TokenizarCasosEspeciales_UDEAM( const string& str, list<string>& tokens ) const {

	// URL 			Y
	// Decimal 		Y
	// Email 		N
	// Acronimo 	Y		
	// Multiw 		N

	int i=0;
	char c;
	string curr_token;
	bool canBeAcronym, canBeMultiword;
	char heading_dot;
	
	while( i<str.size() ) {
		
		canBeAcronym = canBeMultiword;

		if( this->pasarAminuscSinAcentos )
			c = conversion[(unsigned char)str[i]];
		else 
			c = (unsigned char)str[i];
		
		switch(c) {

			case 'h':
				heading_dot = 0;
				if (Tokenizar_http(str,tokens, i, curr_token))
					continue;
				else
					goto token;
			case 'f':
				heading_dot = 0;
				if (Tokenizar_ftp(str,tokens, i, curr_token))
					continue;
				else
					goto token;
			
			case '@':
				heading_dot = 0;
				i++;
				continue;

			case ',':
			case '.':
				i++;
				heading_dot = c;
				break;

			case '0': case '1': case '2': case '3': case '4': // ,123a  la coma se tiene que descartar igual
			case '5': case '6': case '7': case '8': case '9': 
				
				if( Tokenizar_decimal( str, tokens, i, curr_token, heading_dot ) ) // quitará la coma inicial si falla
					continue;
				
			default:
				heading_dot = 0;
				if( this->delimitadores[c] ) {  // es un delimitador indiscutible
					i++;
					continue;
				}

				token:

				if( Tokenizar_email_AM(str, tokens, i, curr_token, canBeAcronym, canBeMultiword) ) 
					continue;

				if( canBeAcronym && Tokenizar_acronimo(str, tokens, i, curr_token) ) 
					continue;

				if( canBeMultiword && Tokenizar_multipalabra(str, tokens, i, curr_token) )
					continue;

				Tokenizar_token_normal(str, tokens, i, curr_token);
		}
	}
}

void Tokenizador::TokenizarCasosEspeciales_U( const unsigned char* input, const size_t& input_size, list<string>& tokens ) const {

	// URL 			Y
	// Decimal 		N 
	// Email 		N
	// Acronimo 	N 		
	// Multiw 		N

	int i=0;
	unsigned char c;
	string curr_token;

	while( i<input_size ) {

		curr_token.clear();
		
		if( this->pasarAminuscSinAcentos )
			c = conversion[(unsigned char)input[i]];
		else
			c = (unsigned char)input[i];

		switch(c) {

			case 'h':
				if (Tokenizar_http(input, input_size, tokens, i, curr_token))
					continue;
				break;
			
			case 'f':
				if (Tokenizar_ftp(input, input_size, tokens, i, curr_token))
					continue;
				break;
				
			default:
				if( this->delimitadores[c] ) {  // es un delimitador indiscutible
					i++;
					continue;
				}
		}
		Tokenizar_token_normal(input, input_size, tokens, i, curr_token);
	}
}

void Tokenizador::TokenizarCasosEspeciales_UA( const unsigned char* input, const size_t& input_size, list<string>& tokens ) const {

	// URL 			Y
	// Decimal 		N (.,)
	// Email 		N
	// Acronimo 	Y 		
	// Multiw 		N

	int i=0;
	unsigned char c;
	string curr_token;
	
	while( i<input_size ) {

		curr_token.clear();
		
		if( this->pasarAminuscSinAcentos )
			c = conversion[(unsigned char)input[i]];
		else
			c = (unsigned char)input[i];

		switch(c) {

			case 'h':
				if (Tokenizar_http(input, input_size, tokens, i, curr_token))
					continue;
				else
					break;

			case 'f':
				if (Tokenizar_ftp(input, input_size, tokens, i, curr_token))
					continue;
				else
					break;
			
			default:

				if( this->delimitadores[c] ) {  // es un delimitador indiscutible
					i++;
					continue;
				}
		}

		if( Tokenizar_acronimo(input, input_size, tokens, i, curr_token) )
			continue;

		Tokenizar_token_normal(input, input_size, tokens, i, curr_token);
	}
}

void Tokenizador::TokenizarCasosEspeciales_UE( const unsigned char* input, const size_t& input_size, list<string>& tokens ) const {

	// URL 			Y
	// Decimal 		N
	// Email 		Y
	// Acronimo 	N		
	// Multiw 		N

	int i=0;
	unsigned char c;
	string curr_token;

	while( i<input_size ) {

		curr_token.clear();
		if( this->pasarAminuscSinAcentos )
			c = conversion[(unsigned char)input[i]];
		else 
			c = (unsigned char)input[i];
		
		switch(c) {

			case 'h':
				if (Tokenizar_http(input, input_size,tokens, i, curr_token))
					continue;
				else
					break;

			case 'f':
				if (Tokenizar_ftp(input, input_size,tokens, i, curr_token))
					continue;
				else
					break;
			
			case '@':
				i++;
				continue;

			default:
			
				if( this->delimitadores[c]  ) {  // es un delimitador indiscutible
					i++;
					continue;
				}
		}
		if( Tokenizar_email_O(input, input_size, tokens, i, curr_token) )
			continue;

		Tokenizar_token_normal(input, input_size, tokens, i, curr_token);
	}
}

void Tokenizador::TokenizarCasosEspeciales_UME( const unsigned char* input, const size_t& input_size, list<string>& tokens ) const {

	// URL 			Y
	// Decimal 		N
	// Email 		Y
	// Acronimo 	N		
	// Multiw 		N

	int i=0;
	unsigned char c;
	string curr_token;
	bool canBeMultiw;

	while( i<input_size ) {

		canBeMultiw = false;
		curr_token.clear();

		if( this->pasarAminuscSinAcentos )
			c = conversion[(unsigned char)input[i]];
		else 
			c = (unsigned char)input[i];

		switch(c) {

			case 'h':
				if (Tokenizar_http(input, input_size,tokens, i, curr_token))
					continue;
				else
					break;

			case 'f':
				if (Tokenizar_ftp(input, input_size,tokens, i, curr_token))
					continue;
				else
					break;

			case '@':
				i++;
				continue;

			default:

				if( this->delimitadores[c] ) {  // es un delimitador indiscutible
					i++;
					continue;
				}

		}
		if( Tokenizar_email_M(input, input_size, tokens, i, curr_token, canBeMultiw) ) // te lo ha empezado 100%, si hay un guion entonces se comprueba multiw
			continue;

		if( canBeMultiw && Tokenizar_multipalabra(input, input_size, tokens, i, curr_token) )
			continue;

		Tokenizar_token_normal(input, input_size, tokens, i, curr_token);
	}
}

void Tokenizador::TokenizarCasosEspeciales_UDA( const unsigned char* input, const size_t& input_size, list<string>& tokens ) const {

	// URL 			Y
	// Decimal 		Y
	// Email 		N
	// Acronimo 	Y, N al encontrar una coma 		
	// Multiw 		N

	int i=0;
	unsigned char c;
	string curr_token;
	bool canBeAcronym;
	char heading_dot;
	
	while( i<input_size ) {
		
		curr_token.clear();

		if( this->pasarAminuscSinAcentos )
			c = conversion[(unsigned char)input[i]];
		else 
			c = (unsigned char)input[i];
		
		switch(c) {

			case 'h':
				heading_dot = 0;
				if (Tokenizar_http(input, input_size,tokens, i, curr_token))
					continue;
				else
					break;
			
			case 'f':
				heading_dot = 0;
				if (Tokenizar_ftp(input, input_size,tokens, i, curr_token))
					continue;
				else
					break;
			
			case ',':
			case '.':
				heading_dot = c;
				i++;
				break;

			case '0': case '1': case '2': case '3': case '4': // ,123a  la coma se tiene que descartar igual
			case '5': case '6': case '7': case '8': case '9': 
				
				if( Tokenizar_decimal( input, input_size, tokens, i, curr_token, heading_dot ) ) // quitará la coma inicial si falla
					continue;
				
			default:
				heading_dot = 0;
				if( this->delimitadores[c] ) {  // es un delimitador indiscutible
					i++;
					continue;
				}
		}
		if( Tokenizar_acronimo(input, input_size, tokens, i, curr_token) ) 
			continue;

		Tokenizar_token_normal(input, input_size, tokens, i, curr_token);
	}
}

void Tokenizador::TokenizarCasosEspeciales_UMA( const unsigned char* input, const size_t& input_size, list<string>& tokens ) const {

	// URL 			Y
	// Decimal 		N (.,)
	// Email 		N
	// Acronimo 	Y 		
	// Multiw 		N

	int i=0;
	unsigned char c;
	string curr_token;
	
	while( i<input_size ) {

		curr_token.clear();
		
		if( this->pasarAminuscSinAcentos )
			c = conversion[(unsigned char)input[i]];
		else 
			c = (unsigned char)input[i];

		switch(c) {

			case 'h':
				if (Tokenizar_http(input, input_size,tokens, i, curr_token))
					continue;
				else
					break;

			case 'f':
				if (Tokenizar_ftp(input, input_size,tokens, i, curr_token))
					continue;
				else
					break;
			
			default:

				if( this->delimitadores[c] ) {  // es un delimitador indiscutible
					i++;
					continue;
				}

		}
		if( Tokenizar_acronimo(input, input_size, tokens, i, curr_token) )
			continue;

		if( Tokenizar_multipalabra(input, input_size, tokens, i, curr_token) )
			continue;

		Tokenizar_token_normal(input, input_size, tokens, i, curr_token);
	}
}

void Tokenizador::TokenizarCasosEspeciales_UAE( const unsigned char* input, const size_t& input_size, list<string>& tokens ) const {

	// URL 			Y
	// Decimal 		N
	// Email 		Y
	// Acronimo 	Y 		
	// Multiw 		N

	int i=0;
	unsigned char c;
	string curr_token;
	bool canBeAcronym;

	while( i<input_size ) {

		curr_token.clear();
		canBeAcronym = false;

		if( this->pasarAminuscSinAcentos )
			c = conversion[(unsigned char)input[i]];
		else 
			c = (unsigned char)input[i];

		switch(c) {

			case 'h':
				if (Tokenizar_http(input, input_size,tokens, i, curr_token))
					continue;
				else
					break;
			case 'f':
				if (Tokenizar_ftp(input, input_size,tokens, i, curr_token))
					continue;
				else
					break;
			
			case '@':
				i++;
				continue;

			default:

				if( this->delimitadores[c] ) {  // es un delimitador de los normales
					i++;
					continue;
				}
		}
		if( Tokenizar_email_A(input, input_size, tokens, i, curr_token, canBeAcronym) )
			continue;

		if ( canBeAcronym && Tokenizar_acronimo(input, input_size, tokens, i, curr_token) )
			continue;

		Tokenizar_token_normal(input, input_size, tokens, i, curr_token);
	}
}

void Tokenizador::TokenizarCasosEspeciales_UDAE( const unsigned char* input, const size_t& input_size, list<string>& tokens ) const {

	// URL 			Y
	// Decimal 		Y
	// Email 		N
	// Acronimo 	Y		
	// Multiw 		N

	int i=0;
	unsigned char c;
	string curr_token;
	bool canBeAcronym;
	char heading_dot;
	
	while( i<input_size ) {
		
		curr_token.clear();
		canBeAcronym = true;

		if( this->pasarAminuscSinAcentos )
			c = conversion[(unsigned char)input[i]];
		else 
			c = (unsigned char)input[i];
		
		//cout << "vamos por " << c << endl;	

		switch(c) {

			case 'h':
				heading_dot = 0;
				if (Tokenizar_http(input, input_size,tokens, i, curr_token))
					continue;
				else
					break;
			
			case 'f':
				heading_dot = 0;
				if (Tokenizar_ftp(input, input_size,tokens, i, curr_token))
					continue;
				else
					break;
			
			case '@':
				i++;
				heading_dot = 0;
				continue;

			case ',':
			case '.':
				heading_dot = c;
				i++;
				break;

			case '0': case '1': case '2': case '3': case '4': // ,123a  la coma se tiene que descartar igual
			case '5': case '6': case '7': case '8': case '9': 
				
				if( Tokenizar_decimal( input, input_size, tokens, i, curr_token, heading_dot ) ) // quitará la coma inicial si falla
					continue;
				
			default:
				heading_dot = 0;
				if( this->delimitadores[c] ) {  // es un delimitador indiscutible
					i++;
					continue;
				}
		}
		
		if( Tokenizar_email_A(input, input_size, tokens, i, curr_token, canBeAcronym) )
			continue;

		if( canBeAcronym && Tokenizar_acronimo(input, input_size, tokens, i, curr_token) ) 
			continue;

		Tokenizar_token_normal(input, input_size, tokens, i, curr_token);
	}
}

void Tokenizador::TokenizarCasosEspeciales_UM( const unsigned char* input, const size_t& input_size, list<string>& tokens ) const {

	// URL 			Y
	// Decimal 		N
	// Email 		N
	// Acronimo 	N		
	// Multiw 		Y

	int i=0;
	unsigned char c;
	string curr_token;

	while( i<input_size ) {

		curr_token.clear();
		if( this->pasarAminuscSinAcentos )
			c = conversion[(unsigned char)input[i]];
		else 
			c = (unsigned char)input[i];
		
		switch(c) {

			case 'h':
				if (Tokenizar_http(input, input_size,tokens, i, curr_token))
					continue;
				else
					break;

			case 'f':
				if (Tokenizar_ftp(input, input_size,tokens, i, curr_token))
					continue;
				else
					break;
				
			default:
				if( this->delimitadores[c] ) {  // es un delimitador indiscutible
					i++;
					continue;
				}
		}

		if( Tokenizar_multipalabra(input, input_size, tokens, i, curr_token) )
			continue;

		Tokenizar_token_normal(input, input_size, tokens, i, curr_token);
	}
}

void Tokenizador::TokenizarCasosEspeciales_UDAM( const unsigned char* input, const size_t& input_size, list<string>& tokens ) const {

	// URL 			Y
	// Decimal 		Y
	// Email 		N
	// Acronimo 	Y		
	// Multiw 		N

	int i=0;
	unsigned char c;
	string curr_token;
	char heading_dot;
	
	while( i<input_size ) {
		
		curr_token.clear();

		if( this->pasarAminuscSinAcentos )
			c = conversion[(unsigned char)input[i]];
		else 
			c = (unsigned char)input[i];
		
		switch(c) {

			case 'h':
				heading_dot = 0;
				if (Tokenizar_http(input, input_size,tokens, i, curr_token))
					continue;
				else
					break;

			case 'f':
				heading_dot = 0;
				if (Tokenizar_ftp(input, input_size,tokens, i, curr_token))
					continue;
				else
					break;
			
			case ',':
			case '.':
				heading_dot = c;
				i++;
				break;	
			case '0': case '1': case '2': case '3': case '4': // ,123a  la coma se tiene que descartar igual
			case '5': case '6': case '7': case '8': case '9': 
				
				if( Tokenizar_decimal( input, input_size, tokens, i, curr_token, heading_dot ) ) // quitará la coma inicial si falla
					continue;
				
			default:
				heading_dot = 0;
				if( this->delimitadores[c] ) {  // es un delimitador indiscutible
					i++;
					continue;
				}
		}

		if( Tokenizar_acronimo(input, input_size, tokens, i, curr_token) ) 
			continue;

		if( Tokenizar_multipalabra(input, input_size, tokens, i, curr_token) )
			continue;

		Tokenizar_token_normal(input, input_size, tokens, i, curr_token);
	}
}

void Tokenizador::TokenizarCasosEspeciales_UMAE( const unsigned char* input, const size_t& input_size, list<string>& tokens ) const  {

	// URL 			Y
	// Decimal 		N
	// Email 		Y
	// Acronimo 	Y 		
	// Multiw 		N

	int i=0;
	unsigned char c;
	string curr_token;
	bool canBeAcronym, canBeMultiword;

	while( i<input_size ) {

		canBeAcronym = canBeMultiword = false;
		curr_token.clear();
		if( this->pasarAminuscSinAcentos )
			c = conversion[(unsigned char)input[i]];
		else 
			c = (unsigned char)input[i];
		switch(c) {

			case 'h':
				if (Tokenizar_http(input, input_size,tokens, i, curr_token))
					continue;
				else
					break;

			case 'f':
				if (Tokenizar_ftp(input, input_size,tokens, i, curr_token))
					continue;
				else
					break;
			
			case '@':
				i++;
				continue;

			default:
				if( this->delimitadores[c] ) {  // es un delimitador de los normales
					i++;
					continue;
				}
		}	
		if( Tokenizar_email_AM(input, input_size, tokens, i, curr_token, canBeAcronym, canBeMultiword) )
			continue;
		
		if ( canBeAcronym && Tokenizar_acronimo(input, input_size, tokens, i, curr_token))
			continue;

		if ( canBeMultiword && Tokenizar_multipalabra(input, input_size, tokens, i, curr_token))
			continue;

		Tokenizar_token_normal(input, input_size, tokens, i, curr_token);
	}

}

void Tokenizador::TokenizarCasosEspeciales_UDEAM( const unsigned char* input, const size_t& input_size, list<string>& tokens ) const {

	// URL 			Y
	// Decimal 		Y
	// Email 		N
	// Acronimo 	Y		
	// Multiw 		N

	int i=0;
	char c;
	string curr_token;
	bool canBeAcronym, canBeMultiword;
	char heading_dot;
	
	while( i<input_size ) {
		
		canBeAcronym = canBeMultiword;

		if( this->pasarAminuscSinAcentos )
			c = conversion[(unsigned char)input[i]];
		else 
			c = (unsigned char)input[i];
		
		switch(c) {

			case 'h':
				heading_dot = 0;
				if (Tokenizar_http(input, input_size, tokens, i, curr_token))
					continue;
				else
					break;

			case 'f':
				heading_dot = 0;
				if (Tokenizar_ftp(input, input_size, tokens, i, curr_token))
					continue;
				else
					break;
			
			case '@':
				heading_dot = 0;
				i++;
				continue;

			case ',':
			case '.':
				i++;
				heading_dot = c;
				break;

			case '0': case '1': case '2': case '3': case '4': // ,123a  la coma se tiene que descartar igual
			case '5': case '6': case '7': case '8': case '9': 
				
				if( Tokenizar_decimal( input, input_size, tokens, i, curr_token, heading_dot ) ) // quitará la coma inicial si falla
					continue;
				
			default:
				heading_dot = 0;
				if( this->delimitadores[c] ) {  // es un delimitador indiscutible
					i++;
					continue;
				}
		}
		if( Tokenizar_email_AM(input, input_size, tokens, i, curr_token, canBeAcronym, canBeMultiword) ) 
			continue;

		if( canBeAcronym && Tokenizar_acronimo(input, input_size, tokens, i, curr_token) ) 
			continue;

		if( canBeMultiword && Tokenizar_multipalabra(input, input_size, tokens, i, curr_token) )
			continue;

		Tokenizar_token_normal(input, input_size, tokens, i, curr_token);
	}
}


void Tokenizador::TokenizarCasosEspeciales( const string& str, list<string>& tokens ) const
{
	// segun los this->delimitadores mas condicionantes,
	// manda a una funcion u otra.
	// para ahorrar ifs dentro de bucles.

	//printf("%d %d %d %d\n", this->delimitadores['.'], this->delimitadores[','], this->delimitadores['-'], this->delimitadores['@']);

	if( this->delimitadores['.'] ) {

		if( this->delimitadores[','] ) {

			if( this->delimitadores['-'] ) {

				if( this->delimitadores['@'] )  	this->TokenizarCasosEspeciales_UDEAM(str, tokens);
					
				else 								this->TokenizarCasosEspeciales_UDAM(str, tokens);
			}
			else {
				if( this->delimitadores['@'] )  	this->TokenizarCasosEspeciales_UDAE(str, tokens);
					
				else 								this->TokenizarCasosEspeciales_UDA(str, tokens);
			}

		}
		else {
			if( this->delimitadores['-'] ) {

				if( this->delimitadores['@'] )  	this->TokenizarCasosEspeciales_UMAE(str, tokens);
					
				else 								this->TokenizarCasosEspeciales_UMA(str, tokens);
			}
			else {
				if( this->delimitadores['@'] )  	this->TokenizarCasosEspeciales_UAE(str, tokens);
					
				else 								this->TokenizarCasosEspeciales_UA(str, tokens);
			}
		}
	}
	else {
		if( this->delimitadores[','] ) {

			if( this->delimitadores['-'] ) {

				if( this->delimitadores['@'] )  	this->TokenizarCasosEspeciales_UME(str, tokens);
					
				else 								this->TokenizarCasosEspeciales_UM(str, tokens);
			}
			else {
				if( this->delimitadores['@'] )  	this->TokenizarCasosEspeciales_UE(str, tokens);
					
				else 								this->TokenizarCasosEspeciales_U(str, tokens);
			}

		}
		else {
			if( this->delimitadores['-'] ) {

				if( this->delimitadores['@'] )  	this->TokenizarCasosEspeciales_UME(str, tokens);
					
				else 								this->TokenizarCasosEspeciales_UM(str, tokens);
			}
			else {
				if( this->delimitadores['@'] )  	this->TokenizarCasosEspeciales_UE(str, tokens);
					
				else 								this->TokenizarCasosEspeciales_U(str, tokens);
			}
		}
	}
}

void Tokenizador::TokenizarCasosEspeciales( const unsigned char* input, const size_t& input_size, list<string>& tokens ) const
{
	// segun los this->delimitadores mas condicionantes,
	// manda a una funcion u otra.
	// para ahorrar ifs dentro de bucles.

	//printf("%d %d %d %d\n", this->delimitadores['.'], this->delimitadores[','], this->delimitadores['-'], this->delimitadores['@']);

	if( this->delimitadores['.'] ) {

		if( this->delimitadores[','] ) {

			if( this->delimitadores['-'] ) {

				if( this->delimitadores['@'] ) {
					
					this->TokenizarCasosEspeciales_UDEAM(input, input_size, tokens);
				}
				else {
					this->TokenizarCasosEspeciales_UDAM(input, input_size, tokens);
				}
			}
			else {
				if( this->delimitadores['@'] ) {
					
					this->TokenizarCasosEspeciales_UDAE(input, input_size, tokens);
				}
				else {
					this->TokenizarCasosEspeciales_UDA(input, input_size, tokens);
				}
			}

		}
		else {
			if( this->delimitadores['-'] ) {

				if( this->delimitadores['@'] ) {
					
					this->TokenizarCasosEspeciales_UMAE(input, input_size, tokens);
				}
				else {
					this->TokenizarCasosEspeciales_UMA(input, input_size, tokens);
				}
			}
			else {
				if( this->delimitadores['@'] ) {
					
					this->TokenizarCasosEspeciales_UAE(input, input_size, tokens);
				}
				else {
					this->TokenizarCasosEspeciales_UA(input, input_size, tokens);
				}
			}
		}
	}
	else {
		if( this->delimitadores[','] ) {

			if( this->delimitadores['-'] ) {

				if( this->delimitadores['@'] ) {
					
					this->TokenizarCasosEspeciales_UME(input, input_size, tokens);
				}
				else {
					this->TokenizarCasosEspeciales_UM(input, input_size, tokens);
				}
			}
			else {
				if( this->delimitadores['@'] ) {
					
					this->TokenizarCasosEspeciales_UE(input, input_size, tokens);
				}
				else {
					this->TokenizarCasosEspeciales_U(input, input_size, tokens);
				}
			}

		}
		else {
			if( this->delimitadores['-'] ) {

				if( this->delimitadores['@'] ) {
					
					this->TokenizarCasosEspeciales_UME(input, input_size, tokens);
				}
				else {
					this->TokenizarCasosEspeciales_UM(input, input_size, tokens);
				}
			}
			else {
				if( this->delimitadores['@'] ) {
					
					this->TokenizarCasosEspeciales_UE(input, input_size, tokens);
				}
				else {
					this->TokenizarCasosEspeciales_U(input, input_size, tokens);
				}
			}
		}
	}
}


void Tokenizador::Tokenizar (const string& str, list<string>& tokens) const
{
	tokens.clear();

	if( !this->casosEspeciales ) {
		
		if( !this->pasarAminuscSinAcentos ) {
		
			string curr_token="";

			for( unsigned char c : str ) {

				if( this->delimitadores[c] ) { // parar aqui
					tokens.push_back(curr_token);
					curr_token.clear();
				}
				else
					curr_token += c;
			}
			if( !curr_token.empty() )
				tokens.push_back(curr_token);
		}
		else {

			string curr_token="";

			for( unsigned char c : str ) {

				if( this->delimitadores[c] ) { // parar aqui
					tokens.push_back(curr_token);
					curr_token.clear();
				}
				else
					curr_token += conversion[c];

			}
			if( !curr_token.empty() )
				tokens.push_back(curr_token);
		}
	}
	else
		this->TokenizarCasosEspeciales(str, tokens);
}

void Tokenizador::Tokenizar (const unsigned char* input, const size_t& input_size, list<string>& tokens) const
{
	tokens.clear();

	if( !this->casosEspeciales ) {
		
		if( !this->pasarAminuscSinAcentos ) {
		
			string curr_token="";

			for( int i=0; i<input_size; i++ ) {

				if( this->delimitadores[input[i]] ) { // parar aqui
					tokens.push_back(curr_token);
					curr_token.clear();
				}
				else
					curr_token += input[i];
			}
			if( !curr_token.empty() )
				tokens.push_back(curr_token);
		}
		else {

			string curr_token="";

			for( int i=0; i<input_size; i++ ) {

				if( this->delimitadores[input[i]] ) { // parar aqui
					tokens.push_back(curr_token);
					curr_token.clear();
				}
				else
					curr_token += conversion[input[i]];

			}
			if( !curr_token.empty() )
				tokens.push_back(curr_token);
		}
	}
	else
		this->TokenizarCasosEspeciales(input, input_size, tokens);
}

bool Tokenizador::Tokenizar (const string& input, const string& output) const
{
	struct stat fileInfo;
	unsigned char* map_input;
	unsigned char* map_output;
	int fd_input, fd_output, i=0;
	list<string> tokens;
	size_t fileSize_input;

	//////////
	// READ //

	fd_input = open(input.c_str(), O_RDONLY);

	if( stat(input.c_str(), &fileInfo) == 0 ) {
		
		if(fd_input == -1) {
			cerr << "Failed to read file\n";
			return false;
		}
		
		fileSize_input = fileInfo.st_size;
		map_input = reinterpret_cast<unsigned char*>(mmap(0, fileSize_input, PROT_READ, MAP_SHARED, fd_input, 0));
		
		if( map_input == MAP_FAILED ) {
			cerr << "Failed to read file\n";
			return false;
		}
		//this->Tokenizar(string((char*)map_input, fileSize_input), tokens);
		this->Tokenizar(map_input, fileInfo.st_size, tokens);
	}
	else return false;

	///////////
	// WRITE //

	fd_output = open(output.c_str(), O_RDWR | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);
	
	if( stat(output.c_str(), &fileInfo) == 0 ) {
		
		if(fd_output == -1) {
			cerr << "!Failed to write into .tk file\n";
			return false;
		}
		unsigned int memory_needed = PAGE_SIZE*(fileSize_input/PAGE_SIZE + ((fileSize_input%PAGE_SIZE)>0)); 
		
		posix_fallocate(fd_output, 0, memory_needed);
		
		map_output = reinterpret_cast<unsigned char*>(mmap(0, fileSize_input, PROT_WRITE, MAP_SHARED, fd_output, 0));

		if( map_output == MAP_FAILED ) {
			cerr << "!!Failed to write into .tk file\n";
			return false;
		}
		
		// copy tokens list into file
		int j = 0;
		for( auto const& it : tokens ) {
			
			for( i=0; i<it.size(); i++ ) {
				map_output[j++] = it[i];
			}
			
			map_output[j++] = '\n';
			
		}
		
		munmap(map_input, fileSize_input);
		close(fd_input);
		msync(map_output, fileSize_input, MS_ASYNC);
		munmap(map_output, fileSize_input);
		ftruncate(fd_output, j);
		close(fd_output);

		return true;
	}
	else return false;
}

bool Tokenizador::Tokenizar (const string & input) const 
{
	return this->Tokenizar(input, input+".tk");
}

// LA IMPORTANTE
bool Tokenizador::TokenizarListaFicheros (const string& input) const 
{
	string file;
	struct stat fileInfo, fileInfoChild;
	int fd;
	unsigned char* map;
	int prev_it = 0;

	fd = open(input.c_str(), O_RDONLY);
	
	if( stat(input.c_str(), &fileInfo) == 0 ) {
		
		if(fd == -1) {
			cerr << "Failed to read file\n";
			return false;
		}
		map = (unsigned char*)mmap(0, fileInfo.st_size, PROT_READ, MAP_SHARED, fd, 0);
		
		int line_counter = 0;
		
		if( map == 	MAP_FAILED ) {
			cerr << "Failed to read file\n";
			return false;
		}
		// we're in the clear
		int counter = 0, it=0;
		for ( it = 0; it<fileInfo.st_size; it++ ) {

			if( map[it]=='\n' ) 
			{
				if( stat(file.c_str(), &fileInfoChild) == 0 ) {

					if( S_ISDIR(fileInfoChild.st_mode) ) 
						this->TokenizarDirectorio(file);
					else {
						this->Tokenizar(file);
					}
					file.clear();
				}
				else {
					return false;
				}
			}
			else {
				file += map[it];
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
void Tokenizador::DelimitadoresPalabra(const string& nuevodelimitadores) { this->delimiters = procesar_delimitadores(nuevodelimitadores); }

void Tokenizador::AnyadirDelimitadoresPalabra(const string& nuevodelimitadores)
{
	for( unsigned char c : nuevodelimitadores )

		if( !this->delimitadores[c] ) {
			this->delimiters += c;
			this->delimitadores[c] = true;
		}
}

// G
string Tokenizador::DelimitadoresPalabra() const { return this->delimiters; } 

// S 
void Tokenizador::CasosEspeciales (const bool& nuevoCasosEspeciales) { 
	this->casosEspeciales = nuevoCasosEspeciales; 
	this->delimitadores[' '] = nuevoCasosEspeciales;
}

// G
bool Tokenizador::CasosEspeciales () { return casosEspeciales; }

// S
void Tokenizador::PasarAminuscSinAcentos (const bool& nuevoPasarAminuscSinAcentos) { this->pasarAminuscSinAcentos = nuevoPasarAminuscSinAcentos; }

// G
bool Tokenizador::PasarAminuscSinAcentos () { return pasarAminuscSinAcentos; }

