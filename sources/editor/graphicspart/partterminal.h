/*
	Copyright 2006-2026 The QElectroTech Team
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

#include "../../properties/terminaldata.h"
#include "customelementgraphicpart.h"

#include <QUuid>

/**
	This class represents a terminal which may be used to compose the drawing of
	an electrical element within the element editor.
*/
class PartTerminal : public CustomElementGraphicPart
{
	Q_OBJECT

	Q_PROPERTY(Qet::Orientation orientation READ orientation WRITE setOrientation)
	Q_PROPERTY(QString terminal_name READ terminalName WRITE setTerminalName)
	Q_PROPERTY(TerminalData::Type terminal_type READ terminalType WRITE setTerminalType)
	Q_PROPERTY(bool show_name READ showName WRITE setShowName)
	Q_PROPERTY(QPointF label_pos READ labelPos WRITE setLabelPos)
	Q_PROPERTY(QFont label_font READ labelFont WRITE setLabelFont)
	Q_PROPERTY(qreal label_rotation READ labelRotation WRITE setLabelRotation)
	Q_PROPERTY(Qt::Alignment label_halignment READ labelHAlignment WRITE setLabelHAlignment)
	Q_PROPERTY(Qt::Alignment label_valignment READ labelVAlignment WRITE setLabelVAlignment)
	Q_PROPERTY(bool label_frame READ labelFrame WRITE setLabelFrame)
	Q_PROPERTY(QColor label_color READ labelColor WRITE setLabelColor)

	public:
		// constructors, destructor
		PartTerminal(QETElementEditor *editor, QGraphicsItem *parent = nullptr);
		~PartTerminal() override;
	private:
		PartTerminal(const PartTerminal &);

	signals:
		void orientationChanged();
		void nameChanged();
		void terminalTypeChanged();
		void showNameChanged();
		void labelPosChanged();
		void labelFontChanged();
		void labelRotationChanged();
		void labelHAlignmentChanged();
		void labelVAlignmentChanged();
		void labelFrameChanged();
		void labelColorChanged();

		// methods
	public:
		enum { Type = UserType + 1106 };
			/**
				Enable the use of qgraphicsitem_cast to safely cast a QGraphicsItem into a PartTerminal.
				@return the QGraphicsItem type
			*/
		int type() const override { return Type; }
		QString xmlName() const override { return(QString("terminal")); }
		void fromXml(const QDomElement &) override;
		const QDomElement toXml(QDomDocument &) const override;
		void paint(
				QPainter *painter,
				const QStyleOptionGraphicsItem *,
				QWidget *) override;

		QPainterPath shape() const override;
		QPainterPath shadowShape() const override;
		QRectF boundingRect() const override;
		bool isUseless() const override;
		QRectF sceneGeometricRect() const override;
		void startUserTransformation(const QRectF &) override;
		void handleUserTransformation(const QRectF &, const QRectF &) override;

		Qet::Orientation orientation() const {return d -> m_orientation;}
		void setOrientation(Qet::Orientation ori);

		void  setRotation(qreal angle);
		qreal rotation() const;
		void  flip();
		void  mirror();
                void nextOrientation();
                void previousOrientation();


		QString terminalName() const { return d -> m_name; }
		void setTerminalName(const QString& name);

		QString name() const override { return QObject::tr("Borne");}

		TerminalData::Type terminalType() const {return d->m_type;}
		void setTerminalType(TerminalData::Type type);

		bool showName() const { return d->m_show_name; }
		void setShowName(bool show);

		QPointF labelPos() const { return d->m_label_pos; }
		void setLabelPos(QPointF pos);

		QFont labelFont() const { return d->m_label_font; }
		void setLabelFont(QFont font);

		qreal labelRotation() const { return d->m_label_rotation; }
		void setLabelRotation(qreal rotation);

		Qt::Alignment labelHAlignment() const { return d->m_label_halignment; }
		void setLabelHAlignment(Qt::Alignment align);

		Qt::Alignment labelVAlignment() const { return d->m_label_valignment; }
		void setLabelVAlignment(Qt::Alignment align);

		bool labelFrame() const { return d->m_label_frame; }
		void setLabelFrame(bool frame);

		QColor labelColor() const { return d->m_label_color; }
		void setLabelColor(QColor color);

		void setNewUuid();

		QRectF labelRect() const;

	protected:
		void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
		void mouseMoveEvent(QGraphicsSceneMouseEvent *event) override;
		void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override;

	private:
		void updateSecondPoint();
		TerminalData* d; // pointer to the terminal data

	private:
		QPointF saved_position_;
		bool m_dragging_label = false;
		QPointF m_original_label_pos;
};
#endif
