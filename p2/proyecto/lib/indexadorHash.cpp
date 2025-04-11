#include "indexadorHash.h"
#include <sys/stat.h> 		// para metadatos de archivos
#include <sys/mman.h> 		// para memory mapping
#include <fcntl.h> 			// para acceso a archivos
#include <unistd.h> 		// set size of output files
#include <time.h>			// last modified date for files

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
	this->stem				= stemmerPorter(); // TODO tipo?
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

IndexadorHash::~IndexadorHash() {}

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

void write_int_into_mmap(unsigned char* const map, int number, int& i) 
{
	map[i++] = (number < 255)*number + (255 <= number)*255;
	number -= map[0];
	while( number > 0 ) {
		map[i++] = number;
		number -= 255;
	}
	map[i++]=0;
}

/*
	Writes natural numbers into a memory mapped file by dividing it into 255-max pieces so that they will fit into each element of the mmap.
	Reading will have to involve adding up the character values until a 0 is found.
	Separates with character '\0'.

*/
bool write_map( const string& path, list<int>contents ) 
{
	int fd;
	unsigned char* map;
	struct stat fileInfo;
	const unsigned estimated_size = 2*contents.size()*sizeof(unsigned char);
	unsigned memory_needed;
	int i=0;

	// create path with parents
	system(( "install -D /dev/null " + path).c_str());
	fd = open(path.c_str(), O_CREAT | O_RDWR, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);

	if( fd==-1 ) {
		cerr << "Failed to create file (errno: " << errno << ")\n";
		return false;
	}

	if( stat(path.c_str(), &fileInfo) == 0 ) {
				
		if(fd == -1) {
			cerr << "Failed to create file\n";
			return false;
		}

		memory_needed = PAGE_SIZE*(estimated_size/PAGE_SIZE + ((estimated_size%PAGE_SIZE)>0)); 
		posix_fallocate(fd, 0, memory_needed);

		//cout << "initial size: " << fileInfo.st_size << " ; memory_needed: " << memory_needed << endl;
		map = (unsigned char*)mmap(0, memory_needed, PROT_WRITE, MAP_SHARED, fd, 0);
		
		int line_counter = 0;
		
		if( map == MAP_FAILED ) {
			cerr << "Failed to create file\n";
			return false;
		}

		// actual writing
		for( int& s : contents ) {
			map[i++] = (s < 255)*s + (255 <= s)*255;
			s -= map[0];
			while( s > 0 ) {
				map[i++] = s;
				s -= 255;
			}
			map[i++]=0;
		}
		msync(map, i*sizeof(unsigned char), MS_ASYNC);
		munmap(map, i*sizeof(unsigned char));
		ftruncate(fd, i*sizeof(unsigned char));
		close(fd);
	}
	else {
		cerr << "Failed to create file\n";
		return false;
	}
	contents.clear();
	return true;
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

		fd = open(ficheroDocumentos.c_str(), O_RDONLY);
		
		if( stat(ficheroDocumentos.c_str(), &fileInfo) == 0 ) {
			
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

				// a .tk file (a document)
				if( map[it]=='\n' ) 
				{
					cout << "*** Indexando documento " << file << endl;

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
							this->indiceDocs.at(file).clear();
							this->clearDoc_fromIndice(doc->second.idDoc);
							
							// has been modified since -> DON'T keep idDoc
							if( !(doc->second.fechaModificacion == *gmtime(&fileInfoChild.st_mtime)) ) 
							this->indiceDocs.at(file).idDoc = InfDoc::nextId++;
						}

						pos = 0;
						for( int it_tk = 0; it_tk<fileInfoChild.st_size; it_tk++ ) {
							
							// a term
							if( map_tk[it_tk]=='\n' ) {

								if( term.size() <= 0 ) continue;

								cout << "*** Indexando término " << term << endl;
							
								pos++; // for posTerm

								this->stem.stemmer(term, this->tipoStemmer);

								unordered_map<string,InformacionTermino>::const_iterator inf_term = this->indice.find(term);
								
								// actualizar InfDoc
								this->indiceDocs[file].numPal++;
								this->indiceDocs[file].numPalSinParada += (this->stopWords.find(term) == this->stopWords.end());
																			// si este término no está indexado at all || si el término no tiene el documento en l_docs
								this->indiceDocs[file].numPalDiferentes += ((inf_term == this->indice.end()) || (inf_term->second.l_docs.find(this->indiceDocs[file].idDoc) != inf_term->second.l_docs.end()));
								this->indiceDocs[file].tamBytes = fileInfoChild.st_size;
								
								// actualizar InformacionTermino::l_docs -> create new InfTermDoc, 
								//										 -> create new InformacionTermino if undefined
								this->indice[term];
								this->indice[term].l_docs[this->indiceDocs[file].idDoc].ft = 1;
								this->indice[term].l_docs[this->indiceDocs[file].idDoc].posTerm.push_back(pos);
								// actualizar InfColeccionDocs
								this->informacionColeccionDocs += this->indiceDocs[file];
								term = "";
							}
							else {
								term += map_tk[it_tk];
							}
						}
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
		return false;
	}
}

