#include "tokenizador.h"
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

					/*
					if( str[i]=='%' || str[i] == '$' ) {
						curr_token.clear();
						curr_token += str[i++];
						tokens.push_back(curr_token);
					}
					*/

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
			case '-':
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
			
			case '-':
				i++;
				continue;

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
			
			case '-':
			case '.':
				i++;
				continue;

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
			case '.':
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
				
			case '-':
				i++;
				continue;

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
			
			case '-':
				i++;
				continue;

			case ',':
			case '.':
				heading_dot = c;
				i++;
				break;	
			case '0': case '1': case '2': case '3': case '4': // ,123a  la coma se tiene que descartar igual
			case '5': case '6': case '7': case '8': case '9': 
				
				if( Tokenizar_decimal( str, tokens, i, curr_token, heading_dot ) ) { // quitará la coma inicial si falla
					heading_dot = 0;
					continue;
				}
				
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
			case '-':
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
			case '-':
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

// X
bool Tokenizador::Tokenizar_ftp( const unsigned char* input, const size_t& input_size, unsigned char* const output, size_t& output_size, int& i) const {

	int token_start = i;
	char c=0;

	if( input_size-i>4 ) {  // to avoid ftp: thats followed by the end of the file

		if( this->pasarAminuscSinAcentos ) {

			// comprobación inicial
			if( (conversion[input[i+1]]=='t') & (conversion[input[i+2]]=='p') & (conversion[input[i+3]]==':') )	{
			
				output[i  ] = conversion[input[i  ]];	output[i+1] = conversion[input[i+1]]; 
				output[i+2] = conversion[input[i+2]]; 	
				
				c = conversion[i+4];  	// despues del :
				//i+=3;  					// en el :

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
						output[output_size+3] = ':'; // :
						output[output_size+4] = c;
						output_size += 5; 
						i += 5;  // después de lo que acabas de leer
						break;

					default:
						if( !this->delimitadores[c] ) {
							output[i+3] = ':'; 
							output[i+4] = c;
							i += 5;
							output_size += 5;
						}
						
						else {
							output[i+3] = '\n'; // en vez del :
							i += 3;
							output_size += 3;
							return true;
						}
				}

				// leer resto del url
				for( ; i<input_size; i++ ) {

					c = conversion[input[i]];

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
							output[output_size++] = c;
							break;
	
						default:
							if( !this->delimitadores[c] ) {
								output[output_size++] = c;
							}
							
							else {
								if( output[output_size] != '\n' )  // no petar a saltos de linea el archivo de salida
									output[output_size++] = '\n';
								return true;
							}
					}
				}
			}
		}
		
		if( this->pasarAminuscSinAcentos ) {

			// comprobación inicial
			if( (input[i+1]=='t') & (input[i+2]=='p') & (input[i+3]==':') )	{
			
				output[i  ] = input[i  ];	output[i+1] = input[i+1]; 
				output[i+2] = input[i+2]; 	
				
				c = i+4;  	// despues del :
				//i+=3;  					// en el :

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
						output[output_size+3] = ':'; // :
						output[output_size+4] = c;
						output_size += 5; 
						i += 5;  // después de lo que acabas de leer
						break;

					default:
						if( !this->delimitadores[c] ) {
							output[i+3] = ':'; 
							output[i+4] = c;
							i += 5;
							output_size += 5;
						}
						
						else {
							output[i+3] = '\n'; // en vez del :
							i += 3;
							output_size += 3;
							return true;
						}
				}

				// leer resto del url
				for( ; i<input_size; i++ ) {

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
							output[output_size++] = c;
							break;
	
						default:
							if( !this->delimitadores[c] ) {
								output[output_size++] = c;
							}
							
							else {
								if( output[output_size] != '\n' )  // no petar a saltos de linea el archivo de salida
									output[output_size++] = '\n';
								return true;
							}
					}
				}
			}
		}
	}

	return false;
}

// X
bool Tokenizador::Tokenizar_http( const unsigned char* input, const size_t& input_size, unsigned char* const output, size_t& output_size, int& i) const {

	int token_start = i;
	int aux_i=i, aux_j=output_size;
	char c=0;

	//cout << "entrando en url, i=" << i << endl;

	if( (input_size-i)>5 ) {  // to avoid http: thats followed by the end of the file

		if( this->pasarAminuscSinAcentos ) {

			// comprobación inicial
			if( (conversion[input[i+1]]=='t') & (conversion[input[i+2]]=='t') & (conversion[input[i+3]]=='p') )	{
			
				output[output_size] 	= conversion[input[i]];		output[output_size+1] = conversion[input[i+1]]; 
				output[output_size+2] 	= conversion[input[i+2]]; 	output[output_size+3] = conversion[input[i+3]];	
				
				if( input[i+4]==':' ) {// http:
					aux_i += 5;
					aux_j += 5;
					//output[output_size+4] = ':';
					
				}
				else if( (input_size-i>6) & conversion[input[i+4]]=='s' & input[i+5]==':' ) {  // https:

					output[output_size+4] = 's';//, output[output_size+5] = ':';
					aux_i += 6;
					aux_j += 6;
				}
				else {
					output_size += 4;  // place yourself after http
					return false;
				}

				c = conversion[input[aux_i]];  	// despues del :

				//cout << "c is " << c << " (" << aux_i << ") (" << aux_j << ")" << endl;

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
						output[aux_j-1] = ':'; 
						output[aux_j] = c;
						output_size = aux_j+1;
						i = aux_i+1;  // después de lo que acabas de leer
						break;

					default:
						if( !this->delimitadores[c] ) {
							output[aux_j-1] = ':'; 
							output[aux_j] = c;
							//cout << "output jsut wrote " << c << " on position " << aux_j << endl;
							i = aux_i+1;
							output_size = aux_j+1;
							break;
						}
						
						else {
							output[aux_j-1] = '\n';
							i = aux_i-1;
							output_size = aux_j;
							return true;
						}
				}

				// leer resto del url
				for( ; i<input_size; i++ ) {

					c = conversion[input[i]];

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
							output[output_size++] = c;
							break;
	
						default:
							if( !this->delimitadores[c] ) {
								output[output_size++] = c;
							}
							
							else {
								if( output[output_size] != '\n' )  // no petar a saltos de linea el archivo de salida
									output[output_size++] = '\n';
								return true;
							}
					}
				}
			}
		}
	
		else {

			// comprobación inicial
			if( (input[i+1]=='t') & (input[i+2]=='t') & (input[i+3]=='p') )	{
			
				output[output_size] 	= input[i];		output[output_size+1] = input[i+1]; 
				output[output_size+2] 	= input[i+2]; 	output[output_size+3] = input[i+3];	
				

				if( input[i+4]==':' ) {// http:
					aux_i, aux_j += 5;
				}

				if( (input_size-i>6) & input[i+4]=='s' & input[i+5]==':' ) {  // https:

					output[output_size+4] = 's';
					aux_i, aux_j += 6;
				}

				c = aux_i;  	// despues del :

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
						
						output[aux_j] = c;
						i = aux_i+1;  // después de lo que acabas de leer
						break;

					default:
						if( !this->delimitadores[c] ) {
							output[aux_j-1] = input[aux_i-1]; 
							output[aux_j] = c;
							i = aux_i+1;
							output_size = aux_j+1;
							break;
						}
						
						else {
							output[aux_j-1] = '\n';
							i = aux_i-1;
							output_size = aux_j;
							return true;
						}
				}

				// leer resto del url
				for( ; i<input_size; i++ ) {

					c = i;

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
							output[output_size++] = c;
							break;
	
						default:
							if( !this->delimitadores[c] ) {
								output[output_size++] = c;
							}
							
							else {
								if( output[output_size] != '\n' )  // no petar a saltos de linea el archivo de salida
									output[output_size++] = '\n';
								return true;
							}
					}
				}
			}
		}
	
	}

	return false;
}

