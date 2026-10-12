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
#include "cable.h"

#include <QDomDocument>
#include <QDomElement>

#include <algorithm>

namespace {
	/**
		@brief The name an alignment is written under in the file, an
		empty string when the alignment is the plain default (the text
		ends where it always did, before the line).
		@param alignment Qt::AlignLeft, Qt::AlignHCenter or Qt::AlignRight
		@return left, center or right
	*/
	QString alignmentName(int alignment)
	{
		switch (alignment)
		{
		case int(Qt::AlignLeft): return QStringLiteral("left");
		case int(Qt::AlignHCenter): return QStringLiteral("center");
		case int(Qt::AlignRight): return QStringLiteral("right");
		default: return QString();
		}
	}

	/**
		@brief The alignment a name written in the file stands for.
		@param name left, center or right
		@return the Qt flags, 0 when the name says nothing
	*/
	int alignmentFromName(const QString &name)
	{
		if (name == QLatin1String("left")) return int(Qt::AlignLeft);
		if (name == QLatin1String("center")) return int(Qt::AlignHCenter);
		if (name == QLatin1String("right")) return int(Qt::AlignRight);
		return 0;
	}
}

/**
	@brief Cable::Cable
	@param parent
*/
Cable::Cable(QObject *parent) :
	QObject(parent),
	m_uuid(QUuid::createUuid())
{
}

/**
	@brief Cable::uuid
	@return the identity of the cable, never empty once constructed
*/
QUuid Cable::uuid() const
{
	return m_uuid;
}

/**
	@brief Cable::setUuid
	Only used while reading a project: a cable keeps the identity it was
	saved with, so the conductors it feeds stay bound to it.
	@param uuid
*/
void Cable::setUuid(const QUuid &uuid)
{
	if (uuid.isNull() || uuid == m_uuid) {
		return;
	}
	m_uuid = uuid;
	emit changed();
}

/**
	@brief Cable::designation
	@return the cable number, e.g. 15W1, which heads the label drawn next
	to the trunk line and opens every core label
*/
QString Cable::designation() const
{
	return m_designation;
}

/**
	@brief Cable::setDesignation
	@param designation
*/
void Cable::setDesignation(const QString &designation)
{
	if (m_designation == designation) return;
	m_designation = designation;
	emit changed();
}

/**
	@brief Cable::designationByHand
	@return true when the name of this cable was typed in rather than
	worked out of the numbering rule
*/
bool Cable::designationByHand() const
{
	return m_designation_by_hand;
}

/**
	@brief Cable::setDesignationByHand
	Says where the name of this cable comes from. Nothing of it is
	drawn, so it changes no label and asks for no redraw.
	@param by_hand
*/
void Cable::setDesignationByHand(bool by_hand)
{
	m_designation_by_hand = by_hand;
}

/**
	@brief Cable::bmk
	@return the BMK of the cable
*/
QString Cable::bmk() const
{
	return m_bmk;
}

/**
	@brief Cable::setBmk
	@param bmk
*/
void Cable::setBmk(const QString &bmk)
{
	if (m_bmk == bmk) return;
	m_bmk = bmk;
	emit changed();
}

/**
	@brief Cable::installation
	@return the installation (Anlage) the cable belongs to
*/
QString Cable::installation() const
{
	return m_installation;
}

/**
	@brief Cable::setInstallation
	@param installation
*/
void Cable::setInstallation(const QString &installation)
{
	if (m_installation == installation) return;
	m_installation = installation;
	emit changed();
}

/**
	@brief Cable::location
	@return the location (Ort) the cable runs through
*/
QString Cable::location() const
{
	return m_location;
}

/**
	@brief Cable::setLocation
	@param location
*/
void Cable::setLocation(const QString &location)
{
	if (m_location == location) return;
	m_location = location;
	emit changed();
}

/**
	@brief Cable::length
	@return the length of the cable as it is written down: a free text,
	because it may come from the drawing or be typed in by hand
*/
QString Cable::length() const
{
	return m_length;
}

/**
	@brief Cable::setLength
	@param length
*/
void Cable::setLength(const QString &length)
{
	if (m_length == length) return;
	m_length = length;
	emit changed();
}

