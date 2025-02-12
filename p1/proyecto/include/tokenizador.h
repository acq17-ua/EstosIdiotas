#include <iostream>
#include <list>
using namespace std;

class Tokenizador {
	friend ostream& operator<<(ostream&, const Tokenizador&);	 
		// cout << "DELIMITADORES: " << delimiters << " TRATA CASOS ESPECIALES: " << casosEspeciales << " PASAR A MINUSCULAS Y SIN ACENTOS: " << pasarAminuscSinAcentos;
		// Aunque se modifique el almacenamiento de los delimitadores por temas de eficiencia, 
		// el campo delimiters se imprimirá con el string leído en el tokenizador (tras las modificaciones y eliminación de los caracteres repetidos correspondientes)
		//

	public:
		Tokenizador (const string& delimitadoresPalabra, const bool& kcasosEspeciales, const bool& minuscSinAcentos);	
		// Inicializa delimiters a delimitadoresPalabra filtrando que no se introduzcan delimitadores repetidos 
		//  (de izquierda a derecha, en cuyo caso se eliminarían los que hayan sido repetidos por la derecha); 
		// casosEspeciales a kcasosEspeciales; pasarAminuscSinAcentos a minuscSinAcentos

		Tokenizador (const Tokenizador&);

		Tokenizador ();	
		// Inicializa: 
		// 		delimiters=",;:.-/+*\\ '\"{}[]()<>¡!¿?&#=\t@"; 
		// 		casosEspeciales a true; 
		// 		pasarAminuscSinAcentos a false

		~Tokenizador ();	// Pone delimiters=""

		Tokenizador& operator= (const Tokenizador&);

		void Tokenizar (const string& str, list<string>& tokens) const;
		// Tokeniza str devolviendo el resultado en tokens. La lista tokens se vaciará antes de almacenar el resultado de la tokenización. 

		bool Tokenizar (const string& i, const string& f) const; 
		// Tokeniza el fichero i guardando la salida en el fichero f (una palabra en cada línea del fichero). 
		//  Devolverá:
		//  	true si se realiza la tokenización de forma correcta; 
		//  	false en caso contrario enviando a cerr el mensaje correspondiente (p.ej. que no exista el archivo i)

		bool Tokenizar (const string & i) const;
		// Tokeniza el fichero i guardando la salida en un fichero de nombre i añadiéndole extensión .tk, y que contendrá una palabra en cada línea del fichero.
		// (sin eliminar previamente la extensión de i por ejemplo, del archivo pp.txt se generaría el resultado en pp.txt.tk) 
		// Devolverá: 
		// 		true si se realiza la tokenización de forma correcta; 
		// 		false en caso contrario enviando a cerr el mensaje correspondiente (p.ej. que no exista el archivo i)

		// LA IMPORTANTE
		bool TokenizarListaFicheros (const string& i) const; 
		// Tokeniza el fichero i que contiene un nombre de fichero por línea 
		// guardando la salida en ficheros (uno por cada línea de i) 
		// cuyo nombre será el leído en i añadiéndole extensión .tk, y que contendrá una palabra en cada línea del fichero leído en i. 
		// Devolverá:
		// 		true si se realiza la tokenización de forma correcta de todos los archivos que contiene i; 
		// 		false en caso contrario enviando a cerr el mensaje correspondiente 
		// 			(p.ej. que no exista el archivo i, o que se trate de un directorio, enviando a "cerr" los archivos de i que no existan o que sean directorios; 
		// 			luego no se ha de interrumpir la ejecución si hay algún archivo en i que no exista)

		bool TokenizarDirectorio (const string& i) const; 
		// Tokeniza todos los archivos que contenga el directorio i, incluyendo los de los subdirectorios, 
		// guardando la salida en ficheros cuyo nombre será el de entrada añadiéndole extensión .tk, y que contendrá una palabra en cada línea del fichero. 
		// Devolverá:
		// 		true si se realiza la tokenización de forma correcta de todos los archivos;
		// 		false en caso contrario enviando a cerr el mensaje correspondiente (p.ej. que no exista el directorio i, o los ficheros que no se hayan podido tokenizar)

		void DelimitadoresPalabra(const string& nuevoDelimiters); 
		// Inicializa delimiters a nuevoDelimiters, filtrando que no se introduzcan delimitadores repetidos 
		// (de izquierda a derecha, en cuyo caso se eliminarían los que hayan sido repetidos por la derecha)

		void AnyadirDelimitadoresPalabra(const string& nuevoDelimiters); // 
		// Añade al final de "delimiters" los nuevos delimitadores que aparezcan en "nuevoDelimiters" (no se almacenarán caracteres repetidos)

		string DelimitadoresPalabra() const; 
		// Devuelve "delimiters" 

		void CasosEspeciales (const bool& nuevoCasosEspeciales);
		// Cambia la variable privada "casosEspeciales" 

		bool CasosEspeciales ();
		// Devuelve el contenido de la variable privada "casosEspeciales" 

		void PasarAminuscSinAcentos (const bool& nuevoPasarAminuscSinAcentos);
		// Cambia la variable privada "pasarAminuscSinAcentos". 
		// Atención al formato de codificación del corpus (comando "file" de Linux). Para la corrección de la práctica se utilizará el formato actual (ISO-8859). 

		bool PasarAminuscSinAcentos ();
		// Devuelve el contenido de la variable privada "pasarAminuscSinAcentos"
		//

	private:
		string delimiters;		// Delimitadores de términos. 
								// Aunque se modifique la forma de almacenamiento interna para mejorar la eficiencia, 
								// este campo debe permanecer para indicar el orden en que se introdujeron los delimitadores

		bool casosEspeciales;
		// Si true detectará palabras compuestas y casos especiales. Sino, trabajará al igual que el algoritmo propuesto en la sección "Versión del tokenizador vista en clase"

		bool pasarAminuscSinAcentos;
		// Si true pasará el token a minúsculas y quitará acentos, antes de realizar la tokenización
};
