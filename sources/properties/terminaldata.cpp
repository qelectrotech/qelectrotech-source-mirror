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
#include "terminaldata.h"

#include <QGraphicsObject>
#include <QDebug>

TerminalData::TerminalData():
	PropertiesInterface()
{
	init();
}

TerminalData::TerminalData(QGraphicsObject *parent):
	PropertiesInterface(),
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
	Save properties to settings file.

	QString is used to prefix a word before the name of each parameter
	@param settings UNUSED
	@param prefix UNUSED
*/
void TerminalData::toSettings(QSettings &settings, const QString prefix) const

{
	Q_UNUSED(settings)
	Q_UNUSED(prefix)
}

/**
	@brief TerminalData::fromSettings
	load properties to settings file.

	QString is used to prefix a word before the name of each parameter
	@param settings UNUSED
	@param prefix UNUSED
*/
void TerminalData::fromSettings(const QSettings &settings, const QString prefix)
{
	Q_UNUSED(settings)
	Q_UNUSED(prefix)
}

/**
	@brief TerminalData::toXml
	Save properties to xml element
	write the name, number, position and orientation of the terminal
	to xml_element

	@note This method is only called from the PartTerminal
	and should never be called from the Terminal class
	@param xml_document
	@return xml_element : DomElement with
	the name, number, position and orientation of the terminal
*/
QDomElement TerminalData::toXml(QDomDocument &xml_document) const
{
	QDomElement xml_element = xml_document.createElement("terminal");

	qreal x = (qRound(q->scenePos().x() * 100.0) / 100.0);
	qreal y = (qRound(q->scenePos().y() * 100.0) / 100.0);

	xml_element.setAttribute("x", QString("%1").arg(x));
	xml_element.setAttribute("y", QString("%1").arg(y));

	xml_element.setAttribute("uuid", m_uuid.toString());
	xml_element.setAttribute("name", m_name);

	xml_element.setAttribute("orientation",
				 Qet::orientationToString(m_orientation));

	xml_element.setAttribute("type", typeToString(m_type));

	return(xml_element);
}

/**
	@brief TerminalData::fromXml
	load properties to xml element

	@note This method is only called from the PartTerminal
	and should never be called from the Terminal class
	@param xml_element
	@return true if succeeded / false if the attribute is not real
*/
bool TerminalData::fromXml (const QDomElement &xml_element)
{
	qreal term_x = 0.0;
	qreal term_y = 0.0;

	// reads the position of the terminal
	// lit la position de la borne
	if (!QET::attributeIsAReal(xml_element, "x", &term_x))
		return false;

	if (!QET::attributeIsAReal(xml_element, "y", &term_y))
		return false;

	m_pos = QPointF(term_x, term_y);

	//emit posFromXML(QPointF(term_x, term_y));

	QString uuid = xml_element.attribute("uuid");
	// update part and add uuid, which is used in the new version
	// to connect terminals together
	// if the attribute not exists, means, the element is created with an
	// older version of qet. So use the legacy approach
	// to identify terminals
	if (!uuid.isEmpty())
	m_uuid = QUuid(uuid);

	m_name = xml_element.attribute("name");

	// read the orientation of the terminal
	// lit l'orientation de la borne
	m_orientation = Qet::orientationFromString(
				xml_element.attribute("orientation"));

	m_type = typeFromString(xml_element.attribute("type"));

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
	return QString("Generic");
}

/**
 * @brief TerminalData::typeFromString
 * @param string
 * @return The type describe in string to TerminalData::Type.
 * if string doesn't describe a type, TerminalData::Generic is returned
 */
TerminalData::Type TerminalData::typeFromString(const QString &string)
{
	if (string.isEmpty() ||
		string == "Generic") {
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
