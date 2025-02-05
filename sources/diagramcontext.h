/*
	Copyright 2006-2025 The QElectroTech Team
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
#ifndef DIAGRAM_CONTEXT_H
#define DIAGRAM_CONTEXT_H
#include "pugixml/src/pugixml.hpp"

#include <QDomElement>
#include <QHash>
#include <QSettings>
#include <QString>
#include <QStringList>
#include <QVariant>
/**
	This class represents a diagram context, i.e. the data (a list of key/value
	pairs) of a diagram at a given time. It is notably used by titleblock
	templates to fetch the informations they need to do their rendering, or
	element for retrieve information about itself
*/

/**
 * Key for element :
 * label                          -> label or identification of element
 * formula                        -> formula used to create the label (formula is make with variable)
 * designation                    -> exhaustive comment used to explain what the element does.
 * description                    -> exhaustive description used to explain what the element does.
 * plant                          -> the plant assigned to the element
 * comment                        -> a little comment which can be displayed in the folio
 * manufacturer                   -> the manufacturer of the element
 * manufacturer_reference         -> the manufacturer reference of the element
 * quantity                       -> quantity of the element
 * unity                          -> unity of the element
 * auxiliary1                     -> auxiliary 1 of element
 * description_auxiliary1
 * designation_auxiliary1
 * manufacturer_auxiliary1
 * manufacturer_reference_auxiliary1
 * machine_manufacturer_reference_auxiliary1
 * supplier_auxiliary1
 * quantity_auxiliary1
 * unity_auxiliary1
 * auxiliary2                     -> auxiliary 2 of element
 * description_auxiliary2
 * designation_auxiliary2
 * manufacturer_auxiliary2
 * manufacturer_reference_auxiliary2
 * machine_manufacturer_reference_auxiliary2
 * supplier_auxiliary2
 * quantity_auxiliary2
 * unity_auxiliary2
 * auxiliary3                     -> auxiliary 3 of element
 * description_auxiliary3
 * designation_auxiliary3
 * manufacturer_auxiliary3
 * manufacturer_reference_auxiliary3
 * machine_manufacturer_reference_auxiliary3
 * supplier_auxiliary3
 * quantity_auxiliary3
 * unity_auxiliary3
 * auxiliary4                     -> auxiliary 4 of element
 * description_auxiliary4
 * designation_auxiliary4
 * manufacturer_auxiliary4
 * manufacturer_reference_auxiliary4
 * machine_manufacturer_reference_auxiliary4
 * supplier_auxiliary4
 * quantity_auxiliary4
 * unity_auxiliary4
 * 
 * machine_manufacturer_reference -> reference of the machine manufacturer
 * supplier                       -> the supplier of the element
 * function                       -> the function of element
 * location                       -> the location assigned to the element
 * frozenLabel                    -> label locked at a given time
 *
*/
class DiagramContext
{
	public:
		enum KeyOrder {
			None,
			Alphabetical,
			DecreasingLength
		};
	
		void add(DiagramContext other);
		void remove(const QString &key);
		QList<QString> keys(KeyOrder = Alphabetical) const;
		bool contains(const QString &) const;
		const QVariant operator[](const QString &) const;
		bool addValue(const QString &, const QVariant &, bool show = true);
		QVariant value(const QString &key) const;
		void clear();
		int count();
		bool keyMustShow (const QString &) const;
		
		bool operator==(const DiagramContext &) const;
		bool operator!=(const DiagramContext &) const;
		
		void toXml(QDomElement &, const QString & = "property") const;
		void fromXml(const QDomElement &, const QString & = "property");
		void fromXml(const pugi::xml_node &dom_element, const QString &tag_name = "property");
		void toSettings(QSettings &, const QString &) const;
		void fromSettings(QSettings &, const QString &);

		static bool isKeyAcceptable(const QString &);
	
	private:
		static bool stringLongerThan(const QString &, const QString &);
		static QString validKeyRegExp();

		/// Diagram context data (key/value pairs)
		QHash<QString, QVariant> m_content;
		QHash<QString, bool> m_content_show;
};

QDebug operator <<(QDebug debug, const DiagramContext &context);
#endif
