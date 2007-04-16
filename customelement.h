#ifndef ELEMENTPERSO_H
#define ELEMENTPERSO_H
#include "fixedelement.h"
#include <QtGui>
#include "nameslist.h"
/**
	Cette classe represente un element electrique. Elle est utilisable
	comme un element fixe. La difference est que l'element perso lit
	sa description (noms, dessin, comportement) dans un fichier XML a fournir
	en parametre.
*/
class CustomElement : public FixedElement {
	// constructeurs, destructeur
	public:
	CustomElement(QString &, QGraphicsItem * = 0, Diagram * = 0, int * = NULL);
	virtual ~CustomElement();
	
	private:
	CustomElement(const CustomElement &);
	
	// attributs
	private:
	int elmt_etat; // contient le code d'erreur si l'instanciation a echoue ou 0 si l'instanciation s'est bien passe
	NamesList names;
	QString nomfichier;
	QPicture dessin;
	int nb_terminals;
	
	// methodes
	public:
	virtual int nbTerminals() const;
	virtual void paint(QPainter *, const QStyleOptionGraphicsItem *);
	QString typeId() const;
	QString fichier() const;
	bool isNull() const;
	int etat() const;
	QString nom() const;
	
	private:
	bool parseElement(QDomElement &, QPainter &, Diagram *);
	bool parseLine(QDomElement &, QPainter &);
	bool parseEllipse(QDomElement &, QPainter &);
	bool parseCircle(QDomElement &, QPainter &);
	bool parseArc(QDomElement &, QPainter &);
	bool parsePolygon(QDomElement &, QPainter &);
	bool parseText(QDomElement &, QPainter &);
	bool parseInput(QDomElement &, Diagram *);
	bool parseTerminal(QDomElement &, Diagram *);
	void setQPainterAntiAliasing(QPainter &, bool);
	bool attributeIsAnInteger(QDomElement &, QString, int * = NULL);
	bool attributeIsAReal(QDomElement &, QString, double * = NULL);
	bool validOrientationAttribute(QDomElement &);
	void setPainterStyle(QDomElement &, QPainter &);
};

/**
	@return L'ID du type de l'element ; pour un CustomElement, cela revient au
	nom du fichier
	@see fichier()
*/
inline QString CustomElement::typeId() const {
	return(nomfichier);
}

/**
	@return L'adresse du fichier contenant la description XML de cet element
*/
inline QString CustomElement::fichier() const {
	return(nomfichier);
}

/**
	@return true si cet element est nul, c'est-a-dire si le chargement de sa
	description XML a echoue
*/
inline bool CustomElement::isNull() const {
	return(elmt_etat != 0);
}

/**
	@return Un entier representant l'etat de l'element :
		- 0 : L'instanciation a reussi
		- 1 : Le fichier n'existe pas
		- 2 : Le fichier n'a pu etre ouvert
		- 3 : Le fichier n'est pas un document XML
		- 4 : Le document XML n'a pas une "definition" comme racine
		- 5 : Les attributs de la definition ne sont pas presents et / ou valides
		- 6 : La definition est vide
		- 7 : L'analyse d'un element XML decrivant une partie du dessin de l'element a echoue
		- 8 : Aucune partie du dessin n'a pu etre chargee
*/
inline int CustomElement::etat() const {
	return(elmt_etat);
}

/**
	@return Le nom de l'element
*/
inline QString CustomElement::nom() const {
	return(names.name(QFileInfo(nomfichier).baseName()));
}

#endif
