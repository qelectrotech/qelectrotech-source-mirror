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
#include "terminalstripitem.h"
#include "../../qetproject.h"

#include "../diagram.h"
#include "../../project/projectpropertieshandler.h"
#include "../../qetgraphicsitem/qgraphicsitemutility.h"
#include "../terminalstrip.h"
#include "../terminalstripbridge.h"
#include "../physicalterminal.h"
#include "../realterminal.h"
#include "../ui/terminalstripeditorwindow.h"
#include "trueterminalstrip.h"

TerminalStripItem::TerminalStripItem(QPointer<TerminalStrip> strip,
									 QGraphicsItem *parent) :
	QetGraphicsItem { parent },
	m_strip { strip },
	m_drawer { QSharedPointer<TerminalStripDrawer::TrueTerminalStrip> {
			  new TerminalStripDrawer::TrueTerminalStrip { strip }}
	}
{
	setFlags(QGraphicsItem::ItemIsMovable | QGraphicsItem::ItemIsSelectable);
	setAcceptHoverEvents(true);
	setDefaultLayout();
	connectStrip(strip);
}

TerminalStripItem::TerminalStripItem(QGraphicsItem *parent) :
	QetGraphicsItem { parent }
{
	setFlags(QGraphicsItem::ItemIsMovable | QGraphicsItem::ItemIsSelectable);
	setAcceptHoverEvents(true);
}

void TerminalStripItem::setTerminalStrip(TerminalStrip *strip)
{
	m_strip = strip;
	connectStrip(strip);
	m_drawer.setStrip(QSharedPointer<TerminalStripDrawer::TrueTerminalStrip> {
						  new TerminalStripDrawer::TrueTerminalStrip { strip }});
	m_pending_strip_uuid = QUuid();

	if (!m_drawer.haveLayout()) {
		setDefaultLayout();
	}
}

/**
 * @brief TerminalStripItem::terminalStrip
 * @return The strip drawed by this item
 */
QPointer<TerminalStrip> TerminalStripItem::terminalStrip() const {
	return m_strip;
}

void TerminalStripItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
	Q_UNUSED(option)
	Q_UNUSED(widget)

	m_drawer.paint(painter);

	if (isSelected() || isHovered())
	{
		QGIUtility::drawBoundingRectSelection(this, painter);
	}
}

QRectF TerminalStripItem::boundingRect() const
{
	auto br_ = m_drawer.boundingRect();
	br_.adjust(-5,-5,5,5);

	return br_;
}

/**
 * @brief TerminalStripItem::name
 * @return usual name of this item
 */
QString TerminalStripItem::name() const {
	return tr("plan of terminals");
}

/**
 * @brief TerminalStripItem::pdfRefs
 * @return the xref texts of the plan which lead to an element,
 * as the folio and rectangles needed to make them clickable in a PDF.
 * The rectangles are those of the last paint, so call this after
 * the item was rendered.
 */
QList<TerminalStripItem::PdfRef> TerminalStripItem::pdfRefs() const
{
	QList<PdfRef> refs;
	if (m_strip.isNull()) {
		return refs;
	}

	const auto xref_rects = m_drawer.xrefRects();
	for (const auto &xref_rect : xref_rects)
	{
		const auto physical_terminal = m_strip->physicalTerminal(xref_rect.physical);
		if (physical_terminal.isNull()) {
			continue;
		}
		const auto real_terminal = physical_terminal->realTerminal(xref_rect.real);
		if (real_terminal.isNull() || !real_terminal->isElement()) {
			continue;
		}
		const QPointer<Element> element = real_terminal->element();
		if (element.isNull() || !element->diagram()) {
			continue;
		}

		PdfRef ref;
		ref.diagram = element->diagram();
		ref.here    = mapRectToScene(xref_rect.rect);
		ref.there   = element->mapRectToScene(element->boundingRect());
		refs.append(ref);
	}
	return refs;
}

void TerminalStripItem::hoverMoveEvent(QGraphicsSceneHoverEvent *event)
{
	QetGraphicsItem::hoverMoveEvent(event);
	m_drawer.setMouseHoverPos(hoverMousePos());
	if (m_drawer.needUpdate()) {
		update();
	}

		//The realistic styles don't write the xref : show it in a tool tip
	if (m_drawer.drawStyle() != TerminalStripLayoutPattern::DrawStyle::Schematic) {
		setToolTip(xrefToolTip(m_drawer.xrefAt(hoverMousePos())));
	}
}

void TerminalStripItem::hoverLeaveEvent(QGraphicsSceneHoverEvent *event)
{
	QetGraphicsItem::hoverLeaveEvent(event);
	m_drawer.setMouseHoverPos(QPointF{});
	setToolTip(QString());
		//Remove the highlight of the xref (or of the terminal) under the mouse
	update();
}

