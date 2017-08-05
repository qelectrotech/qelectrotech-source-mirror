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
#ifndef PART_TERMINAL_H
#define PART_TERMINAL_H

#include "customelementgraphicpart.h"



/**
	This class represents a terminal which may be used to compose the drawing of
	an electrical element within the element editor.
*/
class PartTerminal : public CustomElementGraphicPart
{
		Q_OBJECT

		Q_PROPERTY(Qet::Orientation orientation READ orientation WRITE setOrientation)

	
	public:
		// constructors, destructor
		PartTerminal(QETElementEditor *editor, QGraphicsItem *parent = nullptr);
		~PartTerminal() override;
	private:
		PartTerminal(const PartTerminal &);

	signals:
		void orientationChanged();
	
		// attributes
	private:
		Qet::Orientation m_orientation;
		QPointF second_point;


		// methods
	public:
		enum { Type = UserType + 1106 };
			/**
			 * Enable the use of qgraphicsitem_cast to safely cast a QGraphicsItem into a PartTerminal.
			 * @return the QGraphicsItem type
			 */
		int type() const override { return Type; }
		QString name() const override { return(QObject::tr("borne", "element part name")); }
		QString xmlName() const override { return(QString("terminal")); }
		void fromXml(const QDomElement &) override;
		const QDomElement toXml(QDomDocument &) const override;
		void paint(QPainter *, const QStyleOptionGraphicsItem *, QWidget *) override;

		QPainterPath shape() const override;
		QPainterPath shadowShape() const override {return shape();}
		QRectF boundingRect() const override;
		bool isUseless() const override;
		QRectF sceneGeometricRect() const override;
		void startUserTransformation(const QRectF &) override;
		void handleUserTransformation(const QRectF &, const QRectF &) override;

		Qet::Orientation orientation() const {return m_orientation;}
		void setOrientation(Qet::Orientation ori);
	
	private:
		void updateSecondPoint();
	
	private:
		QPointF saved_position_;
};
#endif
