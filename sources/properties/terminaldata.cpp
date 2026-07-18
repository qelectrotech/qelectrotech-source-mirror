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

	if (m_show_name) {
		xml_element.setAttribute("show_name", "true");
		xml_element.setAttribute("label_x", QString::number(m_label_pos.x()));
		xml_element.setAttribute("label_y", QString::number(m_label_pos.y()));
		xml_element.setAttribute("label_font", m_label_font.toString());
		xml_element.setAttribute("label_rotation", QString::number(m_label_rotation));
		xml_element.setAttribute("label_halign", static_cast<int>(m_label_halignment));
		xml_element.setAttribute("label_valign", static_cast<int>(m_label_valignment));
		if (m_label_frame)
			xml_element.setAttribute("label_frame", "true");
		if (m_label_color != QColor(Qt::black))
			xml_element.setAttribute("label_color", m_label_color.name());
	}

	// Save master label override settings
	if (m_use_master_label) {
		xml_element.setAttribute("use_master_label", "true");
		xml_element.setAttribute("master_label_index", m_master_label_index);
	}

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

	m_show_name = (xml_element.attribute("show_name") == QLatin1String("true"));
	if (m_show_name) {
		qreal lx = xml_element.attribute("label_x", "0").toDouble();
		qreal ly = xml_element.attribute("label_y", "0").toDouble();
		m_label_pos = QPointF(lx, ly);

		QString font_str = xml_element.attribute("label_font");
		if (!font_str.isEmpty())
			m_label_font.fromString(font_str);

		m_label_rotation = xml_element.attribute("label_rotation", "0").toDouble();

		int ha = xml_element.attribute("label_halign", "0").toInt();
		m_label_halignment = static_cast<Qt::Alignment>(ha);

		int va = xml_element.attribute("label_valign", "0").toInt();
		m_label_valignment = static_cast<Qt::Alignment>(va);

		m_label_frame = (xml_element.attribute("label_frame") == QLatin1String("true"));

		QString color_str = xml_element.attribute("label_color");
		if (!color_str.isEmpty())
			m_label_color = QColor(color_str);
	}

	// Read master label override settings
	m_use_master_label = (xml_element.attribute("use_master_label") == QLatin1String("true"));
	m_master_label_index = xml_element.attribute("master_label_index", "0").toInt();

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
		case No :
			return QString("No");
		case Nc :
			return QString("Nc");
		case Common :
			return QString("Common");
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
	} else if (string == "No") {
		return TerminalData::No;
	} else if (string == "Nc") {
		return TerminalData::Nc;
	} else if (string == "Common") {
		return TerminalData::Common;
	} else {
		qDebug() << "TerminalData::typeFromString, argument string is invalid"
					" failsafe type 'TerminalData::Generic' is returned";
		return TerminalData::Generic;
	}
}