// X
bool Tokenizador::Tokenizar_decimal( const unsigned char* input, const size_t& input_size, unsigned char* const output, size_t& output_size, int& i, unsigned char heading_zero ) const {

	// output_size es el contador de por dónde vamos en el archivo de salida

	unsigned char c;
	bool just_dot=false;
	int token_start=i, output_start=output_size;

	// tiene que haber cero al principio
	if( heading_zero!=0 ) {
		//cout << "a heading zero ladies and gentlemen at i=" << i << "(" << heading_zero << ")	" << endl;
		output[output_size++] 	= '0';
		output[output_size++] 	= heading_zero;
	}

	// establecer que el número ya está empezado
	for( ; i<input_size; i++ ) {

		c = input[i];

		switch(c) {

			case ',':  // TODO
			case '.':
				if( just_dot ) {   // its a normal token
					
					i--; 			// habrá que leer el ., otra vez, por si acaso pertenece a un decimal siguiente
					output[output_size-1] = '\n';
									// cambiamos el ., que escribimos por un \n, y dejamos output_size en la siguiente posición
					return true;
				}
				just_dot = true;
				output[output_size++] = c;
				continue;
			
			case '0': case '1': case '2': case '3': case '4':
			case '5': case '6': case '7': case '8': case '9':
				just_dot = false;
				output[output_size++] = c;
				i++;
				goto rest;

			default:
				if( this->delimitadores[c] ) {  // 123,23- ó 123,23& Delimit right here
					
					i++; 		// colocar después del delimitador
					if( just_dot ) { // 123.&

						output[output_size-1] = '\n';
									// cambiamos el ., que escribimos por un \n, y dejamos output_size en la siguiente posición
						return true;
					}

					if( output[output_size] != '\n' )  // no petar a saltos de linea el archivo de salida
						output[output_size++] = '\n';

					return true;
				}
				else {  // 123,3a 123.34,123a ABORT, nunca fue un decimal. Es un acronimo que se cortaba en la coma.

					i=token_start;
					output_size=output_start;
					return false;  
				}
		}
	}

	rest:

	for( ; i<input_size; i++ ) {

		c = input[i];

		switch(c) {

			case ',':  // TODO
			case '.':
				if( just_dot ) {   // its a normal token
					
					i--; 			// habrá que leer el ., otra vez, por si acaso pertenece a un decimal siguiente
					output[output_size-1] = '\n';
									// cambiamos el ., que escribimos por un \n, y dejamos output_size en la siguiente posición
					return true;
				}
				just_dot = true;
				output[output_size++] = c;
				continue;
			
			case '0': case '1': case '2': case '3': case '4':
			case '5': case '6': case '7': case '8': case '9':
				just_dot = false;
				output[output_size++] = c;
				continue;

			case '%':
			case '$':
				if( just_dot ) {  // 123.$ should not be added
					output[output_size-1]='\n';
					return true;
				}
				else { // 123.23$
					output[output_size++] = '\n';
					output[output_size++] = c;
					output[output_size++] = '\n';
					i++;
					return true;
				}

			default:
				if( this->delimitadores[c] ) {  // 123,23- ó 123,23& Delimit right here
					
					i++; 		// colocar después del delimitador
					if( just_dot ) { // 123.&

						output[output_size-1] = '\n';
									// cambiamos el ., que escribimos por un \n, y dejamos output_size en la siguiente posición
						return true;
					}

					if( output[output_size] != '\n' )  // no petar a saltos de linea el archivo de salida
						output[output_size++] = '\n';

					return true;
				}
				else {  // 123,3a 123.34,123a ABORT, nunca fue un decimal. Es un acronimo que se cortaba en la coma.

					i=token_start;
					output_size=output_start;
					return false;   // provisional
				}
		}
	}


	if( just_dot ) 
		output_size--;  // establecer final del archivo?
	
	return true;
}

