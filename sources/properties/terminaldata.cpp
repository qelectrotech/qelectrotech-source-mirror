#include "terminaldata.h"

#include <QGraphicsObject>

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
	Save properties to setting file.

	QString is use for prefix a word befor the name of each paramètre
	@param settings UNUSED
	@param prefix UNUSED
*/
void TerminalData::toSettings(QSettings &settings, const QString prefix) const

{
	Q_UNUSED(settings);
	Q_UNUSED(prefix);
}

/**
	@brief TerminalData::fromSettings
	load properties to setting file.

	QString is use for prefix a word befor the name of each paramètre
	@param settings UNUSED
	@param prefix UNUSED
*/
void TerminalData::fromSettings(const QSettings &settings, const QString prefix)
{
	Q_UNUSED(settings);
	Q_UNUSED(prefix);
}

/**
	@brief TerminalData::toXml
	Save properties to xml element
	write the name, number, position and orientation of the terminal
	to xml_element

	@note This method is only called from the PartTerminal
	and should never called from the Terminal class
	@param xml_document
	@return xml_element : DomElement with
	the name, number, position and orientation of the terminal
*/
QDomElement TerminalData::toXml(QDomDocument &xml_document) const
{
	QDomElement xml_element = xml_document.createElement("terminal");

	// write the position of the terminal
	// ecrit la position de la borne
	xml_element.setAttribute("x", QString("%1").arg(q->scenePos().x()));
	xml_element.setAttribute("y", QString("%1").arg(q->scenePos().y()));

	// Write name and number to XML
	xml_element.setAttribute("uuid", m_uuid.toString());
	xml_element.setAttribute("name", m_name);

	// write the orientation of the terminal
	// ecrit l'orientation de la borne
	xml_element.setAttribute("orientation",
				 Qet::orientationToString(m_orientation));

	return(xml_element);
}

/**
	@brief TerminalData::fromXml
	load properties to xml element

	@note This method is only called from the PartTerminal
	and should never called from the Terminal class
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

	return true;
}
