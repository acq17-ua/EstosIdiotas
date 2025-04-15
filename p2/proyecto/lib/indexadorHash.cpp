#include "indexadorHash.h"
#include <sys/stat.h> 		// para metadatos de archivos
#include <sys/mman.h> 		// para memory mapping
#include <fcntl.h> 			// para acceso a archivos
#include <unistd.h> 		// set size of output files
#include <time.h>			// last modified date for files
#include <cstring>
#include <fstream>

using namespace std;

const int PAGE_SIZE = sysconf(_SC_PAGE_SIZE);
constexpr unsigned char conversion[256] = 	{
	0,   1,   2,   3,   4,   5,   6,   7,   8,   9, 
   10,  11,  12,  13,  14,  15,  16,  17,  18,  19, 
   20,  21,  22,  23,  24,  25,  26,  27,  28,  29, 
   30,  31,  32,  33,  34,  35,  36,  37,  38,  39, 
   40,  41,  42,  43,  44,  45,  46,  47,  48,  49, 
   50,  51,  52,  53,  54,  55,  56,  57,  58,  59, 
   60,  61,  62,  63,  64,  97,  98,  99, 100, 101,   	// letras mayusculas
  102, 103, 104, 105, 106, 107, 108, 109, 110, 111,   	//
  112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 	//
  122,  91,  92,  93,  94,  95,  96,  97,  98,  99,   	// letras minusculas
  100, 101, 102, 103, 104, 105, 106, 107, 108, 109,   	//
  110, 111, 112, 113, 114, 115, 116, 117, 118, 119, 	//
  120, 121, 122, 123, 124, 125, 126, 127, 128, 129, 	//
  130, 131, 132, 133, 134, 135, 136, 137, 138, 139, 
  140, 141, 142, 143, 144, 145, 146, 147, 148, 149, 
  150, 151, 152, 153, 154, 155, 156, 157, 158, 159, 
  160, 161, 162, 163, 164, 165, 166, 167, 168, 169, 
  170, 171, 172, 173, 174, 175, 176, 177, 178, 179, 
  180, 181, 182, 183, 184, 185, 186, 187, 188, 189, 
  190, 191, 192,  97, 194, 195, 196, 197, 198, 199, 	// acentos mayus
  200, 101, 202, 203, 204, 105, 206, 207, 208, 241,   	// �
  210, 111, 212, 213, 214, 215, 216, 217, 117, 219, 
  220, 221, 222, 223, 224, 97, 226, 227, 228, 229,   	// acentos minus
  230, 231, 232, 101, 234, 235, 236, 105, 238, 239,   	//
  240, 241, 242, 111, 244, 245, 246, 247, 248, 249,   	//
  117, 251, 252, 253, 254, 255  						//
};

bool IndexadorHash::procesarStopWords(const string& fichStopWords, unordered_set<string>& stopwords) {

	struct stat fileInfo;
	int fd;
	unsigned char* map;

	ifstream stw(fichStopWords);
	string word;

	if( !stw.is_open() ) {
		cerr << "Failed to open stopwords file\n";
		return false;
	}

	while( getline(stw, word) ) 
	{
		if( !word.empty() )
			stopwords.emplace(word);
	}

	stw.close();

	return true;
}

// privado (unused)
IndexadorHash::IndexadorHash() 
{
	this->tok = Tokenizador();
	this->tok.CasosEspeciales(false);
	this->tok.PasarAminuscSinAcentos(true);

	this->indice = unordered_map<string, InformacionTermino>();
	this->indiceDocs = unordered_map<string, InfDoc>();
	this->informacionColeccionDocs = InfColeccionDocs();
	this->pregunta = "";
	this->indicePregunta = unordered_map<string, InformacionTerminoPregunta>();
	this->infPregunta = InformacionPregunta();
	
	this->ficheroStopWords = "";
	this->stopWords = unordered_set<string>();
	
	this->directorioIndice = "";
	this->tipoStemmer = 0;
	this->almacenarPosTerm = false;	
}		// por defecto no hay stopwords -- my choice


