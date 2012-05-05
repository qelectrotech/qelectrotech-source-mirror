/*
	Copyright 2006-2012 Xavier Guerrin
	This file is part of QElectroTech.
	
	QElectroTech is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 2 of the License, or
	(at your option) any later version.
	
	QElectroTech is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.
	
	You should have received a copy of the GNU General Public License
	along with QElectroTech.  If not, see <http://www.gnu.org/licenses/>.
*/
#ifndef CUSTOM_ELEMENT_H
#define CUSTOM_ELEMENT_H
#include "fixedelement.h"
#include <QtGui>
#include "nameslist.h"
#include "elementslocation.h"
class ElementTextItem;
class Terminal;
/**
	Cette classe represente un element electrique. Elle est utilisable
	comme un element fixe. La difference est que l'element perso lit
	sa description (noms, dessin, comportement) dans un fichier XML a fournir
	en parametre.
*/
class CustomElement : public FixedElement {
	
	Q_OBJECT
	
	// constructeurs, destructeur
	public:
	CustomElement(const ElementsLocation &, QGraphicsItem * = 0, Diagram * = 0, int * = 0);
	CustomElement(const QDomElement &,      QGraphicsItem * = 0, Diagram * = 0, int * = 0);
	virtual ~CustomElement();
	
	private:
	CustomElement(const CustomElement &);
	
	// attributs
	protected:
	int elmt_state; // contient le code d'erreur si l'instanciation a echoue ou 0 si l'instanciation s'est bien passe
	NamesList names;
	ElementsLocation location_;
	QPicture drawing;
	QPicture low_zoom_drawing;
	QList<Terminal *> list_terminals;
	QList<ElementTextItem *> list_texts_;
	bool forbid_antialiasing;
	
	// methodes
	public:
	virtual QList<Terminal *> terminals() const;
	virtual QList<Conductor *> conductors() const;
	virtual QList<ElementTextItem *> texts() const;
	virtual int terminalsCount() const;
	virtual void paint(QPainter *, const QStyleOptionGraphicsItem *);
	QString typeId() const;
	ElementsLocation location() const;
	bool isNull() const;
	int state() const;
	QString name() const;
	
	protected:
	virtual bool buildFromXml(const QDomElement &, int * = 0);
	virtual bool parseElement(QDomElement &, QPainter &);
	virtual bool parseLine(QDomElement &, QPainter &);
	virtual bool parseRect(QDomElement &, QPainter &);
	virtual bool parseEllipse(QDomElement &, QPainter &);
	virtual bool parseCircle(QDomElement &, QPainter &);
	virtual bool parseArc(QDomElement &, QPainter &);
	virtual bool parsePolygon(QDomElement &, QPainter &);
	virtual bool parseText(QDomElement &, QPainter &);
	virtual ElementTextItem *parseInput(QDomElement &);
	virtual Terminal *parseTerminal(QDomElement &);
	virtual void setQPainterAntiAliasing(QPainter &, bool);
	virtual bool validOrientationAttribute(const QDomElement &);
	virtual void setPainterStyle(QDomElement &, QPainter &);
};

/**
	@return L'ID du type de l'element ; pour un CustomElement, cela revient au
	nom du fichier
	@see location()
*/
inline QString CustomElement::typeId() const {
	return(location_.path());
}

/**
	@return L'adresse du fichier contenant la description XML de cet element
*/
inline ElementsLocation CustomElement::location() const {
	return(location_);
}

/**
	@return true si cet element est nul, c'est-a-dire si le chargement de sa
	description XML a echoue
*/
inline bool CustomElement::isNull() const {
	return(elmt_state);
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
inline int CustomElement::state() const {
	return(elmt_state);
}

/**
	@return Le nom de l'element
*/
inline QString CustomElement::name() const {
	return(names.name(location_.baseName()));
}

#endif
