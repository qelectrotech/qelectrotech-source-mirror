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
#ifndef CUSTOM_ELEMENT_GRAPHIC_PART_H
#define CUSTOM_ELEMENT_GRAPHIC_PART_H
#include <QPainter>
#include "customelementpart.h"
#include "styleeditor.h"
class QETElementEditor;
typedef CustomElementGraphicPart CEGP;
/**
	Cette classe represente une partie graphique d'element
	Elle encapsule des methodes afin de gerer les attributs de style communs
	a la plupart des parties d'elements 
*/
class CustomElementGraphicPart : public CustomElementPart {
	public:
	/// Qualifie le style de ligne utilise pour dessiner la partie
	enum LineStyle {
		NormalStyle, ///< Ligne pleine
		DashedStyle, ///< Ligne pointillee (tirets)
		DottedStyle  ///< Ligne pointillee (points)
	};
	
	/// Qualifie l'epaisseur de ligne utilisee pour dessiner la partie
	enum LineWeight {
		NormalWeight, ///< Ligne normale
		ThinWeight,   ///< Ligne fine
		NoneWeight    ///< Ligne invisible
	};
	
	/// Qualifie la couleur utilisee pour remplir la partie
	enum Filling {
		NoneFilling,  ///< Remplissage transparent
		BlackFilling, ///< Remplissage en noir
		WhiteFilling  ///< Remplissage en blanc
	};
	
	/// Qualifie la couleur de ligne utilisee pour dessiner la partie
	enum Color {
		BlackColor, ///< Ligne noire
		WhiteColor  ///< Ligne blanche
	};
	
	// constructeurs, destructeur
	public:
	/**
		Constructeur
		@param editor Editeur d'element auquel cette partie est rattachee
	*/
	CustomElementGraphicPart(QETElementEditor *editor) :
		CustomElementPart(editor),
		_linestyle(NormalStyle),
		_lineweight(NormalWeight),
		_filling(NoneFilling),
		_color(BlackColor),
		_antialiased(false)
	{
	};
	
	/// Destructeur
	virtual ~CustomElementGraphicPart() {
	};
	
	// attributs
	private:
	LineStyle _linestyle;
	LineWeight _lineweight;
	Filling _filling ;
	Color _color;
	bool _antialiased;
	
	//methodes
	public:
	void setLineStyle(LineStyle);
	void setLineWeight(LineWeight);
	void setFilling(Filling);
	void setColor(Color);
	void setAntialiased(bool);
	
	LineStyle lineStyle() const;
	LineWeight lineWeight() const;
	Filling filling() const;
	Color color() const;
	bool antialiased() const;
	
	void setProperty(const QString &, const QVariant &);
	QVariant property(const QString &);
	
	protected:
	void stylesToXml(QDomElement &) const;
	void stylesFromXml(const QDomElement &);
	void resetStyles();
	void applyStylesToQPainter(QPainter &) const;
};

/**
	Change le style de trait
	@param ls Le nouveau style de trait
*/
inline void CustomElementGraphicPart::setLineStyle(LineStyle ls) {
	_linestyle = ls;
}

/**
	Change l'epaisseur de trait
	@param lw La nouvelle epaisseur de trait
*/
inline void CustomElementGraphicPart::setLineWeight(LineWeight lw) {
	_lineweight = lw;
}

/**
	Change la couleur de remplissage
	@param f La nouvelle couleur de remplissage
*/
inline void CustomElementGraphicPart::setFilling(Filling f) {
	_filling = f;
}

/**
	Change la couleur de trait
	@param c La nouvelle couleur de trait
*/
inline void CustomElementGraphicPart::setColor(Color c) {
	_color = c;
}

/**
	@return Le style de trait
*/
inline CustomElementGraphicPart::LineStyle CustomElementGraphicPart::lineStyle() const {
	return(_linestyle);
}

/**
	@return L'epaisseur de trait
*/
inline CustomElementGraphicPart::LineWeight CustomElementGraphicPart::lineWeight() const {
	return(_lineweight);
}

/**
	@return La couleur de remplissage
*/
inline CustomElementGraphicPart::Filling CustomElementGraphicPart::filling() const {
	return(_filling);
}

/**
	@return La couleur de trait
*/
inline CustomElementGraphicPart::Color CustomElementGraphicPart::color() const {
	return(_color);
}

/**
	Definit si la partie doit etre antialiasee ou non
	@param aa True pour activer l'antialiasing, false pour le desactiver
*/
inline void CustomElementGraphicPart::setAntialiased(bool aa) {
	_antialiased = aa;
}

/**
	@return true si la partie est antialiasee, false sinon
*/
inline bool CustomElementGraphicPart::antialiased() const {
	return(_antialiased);
}

#endif