// X
bool Tokenizador::Tokenizar_email_O( const unsigned char* input, const size_t& input_size, unsigned char* const output, size_t& output_size, int& i) const {
	
	int at_position, at_position_output;
	int token_start = i, output_start=output_size;
	unsigned char c;

	// leer antes del @
	for( ; i<input_size; i++ ) {
	
		if( this->pasarAminuscSinAcentos )
			c = conversion[(unsigned char)input[i]];
		else
			c = (unsigned char)input[i];

		switch( c ) {

			case '@':
				at_position = i; 
				at_position_output = output_size;
				output[output_size++] = '@';
				i++;
				goto after_at;

			default:
				if( this->delimitadores[c] ) {  // not email. Over right here.

					if( output[output_size] != '\n' )  // no petar a saltos de linea el archivo de salida
						output[output_size++] = '\n';
					i++;
					return true;
				}
				else 						// content
					output[output_size++] = c;
		}
	}

	// has llegado al final del string y no era un email, era un token sin mas.
	//tokens.push_back(curr_token);
	output_size--;
	return true;

	after_at:

	if( this->delimitadores[(unsigned char)input[i]] ) {   // sdasda@{.-_@ or other delimiter} Not an email anymore, delimited at previous @

		output[output_size-1] = '\n';
		i++;
		return true;
	}

	bool just_dot = false;

	// read rest of email post @
	for( ; i<input_size; i++ ) {

		c = (unsigned char)input[i];
		switch( c ) {

			case '_':
				if( !this->delimitadores[c] ) {
					output[output_size++] = c;
					just_dot = false;
				}
				
				else if( !just_dot ) {
					output[output_size++] = c;
					just_dot = true;
				}
				else { 		// sda@a__ | sad@a_a_a__a  gets delimited at ..

					output[output_size-1] = '\n';
					//tokens.push_back(string(curr_token, 0, curr_token.size()-1));
					i++; // get placed after delimiter
					return true;
				}
				break;
			
			case '@':   // bro is NOT an email. Cortamos el token en el @ y del resto se encarga otro
				
				output[at_position_output] = '\n';
				i = at_position + 1;
				output_size = at_position_output + 1;
			
				return true;

			default:
				if( !this->delimitadores[c] ) {  	// content
					output[output_size++] = c;
					just_dot = false;
				}
				
				else { 					// terminar
					if( !just_dot ) 
						if( output[output_size] != '\n' )  // no petar a saltos de linea el archivo de salida
							output[output_size++] = '\n';
				
					else  // quitar un delimitador especial puesto al final
						output[output_size-1] = '\n';
					
					i++;
					return true;
				}
		}
	}
	// si has llegado aqui, estas al final del string

	// sda@a.a.a.
	if( just_dot ) 
		output_size--;
	
	return true;
}

// X
bool Tokenizador::Tokenizar_email_A( const unsigned char* input, const size_t& input_size, unsigned char* const output, size_t& output_size, int& i, bool& canBeAcronym ) const {
	
	int at_position, at_position_output;
	int token_start = i;
	unsigned char c;

	for( ; i<input_size; i++ ) {
	
		if( this->pasarAminuscSinAcentos )
			c = conversion[(unsigned char)input[i]];
		else
			c = (unsigned char)input[i];

		switch( c ) {

			case '.':  // no era un email, aún podría ser un acrónimo.  Acronimo tendrá que comprobar si curr_token está vacío o no.
				output[output_size++] = c;
				canBeAcronym = true;
				return false;

			case '@':
				at_position = i; 
				at_position_output = output_size;
				output[output_size++] = '@';
				i++;
				goto after_at;

			default:
				if( this->delimitadores[c] ) {  // not email. Over right here.
					if( output[output_size] != '\n' )  // no petar a saltos de linea el archivo de salida
						output[output_size++] = '\n';
					i++;
					return true;
				}
				else 						// content
					output[output_size++] = c;
		}
	}

	// has llegado al final del string y no era un email, era un token sin mas.
	output_size--;
	return true;

	after_at:

	//cout << "curr token is " << curr_token << " after at --" << i << " -- " << input[i]  << endl;

	if( this->delimitadores[input[i]] ) {   // sdasda@{.-_@ or other delimiter} Not an email anymore, delimited at previous @

		output[output_size-1] = '\n';
		//tokens.push_back(string(curr_token, 0, curr_token.size()-1));
		i++;
		return true;
	}

	bool just_dot = false;

	// read rest of email post @
	for( ; i<input_size; i++ ) {

		if( this->pasarAminuscSinAcentos )
			c = conversion[(unsigned char)input[i]];
		else
			c = (unsigned char)input[i];

		switch( c ) {

			case '.':
				if( !just_dot ) {
					just_dot = true;
					output[output_size++] = c;
				}
				else { 		// sda@a__ | sad@a_a_a__a  gets delimited at ..

					output[output_size-1] = '\n';
					i-=2; // get placed before dots
					return true;
				}
				break;
			case '_':
				if( !this->delimitadores[c] ) {
					output[output_size++] = c;
					just_dot = false;
				}
				
				else if( !just_dot ) {
					output[output_size++] = c;
					just_dot = true;
				}
				else { 		// sda@a__ | sad@a_a_a__a  gets delimited at ..

					output[output_size-1] = '\n';
					i++; // get placed after this->delimitadores
					return true;
				}
				break;
			
			case '@':   // bro is NOT an email. Cortamos el token en el @ y del resto se encarga otro
				output[at_position_output] = '\n';
				i = at_position + 1;
				output_size = at_position_output + 1;
				return true;

			default:
				if( !this->delimitadores[c] ) {  	// content
					output[output_size++] = c;
					just_dot = false;
				}
				
				else { 								// terminar
					if( !just_dot ) 
						if( output[output_size] != '\n' )  // no petar a saltos de linea el archivo de salida
							output[output_size++] = '\n';
				
					else  // quitar un delimitador especial puesto al final
						output[output_size-1] = '\n';
					
					i++;
					return true;
				}
		}
	}
	// si has llegado aqui, estas al final del string

	// sda@a.a.a.
	if( just_dot ) 
		output_size--;
	
	return true;
}

