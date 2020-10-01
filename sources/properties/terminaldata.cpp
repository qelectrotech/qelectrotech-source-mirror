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

void TerminalData::toSettings(QSettings& settings, const QString&) const

{
	Q_UNUSED(settings);
}

void TerminalData::fromSettings(const QSettings& settings, const QString&)
{
	Q_UNUSED(settings);
}

QDomElement TerminalData::toXml(QDomDocument &xml_document) const
{
    QDomElement xml_element = xml_document.createElement("terminaldata");

    xml_element.appendChild(createXmlProperty(xml_document, "x", q->scenePos().x()));
    xml_element.appendChild(createXmlProperty(xml_document, "y", q->scenePos().y()));
    xml_element.appendChild(createXmlProperty(xml_document, "uuid", m_uuid));
    xml_element.appendChild(createXmlProperty(xml_document, "name", m_name));
    xml_element.appendChild(createXmlProperty(xml_document, "orientation", orientationToString(m_orientation)));

    return(xml_element);
}
bool TerminalData::fromXml (const QDomElement &xml_element) // RETURNS True
{
    // lit la position de la borne
    qreal term_x = 0.0, term_y = 0.0;

    if (propertyDouble(xml_element, "x", &term_x))
        return false;

    if (propertyDouble(xml_element, "y", &term_y))
        return false;

    m_pos = QPointF(term_x, term_y);

//    emit posFromXML(QPointF(term_x, term_y));

    QUuid uuid;
    // update part and add uuid, which is used in the new version to connect terminals together
    // if the attribute not exists, means, the element is created with an older version of qet. So use the legacy approach
    // to identify terminals
    if (propertyUuid(xml_element, "uuid", &uuid) == PropertyFlags::Success)
        m_uuid = QUuid(uuid);


    //if (propertyString(xml_element, "name", &m_name))
    //    return false;
    propertyString(xml_element, "name", &m_name); // some parts do not have a name. Example: affuteuse_250h.qet, Terminal at x="0" y="-20"

    QString o;
    if (propertyString(xml_element, "orientation", &o))
        return false;

    // lit l'orientation de la borne
    m_orientation = orientationFromString(o);

    return true;
}

bool TerminalData::valideXml(const QDomElement& xml_element) {
    if (propertyDouble(xml_element, "x"))
        return false;

    if (propertyDouble(xml_element, "y"))
        return false;

      // legacy elements do not have an uuid
//    if (propertyUuid(xml_element, "uuid"))
//        return false;

    //if (propertyString(xml_element, "name")) // some parts do not have a name. Example: affuteuse_250h.qet, Terminal at x="0" y="-20"
    //    return false;

    if (propertyString(xml_element, "orientation"))
        return false;
    return true;
}