IndexadorHash::IndexadorHash(	
	const string& fichStopWords,   
	const string& delimitadores,   
	const bool& detectComp, 		 
	const bool& minuscSinAcentos,  	
	const string& dirIndice, 		
	const int& tStemmer, 			
	const bool& almPosTerm) 
{					
	this->tok 						= Tokenizador(delimitadores, detectComp, minuscSinAcentos);
	this->indice 					= unordered_map<string, InformacionTermino>();
	this->indiceDocs 				= unordered_map<string, InfDoc>();
	this->informacionColeccionDocs 	= InfColeccionDocs();
	this->pregunta 					= "";
	this->indicePregunta 			= unordered_map<string, InformacionTerminoPregunta>();
	this->infPregunta 				= InformacionPregunta();
	this->ficheroStopWords 			= fichStopWords;
	
	unordered_set<string> stopwords;
	if (procesarStopWords(this->ficheroStopWords, stopwords)) 
		this->stopWords = stopwords;
	else
		this->stopWords = unordered_set<string>();
	
		this->directorioIndice 	= dirIndice;
	this->tipoStemmer 		= tStemmer;
	this->stem				= stemmerPorter();
	this->almacenarPosTerm 	= almPosTerm;	
}

IndexadorHash::IndexadorHash(const string& directorioIndexacion) 
{
	this->tok = Tokenizador();
	this->indice 					= unordered_map<string, InformacionTermino>();
	this->indiceDocs 				= unordered_map<string, InfDoc>();
	this->informacionColeccionDocs 	= InfColeccionDocs();
	this->indicePregunta 			= unordered_map<string, InformacionTerminoPregunta>();
	this->infPregunta 				= InformacionPregunta();

	this->RecuperarIndexador(directorioIndexacion);
	this->RecuperarIndexacion(directorioIndexacion);
}

IndexadorHash::IndexadorHash(const IndexadorHash& o) 
{
	this->tok 						= Tokenizador(o.tok);
	this->indice 					= o.indice;
	this->indiceDocs 				= o.indiceDocs;
	this->informacionColeccionDocs 	= o.informacionColeccionDocs;
	this->pregunta 					= o.pregunta;
	this->indicePregunta 			= o.indicePregunta;
	this->infPregunta 				= o.infPregunta;
	this->ficheroStopWords 			= o.ficheroStopWords;
	this->stopWords 				= o.stopWords;
	this->directorioIndice 			= o.directorioIndice;
	this->tipoStemmer 				= o.tipoStemmer;
	this->almacenarPosTerm 			= o.almacenarPosTerm;
}

IndexadorHash::~IndexadorHash() 
{
/* 	this->indice.clear();
	this->indiceDocs.clear();
	this->indicePregunta.clear();
	this->informacionColeccionDocs.clear();
	this->infPregunta.clear();
	this->ficheroStopWords.clear();
	this->stopWords.clear();
	this->directorioIndice.clear();
	this->tipoStemmer = 0; */
}

IndexadorHash& IndexadorHash::operator= (const IndexadorHash& o) 
{
	this->tok 						= Tokenizador(o.tok);
	this->indice 					= o.indice;
	this->indiceDocs 				= o.indiceDocs;
	this->informacionColeccionDocs 	= o.informacionColeccionDocs;
	this->pregunta 					= o.pregunta;
	this->indicePregunta 			= o.indicePregunta;
	this->infPregunta 				= o.infPregunta;
	this->ficheroStopWords 			= o.ficheroStopWords;
	this->stopWords 				= o.stopWords;
	this->directorioIndice 			= o.directorioIndice;
	this->tipoStemmer 				= o.tipoStemmer;
	this->almacenarPosTerm 			= o.almacenarPosTerm;

	return *this;
}

bool operator==(const tm& a, const tm& b) {

	return 	a.tm_year 	== b.tm_year,
			a.tm_mon 	== b.tm_mon,
			a.tm_mday 	== b.tm_mday,
			a.tm_hour 	== b.tm_hour,
			a.tm_min 	== b.tm_min,
			a.tm_sec	== b.tm_sec;
}

void print_list(const list<int> contents) {
	cout << "CONTENTS: ";
	for( const int& s : contents ) {
		cout << s << " ";
	}
	cout << "\n";
}

