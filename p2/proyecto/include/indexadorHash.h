using namespace std;
#include <iostream>
#include "indexadorInformacion.h"
#include "tokenizador.h"
#include "stemmer.h"

#include <unordered_set>

class IndexadorHash {

    friend ostream& operator<<(ostream& s, const IndexadorHash& p) {
        s << "Fichero con el listado de palabras de parada: " << p. ficheroStopWords << endl;
        s << "Tokenizador: " << p.tok << endl;
        s << "Directorio donde se almacenara el indice generado: " << p.directorioIndice << endl;
        s << "Stemmer utilizado: " << p.tipoStemmer << endl;
        s << "Informacion de la coleccion indexada: " << p.informacionColeccionDocs << endl;
        s << "Se almacenaran las posiciones de los terminos: " << p.almacenarPosTerm;

        return s;
    }

	public:

		IndexadorHash(	const string& fichStopWords,    // Archivo de palabras de parada -> this.ficheroStopWords
						const string& delimitadores,   	// String de delimitadores -> this.tok
						const bool& detectComp, 		//// 
						const bool& minuscSinAcentos,  	//// Para el tokenizador
						const string& dirIndice, 		// Directorio donde almacenar el índice (si NULL, .)
						const int& tStemmer, 			// Tipo stemmer {0=ninguno,1=castellano,2=ingles}
						const bool& almPosTerm); 		// -> almacenarPosTerm
						// Los indices (indice, indiceDocs, informacionColeccionDocs...) quedan vacios

		IndexadorHash(const string& directorioIndexacion);
		// Indexar con hash a partir de una indexación ya realizada, guardada en el directorio indicado (si no existe o vacío, ERROR)
		// La parte privada se iniciará convenientemente.

		IndexadorHash(const IndexadorHash&);

		~IndexadorHash();

		IndexadorHash& operator= (const IndexadorHash&);

		bool Indexar(const string& ficheroDocumentos);
		// Crea un índice para los documentos especificados en ficheroDocumentos (archivo, un documento por línea)
		// No borra el índice ya existente
		// TRUE si lo consigue
		// FALSE si no, como por falta de memoria -> ERROR, indicando el documento y término en el que se ha quedado, y deja en memoria lo que lleva
		// Si se encuentra uno repetido (misma ruta), lo reindexa (borra y vuelve a indexar)
		// Si su fecha de modificación es más reciente que la almacenada, no lo borra, de modo que mantiene el idDoc pero se sigue reindexando.

		bool IndexarDirectorio(const string& dirAIndexar);
		// TRUE si consigue indexar lo que contiene el directorio (y subs) dirAIndexar
		// No borra el índice ya existente
		// FALSE si no, como por falta de memoria -> ERROR, indicando el documento y término en el que se ha quedado, y deja en memoria lo que lleva
		// Si se encuentra uno repetido (misma ruta), lo reindexa (borra y vuelve a indexar)
		// Si su fecha es más reciente que la almacenada, no lo borra, de modo que mantiene el idDoc pero se sigue reindexando.

		bool GuardarIndexacion() const;
		// Se guarda en el directorio contenido en this.directorioIndice la indexación que hay en memoria, incluyendo toda la parte privada y la indexación de la query
		// El constructor de copia IndexadorHash(IndexadorHash) deberá poder leer el archivo que se genera aquí.
		// ----
		// La secuencia de llamadas será la siguiente:
		// IndexadorHash a("./fichStopWords.txt", "[ ,.", "./dirIndexPrueba", 0, false)
		// a.Indexar("./fichConDocsAIndexar.txt")
		// a.GuardarIndexacion()
		// IndexadorHash b("./dirIndexPrueba")

		// FALSE si falta memoria o directorioIndice es incorrecto, muestra un msg de error y limpia los archivos creados
		// si directorioIndice no existe, hay que crearlo

		bool RecuperarIndexacion (const string& directorioIndexacion);
		// Vacía la indexación de memoria 
		// e inicializa un nuevo IndexadorHash que contiene las indexaciones de directorioIndexacion
		// Toda la parte privada se inicializará según ponga
		// Si no existe el directorio, lanzar excepción y devolver false, dejando la indexación vacía. 

