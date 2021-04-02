/*
		Copyright 2006-2021 The QElectroTech Team
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

class TerminalStripData : public PropertiesInterface
{
		friend class TerminalStrip;

	public:
		TerminalStripData();

		void toSettings(QSettings &/*settings*/, const QString = QString()) const override {}
		void fromSettings (const QSettings &/*settings*/, const QString = QString()) override {}

		QDomElement toXml (QDomDocument &xml_document) const override;
		bool fromXml (const QDomElement &xml_element) override;

	private :
		QString m_installation = QStringLiteral("="),
		m_location = QStringLiteral("+"),
		m_name;

};

#endif // TERMINALSTRIPDATA_H