// X
bool Tokenizador::Tokenizar_email_M( const unsigned char* input, const size_t& input_size, unsigned char* const output, size_t& output_size, int& i, bool& canBeMultiword ) const {
	
	int at_position, at_position_output;
	int token_start = i;
	unsigned char c;

	for( ; i<input_size; i++ ) {
	
		if( this->pasarAminuscSinAcentos )
			c = conversion[(unsigned char)input[i]];
		else
			c = input[i];

		switch( c ) {

			case '-':  // no era un email, aún podría ser un multiword.  Multiword tendrá que comprobar si curr_token está vacío o no.
				output[output_size++] = c;
				canBeMultiword = true;
				return false;

			case '@':
				at_position = i; 
				at_position_output = output_size;
				output[output_size++] = '@';
				i++;
				goto after_at;

			default:
				if( this->delimitadores[c] ) {  // not email. Over right here.
					if( output[output_size] != '\n' )  // no petar a saltos de linea el archivo de salida
						output[output_size++] = '\n';
					i++;
					return true;
				}
				else 						// content
					output[output_size++] = c;
		}
	}

	// has llegado al final del string y no era un email, era un token sin mas.
	output_size--;
	return true;

	after_at:

	if( this->delimitadores[input[i]] ) {   // sdasda@{.-_@ or other delimiter} Not an email anymore, delimited at previous @

		output[output_size-1] = '\n';
		i++;
		return true;
	}

	bool just_dot = false;

	// read rest of email post @
	for( ; i<input_size; i++ ) {

		if( this->pasarAminuscSinAcentos )
			c = conversion[(unsigned char)input[i]];
		else
			c = (unsigned char)input[i];
		
		switch( c ) {

			case '-':
				if( !just_dot ) {
					just_dot = true;
					output[output_size++] = c;
				}
				else { 		// sda@a__ | sad@a_a_a__a  gets delimited at ..

					output[output_size-1] = '\n';
					i++; // get placed after dashes
					return true;
				}
				break;
			case '_':
				if( !this->delimitadores[c] ) {
					output[output_size++] = c;
					just_dot = false;
				}
				
				else if( !just_dot ) {
					output[output_size++] = c;
					just_dot = true;
				}
				else { 		// sda@a__ | sad@a_a_a__a  gets delimited at ..

					output[output_size-1] = '\n';
					i++; // get placed after this->delimitadores
					return true;
				}
				break;
			
			case '@':   // bro is NOT an email. Cortamos el token en el @ y del resto se encarga otro
				output[at_position_output] = '\n';
				i = at_position + 1;
				output_size = at_position_output + 1;
				return true;

			default:
				if( !this->delimitadores[c] ) {  	// content
					output[output_size++] = c;
					just_dot = false;
				}
				
				else { 								// terminar
					if( !just_dot ) 
						if( output[output_size] != '\n' )  // no petar a saltos de linea el archivo de salida
							output[output_size++] = '\n';
				
					else  // quitar un delimitador especial puesto al final
						output[output_size-1] = '\n';
					
					i++;
					return true;
				}
		}
	}
	// si has llegado aqui, estas al final del string

	// sda@a.a.a.
	if( just_dot ) 
		output_size--;
	
	return true;
}

// X
bool Tokenizador::Tokenizar_email_AM( const unsigned char* input, const size_t& input_size, unsigned char* const output, size_t& output_size, int& i, bool& canBeAcronym, bool& canBeMultiword ) const {
	
	int at_position, at_position_output;
	int token_start = i;
	unsigned char c;

	for( ; i<input_size; i++ ) {
	
		if( this->pasarAminuscSinAcentos )
			c = conversion[(unsigned char)input[i]];
		else
			c = (unsigned char)input[i];

		switch( c ) {

			case '.':  // no era un email, aún podría ser un acronimo.  Multiword tendrá que comprobar si curr_token está vacío o no.
				output[output_size++] = c;
				canBeAcronym = true;
				return false;
			case '-':  // no era un email, aún podría ser un multiword.  Multiword tendrá que comprobar si curr_token está vacío o no.
				output[output_size++] = c;
				canBeMultiword = true;
				return false;

			case '@':
				at_position = i; 
				at_position_output = output_size;
				output[output_size++] = '@';
				i++;
				goto after_at;

			default:
				if( this->delimitadores[c] ) {  // not email. Over right here.
					if( output[output_size] != '\n' ) { // no petar a saltos de linea el archivo de salida
						output[output_size++] = '\n';
					}
					i++;
					return true;
				}
				else 						// content
					output[output_size++] = c;
		}
	}

	// has llegado al final del string y no era un email, era un token sin mas.
	output_size--;
	return true;

	after_at:

	if( this->delimitadores[input[i]] ) {   // sdasda@{.-_@ or other delimiter} Not an email anymore, delimited at previous @

		output[output_size-1] = '\n';
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
					output[output_size++] = c;
				}
				else { 		// sda@a__ | sad@a_a_a__a  gets delimited at first dot
					output[output_size-1] = '\n';
					i++; // get placed after dashes
					return true;
				}
				break;
			
			case '_':
				if( !this->delimitadores[c] ) {
					output[output_size++] = c;
					just_dot = false;
				}
				
				else if( !just_dot ) {
					output[output_size++] = c;
					just_dot = true;
				}
				else { 		// sda@a__ | sad@a_a_a__a  gets delimited at ..

					output[output_size-1] = '\n';
					i++; // get placed after this->delimitadores
					return true;
				}
				break;
			
			case '@':   // bro is NOT an email. Cortamos el token en el @ y del resto se encarga otro
				output[at_position_output] = '\n';
				i = at_position + 1;
				output_size = at_position_output + 1;
				return true;

			default:
				if( !this->delimitadores[c] ) {  	// content
					output[output_size++] = c;
					just_dot = false;
				}
				
				else { 								// terminar
					
					if( !just_dot ) 
						if( output[output_size] != '\n' )  // no petar a saltos de linea el archivo de salida
							output[output_size++] = '\n';
				
					else  // quitar un delimitador especial puesto al final
						output[output_size-1] = '\n';
					
					i++;
					return true;
				}
		}
	}
	// si has llegado aqui, estas al final del string

	// sda@a.a.a.
	if( just_dot ) 
		output_size--;
	
	return true;
}

