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
#ifndef TERMINALDATA_H
#define TERMINALDATA_H

#include "../qet.h"
#include "propertiesinterface.h"

#include <QColor>
#include <QFont>
#include <QPointF>
#include <QUuid>

class QGraphicsObject;

/**
	@brief The TerminalData class
	Data of the terminal.
	Stored in extra class so it can be used by PartTerminal
	and Terminal without defining everything again.
	@note tis class needs to be work on
*/
class TerminalData : public PropertiesInterface
{
	Q_GADGET

	public:
		enum Type {
			Generic,
			Inner,
			Outer,
			No,     ///< Normally Open terminal (for SW contacts)
			Nc,     ///< Normally Closed terminal (for SW contacts)
			Common  ///< Common terminal (for SW contacts)
		};
		Q_ENUM(Type)

		TerminalData();
		TerminalData(QGraphicsObject* parent);
		~TerminalData() override;

		void init();

		void setParent(QGraphicsObject* parent);
		void toSettings(QSettings &settings,
				const QString prefix = QString()) const override;
		void fromSettings(const QSettings &settings,
				  const QString prefix = QString()) override;
		QDomElement toXml(QDomDocument &xml_element) const override;
		bool fromXml(const QDomElement &xml_element) override;

		static QString typeToString(TerminalData::Type type);
		static TerminalData::Type typeFromString(const QString &string);

	// must be public, because this class is a private member
	// of PartTerminal/Terminal and they must access this data
	public:
		/**
			@brief m_orientation
			Orientation of the terminal
		*/
		Qet::Orientation m_orientation;
		/**
			@brief second_point
			Position of the second point of the terminal
			in scene coordinates
		*/
		QPointF m_second_point;
		/**
			@brief m_uuid
			Uuid of the terminal.

			In elementscene.cpp an element gets a new uuid when
			saving the element. In the current state
			each connection is made by using the local position
			of the terminal and a dynamic id. In the new
			case, each terminal should have it's own uuid to
			identify it uniquely. When changing each time this
			uuid, the conductor after updating the part is anymore
			valid. So if in the loaded document a uuid exists,
			use this one and don't create a new one.
		*/
		QUuid m_uuid;
		/**
			@brief m_name
			Name of the element.
			It can be used to create wiring harness tables
		*/
		QString m_name;

		/**
			@brief m_pos
			Position of the terminal. The second point is calculated
			from this position and the orientation
			@note
			Important: this variable is only updated during read
			from xml and not during mouse move!
			It is used to store the initial position so that
			PartTerminal and Terminal have access to it.
		*/
		QPointF m_pos;

		TerminalData::Type m_type = TerminalData::Generic;

		/// Whether to display the terminal name as a text label
		bool m_show_name = false;
		/// Position of the text label relative to the terminal
		QPointF m_label_pos{0.0, 0.0};
		/// Font used for the text label
		QFont m_label_font;
		/// Rotation of the text label in degrees
		qreal m_label_rotation = 0.0;
		/// Horizontal alignment of the text label
		Qt::Alignment m_label_halignment = Qt::AlignHCenter;
		/// Vertical alignment of the text label
		Qt::Alignment m_label_valignment = Qt::AlignVCenter;
		/// Whether to draw a frame around the text label
		bool m_label_frame = false;
		/// Color of the text label
		QColor m_label_color = Qt::black;

	private:
		QGraphicsObject* q{nullptr};
};

#endif // TERMINALDATA_H
