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
    QDomElement xml_element = xml_document.createElement("terminaldata");

    // write the position of the terminal
    // Write name and number to XML
    // m_pos cannot be stored, because in the partterminal it will not be updated.
    // In PartTerminal m_pos is the position of the dock, in Terminal m_pos is the second side of the terminal
    // This is hold for legacy compability reason
    xml_element.appendChild(createXmlProperty(xml_document, "x", m_pos.x()));
    xml_element.appendChild(createXmlProperty(xml_document, "y", m_pos.y()));
    xml_element.appendChild(createXmlProperty(xml_document, "name", m_name));
    xml_element.appendChild(createXmlProperty(xml_document, "orientation", orientationToString(m_orientation)));

    return(xml_element);
}

/*
    @brief TerminalData::fromXml
    load properties to xml element

    @note This method is only called from the PartTerminal
    and should never called from the Terminal class
    @param xml_element
    @return true if succeeded / false if the attribute is not real
*/
bool TerminalData::fromXml (const QDomElement &xml_element) // RETURNS True
{
    qreal term_x = 0.0;
    qreal term_y = 0.0;

    // reads the position of the terminal
    // lit la position de la borne

    if (propertyDouble(xml_element, "x", &term_x))
        return false;

    if (propertyDouble(xml_element, "y", &term_y))
        return false;

    m_pos = QPointF(term_x, term_y);

//    emit posFromXML(QPointF(term_x, term_y));

    // do not write uuid from this class, because only PartTerminal::fromXml need
    // to write it to xml file. Terminal::fromXml does not need.
    // if the attribute not exists, means, the element is created with an
    // older version of qet. So use the legacy approach


    //if (propertyString(xml_element, "name", &m_name))
    //    return false;
    propertyString(xml_element, "name", &m_name); // some parts do not have a name. Example: affuteuse_250h.qet, Terminal at x="0" y="-20"

    QString o;
    if (propertyString(xml_element, "orientation", &o))
        return false;

    // read the orientation of the terminal
    // lit l'orientation de la borne
    m_orientation = Qet::orientationFromString(
                xml_element.attribute("orientation"));
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