/**
	@brief Cable::properties
	@return every field the user fills in, together with what of it he
	want to see drawn next to the trunk line
*/
CableProperties Cable::properties() const
{
	CableProperties properties;
	properties.designation = m_designation;
	properties.designation_by_hand = m_designation_by_hand;
	properties.type = m_type;
	properties.installation = m_installation;
	properties.location = m_location;
	properties.length = m_length;
	properties.show_designation = m_show_designation;
	properties.show_type = m_show_type;
	properties.show_installation = m_show_installation;
	properties.show_location = m_show_location;
	properties.show_length = m_show_length;
	properties.texts = m_texts;
	properties.text_offset = m_text_offset;
	properties.core_offset = m_core_offset;
	return properties;
}

/**
	@brief Cable::setProperties
	Write a whole set of fields at once -- one undo step covers it, and
	the label next to the trunk line and the conductors it feeds are
	worked out again once, not ten times.
	@param properties
*/
void Cable::setProperties(const CableProperties &properties)
{
	if (properties == this->properties()) return;

	m_designation = properties.designation;
	m_designation_by_hand = properties.designation_by_hand;
	m_type = properties.type;
	m_installation = properties.installation;
	m_location = properties.location;
	m_length = properties.length;
	m_show_designation = properties.show_designation;
	m_show_type = properties.show_type;
	m_show_installation = properties.show_installation;
	m_show_location = properties.show_location;
	m_show_length = properties.show_length;
	m_texts = properties.texts;
	m_text_offset = properties.text_offset;
	m_core_offset = properties.core_offset;
	emit changed();
}

/**
	@brief Cable::type
	@return the type of the cable, e.g. H07RN-F 4G1,5. The type is copied
	from the cable type file when the cable is created, so a project
	opens even when that file is gone or has changed since.
*/
QString Cable::type() const
{
	return m_type;
}

/**
	@brief Cable::setType
	@param type
*/
void Cable::setType(const QString &type)
{
	if (m_type == type) return;
	m_type = type;
	emit changed();
}

/**
	@brief Cable::coreCount
	@return how many cores the type of this cable has. This is only the
	size of the type, not the number of cores actually wired: see
	usedCoreCount().
*/
int Cable::coreCount() const
{
	return m_core_count;
}

/**
	@brief Cable::setCoreCount
	@param count
*/
void Cable::setCoreCount(int count)
{
	count = qMax(0, count);
	if (m_core_count == count) return;
	m_core_count = count;

		//A type with fewer cores takes the cores it no longer has away
		//from the conductors which showed them: the label of a core the
		//cable doesn't own would be a lie.
	for (int i = m_cores.size() - 1; i >= 0; --i) {
		if (m_cores.at(i).core >= m_core_count) {
			m_cores.removeAt(i);
		}
	}
	emit changed();
}

/**
	@brief Cable::coreColors
	@return the colour of each core, in the order of the cable type file
*/
QStringList Cable::coreColors() const
{
	return m_core_colors;
}

/**
	@brief Cable::setCoreColors
	@param colors
*/
void Cable::setCoreColors(const QStringList &colors)
{
	if (m_core_colors == colors) return;
	m_core_colors = colors;
	emit changed();
}

/**
	@brief Cable::typeState
	@return everything the type of this cable decides, taken in one
	block: changing the type works on that block, and undo puts the very
	same block back -- name, size, colours and the cores which go with
	them, so no mark can be left standing on a core the cable lost.
*/
CableTypeState Cable::typeState() const
{
	CableTypeState state;
	state.type = m_type;
	state.core_count = m_core_count;
	state.core_colors = m_core_colors;
	state.cores = m_cores;
	return state;
}

/**
	@brief Cable::applyTypeState
	Put back a state taken with typeState().

	The count comes before the cores: a cable which has fewer cores than
	it had cannot hold the others, and setCores() drops what stands
	beyond the count -- which is exactly what a type with fewer cores
	means, and exactly what undo has to take back.
	@param state
*/
void Cable::applyTypeState(const CableTypeState &state)
{
	setType(state.type);
	setCoreColors(state.core_colors);
	setCoreCount(state.core_count);
	setCores(state.cores);
}

/**
	@brief Cable::usedCores
	@return every core wired to a conductor, ordered by core number
*/
QList<CableCore> Cable::usedCores() const
{
	QList<CableCore> cores = m_cores;
	std::sort(cores.begin(), cores.end(),
			  [](const CableCore &a, const CableCore &b) {return a.core < b.core;});
	return cores;
}