bool IndexadorHash::Indexar(const string& ficheroDocumentos) 
{
	try {
		this->tok.TokenizarListaFicheros(ficheroDocumentos);

		// leer todos los .tk

		string file, term;
		struct stat fileInfo, fileInfoChild;
		int fd, fd_child;
		unsigned char* map;
		unsigned char* map_tk;
		int pos;
		int aux_id;
		bool stopword;

		fd = open(ficheroDocumentos.c_str(), O_RDONLY);
		
		if( stat(ficheroDocumentos.c_str(), &fileInfo) == 0 ) {
			
			if(fd == -1) {
				cerr << "Failed to read file\n";
				return false;
			}
			map = (unsigned char*)mmap(0, fileInfo.st_size, PROT_READ, MAP_SHARED, fd, 0);
			
			int line_counter = 0;
			
			if( map == 	MAP_FAILED ) {
				cerr << "fFailed to read file\n";
				return false;
			}
			// we're in the clear
			int counter = 0, it=0;
			for ( it = 0; it<fileInfo.st_size; it++ ) {

				// a .tk file (a document)
				if( map[it]=='\n' ) 
				{
					fd_child = open((file+".tk").c_str(), O_RDONLY);

					if( stat((file+".tk").c_str(), &fileInfoChild) == 0 ) {
						
						map_tk = (unsigned char*)mmap(0, fileInfoChild.st_size, PROT_READ, MAP_SHARED, fd_child, 0);
						
						if( map_tk == MAP_FAILED ) {
							cerr << "Failed to read file\n";
							return false;
						}

						unordered_map<string,InfDoc>::const_iterator doc = this->indiceDocs.find(file);
						// this doc is already indexed -> clear it
						if( doc != this->indiceDocs.end() ) 
						{
							aux_id = doc->second.idDoc;
							this->clearDoc_fromIndice(doc->second.idDoc);
							//this->indiceDocs.erase(file);
							this->indiceDocs.at(file).clear();
							
							// has been modified since -> DON'T keep idDoc
							if( !(doc->second.fechaModificacion == *gmtime(&fileInfoChild.st_mtime)) ) 
								this->indiceDocs.at(file).idDoc = InfDoc::nextId++;
						}

						// create entry, wont do anything if it already exists
						this->indiceDocs.emplace(file, InfDoc());
						this->indiceDocs.at(file).fechaModificacion = *gmtime(&fileInfoChild.st_mtime);

						//cout << "documento " << file << endl;
						pos = 0;
						for( int it_tk = 0; it_tk<fileInfoChild.st_size; it_tk++ ) {
							
							// a term
							if( map_tk[it_tk]=='\n' ) {
								
								if( term.size() <= 0 ) continue;
								
								//cout << "*** Indexando término " << term << endl;
								
								stopword = ( this->stopWords.find(term) != this->stopWords.end() );
								
								if( stopword ) {
									this->indiceDocs.at(file).numPal++;
									term = "";
									pos++;
									continue;
								}

								this->stem.stemmer(term, this->tipoStemmer);
								
								// actualizar term
								auto inf_term = this->indice.emplace(term, InformacionTermino());
								this->indice.at(term).ftc++;
								
								//unordered_map<string,InformacionTermino>::const_iterator inf_term = this->indice.find(term);
								
								// actualizar InfDoc
								this->indiceDocs.at(file).numPal++;
								this->indiceDocs.at(file).numPalSinParada++; 

								this->indiceDocs.at(file).numPalDiferentes +=  
																					// el termino se acaba de indexar
																				 ( inf_term.second |   
																					// el termino no tiene al doc en l_docs
																				   inf_term.first->second.l_docs.find( this->indiceDocs.at(file).idDoc )
																				== inf_term.first->second.l_docs.end());
								
								this->indiceDocs.at(file).tamBytes = fileInfoChild.st_size;
								
								// actualizar InformacionTermino::l_docs
								this->indice.emplace(term, InformacionTermino());
								auto inf_l_docs = this->indice.at(term).l_docs.emplace(this->indiceDocs.at(file).idDoc, InfTermDoc());
								
								inf_l_docs.first->second.ft++;
								inf_l_docs.first->second.posTerm.push_back(pos++);
								
								//cout << "[" << file << "] -- {" << term << "}" << "\t\t ft: " << inf_l_docs.first->second.ft << "\t\tnew posterm: " << (pos-1) << endl;

								//cout << "resulting l_docs for term:\n";
								//cout << this->indice.at(term).l_docs.at(this->indiceDocs.at(file).idDoc) << endl;
								//cout << this->indice.at(term) << "\n\n\n";

								// actualizar InfColeccionDocs
								this->informacionColeccionDocs.numTotalPalDiferentes += (inf_term.second & !stopword);

								term = "";
							}
							else {
								term += map_tk[it_tk];
							}
						}
						// actualizar InfColeccionDocs
						this->informacionColeccionDocs += this->indiceDocs.at(file);
						file.clear();
					}
					else { 
						cout << "subfile stat returned false" << endl;
						return false;
					}
				}
				else 
					file += map[it];
			}
			return true;
		}
		return false;

	}catch( bad_alloc& ex ) {
		cout << "Out of memory\n";
		return false;
	}
}

