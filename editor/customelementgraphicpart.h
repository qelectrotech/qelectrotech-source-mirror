#ifndef CUSTOM_ELEMENT_GRAPHIC_PART_H
#define CUSTOM_ELEMENT_GRAPHIC_PART_H
#include <QPainter>
#include "customelementpart.h"
#include "styleeditor.h"
/**
	Cette classe represente une partie graphique d'element
	Elle encapsule des methodes afin de gerer les attributs de style communs
	a la plupart des parties d'elements 
*/
typedef CustomElementGraphicPart CEGP;
class CustomElementGraphicPart : public CustomElementPart {
	public:
	enum LineStyle { NormalStyle, DashedStyle };
	enum LineWeight { NormalWeight, ThinWeight, NoneWeight };
	enum Filling { NoneFilling, BlackFilling, WhiteFilling };
	enum Color { BlackColor, WhiteColor };
	
	// constructeurs, destructeur
	public:
	CustomElementGraphicPart() :
		_linestyle(NormalStyle),
		_lineweight(NormalWeight),
		_filling(NoneFilling),
		_color(BlackColor),
		_antialiased(false)
	{
		style_editor = new StyleEditor(this);
	};
	
	virtual ~CustomElementGraphicPart() {
		qDebug() << "~CustomElementGraphicPart()";
		delete style_editor;
	};
	
	// attributs
	private:
	LineStyle _linestyle;
	LineWeight _lineweight;
	Filling _filling ;
	Color _color;
	bool _antialiased;
	
	protected:
	StyleEditor *style_editor;
	
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
	
	QWidget *elementInformations();
	
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
