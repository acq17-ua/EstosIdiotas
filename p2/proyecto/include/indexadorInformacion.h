using namespace std;
#include <iostream>
#include <unordered_map>
#include <list>
#include <time.h>

// Fecha
/*
bool operator==(const tm& a, const tm& b) {

	return 	a.tm_year 	== b.tm_year,
			a.tm_mon 	== b.tm_mon,
			a.tm_mday 	== b.tm_mday,
			a.tm_hour 	== b.tm_hour,
			a.tm_min 	== b.tm_min,
			a.tm_sec	== b.tm_sec;
}*/

#ifndef _INF_TERM_DOC_
#define _INF_TERM_DOC_

// Información sobre un término concreto respecto a un documento concreto
// Solo se usará si indexadorHash.almacenarPosTerm == true
class InfTermDoc { 
    
	friend class IndexadorHash;
	friend class InformacionTermino;

	friend ostream& operator<<(ostream& s, const InfTermDoc& p) {
		s << "ft: " << p.ft;
		
		for( const auto& posterm : p.posTerm )
		{
			cout << "\t" << posterm;
		}
	
		return s;
	}
	
	public:
		InfTermDoc (const InfTermDoc &);
		InfTermDoc ();		// Inicializa ft = 0
		~InfTermDoc ();		// Pone ft = 0 
		InfTermDoc & operator= (const InfTermDoc &);

		void clear();
		int get_ft() const { return ft; }

	private:
		int ft;	
		list<int> posTerm;		
		// Posiciones de palabra en los que aparece el término en el documento (0 en adelante)
		// (Las palabras de parada también se numeran, ordenadas de menor a mayor posición)
};

#endif

#ifndef _INFORMACION_TERMINO_
#define _INFORMACION_TERMINO_

// Información sobre un término concreto en una colección de documentos
class InformacionTermino { 
    
	friend class IndexadorHash;

	friend ostream& operator<<(ostream& s, const InformacionTermino& p) {
		s 	<< "Frecuencia total: " << p.ftc 
			<< "\tfd: " << p.l_docs.size();
		
		for( const auto& l_doc : p.l_docs )
		{
			s << "\tId.Doc: " << l_doc.first << "\t" << l_doc.second;
		}
	
		return s;
	}
	
	public:
		InformacionTermino (const InformacionTermino &);
		InformacionTermino ();		// Inicializa ftc = 0
		~InformacionTermino ();		// Pone ftc = 0 y vacía l_docs
		InformacionTermino & operator= (const InformacionTermino &);

		void clear();
		bool doc(const int id, InfTermDoc inf) const;
		bool clearDoc(const int id);

	private:
		int ftc;	// Frecuencia total del término en la colección
		unordered_map<int, InfTermDoc> l_docs;
		// hash <doc_id:documento-termino> : Información del término en cada documento
};

#endif

#ifndef _INF_DOC_
#define _INF_DOC_

// Información de un documento
class InfDoc { 

	friend class IndexadorHash;
	friend class InfColeccionDocs;

	friend ostream& operator<<(ostream& s, const InfDoc& p) {
		s 	<< "idDoc: " << p.idDoc 
			<< "\tnumPal: " << p.numPal 
			<< "\tnumPalSinParada: " << p.numPalSinParada 
			<< "\tnumPalDiferentes: " << p.numPalDiferentes 
			<< "\ttamBytes: " << p.tamBytes;
		return s;
	}
	
	public:
		InfDoc (const InfDoc &);
		InfDoc ();	
		~InfDoc ();
		InfDoc & operator= (const InfDoc &);

		void clear();
		int get_idDoc() const { return this->idDoc; }

	private:
		static inline int nextId = 1;
		int idDoc;
		int numPal;
		int numPalSinParada;
		int numPalDiferentes;
		int tamBytes;		
		tm fechaModificacion; 
};

#endif

#ifndef _INF_COLECCION_DOCS_
#define _INF_COLECCION_DOCS_

// Información sobre una colección de documentos
class InfColeccionDocs { 
	
	friend class IndexadorHash;
	
	friend ostream& operator<<(ostream& s, const InfColeccionDocs& p) {
		s 	<< "numDocs: " << p.numDocs 
			<< "\tnumTotalPal: " << p.numTotalPal 
			<< "\tnumTotalPalSinParada: " << p.numTotalPalSinParada 
			<< "\tnumTotalPalDiferentes: " << p.numTotalPalDiferentes 
			<< "\ttamBytes: " << p.tamBytes;
		return s;
	}
	
	public:
		InfColeccionDocs (const InfColeccionDocs&);
		InfColeccionDocs ();
		~InfColeccionDocs ();
		InfColeccionDocs& operator= (const InfColeccionDocs&);
		InfColeccionDocs& operator+= (const InfDoc&);
		InfColeccionDocs& operator-= (const InfDoc&);

		void clear();

		private:
		int numDocs;		
		int numTotalPal;	
		int numTotalPalSinParada;
		int numTotalPalDiferentes;	
		int tamBytes;	
};

#endif

#ifndef _INFORMACION_TERMINO_PREGUNTA_
#define _INFORMACION_TERMINO_PREGUNTA_

// Información sobre un término concreto en una query
// Solo usada si indexadorHash.almacenarPosTerm == true
class InformacionTerminoPregunta { 

	friend class IndexadorHash;

	friend ostream& operator<<(ostream& s, const InformacionTerminoPregunta& p) {
		s << "ft: " << p.ft;
		for( const auto& posterm : p.posTerm )
		{
			cout << "\t" << posterm;
		}
		return s;
	}

	public:
		InformacionTerminoPregunta (const InformacionTerminoPregunta &);
		InformacionTerminoPregunta ();
		~InformacionTerminoPregunta ();
		InformacionTerminoPregunta & operator= (const InformacionTerminoPregunta &);

		void clear();

	private:
		int ft;
		list<int> posTerm;
};

#endif

#ifndef _INFORMACION_PREGUNTA_
#define _INFORMACION_PREGUNTA_

class InformacionPregunta { 
    
	friend class IndexadorHash;

	friend ostream& operator<<(ostream& s, const InformacionPregunta& p) {
		s 	<< "numTotalPal: " << p.numTotalPal 
			<< "\tnumTotalPalSinParada: " << p.numTotalPalSinParada 
			<< "\tnumTotalPalDiferentes: " << p.numTotalPalDiferentes;
		return s;
	}
	
	public:
		InformacionPregunta (const InformacionPregunta &);
		InformacionPregunta ();	
		~InformacionPregunta ();
		InformacionPregunta & operator= (const InformacionPregunta &);

		void clear();

	private:
		int numTotalPal;
		int numTotalPalSinParada;
		int numTotalPalDiferentes;	
};

#endif