		void ImprimirIndexacion() const { // TODO
		cout << "Terminos indexados: " << endl;
		// A continuaci�n aparecer� un listado del contenido del campo privado "�ndice" donde para cada t�rmino indexado se imprimir�: cout << termino << '\t' << InformacionTermino << endl;
		cout << "Documentos indexados: " << endl;
		// A continuaci�n aparecer� un listado del contenido del campo privado "indiceDocs" donde para cada documento indexado se imprimir�: cout << nomDoc << '\t' << InfDoc << endl;
		}

		bool IndexarPregunta(const string& preg);
		// TRUE si consigue crear el índice para la query
		// Vacía los campos indicePregunta e infPregunta antes de hacer la indexación
		// Genera la misma info que para documentos, pero la deja en memoria (en pregunta, indicePregunta e infPregunta)
		// FALSE si no finaliza la operación (falta memoria, query vacía) con mensaje de error.

		bool DevuelvePregunta(string& preg) const;
		// TRUE si hay una pregunta indexada con al menos un término que no sea de parada (al menos un término indexado en indicePregunta)
		// pregunta -> preg

		bool DevuelvePregunta(const string& word, InformacionTerminoPregunta& inf) const; 
		// TRUE si word está indexado en la pregunta, devolviendo su info en inf
		// else, inf vacío

		bool DevuelvePregunta(InformacionPregunta& inf) const; 
		// TRUE si hay una pregunta indexada, devolviendo su info en inf
		// else, inf vacío

		void ImprimirIndexacionPregunta() {
		cout << "Pregunta indexada: " << pregunta << endl;
		cout << "Terminos indexados en la pregunta: " << endl;
		// A continuaci�n aparecer� un listado del contenido de "indicePregunta" donde para cada t�rmino indexado se imprimir�: cout << termino << '\t' << InformacionTerminoPregunta << endl;
		cout << "Informacion de la pregunta: " << infPregunta << endl;
		}

		void ImprimirPregunta() {
		cout << "Pregunta indexada: " << pregunta << endl;
		cout << "Informacion de la pregunta: " << infPregunta << endl;
		}

		bool Devuelve(const string& word, InformacionTermino& inf) const; 
		// Devuelve true si word (aplic�ndole el tratamiento de stemming y may�sculas correspondiente) est� indexado, 
		// devolviendo su informaci�n almacenada "inf". En caso que no est�, devolver�a "inf" vac�o

		bool Devuelve(const string& word, const string& nomDoc, InfTermDoc& InfDoc) const;  
		// Devuelve true si word (aplic�ndole el tratamiento de stemming y may�sculas correspondiente) est� indexado 
		// y aparece en el documento de nombre nomDoc, en cuyo caso devuelve la informaci�n almacenada para word en el documento. 
		// En caso que no est�, devolver�a "InfDoc" vac�o

		bool Existe(const string& word) const;	 
		// Devuelve true si word (aplic�ndole el tratamiento de stemming y may�sculas correspondiente) aparece como t�rmino indexado

		bool BorraDoc(const string& nomDoc); 
		// Devuelve true si nomDoc est� indexado y se realiza el borrado de todos los t�rminos del documento y del documento en los campos privados "indiceDocs" e "informacionColeccionDocs"

		void VaciarIndiceDocs(); 
		// Borra todos los t�rminos del �ndice de documentos: toda la indexaci�n de documentos.

		void VaciarIndicePreg(); 
		// Borra todos los t�rminos del �ndice de la pregunta: toda la indexaci�n de la pregunta actual.

		int NumPalIndexadas() const; 
		// Devolver� el n�mero de t�rminos diferentes indexados (cardinalidad de campo privado "�ndice")

		string DevolverFichPalParada () const; 
		// Devuelve el contenido del campo privado "ficheroStopWords"

		void ListarPalParada() const; 
		// Mostrar� por pantalla las palabras de parada almacenadas (originales, sin aplicar stemming): una palabra por l�nea (salto de l�nea al final de cada palabra)

		int NumPalParada() const; 
		// Devolver� el n�mero de palabras de parada almacenadas

		string DevolverDelimitadores () const; 
		// Devuelve los delimitadores utilizados por el tokenizador

