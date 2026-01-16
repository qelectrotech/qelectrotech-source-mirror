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
#ifndef TERMINALSTRIPDATA_H
#define TERMINALSTRIPDATA_H

#include "../properties/propertiesinterface.h"

#include <QUuid>

class TerminalStripData : public PropertiesInterface
{
		friend class TerminalStrip;
		friend class TerminalStripEditor;

	public:
		TerminalStripData();
		TerminalStripData(const TerminalStripData &other);

		void toSettings(QSettings &/*settings*/, const QString = QString()) const override {}
		void fromSettings (const QSettings &/*settings*/, const QString = QString()) override {}

		QDomElement toXml (QDomDocument &xml_document) const override;
		bool fromXml (const QDomElement &xml_element) override;

		static QString xmlTagName() {return QStringLiteral("terminal_strip_data");}

		TerminalStripData &operator= (const TerminalStripData &other);

	private :
		static QDomElement infoToXml(QDomDocument &xml_doc, const QString &name, const QString &value);

		QString m_installation = QStringLiteral("="),
				m_location = QStringLiteral("+"),
				m_name,
				m_comment,
				m_description;
		QUuid m_uuid = QUuid::createUuid();

};

#endif // TERMINALSTRIPDATA_H