bool IndexadorHash::IndexarDirectorio(const string& dirAIndexar) 
{
	system(("find " + dirAIndexar + " -follow ! -type d ! -name '*.tk' |sort > ./indexar_directorio_res.txt").c_str());
	bool result = Indexar("indexar_directorio_res.txt");
	
	system("rm ./indexar_directorio_res.txt");
	return result;
}

void IndexadorHash::imprimir_full() const
{
	cout 	<< "\n\n -- INDICE -- \n";
	
	for( const auto& term : this->indice ) 
	{
		cout << term.first << " **\n";

		for( const auto& l_doc : term.second.l_docs ) 
			cout << "\t\t" << l_doc.first << "\n";
	}

	cout << "\n\n -- INDICEDOCS -- \n";

	for( const auto& doc : this->indiceDocs )
	{
		cout << doc.first << " **\n";

		cout << "\t\tidDoc: " << doc.second.idDoc << "\n";
		cout << "\t\tnumPal: " << doc.second.numPal << "\n";
		cout << "\t\tnumPalDiferentes: " << doc.second.numPalDiferentes << "\n";
		cout << "\t\tnumPalSinParada: " << doc.second.numPalSinParada << "\n";
	}

	cout << "\n\n -- INDICEPREGUNTA --\n";

	for( const auto& qterm : this->indicePregunta )
	{
		cout << qterm.first << " **\n";
		
		cout << "\t\tft: " << qterm.second.ft << "\n";
		for( const auto& posterm : qterm.second.posTerm )
			cout << "\t\t" << posterm << "\n";
	}	

}

bool IndexadorHash::GuardarIndexador() const 
{
	try {
		string path = this->directorioIndice + "/indexador";
		size_t str_size, obj_size;
		FILE* output;

		system(( "install -D /dev/null " + path).c_str());
		output = fopen(path.c_str(), "wb");
		
		// IndexadorHash
		/// pregunta size
		str_size = strlen(this->pregunta.c_str());
		fwrite(&str_size, sizeof(size_t), 1, output);

		if( str_size ) 
		{
			// pregunta
			fwrite(this->pregunta.c_str(), sizeof(char), str_size, output);
	
			/// InformacionPregunta
			fwrite(&(this->infPregunta), sizeof(InformacionPregunta), 1, output);
		}
		
		/// ficheroStopWords
		str_size = strlen(this->ficheroStopWords.c_str());
		fwrite(&str_size, sizeof(size_t), 1, output);
		fwrite(this->ficheroStopWords.c_str(), sizeof(char), str_size, output);

		/// tok
		//// delimiters
		str_size = strlen(this->tok.delimiters.c_str());
		fwrite(&str_size, sizeof(size_t), 1, output);
		fwrite((this->tok.delimiters).c_str(), sizeof(char), str_size, output);

		//// delimitadores
		obj_size = 256;
		fwrite(&obj_size, sizeof(size_t), 1, output);
		fwrite(&(this->tok.delimitadores), sizeof(bool), obj_size, output);

		//// casosEspeciales
		fwrite(&(this->tok.casosEspeciales), sizeof(bool), 1, output);
		//// pasarAMinuscSinAcentos
		fwrite(&(this->tok.pasarAminuscSinAcentos), sizeof(bool), 1, output);

		/// directorioIndice
		str_size = strlen(this->directorioIndice.c_str());
		fwrite(&str_size, sizeof(size_t), 1, output);
		fwrite(this->directorioIndice.c_str(), sizeof(char), str_size, output);

		// tipo stemmer
		fwrite(&(this->tipoStemmer), sizeof(int), 1, output);

		fclose(output);

		return true;
	} catch( bad_alloc& ex ){
		cerr << "Out of memory\n";
		return false;
	}
}

