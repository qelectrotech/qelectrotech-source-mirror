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
#include "terminaldata.h"

#include <QGraphicsObject>
#include <QDebug>

#include "../qetxml.h"

TerminalData::TerminalData():
    PropertiesInterface("terminaldata")
{
	init();
}

TerminalData::TerminalData(QGraphicsObject *parent):
    PropertiesInterface("terminaldata"),
	q(parent)
{
	init();
}

void TerminalData::init()
{
}

TerminalData::~TerminalData()
{

}

/**
	@brief TerminalData::setParent
	@param parent
*/
void TerminalData::setParent(QGraphicsObject* parent)
{
	q = parent;
}

/**
	@brief TerminalData::toSettings
	Save properties to setting file.

	QString is use for prefix a word befor the name of each paramètre
	@param settings UNUSED
	@param prefix UNUSED
*/
void TerminalData::toSettings(QSettings &settings, const QString &prefix) const

{
	Q_UNUSED(settings)
	Q_UNUSED(prefix)
}

/**
	@brief TerminalData::fromSettings
	load properties to setting file.

	QString is use for prefix a word befor the name of each paramètre
	@param settings UNUSED
	@param prefix UNUSED
*/
void TerminalData::fromSettings(QSettings &settings, const QString& prefix)
{
	Q_UNUSED(settings)
	Q_UNUSED(prefix)
}

/**
    @brief TerminalData::toXmlPriv
	Save properties to xml element
	write the name, number, position and orientation of the terminal
	to xml_element

	@note This method is only called from the PartTerminal
	and should never called from the Terminal class
    @param e: element to store the properties
	the name, number, position and orientation of the terminal
*/
void TerminalData::toXmlPriv(QDomElement& xml_element) const
{
    xml_element.setAttribute("x", m_pos.x());
    xml_element.setAttribute("y", m_pos.y());

    if (!m_uuid.isNull())
        xml_element.setAttribute("uuid", m_uuid.toString());
    xml_element.setAttribute("name", m_name);

    xml_element.setAttribute("orientation",
	orientationToString(m_orientation));

    xml_element.setAttribute("type", typeToString(m_type));
}

/*
    @brief TerminalData::fromXmlPriv
	load properties to xml element

	@note This method is only called from the PartTerminal
	and should never called from the Terminal class
	@param xml_element
	@return true if succeeded / false if the attribute is not real
*/
bool TerminalData::fromXmlPriv(const QDomElement &xml_element)
{
	qreal term_x = 0.0;
	qreal term_y = 0.0;

	// reads the position of the terminal
	// lit la position de la borne

	if (QETXML::propertyDouble(xml_element, "x", &term_x))
		return false;

	if (QETXML::propertyDouble(xml_element, "y", &term_y))
		return false;

	m_pos = QPointF(term_x, term_y);

//	emit posFromXML(QPointF(term_x, term_y));

	// do not write uuid from this class, because only PartTerminal::fromXml need
	// to write it to xml file. Terminal::fromXml does not need.
	// if the attribute not exists, means, the element is created with an
	// older version of qet. So use the legacy approach


    //if (QETXML::propertyString(xml_element, "name", &m_name))
	//	return false;
    QETXML::propertyString(xml_element, "name", &m_name); // some parts do not have a name. Example: affuteuse_250h.qet, Terminal at x="0" y="-20"

	QString o;
    if (QETXML::propertyString(xml_element, "orientation", &o))
		return false;

	// read the orientation of the terminal
	// lit l'orientation de la borne
	m_orientation = orientationFromString(o);
	
    QString type;
    if (QETXML::propertyString(xml_element, "type", &type) == QETXML::PropertyFlags::Success)
        m_type = typeFromString(type);

	return true;
}

bool TerminalData::valideXml(const QDomElement& xml_element) {
	if (QETXML::propertyDouble(xml_element, "x"))
		return false;

    if (QETXML::propertyDouble(xml_element, "y"))
        return false;

    // Old projects do not have this property.
//	if (QETXML::propertyString(xml_element, "type"))
//		return false;


	  // legacy elements do not have an uuid
//	if (QETXML::propertyUuid(xml_element, "uuid"))
//		return false;

    //if (QETXML::propertyString(xml_element, "name")) // some parts do not have a name. Example: affuteuse_250h.qet, Terminal at x="0" y="-20"
	//	return false;

    if (QETXML::propertyString(xml_element, "orientation"))
		return false;
	return true;
}

/**
 * @brief TerminalData::typeToString
 * @param type
 * @return type into a QString
 */
QString TerminalData::typeToString(TerminalData::Type type)
{
	switch (type) {
		case Generic:
			return QString("Generic");
		case Inner :
			return QString("Inner");
		case Outer :
			return QString("Outer");
	}
}

/**
 * @brief TerminalData::typeFromString
 * @param string
 * @return The type describe in string to TerminalData::Type.
 * if string doesn't describe a type, TerminalData::Generic is returned
 */
TerminalData::Type TerminalData::typeFromString(const QString &string)
{
	if (string == "Generic") {
		return TerminalData::Generic;
	} else if (string == "Inner") {
		return TerminalData::Inner;
	} else if (string == "Outer") {
		return TerminalData::Outer;
	} else {
		qDebug() << "TerminalData::typeFromString, argument string is invalid"
					" failsafe type 'TerminalData::Generic' is returned";
		return TerminalData::Generic;
	}
}
