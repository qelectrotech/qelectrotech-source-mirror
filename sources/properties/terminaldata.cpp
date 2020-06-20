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

void TerminalData::init() {
}

TerminalData::~TerminalData()
{

}

void TerminalData::setParent(QGraphicsObject* parent)
{
    q = parent;
}

void TerminalData::toSettings(QSettings &settings, const QString) const

{
	Q_UNUSED(settings);
}

void TerminalData::fromSettings(const QSettings &settings, const QString)
{
	Q_UNUSED(settings);
}

QDomElement TerminalData::toXml(QDomDocument &xml_document) const
{
    QDomElement xml_element = xml_document.createElement("terminal");

    // ecrit la position de la borne
    xml_element.setAttribute("x", QString("%1").arg(q->scenePos().x()));
    xml_element.setAttribute("y", QString("%1").arg(q->scenePos().y()));


    xml_element.setAttribute("uuid", m_uuid.toString());
    xml_element.setAttribute("name", m_name);

    // ecrit l'orientation de la borne
    xml_element.setAttribute("orientation", Qet::orientationToString(m_orientation));
    // Write name and number to XML

    return(xml_element);
}
bool TerminalData::fromXml (const QDomElement &xml_element)
{
    // lit la position de la borne
    qreal term_x = 0.0, term_y = 0.0;
    if (!QET::attributeIsAReal(xml_element, "x", &term_x))
        return false;

    if (!QET::attributeIsAReal(xml_element, "y", &term_y))
        return false;

    m_pos = QPointF(term_x, term_y);

    //emit posFromXML(QPointF(term_x, term_y));

    QString uuid = xml_element.attribute("uuid");
    // update part and add uuid, which is used in the new version to connect terminals together
    // if the attribute not exists, means, the element is created with an older version of qet. So use the legacy approach
    // to identify terminals
    if (!uuid.isEmpty())
        m_uuid = QUuid(uuid);

    m_name = xml_element.attribute("name");

    // lit l'orientation de la borne
    m_orientation = Qet::orientationFromString(xml_element.attribute("orientation"));

    return true;
}