// TODO cambiar todas las llamadas a esta función para incluir canBeMultiword
// TODO cambiar la otra versión de esta función para incluir canBeMultiword
bool Tokenizador::Tokenizar_acronimo( const unsigned char* input, const size_t& input_size, unsigned char* const output, size_t& output_size, int& i, bool& canBeMultiword ) const {

	unsigned char c;
	bool just_dot = ( output_size>0 && output[output_size-1] != '\n');
	i+=just_dot;
	
	//cout << "	-- [A] -- i:" << i << " (" << input[i] << ") output_size:" << output_size << " just_dot:" << just_dot << endl;

	// hasta que te encuentres un . o un -
	for( ; i<input_size; i++ ) {

		if( this->pasarAminuscSinAcentos )
			c = conversion[(unsigned char)input[i]];
		else
			c = (unsigned char)input[i];

		switch (c){
		
			case '.': 			// entonces 100% es un acrónimo, seguimos leyendo abajo
				just_dot = true;
				output[output_size++] = c;
				i++;
				goto rest;

			case '-':
				if( this->delimitadores[(unsigned char)'-'] ) {  // entonces es una multiword, se lo pasamos empezado
					canBeMultiword = true;
					output[output_size++] = '-';
					return false;
				}

				break;
		
			default:
				if( this->delimitadores[c] ) {  // era un token normal
					
					if( output[output_size] != '\n' ) {  // no petar a saltos de linea el archivo de salida
						if( just_dot )
							output[output_size-1] = '\n'; 
						else
							output[output_size++] = '\n';
						//cout << " -- [A] -- salto de linea en " << output_size-1 << endl;
					}

					//cout << "parando acronimo con i=" << i << " (" << output_size << ") en " << input[i] << endl;
					i++;
					//cout << "i after leaving: " << i << endl;
					return true;
				}
				output[output_size++] = c;

			break;
		}
	}
	rest:
	// leer resto del acrónimo
	for( ; i<input_size; i++ ) {

		if( this->pasarAminuscSinAcentos )
			c = conversion[(unsigned char)input[i]];
		else
			c = (unsigned char)input[i];

		switch(c) {
			
			case '.':
				if( !just_dot ) {
					just_dot = true;
					output[output_size++] = c;
				}
				else { 	
					output[output_size-1] = '\n';
					i++; // get placed after dots
					return true;
				}
				break;
			
			default:
				if( this->delimitadores[c] ) {  // entonces cortamos aqui
					if( output[output_size] != '\n' ) { // no petar a saltos de linea el archivo de salida
						if( just_dot ) {
							//cout << "iops just dot is correct" << endl;
							output[output_size-1] = '\n'; 
						}
						else
							output[output_size++] = '\n';
					}
					i++;
					return true;
				}
				output[output_size++] = c;
				just_dot = false;

			break;
		}
	}

	if(just_dot)  // estas al final del string
		output_size--;
	return true;
}

// X
bool Tokenizador::Tokenizar_acronimo( const unsigned char* input, const size_t& input_size, unsigned char* const output, size_t& output_size, int& i ) const {

	unsigned char c;
	//bool just_dot = !curr_token.empty();

	bool just_dot = ( output_size>0 && output[output_size-1] != '\n');
	i+=just_dot;

	for( ; i<input_size; i++ ) {

		if( this->pasarAminuscSinAcentos )
			c = conversion[(unsigned char)input[i]];
		else
			c = (unsigned char)input[i];

		switch(c) {
			
			case '.':
				if( !just_dot ) {
					just_dot = true;
					output[output_size++] = c;
				}
				else { 	
					output[output_size-1] = '\n';
					i++; // get placed after dots
					return true;
				}
				break;
			
			default:
				if( this->delimitadores[c] ) {  // entonces cortamos aqui
					if( output[output_size] != '\n' ) {  // no petar a saltos de linea el archivo de salida
						if( just_dot )
							output[output_size-1] = '\n'; 
						else
							output[output_size++] = '\n';	
					}
					return true;
				}
				output[output_size++] = c;

			break;
		}
	}

	if(just_dot)  // estas al final del string
		output_size--;
	return true;
}

// X
bool Tokenizador::Tokenizar_multipalabra( const unsigned char* input, const size_t& input_size, unsigned char* const output, size_t& output_size, int& i ) const {

	//cout << "	-- [M] -- i:" << i << " (" << input[i] << ") output_size:" << output_size << endl;

	unsigned char c;
	bool just_dash = ( output_size>0 && output[output_size-1] != '\n');
	i+=just_dash;

	for( ; i<input_size; i++ ) {

		if( this->pasarAminuscSinAcentos )
			c = conversion[(unsigned char)input[i]];
		else
			c = (unsigned char)input[i];

		switch(c) {
			
			case '-':
				if( !just_dash ) {
					just_dash = true;
					output[output_size++] = c;
				}
				else { 	
					output[output_size-1] = '\n';
					i++; // get placed after dots
					return true;
				}
				break;
			
			default:
				if( this->delimitadores[c] ) {  // entonces cortamos aqui
					if( output[output_size] != '\n' ) { // no petar a saltos de linea el archivo de salida
						if( just_dash )
							output[output_size-1] = '\n'; 
						else
							output[output_size++] = '\n';
					}
					i++;
					return true;
				}
				output[output_size++] = c;
				just_dash = false;

			break;
		}
	}
	if( just_dash )
		output_size--;
	return true;
}

