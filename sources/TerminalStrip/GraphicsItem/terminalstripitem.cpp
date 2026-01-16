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


#include "../diagram.h"
#include "../../project/projectpropertieshandler.h"
#include "../../qetgraphicsitem/qgraphicsitemutility.h"
#include "../terminalstrip.h"
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
	return tr("plan de bornes");
}

void TerminalStripItem::hoverMoveEvent(QGraphicsSceneHoverEvent *event)
{
	QetGraphicsItem::hoverMoveEvent(event);
	m_drawer.setMouseHoverPos(hoverMousePos());
	if (m_drawer.needUpdate()) {
		update();
	}
}

void TerminalStripItem::hoverLeaveEvent(QGraphicsSceneHoverEvent *event)
{
	QetGraphicsItem::hoverLeaveEvent(event);
	m_drawer.setMouseHoverPos(QPointF{});
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

void TerminalStripItem::setDefaultLayout()
{
	if (m_strip && m_strip->project()) {
		m_drawer.setLayout(m_strip->project()->projectPropertiesHandler().terminalStripLayoutHandler().defaultLayout());
	}
}