		bool DevolverCasosEspeciales () const;
		// Devuelve si el tokenizador analiza los casos especiales

		bool DevolverPasarAminuscSinAcentos () const;
		// Devuelve si el tokenizador pasa a min�sculas y sin acentos

		bool DevolverAlmacenarPosTerm () const;
		// Devuelve el valor de almacenarPosTerm

		string DevolverDirIndice () const; 
		// Devuelve "directorioIndice" (el directorio del disco duro donde se almacenar� el �ndice)

		int DevolverTipoStemming () const; 
		// Devolver� el tipo de stemming realizado en la indexaci�n de acuerdo con el valor indicado en la variable privada "tipoStemmer"

		void ListarInfColeccDocs() const; 
		// Mostrar por pantalla: cout << informacionColeccionDocs << endl;

		void ListarTerminos() const; 
		// Mostrar por pantalla el contenido el contenido del campo privado "�ndice": cout << termino << '\t' << InformacionTermino << endl;

		bool ListarTerminos(const string& nomDoc) const; 
		// Devuelve true si nomDoc existe en la colecci�n y muestra por pantalla todos los t�rminos indexados del documento con nombre "nomDoc": cout << termino << '\t' << InformacionTermino << endl; . Si no existe no se muestra nada

		void ListarDocs() const; 
		// Mostrar por pantalla el contenido el contenido del campo privado "indiceDocs": cout << nomDoc << '\t' << InfDoc << endl;

		bool ListarDocs(const string& nomDoc) const; 
		// Devuelve true si nomDoc existe en la colecci�n y muestra por pantalla el contenido del campo privado "indiceDocs" para el documento con nombre "nomDoc": cout << nomDoc << '\t' << InfDoc << endl; . Si no existe no se muestra nada

		void clearDoc_fromIndice(const int doc);

		void imprimir_full() const;

	private:

		// Este constructor se pone en la parte privada porque no se permitir� crear un indexador sin inicializarlo convenientemente. La inicializaci�n la decidir� el alumno
		IndexadorHash();	

		// �ndice de t�rminos indexados accesible por el t�rmino
		unordered_map<string, InformacionTermino> indice;	 

		// �ndice de documentos indexados accesible por el nombre del documento
		unordered_map<string, InfDoc> indiceDocs;	 
 
		// Informaci�n recogida de la colecci�n de documentos indexada
		InfColeccionDocs informacionColeccionDocs;	

		// Pregunta indexada actualmente. Si no hay ninguna indexada, contendr�a el valor ""
		string pregunta;

		// �ndice de t�rminos indexados en una pregunta
		unordered_map<string, InformacionTerminoPregunta> indicePregunta;	 

		// Informaci�n recogida de la pregunta indexada
		InformacionPregunta infPregunta;	

		// El filtrado de stopWords se realizará en la query y en los documentos, teniendo en cuenta minuscSinAcentos y tipoStemmer
		// También se pasará a minuscSinAcentos el archivo de stopWords
		unordered_set<string> stopWords;

		// Nombre del fichero que contiene las palabras de parada
		string ficheroStopWords;

		// Tokenizador que se usar� en la indexaci�n. 
		Tokenizador tok;	

		stemmerPorter stem;

		string directorioIndice;
		// "directorioIndice" ser� el directorio del disco duro donde se almacenar� el �ndice. 
		// En caso que contenga la cadena vac�a se crear� en el directorio donde se ejecute el indexador

		int tipoStemmer;
		// 0 = no se aplica stemmer: se indexa el t�rmino tal y como aparece tokenizado
		// Los siguientes valores har�n que los t�rminos a indexar se les aplique el stemmer y se almacene solo dicho stem.
		// 1 = stemmer de Porter para espa�ol
		// 2 = stemmer de Porter para ingl�s
		// Para el stemmer de Porter se utilizar�n los archivos stemmer.cpp y stemmer.h, concretamente las funciones de nombre "stemmer"

		bool almacenarPosTerm;	
		// Si es true se almacenar� la posici�n en la que aparecen los t�rminos dentro del documento en la clase InfTermDoc

		bool procesarStopWords(const string&, unordered_set<string>&);

};