// X
void Tokenizador::Tokenizar_token_normal( const unsigned char* input, const size_t& input_size, unsigned char* const output, size_t& output_size, int& i ) const {

	unsigned char c;

	if( this->pasarAminuscSinAcentos ) {
		for( ; i<input_size; i++ ) {

			c = conversion[input[i]];

			if( this->delimitadores[c] ) {
				if( output[output_size] != '\n' )  // no petar a saltos de linea el archivo de salida
					output[output_size++] = '\n';
				i++;
				return;
			}
			else
				output[output_size++] = c;
		}
	}
	else {
		for( ; i<input_size; i++ ) {

			c = input[i];

			if( this->delimitadores[c] ) {
				if( output[output_size] != '\n' )  // no petar a saltos de linea el archivo de salida
					output[output_size++] = '\n';
				i++;
				return;
			}
			else
				output[output_size++] = c;
		}
	}

	// si has llegado aqui, estas al final del string
	output[output_size++] = '\n';
	return;
}


void Tokenizador::TokenizarCasosEspeciales_U( const unsigned char* input, const size_t& input_size, unsigned char* const output, size_t& output_size ) const {

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
				if (Tokenizar_http(input, input_size, output, output_size, i))
					continue;
				break;
			
			case 'f':
				if (Tokenizar_ftp(input, input_size, output, output_size, i))
					continue;
				break;
				
			default:
				if( this->delimitadores[c] ) {  // es un delimitador indiscutible
					i++;
					continue;
				}
		}
		Tokenizar_token_normal(input, input_size, output, output_size, i);
	}
}

void Tokenizador::TokenizarCasosEspeciales_UA( const unsigned char* input, const size_t& input_size, unsigned char* const output, size_t& output_size ) const {

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
				if (Tokenizar_http(input, input_size, output, output_size, i))
					continue;
				else
					break;

			case 'f':
				if (Tokenizar_ftp(input, input_size, output, output_size, i))
					continue;
				else
					break;
			
			default:

				if( this->delimitadores[c] ) {  // es un delimitador indiscutible
					i++;
					continue;
				}
		}

		if( Tokenizar_acronimo(input, input_size, output, output_size, i) )
			continue;

		Tokenizar_token_normal(input, input_size, output, output_size, i);
	}
}

void Tokenizador::TokenizarCasosEspeciales_UE( const unsigned char* input, const size_t& input_size, unsigned char* const output, size_t& output_size ) const {

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
				if (Tokenizar_http(input, input_size, output, output_size, i))
					continue;
				else
					break;

			case 'f':
				if (Tokenizar_ftp(input, input_size, output, output_size, i))
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
		if( Tokenizar_email_O(input, input_size, output, output_size, i) )
			continue;

		Tokenizar_token_normal(input, input_size, output, output_size, i);
	}
}

void Tokenizador::TokenizarCasosEspeciales_UME( const unsigned char* input, const size_t& input_size, unsigned char* const output, size_t& output_size ) const {

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
				if (Tokenizar_http(input, input_size, output, output_size, i))
					continue;
				else
					break;

			case 'f':
				if (Tokenizar_ftp(input, input_size, output, output_size, i))
					continue;
				else
					break;

			case '@':
			case '-':
			case '.':
				i++;
				continue;

			default:

				if( this->delimitadores[c] ) {  // es un delimitador indiscutible
					i++;
					continue;
				}

		}
		if( Tokenizar_email_M(input, input_size, output, output_size, i, canBeMultiw) ) // te lo ha empezado 100%, si hay un guion entonces se comprueba multiw
			continue;

		if( canBeMultiw && Tokenizar_multipalabra(input, input_size, output, output_size, i) )
			continue;

		Tokenizar_token_normal(input, input_size, output, output_size, i);
	}
}

void Tokenizador::TokenizarCasosEspeciales_UDA( const unsigned char* input, const size_t& input_size, unsigned char* const output, size_t& output_size ) const {

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
				if (Tokenizar_http(input, input_size, output, output_size, i))
					continue;
				else
					break;
			
			case 'f':
				heading_dot = 0;
				if (Tokenizar_ftp(input, input_size, output, output_size, i))
					continue;
				else
					break;
			
			case ',':
			case '.':
				heading_dot = c;
				i++;
				continue;

			case '0': case '1': case '2': case '3': case '4': // ,123a  la coma se tiene que descartar igual
			case '5': case '6': case '7': case '8': case '9': 
				
				if( Tokenizar_decimal( input, input_size, output, output_size, i, heading_dot ) ) // quitará la coma inicial si falla
					continue;
				
			default:
				heading_dot = 0;
				if( this->delimitadores[c] ) {  // es un delimitador indiscutible
					i++;
					continue;
				}
		}
		if( Tokenizar_acronimo(input, input_size, output, output_size, i) ) 
			continue;

		Tokenizar_token_normal(input, input_size, output, output_size, i);
	}
}

void Tokenizador::TokenizarCasosEspeciales_UMA( const unsigned char* input, const size_t& input_size, unsigned char* const output, size_t& output_size ) const {

	// URL 			Y
	// Decimal 		N (.,)
	// Email 		N
	// Acronimo 	Y 		
	// Multiw 		N

	int i=0;
	unsigned char c;
	string curr_token;
	bool canBeMultiword = false;
	
	while( i<input_size ) {

		curr_token.clear();
		canBeMultiword = false;
		
		if( this->pasarAminuscSinAcentos )
			c = conversion[(unsigned char)input[i]];
		else 
			c = (unsigned char)input[i];

		switch(c) {

			case 'h':
				if (Tokenizar_http(input, input_size, output, output_size, i))
					continue;
				else
					break;

			case 'f':
				if (Tokenizar_ftp(input, input_size, output, output_size, i))
					continue;
				else
					break;
			
			case '-':
			case '.':
				i++;
				continue;

			default:

				if( this->delimitadores[c] ) {  // es un delimitador indiscutible
					i++;
					continue;
				}

		}
		if( Tokenizar_acronimo(input, input_size, output, output_size, i, canBeMultiword) )
			continue;

		if( canBeMultiword && Tokenizar_multipalabra(input, input_size, output, output_size, i) )
			continue;

		Tokenizar_token_normal(input, input_size, output, output_size, i);
	}
}

