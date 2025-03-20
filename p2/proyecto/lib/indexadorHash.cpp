#include "indexadorHash.h"

IndexadorHash::IndexadorHash(	const string& fichStopWords,   
	const string& delimitadores,   
	const bool& detectComp, 		 
	const bool& minuscSinAcentos,  	
	const string& dirIndice, 		
	const int& tStemmer, 			
	const bool& almPosTerm) {} 		

IndexadorHash::IndexadorHash() {}	

IndexadorHash::IndexadorHash(const string& directorioIndexacion) {}

IndexadorHash::IndexadorHash(const IndexadorHash&) {}

IndexadorHash::~IndexadorHash() {}

IndexadorHash& IndexadorHash::operator= (const IndexadorHash&) {}

bool IndexadorHash::Indexar(const string& ficheroDocumentos) {}

bool IndexadorHash::IndexarDirectorio(const string& dirAIndexar) {}

bool IndexadorHash::GuardarIndexacion() const {}

bool IndexadorHash::RecuperarIndexacion (const string& directorioIndexacion) {}

bool IndexadorHash::IndexarPregunta(const string& preg) {}

bool IndexadorHash::DevuelvePregunta(string& preg) const {}

bool IndexadorHash::DevuelvePregunta(const string& word, InformacionTerminoPregunta& inf) const {} 

bool IndexadorHash::DevuelvePregunta(InformacionPregunta& inf) const {} 

bool IndexadorHash::Devuelve(const string& word, InformacionTermino& inf) const {} 

bool IndexadorHash::Devuelve(const string& word, const string& nomDoc, InfTermDoc& InfDoc) const {}  

bool IndexadorHash::Existe(const string& word) const {}	 

bool IndexadorHash::BorraDoc(const string& nomDoc) {} 

void IndexadorHash::VaciarIndiceDocs() {} 

void IndexadorHash::VaciarIndicePreg() {} 

int IndexadorHash::NumPalIndexadas() const {} 

string IndexadorHash::DevolverFichPalParada () const {} 

void IndexadorHash::ListarPalParada() const {} 

int IndexadorHash::NumPalParada() const {} 

string IndexadorHash::DevolverDelimitadores () const {} 

bool IndexadorHash::DevolverCasosEspeciales () const {}

bool IndexadorHash::DevolverPasarAminuscSinAcentos () const {}

bool IndexadorHash::DevolverAlmacenarPosTerm () const {}

string IndexadorHash::DevolverDirIndice () const {} 

int IndexadorHash::DevolverTipoStemming () const {} 

void IndexadorHash::ListarInfColeccDocs() const {} 

void IndexadorHash::ListarTerminos() const {} 

bool IndexadorHash::ListarTerminos(const string& nomDoc) const {} 

void IndexadorHash::ListarDocs() const {} 

bool IndexadorHash::ListarDocs(const string& nomDoc) const {} 