/**
	@brief Cable::hasCore
	@param core
	@return true when that core is wired to a conductor
*/
bool Cable::hasCore(int core) const
{
	for (const CableCore &c : m_cores) {
		if (c.core == core) return true;
	}
	return false;
}

/**
	@brief Cable::core
	@param core
	@return the assignment of that core, an empty one when it is free
*/
CableCore Cable::core(int core) const
{
	for (const CableCore &c : m_cores) {
		if (c.core == core) return c;
	}
	return CableCore();
}

/**
	@brief Cable::setCore
	Wire a core to a conductor, or move the slash of an already wired
	core. A core number may only appear once, so the previous holder of
	that number, if any, is replaced.
	@param core
*/
void Cable::setCore(const CableCore &core)
{
	if (core.core < 0 || core.core >= m_core_count) return;

	for (int i = 0; i < m_cores.size(); ++i) {
		if (m_cores.at(i).core == core.core) {
			if (m_cores.at(i).conductor == core.conductor
				&& m_cores.at(i).part == core.part
				&& m_cores.at(i).position == core.position) {
				return;
			}
			m_cores[i] = core;
			emit changed();
			return;
		}
	}
	m_cores.append(core);
	emit changed();
}

/**
	@brief Cable::setCores
	Write a whole set of assignments in place of the current one, the way
	undoing a dragged slash has to: every core which was moved, swapped
	or dropped comes back at once.
	@param cores
*/
void Cable::setCores(const QList<CableCore> &cores)
{
	m_cores = cores;
	for (int i = m_cores.size() - 1; i >= 0; --i) {
		if (m_cores.at(i).core < 0 || m_cores.at(i).core >= m_core_count) {
			m_cores.removeAt(i);
		}
	}
	emit changed();
}

/**
	@brief Cable::removeCore
	Wire a core back to nothing: its conductor stops showing the label
	this cable gave it.
	@param core
*/
void Cable::removeCore(int core)
{
	for (int i = 0; i < m_cores.size(); ++i) {
		if (m_cores.at(i).core == core) {
			m_cores.removeAt(i);
			emit changed();
			return;
		}
	}
}

/**
	@brief Cable::usedCoreCount
	@return how many cores of this cable are wired to a conductor
*/
int Cable::usedCoreCount() const
{
	return m_cores.size();
}

/**
	@brief Cable::freeCoreCount
	@return how many cores the type offers and no conductor shows yet
*/
int Cable::freeCoreCount() const
{
	return qMax(0, m_core_count - m_cores.size());
}

/**
	@brief Cable::firstFreeCore
	@return the lowest core number which is free, -1 when every core of
	the type is already wired
*/
int Cable::firstFreeCore() const
{
	for (int i = 0; i < m_core_count; ++i) {
		if (!hasCore(i)) return i;
	}
	return -1;
}

/**
	@brief Cable::colorOfCore
	@param core
	@return the colour this core has in the cable type file, an empty
	string when the type doesn't describe that many cores
*/
QString Cable::colorOfCore(int core) const
{
	return m_core_colors.value(core);
}

/**
	@brief Cable::labelOfCore
	The text written into the cable field of the conductor this core is
	wired to. It is never typed by hand: it is worked out here, from the
	cable and the core, so renaming the cable or changing its type
	updates every conductor at once.
	@param core
	@return e.g. "15W1:br"
*/
QString Cable::labelOfCore(int core) const
{
	if (m_designation.isEmpty()) {
		return QString();
	}
	const QString color = colorOfCore(core);
	if (color.isEmpty()) {
		return m_designation;
	}
	return m_designation + QLatin1Char(':') + color;
}

/**
	@brief Cable::parts
	@return every drawn section of this cable, whatever folio it sits on
*/
QList<CablePartData> Cable::parts() const
{
	return m_parts;
}

/**
	@brief Cable::hasPart
	@param part
	@return true when the cable has a section with that identity
*/
bool Cable::hasPart(const QUuid &part) const
{
	if (part.isNull()) return false;
	for (const CablePartData &p : m_parts) {
		if (p.uuid == part) return true;
	}
	return false;
}

/**
	@brief Cable::part
	@param part
	@return the section with that identity, an invalid one when there is none
*/
CablePartData Cable::part(const QUuid &part) const
{
	for (const CablePartData &p : m_parts) {
		if (p.uuid == part) return p;
	}
	return CablePartData();
}