void Tokenizador::TokenizarCasosEspeciales_UAE( const unsigned char* input, const size_t& input_size, unsigned char* const output, size_t& output_size ) const {

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
				if (Tokenizar_http(input, input_size, output, output_size, i))
					continue;
				else
					break;
			case 'f':
				if (Tokenizar_ftp(input, input_size, output, output_size, i))
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
		if( Tokenizar_email_A(input, input_size, output, output_size, i, canBeAcronym) )
			continue;

		if ( canBeAcronym && Tokenizar_acronimo(input, input_size, output, output_size, i) )
			continue;

		Tokenizar_token_normal(input, input_size, output, output_size, i);
	}
}

void Tokenizador::TokenizarCasosEspeciales_UDAE( const unsigned char* input, const size_t& input_size, unsigned char* const output, size_t& output_size ) const {

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
				if (Tokenizar_http(input, input_size, output, output_size, i))
					continue;
				else
					break;
			
			case 'f':
				heading_dot = 0;
				if (Tokenizar_ftp(input, input_size, output, output_size, i))
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
				continue;

			case '0': case '1': case '2': case '3': case '4': // ,123a  la coma se tiene que descartar igual
			case '5': case '6': case '7': case '8': case '9': 
				
				if( Tokenizar_decimal( input, input_size, output, output_size, i, heading_dot ) ) // quitará la coma inicial si falla
					continue;
				
			default:
				heading_dot = 0;
				if( this->delimitadores[c] ) {  // es un delimitador indiscutible
					i++;
					continue;
				}
		}
		
		if( Tokenizar_email_A(input, input_size, output, output_size, i, canBeAcronym) )
			continue;

		if( canBeAcronym && Tokenizar_acronimo(input, input_size, output, output_size, i) ) 
			continue;

		Tokenizar_token_normal(input, input_size, output, output_size, i);
	}
}

void Tokenizador::TokenizarCasosEspeciales_UM( const unsigned char* input, const size_t& input_size, unsigned char* const output, size_t& output_size ) const {

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
				if (Tokenizar_http(input, input_size, output, output_size, i))
					continue;
				else
					break;

			case 'f':
				if (Tokenizar_ftp(input, input_size, output, output_size, i))
					continue;
				else
					break;

			case '-':
				i++;
				continue;

			default:
				if( this->delimitadores[c] ) {  // es un delimitador indiscutible
					i++;
					continue;
				}
		}

		if( Tokenizar_multipalabra(input, input_size, output, output_size, i) )
			continue;

		Tokenizar_token_normal(input, input_size, output, output_size, i);
	}
}

void Tokenizador::TokenizarCasosEspeciales_UDAM( const unsigned char* input, const size_t& input_size, unsigned char* const output, size_t& output_size ) const {

	// URL 			Y
	// Decimal 		Y
	// Email 		N
	// Acronimo 	Y		
	// Multiw 		N

	int i=0;
	unsigned char c;
	char heading_dot;
	bool canBeMultiword;
	
	while( i<input_size ) {
		
		//cout << "[" << i << "] " << conversion[(unsigned char)input[i]] << endl; 
		canBeMultiword = false;

		if( this->pasarAminuscSinAcentos )
			c = conversion[(unsigned char)input[i]];
		else 
			c = (unsigned char)input[i];
		
		switch(c) {

			case 'h':
				heading_dot = 0;
				if (Tokenizar_http(input, input_size, output, output_size, i))
					continue;
				else
					break;

			case 'f':
				heading_dot = 0;
				if (Tokenizar_ftp(input, input_size, output, output_size, i))
					continue;
				else
					break;
			
			case '-':
				heading_dot = 0;
				i++;
				continue;

			case ',':
			case '.':
				heading_dot = c;
				i++;
				continue;

			case '0': case '1': case '2': case '3': case '4': // ,123a  la coma se tiene que descartar igual
			case '5': case '6': case '7': case '8': case '9': 
				
				if( Tokenizar_decimal( input, input_size, output, output_size, i, heading_dot ) ) { // quitará la coma inicial si falla
					heading_dot = 0;
					continue;
				}
				
			default:
				heading_dot = 0;
				if( this->delimitadores[c] ) {  // es un delimitador indiscutible
					i++;
					continue;
				}
		}

		if( Tokenizar_acronimo(input, input_size, output, output_size, i, canBeMultiword) ) 
			continue;

		if( canBeMultiword && Tokenizar_multipalabra(input, input_size, output, output_size, i) )
			continue;

		Tokenizar_token_normal(input, input_size, output, output_size, i);
	}
}

void Tokenizador::TokenizarCasosEspeciales_UMAE( const unsigned char* input, const size_t& input_size, unsigned char* const output, size_t& output_size ) const  {

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
				if (Tokenizar_http(input, input_size, output, output_size, i))
					continue;
				else
					break;

			case 'f':
				if (Tokenizar_ftp(input, input_size, output, output_size, i))
					continue;
				else
					break;
			
			case '@':
			case '-':
			case '.':
				i++;
				continue;

			default:
				if( this->delimitadores[c] ) {  // es un delimitador de los normales
					i++;
					continue;
				}
		}	
		if( Tokenizar_email_AM(input, input_size, output, output_size, i, canBeAcronym, canBeMultiword) )
			continue;
		
		if ( canBeAcronym && Tokenizar_acronimo(input, input_size, output, output_size, i))
			continue;

		if ( canBeMultiword && Tokenizar_multipalabra(input, input_size, output, output_size, i))
			continue;

		Tokenizar_token_normal(input, input_size, output, output_size, i);
	}

}

