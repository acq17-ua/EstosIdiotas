#include <iostream>
#include <list>
using namespace std;

class Tokenizador {

	friend ostream& operator<<(ostream&, const Tokenizador&);	 
		// cout << "DELIMITADORES: " << delimiters << " TRATA CASOS ESPECIALES: " << casosEspeciales << " PASAR A MINUSCULAS Y SIN ACENTOS: " << pasarAminuscSinAcentos;
		// Aunque se modifique el almacenamiento de los delimitadores por temas de eficiencia, 
		// el campo delimiters se imprimir? con el string le?do en el tokenizador (tras las modificaciones y eliminaci?n de los caracteres repetidos correspondientes)
		//

	public:
		Tokenizador (const string& delimitadoresPalabra, const bool& kcasosEspeciales, const bool& minuscSinAcentos);	
		// Inicializa delimiters a delimitadoresPalabra filtrando que no se introduzcan delimitadores repetidos 
		//  (de izquierda a derecha, en cuyo caso se eliminar?an los que hayan sido repetidos por la derecha); 
		// casosEspeciales a kcasosEspeciales; pasarAminuscSinAcentos a minuscSinAcentos

		Tokenizador (const Tokenizador&);

		Tokenizador ();	
		// Inicializa: 
		// 		delimiters=",;:.-/+*\\ '\"{}[]()<>?!??&#=\t@"; 
		// 		casosEspeciales a true; 
		// 		pasarAminuscSinAcentos a false

		~Tokenizador ();	// Pone delimiters=""

		Tokenizador& operator= (const Tokenizador&);

		void Tokenizar (const string& str, list<string>& tokens) const;
		// Tokeniza str devolviendo el resultado en tokens. La lista tokens se vaciar? antes de almacenar el resultado de la tokenizaci?n. 

		bool Tokenizar (const string& i, const string& f) const; 
		// Tokeniza el fichero i guardando la salida en el fichero f (una palabra en cada l?nea del fichero). 
		//  Devolver?:
		//  	true si se realiza la tokenizaci?n de forma correcta; 
		//  	false en caso contrario enviando a cerr el mensaje correspondiente (p.ej. que no exista el archivo i)

		bool Tokenizar (const string & i) const;
		// Tokeniza el fichero i guardando la salida en un fichero de nombre i a?adi?ndole extensi?n .tk, y que contendr? una palabra en cada l?nea del fichero.
		// (sin eliminar previamente la extensi?n de i por ejemplo, del archivo pp.txt se generar?a el resultado en pp.txt.tk) 
		// Devolver?: 
		// 		true si se realiza la tokenizaci?n de forma correcta; 
		// 		false en caso contrario enviando a cerr el mensaje correspondiente (p.ej. que no exista el archivo i)

		// LA IMPORTANTE
		bool TokenizarListaFicheros (const string& i) const; 
		// Tokeniza el fichero i que contiene un nombre de fichero por l?nea 
		// guardando la salida en ficheros (uno por cada l?nea de i) 
		// cuyo nombre ser? el le?do en i a?adi?ndole extensi?n .tk, y que contendr? una palabra en cada l?nea del fichero le?do en i. 
		// Devolver?:
		// 		true si se realiza la tokenizaci?n de forma correcta de todos los archivos que contiene i; 
		// 		false en caso contrario enviando a cerr el mensaje correspondiente 
		// 			(p.ej. que no exista el archivo i, o que se trate de un directorio, enviando a "cerr" los archivos de i que no existan o que sean directorios; 
		// 			luego no se ha de interrumpir la ejecuci?n si hay alg?n archivo en i que no exista)

		bool TokenizarDirectorio (const string& i) const; 
		// Tokeniza todos los archivos que contenga el directorio i, incluyendo los de los subdirectorios, 
		// guardando la salida en ficheros cuyo nombre ser? el de entrada a?adi?ndole extensi?n .tk, y que contendr? una palabra en cada l?nea del fichero. 
		// Devolver?:
		// 		true si se realiza la tokenizaci?n de forma correcta de todos los archivos;
		// 		false en caso contrario enviando a cerr el mensaje correspondiente (p.ej. que no exista el directorio i, o los ficheros que no se hayan podido tokenizar)