/**
	@brief Cable::addPart
	@param part
*/
void Cable::addPart(const CablePartData &part)
{
	if (part.uuid.isNull() || hasPart(part.uuid)) return;
	m_parts.append(part);
	emit changed();
}

/**
	@brief Cable::removePart
	Also drops the cores drawn on that section, since their slash and
	their colour label live with it.
	@param part
*/
void Cable::removePart(const QUuid &part)
{
	bool touched = false;
	for (int i = m_parts.size() - 1; i >= 0; --i) {
		if (m_parts.at(i).uuid == part) {
			m_parts.removeAt(i);
			touched = true;
		}
	}
	for (int i = m_cores.size() - 1; i >= 0; --i) {
		if (m_cores.at(i).part == part) {
			m_cores.removeAt(i);
			touched = true;
		}
	}
	if (touched) {
		emit changed();
	}
}

/**
	@brief Cable::setPartGeometry
	Moving or lengthening the trunk line is a purely optical change: no
	core is added, none is removed, nothing is reassigned. Only the line
	itself moves -- which is why this never emits changed(): the labels
	the cable writes into the conductors do not depend on it.
	@param part
	@param p1
	@param p2
*/
void Cable::setPartGeometry(const QUuid &part, const QPointF &p1, const QPointF &p2)
{
	for (int i = 0; i < m_parts.size(); ++i) {
		if (m_parts.at(i).uuid != part) continue;
		if (m_parts.at(i).p1 == p1 && m_parts.at(i).p2 == p2) return;
		m_parts[i].p1 = p1;
		m_parts[i].p2 = p2;
		return;
	}
}

/**
	@brief Cable::setPartDiagram
	@param part
	@param diagram
*/
void Cable::setPartDiagram(const QUuid &part, const QUuid &diagram)
{
	for (int i = 0; i < m_parts.size(); ++i) {
		if (m_parts.at(i).uuid != part) continue;
		if (m_parts.at(i).diagram == diagram) return;
		m_parts[i].diagram = diagram;
		emit changed();
		return;
	}
}

/**
	@brief Cable::movePartCores
	Carry the colour labels of one section along when that section is
	moved as a whole: when a copy is put down, when it is stamped a grid
	way beside the original, and when the line travels with the rest of
	a selection rather than on its own.

	Quiet on purpose. Where a slash stands says nothing about the text
	a conductor shows, so there is nothing to work out again -- and this
	runs while items travel under the mouse, where one full pass over
	every wire of the project would cost more than the movement itself.
	@param part the section which moved
	@param delta how far it came, in scene coordinates
*/
void Cable::movePartCores(const QUuid &part, const QPointF &delta)
{
	if (part.isNull() || delta.isNull()) return;

	for (CableCore &core : m_cores)
	{
		if (core.part == part) {
			core.position += delta;
		}
	}
}