/**
 * @brief TerminalStripItem::xrefToolTip
 * @param terminal : physical and real index of a terminal of the strip
 * @return the text of the tool tip of @a terminal : its label and its xref,
 * or an empty string if @a terminal is invalid.
 */
QString TerminalStripItem::xrefToolTip(const TerminalStripDrawer::hoverTerminal &terminal) const
{
	if (!m_strip || terminal.physical < 0 || terminal.real < 0) {
		return QString();
	}
	const auto physical_terminal{m_strip->physicalTerminal(terminal.physical)};
	if (physical_terminal.isNull()) {
		return QString();
	}
	const auto real_terminal{physical_terminal->realTerminal(terminal.real)};
	if (real_terminal.isNull() || real_terminal->Xref().isEmpty()) {
		return QString();
	}
	return tr("%1 : %2\nDouble-click to go to the element")
			.arg(real_terminal->label(), real_terminal->Xref());
}

void TerminalStripItem::mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event)
{
	Q_UNUSED (event);

	const auto hovered = m_drawer.hoveredXref();

	if (m_strip) {
		if (hovered.physical >= 0 &&
			hovered.real >= 0)
		{
			if (const auto physical_terminal = m_strip->physicalTerminal(hovered.physical);
				!physical_terminal.isNull())
			{
				if (const auto real_terminal = physical_terminal->realTerminal(hovered.real);
					!real_terminal.isNull() &&
					real_terminal->isElement())
				{
					if (QPointer<Element> element = real_terminal->element();
						!element.isNull())
					{
						//Unselect and ungrab mouse to prevent unwanted
						//move when element is in the same scene of this.
						setSelected(false);
						ungrabMouse();
						QetGraphicsItem::showItem(element);
					}
				}
			}
		} else {
			TerminalStripEditorWindow::edit(m_strip);
		}
	}
}

void TerminalStripItem::refreshPending()
{
	if (!m_pending_strip_uuid.isNull()
		&& diagram()
		&& diagram()->project())
	{
		for (const auto &strip_ : diagram()->project()->terminalStrip()) {
			if (strip_->uuid() == m_pending_strip_uuid) {
				setTerminalStrip(strip_);
				m_pending_strip_uuid = QUuid();
				break;
			}
		}
	}
}

void TerminalStripItem::setLayout(QSharedPointer<TerminalStripLayoutPattern> layout)
{
	m_drawer.setLayout(layout);
}

/**
 * @brief TerminalStripItem::setDrawStyle
 * Set how this item draw the terminals (schematic, realistic...).
 * The style is chosen when the item is added to a folio and saved with the item.
 * @param style
 */
void TerminalStripItem::setDrawStyle(TerminalStripLayoutPattern::DrawStyle style)
{
	if (style == m_drawer.drawStyle()) {
		return;
	}
	prepareGeometryChange();
	m_drawer.setDrawStyle(style);
	update();
}

TerminalStripLayoutPattern::DrawStyle TerminalStripItem::drawStyle() const {
	return m_drawer.drawStyle();
}

/**
 * @brief TerminalStripItem::connectStrip
 * Redraw this item as soon as @a strip or one of its terminals changes
 * (terminal strip editor applied, undo, redo...).
 * The connections to the previous strip are removed.
 * @param strip
 */
void TerminalStripItem::connectStrip(TerminalStrip *strip)
{
	for (const auto &connection_ : std::as_const(m_strip_connections)) {
		disconnect(connection_);
	}
	m_strip_connections.clear();

	if (strip)
	{
		m_strip_connections << connect(strip, &TerminalStrip::orderChanged, this, &TerminalStripItem::stripChanged)
							<< connect(strip, &TerminalStrip::bridgeChanged, this, &TerminalStripItem::stripChanged)
							<< connect(strip, &TerminalStrip::bridgeColorChanged, this, &TerminalStripItem::stripChanged)
							<< connect(strip, &TerminalStrip::dataChanged, this, &TerminalStripItem::stripChanged);
	}
}

/**
 * @brief TerminalStripItem::stripChanged
 * The strip or one of its terminals changed : the size of the drawing
 * can change (width of the terminals, number of terminals...), redraw it.
 */
void TerminalStripItem::stripChanged()
{
	prepareGeometryChange();
	update();
}

void TerminalStripItem::setDefaultLayout()
{
	if (m_strip && m_strip->project()) {
		m_drawer.setLayout(m_strip->project()->projectPropertiesHandler().terminalStripLayoutHandler().defaultLayout());
	}
}
