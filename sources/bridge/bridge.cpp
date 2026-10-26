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
#include "bridge.h"

#include "../qetproject.h"
#include "../qetxml.h"
#include "bridgemanager.h"

#include <QDomDocument>

/**
	@brief Bridge::Bridge
	@param parent
*/
Bridge::Bridge(QObject *parent) :
	QObject(parent),
	m_uuid(QUuid::createUuid())
{
}

/**
	@return the identity of this bridge
*/
QUuid Bridge::uuid() const {
	return m_uuid;
}

/**
	@param uuid
*/
void Bridge::setUuid(const QUuid &uuid) {
	m_uuid = uuid;
}

/**
	Give this bridge a brand new identity: used when a drawn bridge is
	duplicated, so that two bridges never share one identity.
*/
void Bridge::newUuid() {
	m_uuid = QUuid::createUuid();
}

/**
	@return the colour this bridge is drawn in, black unless the user
	chose another one
*/
QColor Bridge::color() const {
	return m_color;
}

/**
	@param color
*/
void Bridge::setColor(const QColor &color)
{
	if (color == m_color) {
		return;
	}
	m_color = color;
	emit changed();
}

/**
	@return how thick the lines of this bridge are drawn, in the same
	way a conductor takes its size
*/
qreal Bridge::lineWidth() const {
	return m_line_width;
}

/**
	@param width
*/
void Bridge::setLineWidth(qreal width)
{
	const qreal clamped = qMax(width, 0.1);
	if (qFuzzyCompare(clamped, m_line_width)) {
		return;
	}
	m_line_width = clamped;
	emit changed();
}

/**
	@return how many terminals this bridge may hold,
	unlimited_connections (0) when there is no limit
*/
int Bridge::maxConnections() const {
	return m_max_connections;
}

/**
	@param max the new limit, unlimited_connections (0) for none
*/
void Bridge::setMaxConnections(int max)
{
	const int clamped = qMax(max, 0);
	if (clamped == m_max_connections) {
		return;
	}
	m_max_connections = clamped;
	emit changed();
}

/**
	@param adding how many terminals would be added to this bridge
	@return true when the bridge may not take that many: the limit of
	its connections is the one thing which is never crossed, whatever is
	drawn afterwards (a bridge which cannot be built in the cabinet is
	not drawn either)
*/
bool Bridge::wouldExceed(int adding) const
{
	if (m_max_connections == unlimited_connections) {
		return false;
	}
	return (klemmCount() + qMax(adding, 0)) > m_max_connections;
}

/**
	@return the parallel track this bridge runs in at the terminals it
	crosses, 1 to max_columns, 0 when it was never worked out
*/
int Bridge::column() const {
	return m_column;
}

/**
	@param column
*/
void Bridge::setColumn(int column)
{
	const int clamped = qBound(0, column, max_columns);
	if (clamped == m_column) {
		return;
	}
	m_column = clamped;
	emit changed();
}

/**
	@return the name the hand gave this bridge, empty when nobody did
*/
QString Bridge::name() const {
	return m_name;
}

/**
	@param name
*/
void Bridge::setName(const QString &name)
{
	if (name == m_name) {
		return;
	}
	m_name = name;
	emit changed();
}

/**
	@return how many Klemmen this bridge holds. A sheet reference it is
	carried on by is no Klemme: it never counts as one, and never fills
	one of the places the bridge holds.
*/
int Bridge::klemmCount() const {
	return terminals().size();
}

/**
	@return the voltage/protocol this bridge carries: what the wires at
	its terminals say, read live, so the bridge follows them the way
	they follow each other. What was answered when it was drawn stands
	in for the moment they say nothing.
*/
QString Bridge::tensionText() const {
	const QString tension = liveTension();
	return tension.isEmpty() ? m_tension_text : tension;
}

/**
	@param text
*/
void Bridge::setTensionText(const QString &text)
{
	if (text == m_tension_text) {
		return;
	}
	m_tension_text = text;
	emit changed();
}

/**
	@brief Bridge::liveTension
	@return what the wires at the first terminal of this bridge say at
	this very moment -- the bridge sits in the cabinet where that
	potential is and follows it
*/
QString Bridge::liveTension() const
{
	auto *project = qobject_cast<QETProject *>(parent());
	if (!project || m_terminals.isEmpty()) {
		return {};
	}
	return BridgeManager::informationOf(project, m_terminals.first()).tension;
}