/**
	@brief Cable::toXml
	@param document
	@return the &lt;cable&gt; element holding every part and every wired core
*/
QDomElement Cable::toXml(QDomDocument &document) const
{
	QDomElement root = document.createElement(QLatin1String("cable"));
	root.setAttribute(QLatin1String("uuid"), m_uuid.toString());
	if (!m_designation.isEmpty()) {
		root.setAttribute(QLatin1String("designation"), m_designation);
	}
		//Only a name which was typed in says so: the ones the numbering
		//rule handed out are the ordinary case and need no marking
	if (m_designation_by_hand) {
		root.setAttribute(QLatin1String("designation_by_hand"), QLatin1String("1"));
	}
	if (!m_bmk.isEmpty()) {
		root.setAttribute(QLatin1String("bmk"), m_bmk);
	}
	if (!m_installation.isEmpty()) {
		root.setAttribute(QLatin1String("installation"), m_installation);
	}
	if (!m_location.isEmpty()) {
		root.setAttribute(QLatin1String("location"), m_location);
	}
	if (!m_length.isEmpty()) {
		root.setAttribute(QLatin1String("length"), m_length);
	}
		//Only what was switched off is written: a cable written before
		//these existed shows everything, like the ones written from now
	if (!m_show_designation) {
		root.setAttribute(QLatin1String("show_designation"), QLatin1String("0"));
	}
	if (!m_show_type) {
		root.setAttribute(QLatin1String("show_type"), QLatin1String("0"));
	}
	if (!m_show_installation) {
		root.setAttribute(QLatin1String("show_installation"), QLatin1String("0"));
	}
	if (!m_show_location) {
		root.setAttribute(QLatin1String("show_location"), QLatin1String("0"));
	}
	if (!m_show_length) {
		root.setAttribute(QLatin1String("show_length"), QLatin1String("0"));
	}
	if (!m_type.isEmpty()) {
		root.setAttribute(QLatin1String("type"), m_type);
	}
	if (m_core_count > 0) {
		root.setAttribute(QLatin1String("cores"), QString::number(m_core_count));
	}

	if (!m_core_colors.isEmpty())
	{
		QDomElement colors = document.createElement(QLatin1String("colors"));
		for (const QString &color : m_core_colors) {
			QDomElement e = document.createElement(QLatin1String("color"));
			e.appendChild(document.createTextNode(color));
			colors.appendChild(e);
		}
		root.appendChild(colors);
	}

		//Only the texts this cable does not write the way the preferences
		//say are stored, so a cable stays silent as long as it merely
		//follows the default -- and a later change of that default still
		//reaches it. The nudges are kept in the same element: they say
		//where those texts stand rather than what the cable is.
	if (!m_texts.isEmpty() || !m_text_offset.isNull() || !m_core_offset.isNull())
	{
		QDomElement texts = document.createElement(QLatin1String("texts"));
		bool stored = false;
		for (auto it = m_texts.cbegin(); it != m_texts.cend(); ++it)
		{
			const CableTextFormat &format = it.value();
			if (format.font.isEmpty() && format.alignment == 0) {
				continue;
			}
			QDomElement e = document.createElement(QLatin1String("text"));
			e.setAttribute(QLatin1String("key"), it.key());
			if (!format.font.isEmpty()) {
				e.setAttribute(QLatin1String("font"), format.font);
			}
			const QString align = alignmentName(format.alignment);
			if (!align.isEmpty()) {
				e.setAttribute(QLatin1String("align"), align);
			}
			texts.appendChild(e);
			stored = true;
		}
		if (!m_text_offset.isNull())
		{
			texts.setAttribute(QLatin1String("text_dx"),
							   QString::number(m_text_offset.x(), 'f', 2));
			texts.setAttribute(QLatin1String("text_dy"),
							   QString::number(m_text_offset.y(), 'f', 2));
			stored = true;
		}
		if (!m_core_offset.isNull())
		{
			texts.setAttribute(QLatin1String("core_dx"),
							   QString::number(m_core_offset.x(), 'f', 2));
			texts.setAttribute(QLatin1String("core_dy"),
							   QString::number(m_core_offset.y(), 'f', 2));
			stored = true;
		}
		if (stored) {
			root.appendChild(texts);
		}
	}

	for (const CablePartData &part : m_parts)
	{
		QDomElement e = document.createElement(QLatin1String("part"));
		e.setAttribute(QLatin1String("uuid"), part.uuid.toString());
		e.setAttribute(QLatin1String("diagram"), part.diagram.toString());
		e.setAttribute(QLatin1String("x1"), QString::number(part.p1.x(), 'f', 2));
		e.setAttribute(QLatin1String("y1"), QString::number(part.p1.y(), 'f', 2));
		e.setAttribute(QLatin1String("x2"), QString::number(part.p2.x(), 'f', 2));
		e.setAttribute(QLatin1String("y2"), QString::number(part.p2.y(), 'f', 2));
		root.appendChild(e);
	}

	for (const CableCore &core : usedCores())
	{
		QDomElement e = document.createElement(QLatin1String("core"));
		e.setAttribute(QLatin1String("index"), QString::number(core.core));
		if (!core.part.isNull()) {
			e.setAttribute(QLatin1String("part"), core.part.toString());
		}
		if (!core.conductor.isNull()) {
			e.setAttribute(QLatin1String("conductor"), core.conductor.toString());
		}
		e.setAttribute(QLatin1String("x"), QString::number(core.position.x(), 'f', 2));
		e.setAttribute(QLatin1String("y"), QString::number(core.position.y(), 'f', 2));
		root.appendChild(e);
	}

	return root;
}

