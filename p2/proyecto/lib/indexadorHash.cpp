#include "indexadorHash.h"
#include <sys/stat.h> 		// para metadatos de archivos
#include <sys/mman.h> 		// para memory mapping
#include <fcntl.h> 			// para acceso a archivos
#include <unistd.h> 		// set size of output files

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

	if( stat(fichStopWords.c_str(), &fileInfo) == 0 ) {
		
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
		int it=0;
		string stopword="";
		for ( it = 0; it<fileInfo.st_size; it++ ) {

			if( map[it]=='\n' ) {
				
				if ( !stopword.empty() ) {
					stopwords.insert(stopword);
					stopword.clear();
				}	
			}
			else
				stopword += map[it];
		}
		munmap(map, fileInfo.st_size);
		close(fd);
		return true;
	}
	return false;
}

// privado
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
	
	// por defecto no hay stopwords -- my choice
	this->ficheroStopWords = "";
	this->stopWords = unordered_set<string>();
	
	this->directorioIndice = "";
	this->tipoStemmer = 0;
	this->almacenarPosTerm = false;	
}	

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
	this->ficheroStopWords 			= ficheroStopWords;

	unordered_set<string> stopwords;
	if (procesarStopWords(this->ficheroStopWords, stopwords)) 
		this->stopWords = stopwords;
	else
		this->stopWords = unordered_set<string>();
	
	this->directorioIndice 	= dirIndice;
	this->tipoStemmer 		= tStemmer;
	this->almacenarPosTerm 	= almPosTerm;	
}

IndexadorHash::IndexadorHash(const string& directorioIndexacion) 
{
	// TODO atado a guardarIndexacion(), leer e inicializar
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
	this->tok.~Tokenizador();
	this->indice.~unordered_map(); 				
	this->indiceDocs.~unordered_map(); 				
	this->informacionColeccionDocs.~InfColeccionDocs();
	this->pregunta = "";
	this->indicePregunta.~unordered_map();
	this->infPregunta.~InformacionPregunta();
	this->ficheroStopWords = "";
	this->stopWords.~unordered_set();
	this->directorioIndice = "";
	this->tipoStemmer = 0;
	this->almacenarPosTerm = false;	
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

bool IndexadorHash::Indexar(const string& ficheroDocumentos) 
{

}

bool IndexadorHash::IndexarDirectorio(const string& dirAIndexar) 
{

}

bool IndexadorHash::GuardarIndexacion() const 
{

}

bool IndexadorHash::RecuperarIndexacion (const string& directorioIndexacion) 
{

}

bool IndexadorHash::IndexarPregunta(const string& preg) 
{
	// TODO
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
	if( this->tok.PasarAminuscSinAcentos() ) {
		string new_word="";
		
		for( const unsigned char c : word )
			new_word += conversion[c];

		if( this->indice.find(new_word) != this->indice.end() ) {
			inf = this->indice.at(new_word);
			return true;
		}
		inf.clear();
		return false;
	}
	if( this->indice.find(word) != this->indice.end() ) {
		inf = this->indice.at(word);
		return true;
	}
	inf.clear();
	return false;
} 

bool IndexadorHash::Devuelve(const string& word, const string& nomDoc, InfTermDoc& infDoc) const 
{
	if( this->tok.PasarAminuscSinAcentos() ) {
		string new_word="";
		
		for( const unsigned char c : word )
			new_word += conversion[c];

		unordered_map<std::string,InfDoc>::const_iterator doc = this->indiceDocs.find(nomDoc);
			
		// el documento no está indexado
		if( doc == this->indiceDocs.end() )  	return false;
		
		unordered_map<std::string,InformacionTermino>::const_iterator term = this->indice.find(new_word);
		// la palabra no está indexada
		if( term == this->indice.end() ) 		return false;

		InfTermDoc inf;
		
		if(term->second.doc(doc->second.get_idDoc(), inf)) {
			infDoc = inf;
			return true;
		}
		else {
			infDoc.clear();
			return false;
		}
	}
	else {

		unordered_map<std::string,InfDoc>::const_iterator doc = this->indiceDocs.find(nomDoc);
			
		// el documento no está indexado
		if( doc == this->indiceDocs.end() ) 	return false;
		
		unordered_map<std::string,InformacionTermino>::const_iterator term = this->indice.find(word);
		// la palabra no está indexada
		if( term == this->indice.end() )  		return false;

		InfTermDoc inf;
		
		if(term->second.doc(doc->second.get_idDoc(), inf)) {
			infDoc = inf;
			return true;
		}
		else {
			infDoc.clear();
			return false;
		}
	}
}

bool IndexadorHash::Existe(const string& word) const 
{
	return (this->indice.find(word) != this->indice.end());
}	 

bool IndexadorHash::BorraDoc(const string& nomDoc) 
{
	unordered_map<std::string,InfDoc>::const_iterator doc = this->indiceDocs.find(nomDoc);

	if( doc != this->indiceDocs.end() ) {

		// a cada término
		for( auto t : this->indice ) {   // pair<string,InformacionTermino>
			
			// actualizar ftc
			// borrar entrada del doc en el término (si la hay)
			t.second.clearDoc(doc->second.get_idDoc());
		}

		// Actualizar colección de documentos
		this->informacionColeccionDocs.clearDoc(indiceDocs[nomDoc]);
		
		// Borrar el documento
		this->indiceDocs.erase(nomDoc);

		return true;
	}
	return false;
} 

void IndexadorHash::VaciarIndiceDocs() 	{ this->indiceDocs.clear(); } 

void IndexadorHash::VaciarIndicePreg() 	{ this->indicePregunta.clear(); } 

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

	if( doc != this->indiceDocs.end() ) {
		cout << nomDoc << '\t' << this->indiceDocs.at(nomDoc) << endl; // deberia printear terminos, no info
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

} 


