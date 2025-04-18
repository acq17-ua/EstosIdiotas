#include "indexadorInformacion.h"

InfTermDoc::InfTermDoc () 
{
	this->ft = 0;
	this->posTerm = list<int>();
}	

InfTermDoc::InfTermDoc (const InfTermDoc& o) 
{
	this->ft = o.ft;
	this->posTerm = o.posTerm;
}

InfTermDoc::~InfTermDoc ()
{
	/* cout << "Inftermdoc destructor" << endl;
	this->ft = 0;
	this->posTerm.clear();
	cout << "done" << endl; */
}

InfTermDoc& InfTermDoc::operator= (const InfTermDoc& o) 
{
	this->ft = o.ft;
	this->posTerm = list<int>(o.posTerm);
	return *this;
}

void InfTermDoc::clear()
{
	this->ft = 0;
	this->posTerm.clear();
}

InformacionTermino::InformacionTermino() 
{
	this->ftc = 0;
	this->l_docs = unordered_map<int,InfTermDoc>();
}	

InformacionTermino::InformacionTermino(const InformacionTermino& o) 
{
	this->ftc = o.ftc;
	this->l_docs = o.l_docs;
}

InformacionTermino::~InformacionTermino() 
{
	/* this->ftc = 0;
	cout << "Informaciontermino destructor" << endl;
	for( auto& l_doc : this->l_docs ) {
		l_doc.second.ft=0;
		l_doc.second.posTerm.clear();
	}
	this->l_docs.clear();
	cout << "done" << endl; */
}

InformacionTermino& InformacionTermino::operator=(const InformacionTermino& o) 
{
	this->ftc = o.ftc;
	this->l_docs = o.l_docs;
	return *this;
}

void InformacionTermino::clear() 
{
	this->ftc = 0;
	this->l_docs.clear();
}	

bool InformacionTermino::doc(const int id, InfTermDoc inf) const 
{
	unordered_map<int,InfTermDoc>::const_iterator doc = this->l_docs.find(id);
	
	if( doc != this->l_docs.end() ) {
		inf = doc->second;
		return true;
	}
	return false;
}

bool InformacionTermino::clearDoc(const int id) 
{
	unordered_map<int,InfTermDoc>::const_iterator doc = this->l_docs.find(id);

	if( doc != this->l_docs.end() ) {
		this->ftc -= this->l_docs[id].get_ft();
		return (this->l_docs.erase(id) != 0);
	}
	return false;
}

InfDoc::InfDoc() 
{
	idDoc = InfDoc::nextId++;
	numPal = numPalSinParada = numPalDiferentes = tamBytes = 0;
	fechaModificacion = { 0, 0, 0, 0, 0, 0 };
}	

InfDoc::InfDoc(const InfDoc& o) 
{
	this->idDoc = o.idDoc;
	this->numPal = o.numPal;
	this->numPalSinParada = o.numPalSinParada;
	this->numPalDiferentes = o.numPalDiferentes;
	this->tamBytes = o.tamBytes;
	this->fechaModificacion = o.fechaModificacion; 
}

InfDoc::~InfDoc() 
{
	/* cout << "infdoc destructor" << endl;
	idDoc = 0;
	numPal = numPalSinParada = numPalDiferentes = tamBytes = 0;
	fechaModificacion = { 0, 0, 0, 0, 0, 0 };
	cout << "done" << endl; */
}

InfDoc& InfDoc::operator=(const InfDoc& o) 
{
	this->idDoc = o.idDoc;
	this->numPal = o.numPal;
	this->numPalSinParada = o.numPalSinParada;
	this->numPalDiferentes = o.numPalDiferentes;
	this->tamBytes = o.tamBytes;
	this->fechaModificacion = o.fechaModificacion; 
	
	return *this;
}

void InfDoc::clear() 
{
	numPal = numPalSinParada = numPalDiferentes = tamBytes = 0;
	fechaModificacion = { 0, 0, 0, 0, 0, 0 };
}

InfColeccionDocs::InfColeccionDocs () 
{
	this->numDocs = 
	this->numTotalPal = 
	this->numTotalPalSinParada = 
	this->numTotalPalDiferentes = 
	this->tamBytes = 0;
}

