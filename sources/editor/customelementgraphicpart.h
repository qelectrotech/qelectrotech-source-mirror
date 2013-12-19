/*
	Copyright 2006-2013 The QElectroTech Team
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
	This class represents an element visual/geometric primitive. It provides
	methods to manage style attributes common to most primitives.
*/
class CustomElementGraphicPart : public CustomElementPart {
	public:
	/// This enum lists the various line styles available to draw primitives.
	enum LineStyle {
		NormalStyle, ///< Normal line
		DashedStyle, ///< Dashed line
		DottedStyle,  ///< Dotted line
		DashdottedStyle  ///< Dashdotted  line
	};
	
	/// This enum lists the various line weights available to draw primitives.
	enum LineWeight {
		NoneWeight,    ///< Invisible line
		ThinWeight,   ///< Thin line
		NormalWeight, ///< Normal line 1px
		UltraWeight, ///< Normal line 2px
		BigWeight    ///< Big Line

	};
	
	/// This enum lists the various filling colors available to draw primitives.
	enum Filling {
		NoneFilling,  ///< No filling (i.e. transparent)
		BlackFilling, ///< Black filling
		WhiteFilling,  ///< White filling
		GreenFilling, ///< Green filling
		RedFilling,  ///< Red filling
		BlueFilling  ///< Green filling
	};
	
	/// This enum lists the various line colors available to draw primitives.
	enum Color {
		BlackColor, ///< Black line
		WhiteColor, ///< White line
		GreenColor, ///< Green line
		RedColor,  ///<  Red line
		BlueColor  ///<  Blue line
	};
	
	// constructors, destructor
	public:
	/**
		Constructor
		@param editor Element editor this primitive lives in.
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
	
	/// Destructor
	virtual ~CustomElementGraphicPart() {
	};
	
	// attributes
	private:
	LineStyle _linestyle;
	LineWeight _lineweight;
	Filling _filling ;
	Color _color;
	bool _antialiased;
	
	// methods
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
	Set the primitive line style.
	@param ls the new line style
*/
inline void CustomElementGraphicPart::setLineStyle(LineStyle ls) {
	_linestyle = ls;
}

/**
	Set the primitive line weight.
	@param lw the new line weight
*/
inline void CustomElementGraphicPart::setLineWeight(LineWeight lw) {
	_lineweight = lw;
}

/**
	Set the filling color.
	@param f the new filling color
*/
inline void CustomElementGraphicPart::setFilling(Filling f) {
	_filling = f;
}

/**
	Set the line color.
	@param c the new line color
*/
inline void CustomElementGraphicPart::setColor(Color c) {
	_color = c;
}

/**
	@return the current line style
*/
inline CustomElementGraphicPart::LineStyle CustomElementGraphicPart::lineStyle() const {
	return(_linestyle);
}

/**
	@return the current line weight
*/
inline CustomElementGraphicPart::LineWeight CustomElementGraphicPart::lineWeight() const {
	return(_lineweight);
}

/**
	@return the current filling color
*/
inline CustomElementGraphicPart::Filling CustomElementGraphicPart::filling() const {
	return(_filling);
}

/**
	@return the current line color
*/
inline CustomElementGraphicPart::Color CustomElementGraphicPart::color() const {
	return(_color);
}

/**
	Set whether the primitive should be drawn antialiased.
	@param aa True to enable antialiasing, false to disable it.
*/
inline void CustomElementGraphicPart::setAntialiased(bool aa) {
	_antialiased = aa;
}

/**
	@return whether the primitive is drawn antialiased.
*/
inline bool CustomElementGraphicPart::antialiased() const {
	return(_antialiased);
}

#endif