/**
	@return every place this bridge is plugged in: the Klemmen and the
	sheet references alike, in the order they were drawn over
*/
QList<BridgeTerminal> Bridge::landings() const {
	return m_terminals;
}

/**
	@return the Klemmen this bridge lands on. A sheet reference is the
	way to the next sheet and never an Anschluss of the bridge: it is
	not in here and never counted as one.
*/
QList<BridgeTerminal> Bridge::terminals() const {
	QList<BridgeTerminal> list;
	for (const BridgeTerminal &terminal : m_terminals) {
		if (!terminal.reference) {
			list << terminal;
		}
	}
	return list;
}

/**
	@return the sheet references this bridge is carried on by: the way
	to the next sheet, kept apart from its Anschluesse
*/
QList<BridgeTerminal> Bridge::references() const {
	QList<BridgeTerminal> list;
	for (const BridgeTerminal &terminal : m_terminals) {
		if (terminal.reference) {
			list << terminal;
		}
	}
	return list;
}

/**
	@param terminal
	@return true when this bridge already lands on that terminal
*/
bool Bridge::hasTerminal(const BridgeTerminal &terminal) const {
	return m_terminals.contains(terminal);
}

/**
	@param terminal
*/
void Bridge::addTerminal(const BridgeTerminal &terminal)
{
	if (m_terminals.contains(terminal)) {
		return;
	}
	m_terminals.append(terminal);
	emit changed();
}

/**
	@param terminal
*/
void Bridge::removeTerminal(const BridgeTerminal &terminal)
{
	if (!m_terminals.removeOne(terminal)) {
		return;
	}
	emit changed();
}

/**
	@param terminals
*/
void Bridge::setTerminals(const QList<BridgeTerminal> &terminals)
{
	if (m_terminals == terminals) {
		return;
	}
	m_terminals = terminals;
	emit changed();
}

/**
	@return how many terminals this bridge lands on
*/
	/**
		@return how many Klemmen this bridge lands on: a sheet reference
		is no Anschluss and never counted here
	*/
int Bridge::terminalCount() const {
	return terminals().size();
}

/**
	@return every section this bridge is drawn as, one per folio it may
	have a line on
*/
QList<BridgePartData> Bridge::parts() const {
	return m_parts;
}

/**
	@param part
	@return true when this bridge holds that section
*/
bool Bridge::hasPart(const QUuid &part) const
{
	for (const BridgePartData &data : m_parts) {
		if (data.uuid == part) {
			return true;
		}
	}
	return false;
}

/**
	@param part
*/
void Bridge::addPart(const BridgePartData &part)
{
	if (hasPart(part.uuid)) {
		return;
	}
	m_parts.append(part);
	emit changed();
}

/**
	@param part
*/
void Bridge::removePart(const QUuid &part)
{
	for (int i = 0 ; i < m_parts.size() ; ++i) {
		if (m_parts.at(i).uuid == part) {
			m_parts.removeAt(i);
			emit changed();
			return;
		}
	}
}

/**
	@param part
	@param p1
	@param p2
*/
void Bridge::setPartGeometry(const QUuid &part, const QPointF &p1, const QPointF &p2)
{
	for (BridgePartData &data : m_parts) {
		if (data.uuid == part) {
			if (data.p1 == p1 && data.p2 == p2) {
				return;
			}
			data.p1 = p1;
			data.p2 = p2;
			emit changed();
			return;
		}
	}
}

