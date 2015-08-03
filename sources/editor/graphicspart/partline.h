/*
	Copyright 2006-2015 The QElectroTech Team
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
#ifndef PART_LINE_H
#define PART_LINE_H

#include "customelementgraphicpart.h"
#include "qet.h"
#include "QetGraphicsItemModeler/qetgraphicshandlerutility.h"

class QPropertyUndoCommand;

/**
	This class represents a line primitive which may be used to compose the
	drawing of an electrical element within the element editor. Lines may have
	specific visual ends (e.g. arrows) through the setFirstEndType and
	setSecondEndType methods. Their size can be defined using the
	setFirstEndLength and setSecondEndLength methods. Please note ends are not
	drawn if the required length for their drawing is longer than the line itself.
	In case there is room for a single end only, the first one get priority.
*/
class PartLine : public CustomElementGraphicPart
{
		Q_OBJECT

		Q_PROPERTY(Qet::EndType end1 READ firstEndType WRITE setFirstEndType)
		Q_PROPERTY(Qet::EndType end2 READ secondEndType WRITE setSecondEndType)
		Q_PROPERTY(qreal length1 READ firstEndLength WRITE setFirstEndLength)
		Q_PROPERTY(qreal length2 READ secondEndLength WRITE setSecondEndLength)
		Q_PROPERTY(QLineF line READ line WRITE setLine)

		// constructors, destructor
	public:
		PartLine(QETElementEditor *, QGraphicsItem * = 0);
		virtual ~PartLine();
	private:
		PartLine(const PartLine &);

	signals:
		void lineChanged();
		void firstEndTypeChanged();
		void secondEndTypeChanged();
		void firstEndLengthChanged();
		void secondEndLengthChanged();

	
	// methods
	public:
		enum { Type = UserType + 1104 };
	
		 /**
		  * Enable the use of qgraphicsitem_cast to safely cast a QGraphicsItem into a PartLine.
		  * @return the QGraphicsItem type
		  */
		virtual int type() const { return Type; }
		virtual void paint(QPainter *, const QStyleOptionGraphicsItem *, QWidget * = 0);
		virtual QString name() const { return(QObject::tr("ligne", "element part name")); }
		virtual QString xmlName() const { return(QString("line")); }
		virtual const QDomElement toXml(QDomDocument &) const;
		virtual void fromXml(const QDomElement &);
		virtual QPointF sceneP1() const;
		virtual QPointF sceneP2() const;
		virtual QPainterPath shape() const;
		virtual QPainterPath shadowShape() const;
		virtual QRectF boundingRect() const;
		virtual bool isUseless() const;
		virtual QRectF sceneGeometricRect() const;
		virtual void startUserTransformation(const QRectF &);
		virtual void handleUserTransformation(const QRectF &, const QRectF &);
		static uint requiredLengthForEndType(const Qet::EndType &);
		static QList<QPointF> fourEndPoints(const QPointF &, const QPointF &, const qreal &);

		QLineF line() const;
		void setLine(const QLineF &line);
		Qet::EndType firstEndType() const {return first_end;}
		void setFirstEndType(const Qet::EndType &et);
		Qet::EndType secondEndType() const {return second_end;}
		void setSecondEndType(const Qet::EndType &et);
		qreal firstEndLength() const {return first_length;}
		void setFirstEndLength(const qreal &l);
		qreal secondEndLength() const {return second_length;}
		void setSecondEndLength(const qreal &l);

	protected:
		virtual void hoverMoveEvent(QGraphicsSceneHoverEvent *event);
		virtual void mousePressEvent(QGraphicsSceneMouseEvent *event);
		virtual void mouseMoveEvent(QGraphicsSceneMouseEvent *event);
		virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);
	
	private:
		QPainterPath path() const;

		QList<QPointF> fourShapePoints() const;
		QRectF firstEndCircleRect() const;
		QRectF secondEndCircleRect() const;
		void debugPaint(QPainter *);

		Qet::EndType first_end;
		qreal        first_length;

		Qet::EndType second_end;
		qreal        second_length;
		QList<QPointF> saved_points_;
		QLineF m_line;
		QetGraphicsHandlerUtility m_handler;
		int m_handler_index;
		QPropertyUndoCommand *m_undo_command;
};
#endif