		void DelimitadoresPalabra(const string& nuevoDelimiters); 
		// Inicializa delimiters a nuevoDelimiters, filtrando que no se introduzcan delimitadores repetidos 
		// (de izquierda a derecha, en cuyo caso se eliminar?an los que hayan sido repetidos por la derecha)

		void AnyadirDelimitadoresPalabra(const string& nuevoDelimiters); // 
		// A?ade al final de "delimiters" los nuevos delimitadores que aparezcan en "nuevoDelimiters" (no se almacenar?n caracteres repetidos)

		string DelimitadoresPalabra() const; 
		// Devuelve "delimiters" 

		void CasosEspeciales (const bool& nuevoCasosEspeciales);
		// Cambia la variable privada "casosEspeciales" 

		bool CasosEspeciales () const;
		// Devuelve el contenido de la variable privada "casosEspeciales" 

		void PasarAminuscSinAcentos (const bool& nuevoPasarAminuscSinAcentos);
		// Cambia la variable privada "pasarAminuscSinAcentos". 
		// Atenci?n al formato de codificaci?n del corpus (comando "file" de Linux). Para la correcci?n de la pr?ctica se utilizar? el formato actual (ISO-8859). 

		bool PasarAminuscSinAcentos () const;
		// Devuelve el contenido de la variable privada "pasarAminuscSinAcentos"
		//

	private:
		string delimiters;		// Delimitadores de t?rminos. 
								// Aunque se modifique la forma de almacenamiento interna para mejorar la eficiencia, 
								// este campo debe permanecer para indicar el orden en que se introdujeron los delimitadores

		bool casosEspeciales;
		// Si true detectar? palabras compuestas y casos especiales. Sino, trabajar? al igual que el algoritmo propuesto en la secci?n "Versi?n del tokenizador vista en clase"

		bool pasarAminuscSinAcentos;
		// Si true pasar? el token a min?sculas y quitar? acentos, antes de realizar la tokenizaci?n

		bool delimitadores[256] = {0};

		string procesar_delimitadores(string s);

		void Tokenizar (const unsigned char* input, const size_t& input_size, unsigned char* const output, size_t& output_size) const;

		void TokenizarCasosEspeciales (const unsigned char* input, const size_t& input_size, unsigned char* const output, size_t& output_size) const;
		void TokenizarCasosEspeciales( const string& str, list<string>& tokens ) const;
		
		void TokenizarCasosEspeciales_U( const string& str, list<string>& tokens ) const;
		void TokenizarCasosEspeciales_UA( const string& str, list<string>& tokens ) const;
		void TokenizarCasosEspeciales_UE( const string& str, list<string>& tokens ) const;
		void TokenizarCasosEspeciales_UME( const string& str, list<string>& tokens ) const;
		void TokenizarCasosEspeciales_UDA( const string& str, list<string>& tokens ) const;
		void TokenizarCasosEspeciales_UMA( const string& str, list<string>& tokens ) const;
		void TokenizarCasosEspeciales_UAE( const string& str, list<string>& tokens ) const;
		void TokenizarCasosEspeciales_UDAE( const string& str, list<string>& tokens ) const;
		void TokenizarCasosEspeciales_UM( const string& str, list<string>& tokens ) const;
		void TokenizarCasosEspeciales_UDAM( const string& str, list<string>& tokens ) const;
		void TokenizarCasosEspeciales_UMAE( const string& str, list<string>& tokens ) const;
		void TokenizarCasosEspeciales_UDEAM( const string& str, list<string>& tokens ) const;

