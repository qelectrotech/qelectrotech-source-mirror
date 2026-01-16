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
			Outer
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

	private:
		QGraphicsObject* q{nullptr};
};

#endif // TERMINALDATA_H