/**
	@brief Bridge::toXml
	@param document
	@return the xml description of this bridge
*/
QDomElement Bridge::toXml(QDomDocument &document) const
{
	auto root = document.createElement(xmlTagName());
	root.setAttribute(QStringLiteral("uuid"), m_uuid.toString());
	root.setAttribute(QStringLiteral("color"), m_color.name());
	if (!qFuzzyCompare(m_line_width, default_line_width)) {
		root.setAttribute(QStringLiteral("line_width"), QString::number(m_line_width));
	}
	if (m_max_connections != unlimited_connections) {
		root.setAttribute(QStringLiteral("max_connections"), m_max_connections);
	}
	if (m_column) {
		root.setAttribute(QStringLiteral("column"), m_column);
	}
	if (!m_tension_text.isEmpty()) {
		root.setAttribute(QStringLiteral("tension_protocol"), m_tension_text);
	}
	if (!m_name.isEmpty()) {
		root.setAttribute(QStringLiteral("name"), m_name);
	}

	auto xml_terminals = document.createElement(QStringLiteral("terminals"));
	for (const BridgeTerminal &terminal : m_terminals) {
		auto xml_terminal = document.createElement(QStringLiteral("terminal"));
		xml_terminal.setAttribute(QStringLiteral("element"), terminal.element.toString());
		if (!terminal.terminal.isEmpty()) {
			xml_terminal.setAttribute(QStringLiteral("name"), terminal.terminal);
		}
		if (terminal.reference) {
			xml_terminal.setAttribute(QStringLiteral("reference"), QStringLiteral("1"));
		}
		xml_terminals.appendChild(xml_terminal);
	}
	root.appendChild(xml_terminals);

	auto xml_parts = document.createElement(QStringLiteral("parts"));
	for (const BridgePartData &part : m_parts) {
		auto xml_part = document.createElement(QStringLiteral("part"));
		xml_part.setAttribute(QStringLiteral("uuid"), part.uuid.toString());
		xml_part.setAttribute(QStringLiteral("diagram"), part.diagram.toString());
		xml_part.setAttribute(QStringLiteral("x1"), QString::number(part.p1.x()));
		xml_part.setAttribute(QStringLiteral("y1"), QString::number(part.p1.y()));
		xml_part.setAttribute(QStringLiteral("x2"), QString::number(part.p2.x()));
		xml_part.setAttribute(QStringLiteral("y2"), QString::number(part.p2.y()));
		xml_parts.appendChild(xml_part);
	}
	root.appendChild(xml_parts);

	return root;
}

/**
	@brief Bridge::fromXml
	@param element
*/
void Bridge::fromXml(const QDomElement &element)
{
	if (element.tagName() != xmlTagName()) {
		return;
	}

	m_uuid = QUuid(element.attribute(QStringLiteral("uuid")));
	if (m_uuid.isNull()) {
		newUuid();
	}
	m_color = QColor(element.attribute(QStringLiteral("color"), QStringLiteral("#000000")));
	m_line_width = element.attribute(QStringLiteral("line_width")).toDouble();
	if (m_line_width <= 0.0) {
		m_line_width = default_line_width;
	}
	m_max_connections = qMax(0, element.attribute(QStringLiteral("max_connections")).toInt());
	m_column = qBound(0, element.attribute(QStringLiteral("column")).toInt(), max_columns);
	m_tension_text = element.attribute(QStringLiteral("tension_protocol"));
	m_name = element.attribute(QStringLiteral("name"));

	m_terminals.clear();
	auto xml_terminals = element.firstChildElement(QStringLiteral("terminals"));
	for (auto xml_terminal = xml_terminals.firstChildElement(QStringLiteral("terminal")) ;
		 !xml_terminal.isNull() ;
		 xml_terminal = xml_terminal.nextSiblingElement(QStringLiteral("terminal")))
	{
		BridgeTerminal terminal;
		terminal.element = QUuid(xml_terminal.attribute(QStringLiteral("element")));
		terminal.terminal = xml_terminal.attribute(QStringLiteral("name"));
		terminal.reference = xml_terminal.attribute(QStringLiteral("reference")) == QLatin1String("1");
		if (!terminal.element.isNull()) {
			m_terminals.append(terminal);
		}
	}

	m_parts.clear();
	auto xml_parts = element.firstChildElement(QStringLiteral("parts"));
	for (auto xml_part = xml_parts.firstChildElement(QStringLiteral("part")) ;
		 !xml_part.isNull() ;
		 xml_part = xml_part.nextSiblingElement(QStringLiteral("part")))
	{
		BridgePartData part;
		part.uuid = QUuid(xml_part.attribute(QStringLiteral("uuid")));
		if (part.uuid.isNull()) {
			part.uuid = QUuid::createUuid();
		}
		part.diagram = QUuid(xml_part.attribute(QStringLiteral("diagram")));
		part.p1 = QPointF(xml_part.attribute(QStringLiteral("x1")).toDouble(),
						  xml_part.attribute(QStringLiteral("y1")).toDouble());
		part.p2 = QPointF(xml_part.attribute(QStringLiteral("x2")).toDouble(),
						  xml_part.attribute(QStringLiteral("y2")).toDouble());
		m_parts.append(part);
	}

	emit changed();
}