		void TokenizarCasosEspeciales_U( const unsigned char* input, const size_t& input_size, unsigned char* const output, size_t& output_size ) const;
		void TokenizarCasosEspeciales_UA( const unsigned char* input, const size_t& input_size, unsigned char* const output, size_t& output_size ) const;
		void TokenizarCasosEspeciales_UE( const unsigned char* input, const size_t& input_size, unsigned char* const output, size_t& output_size ) const;
		void TokenizarCasosEspeciales_UME( const unsigned char* input, const size_t& input_size, unsigned char* const output, size_t& output_size ) const;
		void TokenizarCasosEspeciales_UDA( const unsigned char* input, const size_t& input_size, unsigned char* const output, size_t& output_size ) const;
		void TokenizarCasosEspeciales_UMA( const unsigned char* input, const size_t& input_size, unsigned char* const output, size_t& output_size ) const;
		void TokenizarCasosEspeciales_UAE( const unsigned char* input, const size_t& input_size, unsigned char* const output, size_t& output_size ) const;
		void TokenizarCasosEspeciales_UDAE( const unsigned char* input, const size_t& input_size, unsigned char* const output, size_t& output_size ) const;
		void TokenizarCasosEspeciales_UM( const unsigned char* input, const size_t& input_size, unsigned char* const output, size_t& output_size ) const;
		void TokenizarCasosEspeciales_UDAM( const unsigned char* input, const size_t& input_size, unsigned char* const output, size_t& output_size ) const;
		void TokenizarCasosEspeciales_UMAE( const unsigned char* input, const size_t& input_size, unsigned char* const output, size_t& output_size ) const;
		void TokenizarCasosEspeciales_UDEAM( const unsigned char* input, const size_t& input_size, unsigned char* const output, size_t& output_size ) const;


		bool Tokenizar_ftp( const string& str, list<string>& tokens, int& i, string& curr_token ) const;
		bool Tokenizar_http( const string& str, list<string>& tokens, int& i, string& curr_token ) const;
		bool Tokenizar_decimal( const string& str, list<string>& tokens, int& i, string& curr_token, char heading_zero ) const;
		bool Tokenizar_email_O( const string& str, list<string>& tokens, int& i, string& curr_token ) const;
		bool Tokenizar_email_A( const string& str, list<string>& tokens, int& i, string& curr_token, bool& canBeAcronym ) const;
		bool Tokenizar_email_M( const string& str, list<string>& tokens, int& i, string& curr_token, bool& canBeMultiword ) const;
		bool Tokenizar_email_AM( const string& str, list<string>& tokens, int& i, string& curr_token, bool& canBeAcronym, bool& canBeMultiword ) const;
		bool Tokenizar_acronimo( const string& str, list<string>& tokens, int&i, string& curr_token ) const;
		bool Tokenizar_acronimo( const string& str, list<string>& tokens, int&i, string& curr_token, bool& canBeMultiword ) const;
		bool Tokenizar_multipalabra( const string& str, list<string>& tokens, int&i, string& curr_token ) const;
		void Tokenizar_token_normal( const string& str, list<string>& tokens, int&i, string& curr_token ) const;

		bool Tokenizar_ftp( const unsigned char* input, const size_t& input_size, unsigned char* const output, size_t& output_size, int& i ) const;
		bool Tokenizar_http( const unsigned char* input, const size_t& input_size, unsigned char* const output, size_t& output_size, int& i ) const;
		bool Tokenizar_decimal( const unsigned char* input, const size_t& input_size, unsigned char* const output, size_t& output_size, int& i, unsigned char heading_zero ) const;
		bool Tokenizar_email_O( const unsigned char* input, const size_t& input_size, unsigned char* const output, size_t& output_size, int& i ) const;
		bool Tokenizar_email_A( const unsigned char* input, const size_t& input_size, unsigned char* const output, size_t& output_size, int& i, bool& canBeAcronym ) const;
		bool Tokenizar_email_M( const unsigned char* input, const size_t& input_size, unsigned char* const output, size_t& output_size, int& i, bool& canBeMultiword ) const;
		bool Tokenizar_email_AM( const unsigned char* input, const size_t& input_size, unsigned char* const output, size_t& output_size, int& i, bool& canBeAcronym, bool& canBeMultiword ) const;
		bool Tokenizar_acronimo( const unsigned char* input, const size_t& input_size, unsigned char* const output, size_t& output_size, int&i ) const;
		bool Tokenizar_acronimo( const unsigned char* input, const size_t& input_size, unsigned char* const output, size_t& output_size, int&i, bool& canBeMultiword ) const;
		bool Tokenizar_multipalabra( const unsigned char* input, const size_t& input_size, unsigned char* const output, size_t& output_size, int&i ) const;
		void Tokenizar_token_normal( const unsigned char* input, const size_t& input_size, unsigned char* const output, size_t& output_size, int&i ) const;
};
