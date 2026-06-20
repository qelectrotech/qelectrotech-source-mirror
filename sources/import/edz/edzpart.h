/*
	Copyright 2006-2026 The QElectroTech Team
	This file is part of QElectroTech.

	QElectroTech is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 2 of the License, or
	(at your option) any later version.

	QElectroTech is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with QElectroTech. If not, see <http://www.gnu.org/licenses/>.
*/
#ifndef EDZPART_H
#define EDZPART_H

#include <QString>
#include <QList>
#include <QMap>

/** One physical connection of an EPLAN part (a pin/terminal). */
struct EdzPin {
	QString designation;   ///< terminal id, e.g. "1", "PE"
	QString description;   ///< function label, e.g. "L+"
};

/**
	@brief The portable data of an EPLAN part, read from its part.xml.

	Mirrors the fields the standalone edz2qet.py prototype maps: identity and
	metadata, localized names (2-letter language -> text) and the connection
	list. Geometry from the EPLAN macro is intentionally ignored — the importer
	generates a generic symbol from the pin list instead.
*/
class EdzPart
{
	public:
		bool parse(const QString &part_xml_path);
		QString errorString() const { return m_error; }

		QString partNumber()   const { return m_part_number; }
		QString manufacturer() const { return m_manufacturer; }
		QString orderNumber()  const { return m_order_number; }
		QString typeNumber()   const { return m_type_number; }
		QString description()  const { return m_description; }
		QString comment()      const { return m_comment; }
		QString picture()      const { return m_picture; }
		QMap<QString, QString> names() const { return m_names; }
		QList<EdzPin> pins()   const { return m_pins; }

	private:
		QString m_error;
		QString m_part_number;
		QString m_manufacturer;
		QString m_order_number;
		QString m_type_number;
		QString m_description;   ///< DESCR1, preferred English (for element-info)
		QString m_comment;       ///< DESCR2, preferred English
		QString m_picture;       ///< picture file name, if any
		QMap<QString, QString> m_names;   ///< 2-letter lang -> localized name
		QList<EdzPin> m_pins;
};

#endif // EDZPART_H
