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
#include "terminalelement.h"
#include <TerminalStrip/realterminal.h>
#include <TerminalStrip/terminalstriplabels.h>
#include <diagram.h>
#include <qetapp.h>
#include <shownkinds.h>

#include <QGraphicsLineItem>
#include <QGraphicsSimpleTextItem>
#include <QPen>

/**
	@brief TerminalElement::TerminalElement
	Default constructor
	@param location location of xml definition
	@param qgi parent QGraphicItem
	@param state int used to know if the creation of element have error
*/
TerminalElement::TerminalElement(const ElementsLocation &location,
				 QGraphicsItem *qgi, int *state) :
	Element(location, qgi, state, Element::Terminale)
{
	auto rt = new RealTerminal(this);
	m_real_terminal = rt->sharedRef();

		//A terminal that moves or turns may now be, or no longer be, the
		//first of its strip on its folio (TerminalStripLabels)
	auto moved = [this]() {
		if (auto *d = diagram())
			TerminalStripLabels::schedule(d);
		placeStripLabel();
	};
	connect(this, &Element::xChanged, this, moved);
	connect(this, &Element::yChanged, this, moved);
	connect(this, &Element::rotationChanged, this, moved);
	connect(this, &Element::mirrorChanged, this, moved);
}

TerminalElement::~TerminalElement()
{}

/**
	@brief TerminalElement::initLink
	@param project
*/
void TerminalElement::initLink(QETProject *project) {
	Element::initLink(project);
}

/**
 * @brief TerminalElement::realTerminal
 * @return the real terminal of this terminal element.
 */
QSharedPointer<RealTerminal> TerminalElement::realTerminal() const {
	return m_real_terminal;
}

/**
 * @brief TerminalElement::parentTerminalStrip
 * @return The parent terminal strip of this
 * terminal element or nullptr if not.
 */
TerminalStrip *TerminalElement::parentTerminalStrip() const {
	if (m_real_terminal) {
		return m_real_terminal->parentStrip();
	}
	return nullptr;
}

/**
	@brief TerminalElement::setStripLabel
	Show @a text, the label of the terminal strip, beside this terminal;
	an empty text takes it away.
*/
void TerminalElement::setStripLabel(const QString &text)
{
	if (text.isEmpty()) {
		delete m_strip_label;
		m_strip_label = nullptr;
		return;
	}
	if (!m_strip_label)
	{
		m_strip_label = new QGraphicsSimpleTextItem(this);
		m_strip_label->setFont(QETApp::diagramTextsFont(9));
			//Not an item of its own: clicks go to what lies beneath
		m_strip_label->setAcceptedMouseButtons(Qt::NoButton);
		m_strip_label->setAcceptHoverEvents(false);
			//Hidden with the symbol texts (View > Show)
		ShownKinds::tag(m_strip_label, ShownKinds::SymbolTexts);
	}
	if (m_strip_label->text() != text)
		m_strip_label->setText(text);
	placeStripLabel();
}

/**
	@brief TerminalElement::stripLabel
	@return the label of the terminal strip shown beside this terminal, or
	an empty string
*/
QString TerminalElement::stripLabel() const
{
	return m_strip_label ? m_strip_label->text() : QString();
}

/**
	@brief TerminalElement::setBridgeLine
	Draw @a line, a bridge to the next terminal of the row, behind this
	terminal, so its connection points cover the ends; a null line takes
	it away.
*/
void TerminalElement::setBridgeLine(const QLineF &line)
{
	if (line.isNull()) {
		delete m_bridge_line;
		m_bridge_line = nullptr;
		return;
	}
	if (!m_bridge_line)
	{
		m_bridge_line = new QGraphicsLineItem(this);
		m_bridge_line->setFlag(QGraphicsItem::ItemStacksBehindParent);
		QPen pen(Qt::black, 1);
		pen.setCapStyle(Qt::FlatCap);
		m_bridge_line->setPen(pen);
			//Not an item of its own: clicks go to what lies beneath
		m_bridge_line->setAcceptedMouseButtons(Qt::NoButton);
		m_bridge_line->setAcceptHoverEvents(false);
	}
	if (m_bridge_line->line() != line)
		m_bridge_line->setLine(line);
}

/**
	@brief TerminalElement::bridgeLine
	@return the bridge drawn to the next terminal of the row, or a null line
*/
QLineF TerminalElement::bridgeLine() const
{
	return m_bridge_line ? m_bridge_line->line() : QLineF();
}

/**
	@brief TerminalElement::placeStripLabel
	Before the terminal, 3 away from its frame, upright whichever way the
	terminal is turned: on the left of a terminal in a row, its middle on
	the row; above a terminal turned upright, centred on it.
*/
void TerminalElement::placeStripLabel()
{
	if (!m_strip_label)
		return;
		//Upright on the folio: undo the terminal's turn and mirror
	const QTransform turn = sceneTransform();
	m_strip_label->setTransform(
				QTransform(turn.m11(), turn.m12(), turn.m21(), turn.m22(), 0, 0).inverted());

		//The point before the terminal, and which way "before" is on the folio
	const QPointF anchor = mapToScene(QPointF(boundingRect().left() - 3, 0));
	const QPointF way = mapToScene(QPointF(-1, 0)) - mapToScene(QPointF(0, 0));
	const QSizeF size = m_strip_label->boundingRect().size();
	QPointF top_left;
	if (way.x() < -0.5)      top_left = anchor - QPointF(size.width(), size.height() / 2);
	else if (way.x() > 0.5)  top_left = anchor - QPointF(0, size.height() / 2);
	else if (way.y() < -0.5) top_left = anchor - QPointF(size.width() / 2, size.height());
	else                     top_left = anchor - QPointF(size.width() / 2, 0);
	m_strip_label->setPos(mapFromScene(top_left));
}

/**
	@brief TerminalElement::itemChange
	A terminal that comes onto or leaves a folio may now be, or no longer
	be, the first of its strip there; moves and turns come by the
	element's signals (see the constructor).
*/
QVariant TerminalElement::itemChange(GraphicsItemChange change, const QVariant &value)
{
	if (change == ItemSceneChange) {
			//The folio it leaves works its labels out again without it
		if (auto *old_diagram = qobject_cast<Diagram *>(scene()))
			TerminalStripLabels::schedule(old_diagram);
	}
	const QVariant result = Element::itemChange(change, value);
	if (change == ItemSceneHasChanged) {
		if (auto *d = qobject_cast<Diagram *>(scene()))
			TerminalStripLabels::schedule(d);
		placeStripLabel();
	}
	return result;
}