bool IndexadorHash::RecuperarIndexador(const string& directorioIndexacion)
{
	string path = directorioIndexacion + "/indexador";
	size_t str_size, obj_size;
	char* str_read; bool bool_read;
	FILE* input;

	input = fopen(path.c_str(), "rb");

	if( input == NULL ) {
		cerr << "Failed to open index file\n";
		return false;
	}
	
	// IndexadorHash
	/// pregunta
	fread(&str_size, sizeof(size_t), 1, input);

	if( str_size ) 
	{
		str_read = new char[str_size];
		fread(str_read, sizeof(char), str_size, input);
		str_read[str_size] = '\0';
		this->pregunta = string(str_read);
		delete[] str_read;
		
		/// InformacionPregunta
		fread(&(this->infPregunta), sizeof(InformacionPregunta), 1, input);
	}

	/// ficheroStopWords
	fread(&str_size, sizeof(size_t), 1, input);
	str_read = new char[str_size];
	fread(str_read, sizeof(char), str_size, input);
	str_read[str_size] = '\0';
	this->ficheroStopWords = string(str_read);
	delete[] str_read;
	
	/// tok
	//// delimiters
	fread(&str_size, sizeof(size_t), 1, input);
	str_read = new char[str_size];
	fread(str_read, sizeof(char), str_size, input);
	str_read[str_size] = '\0';
	this->tok.delimiters = string(str_read);
	delete[] str_read;

	//// delimitadores
	fread(&obj_size, sizeof(size_t), 1, input);
	this->tok.delimitadores;
	fread(&(this->tok.delimitadores), sizeof(bool), obj_size, input);

	//// casosEspeciales
	fread(&(this->tok.casosEspeciales), sizeof(bool), 1, input);
	//// pasarAMinuscSinAcentos
	fread(&(this->tok.pasarAminuscSinAcentos), sizeof(bool), 1, input);

	/// directorioIndice
	fread(&str_size, sizeof(size_t), 1, input);
	str_read = new char[str_size];
	fread(str_read, sizeof(char), str_size, input);
	this->directorioIndice = string(str_read);
	delete[] str_read;

	// tipoStemmer
	fread(&(this->tipoStemmer), sizeof(int), 1, input);

	fclose(input);

	return true;
}

/*
	Saves indexation to a single file named ./indice

*/
bool IndexadorHash::GuardarIndexacion() const 
{
	try {
		string path1=this->directorioIndice;
		FILE* output, test;
		size_t obj_size, str_size;

		path1.append("/indice");

		system(( "install -D /dev/null " + path1).c_str());
		output = fopen(path1.c_str(), "wb");
		
		if( !output ) {
			cerr << "Failed to open index file\n";
			return false;
		}

		// Campos del indexador
		this->GuardarIndexador();

		// informacionColeccionDocs
	 	fwrite(&(this->informacionColeccionDocs), sizeof(InfColeccionDocs), 1, output);
		
		// indiceDocs
		obj_size = this->indiceDocs.size();
		fwrite(&obj_size, sizeof(size_t), 1, output);

		for( const auto& doc : this->indiceDocs )
		{
			// key
			str_size = strlen(doc.first.c_str());
			fwrite(&str_size, sizeof(size_t), 1, output);
			fwrite(doc.first.c_str(), sizeof(char), str_size, output);
			// value
			fwrite(&doc.second, sizeof(InfDoc), 1, output);
		}

		// indice
		obj_size = this->indice.size();
		fwrite(&obj_size, sizeof(size_t), 1, output); 							// indice size
		for( const auto& term : this->indice ) 
		{
			// key
			//   string size
			str_size = term.first.size();
			fwrite(&str_size, sizeof(size_t), 1, output); 						// term.key size
			fwrite(term.first.c_str(), sizeof(char), str_size, output); 		// term.key
			
			// value
			fwrite(&term.second.ftc, sizeof(int), 1, output); 					// value.ftc

			obj_size = term.second.l_docs.size();
;			fwrite(&obj_size, sizeof(size_t), 1, output); 						// value.l_docs size
			for( const auto& l_doc : term.second.l_docs ) {
				fwrite(&l_doc.first, sizeof(int), 1, output); 					// value.l_docs.key
				//fwrite(&l_doc.second, sizeof(InfTermDoc), 1, output);
 
				fwrite(&l_doc.second.ft, sizeof(int), 1, output); 				// value.l_docs.value.ft

				obj_size = l_doc.second.posTerm.size();
				fwrite(&obj_size, sizeof(size_t), 1, output); 					// value.l_docs.value.posTerm size
				for( const auto& posterm : l_doc.second.posTerm )
					fwrite( &posterm, sizeof(int), 1, output );					// value.l_docs.value.posTerm[i]
			}
		}

		// indicePregunta
		obj_size = this->indicePregunta.size();
		fwrite(&obj_size, sizeof(size_t), 1, output);
		for( const auto& qterm : this->indicePregunta )
		{
			// key
			str_size = qterm.first.size();
			fwrite(&str_size, sizeof(size_t), 1, output);
			fwrite(qterm.first.c_str(), sizeof(char), str_size, output);
			
			// value
			fwrite(&qterm.second.ft, sizeof(int), 1, output);
			obj_size = qterm.second.posTerm.size();
			fwrite(&obj_size, sizeof(size_t), 1, output);
			for( const auto& posterm : qterm.second.posTerm ) 
				fwrite(&posterm, sizeof(int), 1, output);
		}

		fwrite(&this->indicePregunta, sizeof(InformacionPregunta), 1, output);
		fclose(output);

		return true;
	}
	catch( bad_alloc& ex ) {
		cout << "Out of memory\n";
		return false;
	}
}