void Tokenizador::TokenizarCasosEspeciales_UDEAM( const unsigned char* input, const size_t& input_size, unsigned char* const output, size_t& output_size ) const {

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
				if (Tokenizar_http(input, input_size, output, output_size, i))
					continue;
				else
					break;

			case 'f':
				heading_dot = 0;
				if (Tokenizar_ftp(input, input_size, output, output_size, i))
					continue;
				else
					break;
			
			case '@':
			case '-':
				heading_dot = 0;
				i++;
				continue;

			case ',':
			case '.':
				i++;
				heading_dot = c;
				continue;

			case '0': case '1': case '2': case '3': case '4': // ,123a  la coma se tiene que descartar igual
			case '5': case '6': case '7': case '8': case '9': 
				
				if( Tokenizar_decimal( input, input_size, output, output_size, i, heading_dot ) ) // quitará la coma inicial si falla
					continue;
				
			default:
				heading_dot = 0;
				if( this->delimitadores[c] ) {  // es un delimitador indiscutible
					i++;
					continue;
				}
		}
		if( Tokenizar_email_AM(input, input_size, output, output_size, i, canBeAcronym, canBeMultiword) ) 
			continue;

		if( canBeAcronym && Tokenizar_acronimo(input, input_size, output, output_size, i) ) 
			continue;

		if( canBeMultiword && Tokenizar_multipalabra(input, input_size, output, output_size, i) )
			continue;

		Tokenizar_token_normal(input, input_size, output, output_size, i);
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

void Tokenizador::TokenizarCasosEspeciales( const unsigned char* input, const size_t& input_size, unsigned char* const output, size_t& output_size) const
{
	// segun los this->delimitadores mas condicionantes,
	// manda a una funcion u otra.
	// para ahorrar ifs dentro de bucles.

	//printf("%d %d %d %d\n", this->delimitadores['.'], this->delimitadores[','], this->delimitadores['-'], this->delimitadores['@']);

	if( this->delimitadores['.'] ) {

		if( this->delimitadores[','] ) {

			if( this->delimitadores['-'] ) {

				if( this->delimitadores['@'] ) {
					
					this->TokenizarCasosEspeciales_UDEAM(input, input_size, output, output_size);
				}
				else {
					//cout << "entering " << endl;
					this->TokenizarCasosEspeciales_UDAM(input, input_size, output, output_size);
				}
			}
			else {
				if( this->delimitadores['@'] ) {
					
					this->TokenizarCasosEspeciales_UDAE(input, input_size, output, output_size);
				}
				else {
					this->TokenizarCasosEspeciales_UDA(input, input_size, output, output_size);
				}
			}

		}
		else {
			if( this->delimitadores['-'] ) {

				if( this->delimitadores['@'] ) {
					
					this->TokenizarCasosEspeciales_UMAE(input, input_size, output, output_size);
				}
				else {
					this->TokenizarCasosEspeciales_UMA(input, input_size, output, output_size);
				}
			}
			else {
				if( this->delimitadores['@'] ) {
					
					this->TokenizarCasosEspeciales_UAE(input, input_size, output, output_size);
				}
				else {
					this->TokenizarCasosEspeciales_UA(input, input_size, output, output_size);
				}
			}
		}
	}
	else {
		if( this->delimitadores[','] ) {

			if( this->delimitadores['-'] ) {

				if( this->delimitadores['@'] ) {
					
					this->TokenizarCasosEspeciales_UME(input, input_size, output, output_size);
				}
				else {
					this->TokenizarCasosEspeciales_UM(input, input_size, output, output_size);
				}
			}
			else {
				if( this->delimitadores['@'] ) {
					
					this->TokenizarCasosEspeciales_UE(input, input_size, output, output_size);
				}
				else {
					this->TokenizarCasosEspeciales_U(input, input_size, output, output_size);
				}
			}

		}
		else {
			if( this->delimitadores['-'] ) {

				if( this->delimitadores['@'] ) {
					
					this->TokenizarCasosEspeciales_UME(input, input_size, output, output_size);
				}
				else {
					this->TokenizarCasosEspeciales_UM(input, input_size, output, output_size);
				}
			}
			else {
				if( this->delimitadores['@'] ) {
					
					this->TokenizarCasosEspeciales_UE(input, input_size, output, output_size);
				}
				else {
					this->TokenizarCasosEspeciales_U(input, input_size, output, output_size);
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

void Tokenizador::Tokenizar ( const unsigned char* input, const size_t& input_size, unsigned char* const output, size_t& output_size ) const
{
	//tokens.clear();

	int i;

	if( !this->casosEspeciales ) {
		
		if( !this->pasarAminuscSinAcentos ) {
		
			bool token_vacio = true;

			for( i=0; i<input_size; i++ ) {

				if( this->delimitadores[input[i]] ) { // salto de linea en el archivo de salida, sin más

					if( !token_vacio ) 
						output[i] = '\n';
				}
				else {
					token_vacio = false;
					output[i] = conversion[input[i]];
				}
			}
		}
		else {

			bool token_vacio = true;

			for( i=0; i<input_size; i++ ) {

				if( this->delimitadores[input[i]] ) { // salto de linea en el archivo de salida, sin más

					if( !token_vacio ) 
						output[i] = '\n';
				}
				else {
					token_vacio = false;
					output[i] = input[i];
				}
			}
		}
		output_size = i;
	}
	else
		this->TokenizarCasosEspeciales(input, input_size, output, output_size);
}

bool Tokenizador::Tokenizar (const string& input, const string& output) const
{
	struct stat fileInfo;
	unsigned char* map_input;
	unsigned char* map_output;
	int fd_input, fd_output, i=0;
	size_t fileSize_input, fileSize_output=0;

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
		
		// copies tokens list into file and updates fileSize_output 
		//cout << "right before call " << fileSize_input << " " << fileSize_output << " " << memory_needed << endl;
		this->Tokenizar(map_input, fileSize_input, map_output, fileSize_output);
		//cout << "right after call" << endl;

		// CLOSE EVERYTHING
		munmap(map_input, fileSize_input);
		close(fd_input);
		msync(map_output, fileSize_input, MS_ASYNC);
		munmap(map_output, fileSize_input);
		ftruncate(fd_output, fileSize_output);  // remove excess that was reserved
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

