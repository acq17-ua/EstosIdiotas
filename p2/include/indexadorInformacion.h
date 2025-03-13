class InformacionTermino { 
    friend ostream& operator<<(ostream& s, const InformacionTermino& p);

	public:
		InformacionTermino (const InformacionTermino &);
		InformacionTermino ();		// Inicializa ftc = 0
		~InformacionTermino ();		// Pone ftc = 0 y vac�a l_docs
		InformacionTermino & operator= (const InformacionTermino &);

	private:
		int ftc;	// Frecuencia total del t�rmino en la colecci�n
		unordered_map<int, InfTermDoc> l_docs; 
		// Tabla Hash que se acceder� por el id del documento, devolviendo un objeto de la clase InfTermDoc que contiene toda la informaci�n de aparici�n del t�rmino en el documento
	};

ostream& operator<<(ostream& s, const InformacionTermino& p) {
    s << "Frecuencia total: " << p.ftc << "\tfd: " << p.l_docs.size();
    // A continuaci�n se mostrar�an todos los elementos de p.l_docs: s << "\tId.Doc: " << idDoc << "\t" << InfTermDoc;

    return s;
}

class InfTermDoc { 
    friend ostream& operator<<(ostream& s, const InfTermDoc& p);
	public:
		InfTermDoc (const InfTermDoc &);
		InfTermDoc ();		// Inicializa ft = 0
		~InfTermDoc ();		// Pone ft = 0 
		InfTermDoc & operator= (const InfTermDoc &);

		// A�adir cuantos m�todos se consideren necesarios para manejar la parte privada de la clase
	private:
		int ft;	// Frecuencia del t�rmino en el documento
		list<int> posTerm;	
		// Solo se almacenar� esta informaci�n si el campo privado del indexador almacenarPosTerm == true
		// Lista de n�meros de palabra en los que aparece el t�rmino en el documento. Los n�meros de palabra comenzar�n desde cero (la primera palabra del documento). Se numerar�n las palabras de parada. Estar� ordenada de menor a mayor posici�n. 
	};

ostream& operator<<(ostream& s, const InfTermDoc& p) {
    s << "ft: " << p.ft;
    // A continuaci�n se mostrar�an todos los elementos de p.posTerm ("posicion TAB posicion TAB ... posicion, es decir nunca finalizar� en un TAB"): s << "\t" << posicion;

    return s;
}

class InfDoc { 
    friend ostream& operator<<(ostream& s, const InfDoc& p);
	public:
		InfDoc (const InfDoc &);
		InfDoc ();	
		~InfDoc ();
		InfDoc & operator= (const InfDoc &);

		// A�adir cuantos m�todos se consideren necesarios para manejar la parte privada de la clase
	private:
		int idDoc;	
		// Identificador del documento. El primer documento indexado en la colecci�n ser� el identificador 1
		int numPal;	// N� total de palabras del documento
		int numPalSinParada;	// N� total de palabras sin stop-words del documento
		int numPalDiferentes;	
		// N� total de palabras diferentes que no sean stop-words (sin acumular la frecuencia de cada una de ellas)
		int tamBytes;	// Tama�o en bytes del documento
		Fecha fechaModificacion;
		// Atributo correspondiente a la fecha y hora (completa) de modificaci�n del documento. El tipo "Fecha/hora" lo elegir�/implementar� el alumno
};

ostream& operator<<(ostream& s, const InfDoc& p) {
    s << "idDoc: " << p.idDoc << "\tnumPal: " << p.numPal << "\tnumPalSinParada: " << p.numPalSinParada << "\tnumPalDiferentes: " << p.numPalDiferentes << "\ttamBytes: " << p.tamBytes;

    return s;
}

class InfColeccionDocs { 
    friend ostream& operator<<(ostream& s, const InfColeccionDocs& p);
	public:
		InfColeccionDocs (const InfColeccionDocs &);
		InfColeccionDocs ();
		~InfColeccionDocs ();
		InfColeccionDocs & operator= (const InfColeccionDocs &);

		// A�adir cuantos m�todos se consideren necesarios para manejar la parte privada de la clase
	private:
		int numDocs;	// N� total de documentos en la colecci�n
		int numTotalPal;	
		// N� total de palabras en la colecci�n 
		int numTotalPalSinParada;
		// N� total de palabras sin stop-words en la colecci�n 
		int numTotalPalDiferentes;	
		// N� total de palabras diferentes en la colecci�n que no sean stop-words (sin acumular la frecuencia de cada una de ellas)
		int tamBytes;	// Tama�o total en bytes de la colecci�n
};

ostream& operator<<(ostream&, const InfColeccionDocs&);
    s << "numDocs: " << p.numDocs << "\tnumTotalPal: " << p.numTotalPal << "\tnumTotalPalSinParada: " << p.numTotalPalSinParada << "\tnumTotalPalDiferentes: " << numTotalPalDiferentes << "\ttamBytes: " << p.tamBytes;

    return s;
}

class InformacionTerminoPregunta { 
    friend ostream& operator<<(ostream& s, const InformacionTerminoPregunta& p);
	public:
		InformacionTerminoPregunta (const InformacionTerminoPregunta &);
		InformacionTerminoPregunta ();
		~InformacionTerminoPregunta ();
		InformacionTerminoPregunta & operator= (const InformacionTerminoPregunta &);

		// A�adir cuantos m�todos se consideren necesarios para manejar la parte privada de la clase
	private:
		int ft;	// Frecuencia total del t�rmino en la pregunta
		list<int> posTerm;	
		// Solo se almacenar� esta informaci�n si el campo privado del indexador almacenarPosTerm == true
		// Lista de n�meros de palabra en los que aparece el t�rmino en la pregunta. Los n�meros de palabra comenzar�n desde cero (la primera palabra de la pregunta). Se numerar�n las palabras de parada. Estar� ordenada de menor a mayor posici�n.
};

ostream& operator<<(ostream& s, const InformacionTerminoPregunta& p) {
    s << "ft: " << p.ft;
    // A continuaci�n se mostrar�an todos los elementos de p.posTerm ("posicion TAB posicion TAB ... posicion, es decir nunca finalizar� en un TAB"): s << "\t" << posicion;

    return s;
}

class InformacionPregunta { 
    friend ostream& operator<<(ostream& s, const InformacionPregunta& p);
	public:
		InformacionPregunta (const InformacionPregunta &);
		InformacionPregunta ();	
		~InformacionPregunta ();
		InformacionPregunta & operator= (const InformacionPregunta &);

		// A�adir cuantos m�todos se consideren necesarios para manejar la parte privada de la clase
	private:
		int numTotalPal;	
		// N� total de palabras en la pregunta
		int numTotalPalSinParada;
		// N� total de palabras sin stop-words en la pregunta
		int numTotalPalDiferentes;	
		// N� total de palabras diferentes en la pregunta que no sean stop-words (sin acumular la frecuencia de cada una de ellas)
};

ostream& operator<<(ostream&, const InformacionPregunta&);
    s << "numTotalPal: " << p.numTotalPal << "\tnumTotalPalSinParada: "<< p.numTotalPalSinParada << "\tnumTotalPalDiferentes: " << numTotalPalDiferentes;

    return s;
}