bool IndexadorHash::RecuperarIndexacion (const string& directorioIndexacion)
{
	char* string_read;
	string path1=directorioIndexacion;
	FILE* input;
	size_t obj_size, obj_size_child, obj_size_grandchild;
	string key; int value, value2;
	InformacionTermino term; InfTermDoc term_doc;
	InfDoc doc; InformacionPregunta query;
	int i,j;

	this->indiceDocs.clear();
	this->indice.clear();
	this->indicePregunta.clear();
	this->informacionColeccionDocs.clear();

	path1.append("/indice");

	input = fopen(path1.c_str(), "rb");

	if( input == NULL ) {
		cerr << "Failed to open index file\n";
		return false;
	}

	// infColeccionDocs
	fread(&(this->informacionColeccionDocs), sizeof(InfColeccionDocs), 1, input);

	// indiceDocs
	fread(&obj_size, sizeof(size_t), 1, input); 						// indiceDocs size
	
	for( i=0; i<obj_size; i++ ) 
	{
		// key
		//   string size
		fread(&obj_size_child, sizeof(size_t), 1, input); 				// doc.key size
		string_read = new char[obj_size_child];
		fread(string_read, sizeof(char), obj_size_child, input); 		// doc.key
		string_read[obj_size_child] = '\0';
		// value
		fread(&doc, sizeof(InfDoc), 1, input); 							// doc.value
		
		//this->indiceDocs[key] = doc;
		this->indiceDocs.emplace(string_read, doc);
		delete[] string_read;
	}

	// indice
	fread(&obj_size, sizeof(size_t), 1, input); 						// indice size
	for( i=0; i<obj_size; i++ ) 
	{
		// key
		//   string size
		fread(&obj_size_child, sizeof(size_t), 1, input); 				// term.key size
		string_read = new char[obj_size_child];
		fread(string_read, sizeof(char), obj_size_child, input);		// term.key
		string_read[obj_size_child] = '\0';

		// value.ftc
		fread(&obj_size_child, sizeof(int), 1, input); 					// value.ftc
		term.ftc = obj_size_child;

		this->indice.emplace(string_read, term);

		// field of the value
		//   size of l_docs
		fread(&obj_size_child, sizeof(size_t), 1, input); 				// value.l_docs size
		
		// l_docs
		for( j=0; j<obj_size_child; j++ )
		{
			fread(&value, sizeof(int), 1, input); 						// value.l_docs.key
			this->indice.at(string_read).l_docs.emplace(value, term_doc);
			
			// ft
			fread(&(this->indice.at(string_read).l_docs.at(value).ft), sizeof(int), 1, input); 	// value.l_docs.value.ft
			
			// posterm.size
			fread(&obj_size_grandchild, sizeof(size_t), 1, input); 						// value.l_docs.value.posTerm size
			
			// posterm
			for( int k=0; k<obj_size_grandchild; k++ )
			{ 	
				fread(&value2, sizeof(int), 1, input); 									// value.l_docs.value.posTerm[i]
				this->indice.at(string_read).l_docs.at(value).posTerm.push_back(value2);
				//this->indice[key].l_docs[value].posTerm.push_back(value2);
			}
		}
		delete[] string_read;
	}
	
	// indicePregunta
	fread(&obj_size, sizeof(size_t), 1, input);
	for( i=0; i<obj_size; i++ )
	{
		// key
		fread(&obj_size_child, sizeof(size_t), 1, input);
		fread(&key[0], sizeof(char), obj_size_child, input);
		
		// value
		fread(&value, sizeof(int), 1, input);
		this->indicePregunta.emplace(key, InformacionTerminoPregunta());
		this->indicePregunta.at(key).ft = value;

		// value.posterm
		fread(&obj_size_child, sizeof(size_t), 1, input);
		for( j=0; j<obj_size_child; j++ )
		{
			fread(&value, sizeof(int), 1, input);
			this->indicePregunta.at(key).posTerm.push_back(value);
		}
	}

	fread(&(this->infPregunta), sizeof(InformacionPregunta), 1, input);
	fclose(input); // successful

	return true;

}

