/*
	Copyright 2006-2014 The QElectroTech Team
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
#include <QObject>
#include "customelementpart.h"
#include "styleeditor.h"
class QETElementEditor;
typedef CustomElementGraphicPart CEGP;
/**
	This class represents an element visual/geometric primitive. It provides
	methods to manage style attributes common to most primitives.
*/
class CustomElementGraphicPart : public QObject, public CustomElementPart {
	Q_OBJECT

	public:
	/// This enum lists the various line styles available to draw primitives.
	Q_ENUMS(LineStyle)
	enum LineStyle {
		NormalStyle, ///< Normal line
		DashedStyle, ///< Dashed line
		DottedStyle,  ///< Dotted line
		DashdottedStyle  ///< Dashdotted  line
	};
	
	/// This enum lists the various line weights available to draw primitives.
	Q_ENUMS(LineWeight)
	enum LineWeight {
		NoneWeight,    ///< Invisible line
		ThinWeight,   ///< Thin line
		NormalWeight, ///< Normal line 1px
		UltraWeight, ///< Normal line 2px
		BigWeight    ///< Big Line

	};
	
	/// This enum lists the various filling colors available to draw primitives.
	Q_ENUMS(Filling)
	enum Filling {
		NoneFilling,  ///< No filling (i.e. transparent)
		BlackFilling, ///< Black filling
		WhiteFilling,  ///< White filling
		GreenFilling, ///< Green filling
		RedFilling,  ///< Red filling
		BlueFilling  ///< Green filling
	};
	
	/// This enum lists the various line colors available to draw primitives.
	Q_ENUMS(Color)
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

	/// PROPERTY
	Q_PROPERTY(LineStyle line_style READ lineStyle WRITE setLineStyle)
		LineStyle lineStyle() const {return _linestyle;}
		void setLineStyle(const LineStyle ls) {_linestyle = ls;}
	Q_PROPERTY(LineWeight line_weight READ lineWeight WRITE setLineWeight)
		LineWeight lineWeight() const {return _lineweight;}
		void setLineWeight(const LineWeight lw) {_lineweight = lw;}
	Q_PROPERTY(Filling filling READ filling WRITE setFilling)
		Filling filling() const {return _filling;}
		void setFilling(const Filling f) {_filling = f;}
	Q_PROPERTY(Color color READ color WRITE setColor)
		Color color() const {return _color;}
		void setColor(const Color c) {_color = c;}
	Q_PROPERTY(bool antialias READ antialiased WRITE setAntialiased)
		bool antialiased() const {return _antialiased;}
		void setAntialiased(const bool b) {_antialiased = b;}
	
	virtual void setProperty(const char *name, const QVariant &value) {QObject::setProperty(name, value);}
	virtual QVariant property(const char *name) const {return QObject::property(name);}
	
	protected:
	void stylesToXml(QDomElement &) const;
	void stylesFromXml(const QDomElement &);
	void resetStyles();
	void applyStylesToQPainter(QPainter &) const;
};
#endif