bool IndexadorHash::IndexarDirectorio(const string& dirAIndexar) 
{
	system(("find " + dirAIndexar + " ! -type d ! -name '*.tk' > ./indexar_directorio_res.txt").c_str());
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

/*
	Saves indexation from memory into 3 folders: id, i, iq
	· id -- Contains a file for each InfDoc in this->indiceDocs.
	· i -- Contains a folder for each InformacionTermino in this->indice. Each one contains a __ file with metadata and a file for each InfTermDoc.
	· iq -- Contains a file for each InformacionTerminoPregunta in this->indicePregunta, and also a __ file with metadata.

*/
bool IndexadorHash::GuardarIndexacion() const 
{
	try {
		string path1=this->directorioIndice, path2="", path3="";

		list<int> contents;

		// indiceDocs
		path1.append("/id/");
		
		cout << "saving indiceDocs on " << path1 << endl;
		
		for( const auto& doc : this->indiceDocs )
		{
			path2="";
			path2.append(path1).append(doc.first);

			contents.push_back(doc.second.idDoc);
			contents.push_back(doc.second.numPal);
			contents.push_back(doc.second.numPalSinParada);
			contents.push_back(doc.second.numPalDiferentes);
			contents.push_back(doc.second.tamBytes);
			contents.push_back(doc.second.fechaModificacion.tm_year);
			contents.push_back(doc.second.fechaModificacion.tm_mon);
			contents.push_back(doc.second.fechaModificacion.tm_yday);
			contents.push_back(doc.second.fechaModificacion.tm_hour);
			contents.push_back(doc.second.fechaModificacion.tm_min);
			contents.push_back(doc.second.fechaModificacion.tm_sec);

			if (!write_map(	path2, contents)) 
			{
				cerr << "Failed to create doc file\n";
				return false;
			}
		}

		// INDICE
		path1=this->directorioIndice, path2="";
		path1.append("/i/");
		for( const auto& term : this->indice ) 
		{
			cout << "--- on term " << term.first << " <-- len: " << term.first.size() << endl;
			path2="";
			path2.append(path1).append(term.first).append("/");
			cout << "second" << endl;
			cout << "path1: " << path1 << endl;
			cout << "path2: " << path2 << endl;
			
			// InformacionTermino
			path3="";
			path3.append(path2).append("__");

			contents.push_back(term.second.ftc);
			write_map(path3, contents);
			
			// InfTermDocs
			for( const auto& doc : term.second.l_docs ) 
			{
				path3="";
				path3.append(path2).append(to_string(doc.first));
				
				for( const auto& posterm : doc.second.posTerm ) 
					contents.push_back(posterm);
				
				write_map(path3, contents);
			}
		}

		// QUERY

		path1 = this->directorioIndice, path2="";
		path1.append("/iq/");

		// InformacionPregunta
		path2="";
		path2.append(path1).append("__");

		contents.push_back(this->infPregunta.numTotalPal);
		contents.push_back(this->infPregunta.numTotalPalDiferentes);
		contents.push_back(this->infPregunta.numTotalPalSinParada);

		write_map(path2, contents);


		for( const auto& qterm : this->indicePregunta ) 
		{
			// InformacionTerminoPregunta
			path2="";
			path2.append(path1).append(qterm.first);

			contents.push_back(qterm.second.ft);

			for( const auto& posterm : qterm.second.posTerm ) 
				contents.push_back(posterm);

			write_map(path2, contents);

		}
		return true;
	}
	catch( bad_alloc& ex ) {
		return false;
	}

}

bool IndexadorHash::RecuperarIndexacion (const string& directorioIndexacion)
{
	int number;
	int fd;
	unsigned char* map;
	struct stat fileInfo;

	string path1="";

	path1.append(directorioIndexacion).append("/id/");
	


	fd = open(path1.c_str(), O_RDONLY);

	if( stat(path1.c_str(), &fileInfo) == 0 ) {
		
		if(fd == -1) {
			cerr << "Failed to read file\n";
			return false;
		}
		
		//fileSize_input = fileInfo.st_size;
		//map_input = reinterpret_cast<unsigned char*>(mmap(0, fileSize_input, PROT_READ, MAP_SHARED, fd_input, 0));
		
		if( map == MAP_FAILED ) {
			cerr << "Failed to read file\n";
			return false;
		}
	}
	return false;
}

bool IndexadorHash::IndexarPregunta(const string& preg) 
{
	if( preg.size()==0 )
		return false;

	try {

		list<string> tokens;
		this->tok.Tokenizar(preg, tokens);
		this->indicePregunta.clear();
		short int i=0;

		for( string& term : tokens ) 
		{
			i++;
			this->stem.stemmer(term, this->tipoStemmer);
			// si es la primera ocurrencia, se crea aqui el objeto
			this->indicePregunta[term].ft++;
			this->indicePregunta[term].posTerm.push_back(i);
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
		if( doc == this->indiceDocs.end() )  	
			return false;
		
		unordered_map<std::string,InformacionTermino>::const_iterator term = this->indice.find(new_word);
		// la palabra no está indexada
		if( term == this->indice.end() ) 		
			return false;

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
		this->informacionColeccionDocs -= indiceDocs[nomDoc];
		
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

		for( const auto& t : this->indiceDocs.at(nomDoc).get_terms() )
			cout << t.first << '\t' << t.second << endl;

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