bool IndexadorHash::IndexarPregunta(const string& preg) 
{
	if( preg.size()==0 )
		return false;

	try {

		list<string> tokens;
		pair<unordered_map<string, InformacionTerminoPregunta>::iterator, bool> insertion;
		bool stopword;
		short int i=0;

		this->indicePregunta.clear();
		this->pregunta.clear();
		this->infPregunta.clear();
		
		this->tok.Tokenizar(preg, tokens);
		for( string& term : tokens ) 
		{
			this->stem.stemmer(term, this->tipoStemmer);

			stopword = (this->stopWords.find(term) != this->stopWords.end());

			insertion = this->indicePregunta.emplace(term, InformacionTerminoPregunta());

			insertion.first->second.ft++;
			insertion.first->second.posTerm.push_back(i++);

			this->pregunta.append(term).push_back(' ');

			this->infPregunta.numTotalPal++;
			this->infPregunta.numTotalPalDiferentes += (insertion.second & !stopword);
			this->infPregunta.numTotalPalSinParada += !stopword;
		}

		return true;
		
	}catch( bad_alloc& ex ) {
		return false;
	}
	
}

bool IndexadorHash::DevuelvePregunta(string& preg) const 
{
	if( !this->indicePregunta.empty() ) {
		preg = this->pregunta;
		return true;
	}
	return false;
}

bool IndexadorHash::DevuelvePregunta(const string& word, InformacionTerminoPregunta& inf) const 
{
	if( this->tok.PasarAminuscSinAcentos() ) {
		string new_word="";
		
		for( const unsigned char c : word )
			new_word += conversion[c];

		if( this->indicePregunta.find(new_word) != this->indicePregunta.end() ) {
			inf = this->indicePregunta.at(new_word);
			return true;
		}
		inf.clear();
		return false;
	}
	if( this->indicePregunta.find(word) != this->indicePregunta.end() ) {
		inf = this->indicePregunta.at(word);
		return true;
	}
	inf.clear();
	return false;
}

bool IndexadorHash::DevuelvePregunta(InformacionPregunta& inf) const 
{
	if( !this->indicePregunta.empty() ) {
		inf = this->infPregunta;
		return true;
	}
	inf.clear();
	return false;
}

bool IndexadorHash::Devuelve(const string& word, InformacionTermino& inf) const 
{
	string new_word="";
	if( this->tok.PasarAminuscSinAcentos() )
	{
		for( const unsigned char c : word )
			new_word += conversion[c];
	}
	else
		new_word = word;

	stemmerPorter stemmer;
	stemmer.stemmer(new_word, this->tipoStemmer);

	auto term = this->indice.find(new_word);
	
	if( term != this->indice.end() )
	{
		inf = term->second;
		return true;
	}
	else {
		inf = InformacionTermino();
		return false;
	}
} 

bool IndexadorHash::Devuelve(const string& word, const string& nomDoc, InfTermDoc& infDoc) const 
{
/* 	cout << "has llamado a Devuelve() con termino: " << word << " y doc: " << nomDoc << endl << "\t--- Indice ---\n";
	for( const auto& a : this->indice ) {
		cout << "\t" << a.first << "\n\t\tIn docs: ";
		
		for( const auto& b : a.second.l_docs ) {
			cout <<  b.first << "\t";
		}
		cout << "\n";
	}
	cout << endl << "\t--- IndiceDocs ---\n";
	for( const auto& a : this->indiceDocs ) {
		cout << "\t(" << a.second.idDoc << ") - " << a.first;
		
	}
	cout << endl; */


	string new_word="";
	if( this->tok.PasarAminuscSinAcentos() )
	{
		for( const unsigned char c : word )
			new_word += conversion[c];
	}
	else
		new_word = word;

	stemmerPorter stemmer;
	stemmer.stemmer(new_word, this->tipoStemmer);

	auto doc = this->indiceDocs.find(nomDoc);
	auto term = this->indice.find(new_word);

	if( doc != this->indiceDocs.end() & term != this->indice.end() )
	{
		auto termdoc = term->second.l_docs.find(doc->second.idDoc);
		infDoc = termdoc->second;

		return termdoc != term->second.l_docs.end();
	}
	else {
		infDoc = InfTermDoc();
		return false;
	}
}

