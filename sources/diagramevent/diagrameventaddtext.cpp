/*
	Copyright 2006-2026 The QElectroTech Team
	This file is part of QElectroTech.

	QElectroTech is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 2 of the License, or
	(at your option) any later version.

	QElectroTech is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with QElectroTech. If not, see <http://www.gnu.org/licenses/>.
*/

#include "diagrameventaddtext.h"

#include <diagram.h>
#include <undocommand/addgraphicsobjectcommand.h>
#include <qetgraphicsitem/independenttextitem.h>
#include <qeticons.h>

#include <QGraphicsSceneMouseEvent>
#include <QGraphicsView>
#include <QPainter>
#include <QPainterPath>
#include <QPen>
#include <QBrush>
#include <QPixmap>

/**
	@brief DiagramEventAddText::DiagramEventAddText
	Default constructor
	@param diagram : the diagram where this event must operate
*/
DiagramEventAddText::DiagramEventAddText(Diagram *diagram) :
	DiagramEventInterface(diagram)
{
		//The tool is armed from the moment it is attached: the next left
		//click places a text. DiagramView::keyPressEvent() asks
		//Diagram::eventInterfaceIsRunning() -- which is isRunning(), i.e.
		//m_running -- before letting Escape through to the tool, so without
		//this the view swallows Escape and the tool cannot be cancelled.
	m_running = true;
	init();
}

/**
	@brief DiagramEventAddText::~DiagramEventAddText
*/
DiagramEventAddText::~DiagramEventAddText()
{
	if (m_diagram) {
		for (auto *view : m_diagram->views()) {
			view->viewport()->unsetCursor();
			view->viewport()->update();
		}
	}
}

/**
	@brief DiagramEventAddText::init
*/
void DiagramEventAddText::init()
{
	m_running = true;
	if (m_diagram) {
		for (auto *view : m_diagram->views()) {
			view->viewport()->setCursor(textCursor());
			view->viewport()->update();
		}
	}
}

/**
	@brief DiagramEventAddText::textCursor
	@return Custom cursor showing an arrow pointer with the text field icon badge
*/
QCursor DiagramEventAddText::textCursor()
{
	static QCursor cursor;
	static bool initialized = false;
	if (!initialized) {
		QPixmap pixmap(32, 32);
		pixmap.fill(Qt::transparent);

		QPainter painter(&pixmap);
		painter.setRenderHint(QPainter::Antialiasing, true);

		// Arrow pointer at (0, 0)
		QPainterPath path;
		path.addPolygon(QPolygonF({{0, 0}, {0, 16}, {4, 12}, {7, 18}, {9.5, 17}, {6.5, 11}, {11.5, 11}, {0, 0}}));
		painter.setPen(QPen(Qt::white, 1));
		painter.setBrush(Qt::black);
		painter.drawPath(path);

		// Text field icon badge beside the arrow
		QPixmap badge = QET::Icons::PartTextField.pixmap(16, 16);
		if (!badge.isNull()) {
			painter.drawPixmap(13, 8, badge);
		}

		cursor = QCursor(pixmap, 0, 0);
		initialized = !badge.isNull();
	}
	return cursor;
}

/**
	@brief DiagramEventAddText::mousePressEvent
	@param event : event of mouse press event.
*/
void DiagramEventAddText::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
	if (event->button() == Qt::LeftButton)
	{
		IndependentTextItem *text = new IndependentTextItem();
		m_diagram->undoStack().push(new AddGraphicsObjectCommand(
										text,
										m_diagram,
										event->scenePos()));
		text->setTextInteractionFlags(Qt::TextEditorInteraction);
		text->setFocus(Qt::MouseFocusReason);
			//Placed: the tool is done before it announces it.
		m_running = false;
		emit finish();
		event->setAccepted(true);
	}
}
