/*
	Copyright 2006-2017 The QElectroTech Team
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

#include <QGraphicsObject>
#include "customelementpart.h"

class QETElementEditor;
class QPainter;


/**
 * @brief The CustomElementGraphicPart class
 * This class is the base for all home-made primitive like line, rectangle, ellipse etc....
 * It provides methods and enums to manage style attributes available for primitive (color, pen style, etc...)
 */
class CustomElementGraphicPart : public QGraphicsObject, public CustomElementPart
{
		#define SHADOWS_HEIGHT 4.0

		Q_OBJECT

			//Made this Q_ENUMS to be used by the Q_PROPERTY system.
		Q_ENUMS (LineStyle)
		Q_ENUMS (LineWeight)
		Q_ENUMS (Filling)
		Q_ENUMS (Color)

		Q_PROPERTY(LineStyle line_style   READ lineStyle   WRITE setLineStyle)
		Q_PROPERTY(LineWeight line_weight READ lineWeight  WRITE setLineWeight)
		Q_PROPERTY(Filling filling        READ filling     WRITE setFilling)
		Q_PROPERTY(Color color            READ color       WRITE setColor)
		Q_PROPERTY(bool antialias         READ antialiased WRITE setAntialiased)

	public:
			//Line style
		enum LineStyle {NormalStyle, DashedStyle, DottedStyle, DashdottedStyle};

			//Line weight : invisible, 0px, 1px, 2px, 5px
		enum LineWeight {NoneWeight, ThinWeight, NormalWeight, UltraWeight, BigWeight};

			//Filling color of the part : NoneFilling -> No filling (i.e. transparent)
		enum Filling { NoneFilling, BlackFilling, WhiteFilling, GreenFilling, RedFilling, BlueFilling, GrayFilling, BrunFilling, YellowFilling, CyanFilling, MagentaFilling, LightgrayFilling, OrangeFilling, PurpleFilling, HorFilling, VerFilling, BdiagFilling, FdiagFilling};

			//Line color
		enum Color {BlackColor, WhiteColor, GreenColor, RedColor, BlueColor, GrayColor, BrunColor, YellowColor, CyanColor, MagentaColor, LightgrayColor, OrangeColor, PurpleColor, NoneColor};
	
		// constructors, destructor
	public:

		CustomElementGraphicPart(QETElementEditor *editor, QGraphicsItem *parent = 0);
		virtual ~CustomElementGraphicPart();

		static void drawCross (const QPointF &center, QPainter *painter);

			//Getter and setter
		LineStyle lineStyle    () const {return _linestyle;}
		void      setLineStyle (const LineStyle ls);

		LineWeight lineWeight    () const {return _lineweight;}
		void       setLineWeight (const LineWeight lw);
		qreal      penWeight     () const;

		Filling filling   () const {return _filling;}
		void    setFilling(const Filling f);

		Color color   () const {return _color;}
		void  setColor(const Color c);

		bool antialiased   () const {return _antialiased;}
		void setAntialiased(const bool b);
			//End of getter and setter


			//Rediriged to QObject Q_PROPERTY system
		virtual void     setProperty (const char *name, const QVariant &value) {QObject::setProperty(name, value);}
		virtual QVariant property    (const char *name) const                  {return QObject::property(name);}

		virtual QPainterPath shadowShape ()const = 0;

	protected:
		void stylesToXml  (QDomElement &) const;
		void stylesFromXml(const QDomElement &);
		void resetStyles  ();
		void applyStylesToQPainter(QPainter &) const;
		void drawShadowShape (QPainter *painter);

		QVariant itemChange(GraphicsItemChange change, const QVariant &value);
		void hoverEnterEvent(QGraphicsSceneHoverEvent *event);
		void hoverMoveEvent (QGraphicsSceneHoverEvent *event);
		void hoverLeaveEvent(QGraphicsSceneHoverEvent *event);

		virtual void mousePressEvent(QGraphicsSceneMouseEvent *event);
		virtual void mouseMoveEvent(QGraphicsSceneMouseEvent *event);
		virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);

		// attributes
		bool m_hovered;
	private:
		LineStyle _linestyle;
		LineWeight _lineweight;
		Filling _filling ;
		Color _color;
		bool _antialiased;
		QPointF m_origin_pos, m_mouse_to_origin_pos;
};

typedef CustomElementGraphicPart CEGP;
#endif