bool IndexadorHash::Existe(const string& word) const 
{
	return (this->indice.find(word) != this->indice.end());
}	 

bool IndexadorHash::BorraDoc(const string& nomDoc) 
{
	list<string> terms_to_delete;

	auto doc = this->indiceDocs.find(nomDoc);

	if( doc != this->indiceDocs.end() ) 
	{
		for( auto& term : this->indice )
		{
			// si estamos en su l_docs
			if( (term.second.l_docs.find(doc->second.idDoc) != term.second.l_docs.end()) )
			{
				// si somos los unicos -> borrarlo
				if( term.second.l_docs.size() == 1 ) {
					
					//this->indice.erase(term.first);
					terms_to_delete.push_back(term.first);
					this->informacionColeccionDocs.numTotalPalDiferentes--;
				}
				
				// si no -> actualizar sus campos
				else {
					term.second.clearDoc(doc->second.idDoc);
				}
			}
		}

		// Actualizar campos de la colección
		this->informacionColeccionDocs -= indiceDocs[nomDoc];

		// Borrar el documento
		this->indiceDocs.erase(nomDoc);

		for( const auto& term : terms_to_delete )
			this->indice.erase(term);

		return true;
	}

	return false;
} 

void IndexadorHash::VaciarIndiceDocs() 	{ this->indiceDocs.clear(); } 

void IndexadorHash::VaciarIndicePreg() 	{ this->indicePregunta.clear(); this->infPregunta.clear(); } 

int IndexadorHash::NumPalIndexadas() const 	{ return this->indice.size(); } 

string IndexadorHash::DevolverFichPalParada () const { return this->ficheroStopWords; } 

void IndexadorHash::ListarPalParada() const 
{
	for( const string& s : this->stopWords ) { // TODO
		cout << s << endl;
	}
} 

int IndexadorHash::NumPalParada() const	{ return this->stopWords.size(); } 

string IndexadorHash::DevolverDelimitadores () const { return this->tok.DelimitadoresPalabra(); } 

bool IndexadorHash::DevolverCasosEspeciales () const { return this->tok.CasosEspeciales(); }

bool IndexadorHash::DevolverPasarAminuscSinAcentos () const { return this->tok.PasarAminuscSinAcentos(); }

bool IndexadorHash::DevolverAlmacenarPosTerm () const { return this->almacenarPosTerm; }

string IndexadorHash::DevolverDirIndice () const { return this->directorioIndice; } 

int IndexadorHash::DevolverTipoStemming () const { return this->tipoStemmer; } 

void IndexadorHash::ListarInfColeccDocs() const { cout << this->informacionColeccionDocs << endl; } 

void IndexadorHash::ListarTerminos() const 
{
	for( const auto& c : this->indice ) 
		cout << c.first << '\t' << c.second << endl;
} 

bool IndexadorHash::ListarTerminos(const string& nomDoc) const 
{
	unordered_map<std::string,InfDoc>::const_iterator doc = this->indiceDocs.find(nomDoc);

	if( doc != this->indiceDocs.end() ) 
	{
		for( const auto& term : this->indice )
		{
			if( term.second.l_docs.find(doc->second.idDoc) != term.second.l_docs.end() )
			{
				cout << term.first << '\t' << term.second << endl;
			}
		} 
		return true;
	}
	return false;
} 

void IndexadorHash::ListarDocs() const 
{
	for( const auto& d : this->indiceDocs ) 
		cout << d.first << '\t' << d.second << endl;
} 

bool IndexadorHash::ListarDocs(const string& nomDoc) const 
{
	unordered_map<std::string,InfDoc>::const_iterator doc = this->indiceDocs.find(nomDoc);

	if( doc != this->indiceDocs.end() ) {
		cout << nomDoc << '\t' << this->indiceDocs.at(nomDoc) << endl; 
		return true;
	}
	return false;
}

void IndexadorHash::clearDoc_fromIndice(const int doc) 
{
	for( auto& term : this->indice )
	{
		if( term.second.l_docs.find(doc) != term.second.l_docs.end() ) 
		{
			term.second.ftc -= term.second.l_docs.at(doc).ft;
			term.second.l_docs.erase(doc);
		}
	}
}