/**
	@brief Cable::fromXml
	@param element a &lt;cable&gt; element
*/
void Cable::fromXml(const QDomElement &element)
{
	m_uuid = QUuid(element.attribute(QLatin1String("uuid")));
	if (m_uuid.isNull()) {
		m_uuid = QUuid::createUuid();
	}
	m_designation = element.attribute(QLatin1String("designation"));
		//A file written before this existed holds names the program
		//gave, which is what not marking them means
	m_designation_by_hand = element.attribute(
		QLatin1String("designation_by_hand")) == QLatin1String("1");
	m_bmk = element.attribute(QLatin1String("bmk"));
	m_installation = element.attribute(QLatin1String("installation"));
	m_location = element.attribute(QLatin1String("location"));
	m_length = element.attribute(QLatin1String("length"));
	m_show_designation = element.attribute(
		QLatin1String("show_designation"), QLatin1String("1")).toInt() != 0;
	m_show_type = element.attribute(
		QLatin1String("show_type"), QLatin1String("1")).toInt() != 0;
	m_show_installation = element.attribute(
		QLatin1String("show_installation"), QLatin1String("1")).toInt() != 0;
	m_show_location = element.attribute(
		QLatin1String("show_location"), QLatin1String("1")).toInt() != 0;
	m_show_length = element.attribute(
		QLatin1String("show_length"), QLatin1String("1")).toInt() != 0;
	m_type = element.attribute(QLatin1String("type"));
	m_core_count = qMax(0, element.attribute(QLatin1String("cores"), QLatin1String("0")).toInt());

	m_core_colors.clear();
	const QDomElement colors = element.firstChildElement(QLatin1String("colors"));
	for (QDomElement e = colors.firstChildElement(QLatin1String("color"));
		 !e.isNull();
		 e = e.nextSiblingElement(QLatin1String("color"))) {
		m_core_colors.append(e.text());
	}

		//The texts this cable writes its own way; everything not in here
		//keeps following what the preferences say.
	m_texts.clear();
	m_text_offset = QPointF();
	m_core_offset = QPointF();
	const QDomElement texts = element.firstChildElement(QLatin1String("texts"));
	for (QDomElement e = texts.firstChildElement(QLatin1String("text"));
		 !e.isNull();
		 e = e.nextSiblingElement(QLatin1String("text")))
	{
		const QString key = e.attribute(QLatin1String("key"));
		if (key.isEmpty()) continue;
		CableTextFormat format;
		format.font = e.attribute(QLatin1String("font"));
		format.alignment = alignmentFromName(e.attribute(QLatin1String("align")));
		if (format.font.isEmpty() && format.alignment == 0) continue;
		m_texts.insert(key, format);
	}
	if (!texts.isNull())
	{
		m_text_offset.setX(texts.attribute(QLatin1String("text_dx")).toDouble());
		m_text_offset.setY(texts.attribute(QLatin1String("text_dy")).toDouble());
		m_core_offset.setX(texts.attribute(QLatin1String("core_dx")).toDouble());
		m_core_offset.setY(texts.attribute(QLatin1String("core_dy")).toDouble());
	}

	m_parts.clear();
	for (QDomElement e = element.firstChildElement(QLatin1String("part"));
		 !e.isNull();
		 e = e.nextSiblingElement(QLatin1String("part")))
	{
		CablePartData part;
		part.uuid = QUuid(e.attribute(QLatin1String("uuid")));
		if (part.uuid.isNull()) {
			continue;
		}
		part.diagram = QUuid(e.attribute(QLatin1String("diagram")));
		part.p1 = QPointF(e.attribute(QLatin1String("x1")).toDouble(),
						  e.attribute(QLatin1String("y1")).toDouble());
		part.p2 = QPointF(e.attribute(QLatin1String("x2")).toDouble(),
						  e.attribute(QLatin1String("y2")).toDouble());
		m_parts.append(part);
	}

	m_cores.clear();
	for (QDomElement e = element.firstChildElement(QLatin1String("core"));
		 !e.isNull();
		 e = e.nextSiblingElement(QLatin1String("core")))
	{
		CableCore core;
		core.core = e.attribute(QLatin1String("index"), QLatin1String("-1")).toInt();
		if (core.core < 0) {
			continue;
		}
		core.part = QUuid(e.attribute(QLatin1String("part")));
		core.conductor = QUuid(e.attribute(QLatin1String("conductor")));
		core.position = QPointF(e.attribute(QLatin1String("x")).toDouble(),
								e.attribute(QLatin1String("y")).toDouble());
		if (hasCore(core.core)) {
			continue;
		}
		m_cores.append(core);
	}
}