InfColeccionDocs::InfColeccionDocs (const InfColeccionDocs& o) 
{
	this->numDocs = o.numDocs;
	this->numTotalPal = o.numTotalPal;
	this->numTotalPalSinParada = o.numTotalPalSinParada;
	this->numTotalPalDiferentes = o.numTotalPalDiferentes;
	this->tamBytes = o.tamBytes;
}

InfColeccionDocs::~InfColeccionDocs () 
{
	/* cout << "infdoldocs destructor" << endl;
	this->numDocs = 
	this->numTotalPal = 
	this->numTotalPalSinParada = 
	this->numTotalPalDiferentes = 
	this->tamBytes = 0;
	cout << "done\n"; */
}

InfColeccionDocs& InfColeccionDocs::operator= (const InfColeccionDocs& o) 
{
	this->numDocs = o.numDocs;
	this->numTotalPal = o.numTotalPal;
	this->numTotalPalSinParada = o.numTotalPalSinParada;
	this->numTotalPalDiferentes = o.numTotalPalDiferentes;
	this->tamBytes = o.tamBytes;

	return *this;
}

void InfColeccionDocs::clear () 
{
	this->numDocs = 
	this->numTotalPal = 
	this->numTotalPalSinParada = 
	this->numTotalPalDiferentes = 
	this->tamBytes = 0;
}

InfColeccionDocs& InfColeccionDocs::operator+=(const InfDoc& doc) 
{
	this->numDocs++;
	this->numTotalPal += doc.numPal;
	this->numTotalPalSinParada += doc.numPalSinParada;
	//this->numTotalPalDiferentes += doc.numPalDiferentes; // se actualiza al indexar un término
	this->tamBytes += doc.tamBytes;

	return *this;
}

InfColeccionDocs& InfColeccionDocs::operator-=(const InfDoc& doc) 
{
	this->numDocs--;
	this->numTotalPal -= doc.numPal;
	this->numTotalPalSinParada -= doc.numPalSinParada;
	//this->numTotalPalDiferentes -= doc.numPalDiferentes;
	this->tamBytes -= doc.tamBytes;

	return *this;
}

InformacionTerminoPregunta::InformacionTerminoPregunta() 
{
	this->ft = 0;
	this->posTerm = list<int>();
}

InformacionTerminoPregunta::InformacionTerminoPregunta(const InformacionTerminoPregunta& o) 
{
	this->ft = o.ft;
	this->posTerm = o.posTerm;	
}

InformacionTerminoPregunta::~InformacionTerminoPregunta() 
{
	/* cout << "inftermquery destructor" << endl;
	this->ft = 0;
	this->posTerm.clear();
	cout << "done\n"; */
}

InformacionTerminoPregunta& InformacionTerminoPregunta::operator=(const InformacionTerminoPregunta& o) 
{
	this->ft = o.ft;
	this->posTerm = o.posTerm;
	return *this;
}

void InformacionTerminoPregunta::clear() 
{
	this->ft = 0;
	this->posTerm.clear();
}

InformacionPregunta::InformacionPregunta() 
{
	this->numTotalPal = 
	this->numTotalPalSinParada = 
	this->numTotalPalDiferentes = 0;
}	

InformacionPregunta::InformacionPregunta(const InformacionPregunta& o) 
{
	this->numTotalPal = o.numTotalPal;
	this->numTotalPalSinParada = o.numTotalPalSinParada;
	this->numTotalPalDiferentes = o.numTotalPalDiferentes;
}

InformacionPregunta::~InformacionPregunta() 
{
/* 	cout << "infquery destructor" << endl;
	this->numTotalPal = 
	this->numTotalPalSinParada = 
	this->numTotalPalDiferentes = 0;
	cout << "done\n"; */
}

InformacionPregunta& InformacionPregunta::operator=(const InformacionPregunta& o) 
{
	this->numTotalPal = o.numTotalPal;
	this->numTotalPalSinParada = o.numTotalPalSinParada;
	this->numTotalPalDiferentes = o.numTotalPalDiferentes;

	return *this;
}

void InformacionPregunta::clear() 
{
	this->numTotalPal = 
	this->numTotalPalSinParada = 
	this->numTotalPalDiferentes = 0;
}