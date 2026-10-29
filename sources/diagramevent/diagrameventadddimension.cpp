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
#include "diagrameventadddimension.h"

#include "../diagram.h"
#include "../qetapp.h"
#include "../qetdiagrameditor.h"
#include "../undocommand/addgraphicsobjectcommand.h"

#include <cmath>
#include <QCursor>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsView>
#include <QKeyEvent>
#include <QPainter>
#include <QPainterPath>
#include <QStatusBar>

/**
	@brief DiagramEventAddDimension::DiagramEventAddDimension
	Constructor
*/
DiagramEventAddDimension::DiagramEventAddDimension(Diagram *diagram, DimensionItem::DimensionMode mode) :
	DiagramEventInterface(diagram),
	m_mode(mode),
	m_state(WaitingForP1),
	m_dimension_item(nullptr)
{
	m_running = true;
	init();
}

/**
	@brief DiagramEventAddDimension::~DiagramEventAddDimension
	Destructor
*/
DiagramEventAddDimension::~DiagramEventAddDimension()
{
	if (m_dimension_item) {
		m_diagram->removeItem(m_dimension_item);
		delete m_dimension_item;
		m_dimension_item = nullptr;
	}

	if (m_diagram) {
		for (auto *view : m_diagram->views()) {
			view->viewport()->unsetCursor();
			view->viewport()->update();
		}
		if (!m_diagram->views().isEmpty()) {
			if (auto *editor = QETApp::diagramEditorAncestorOf(m_diagram->views().constFirst())) {
				editor->statusBar()->clearMessage();
			}
		}
	}
}

void DiagramEventAddDimension::init()
{
	m_running = true;
	if (m_diagram) {
		for (auto *view : m_diagram->views()) {
			view->viewport()->setCursor(dimensionCursor());
			view->viewport()->update();
		}
	}
	updateStatusHint();
}

QCursor DiagramEventAddDimension::dimensionCursor()
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

		// Dimension badge |<->| beside the arrow
		QPen haloPen(Qt::white, 2.5, Qt::SolidLine, Qt::SquareCap, Qt::MiterJoin);
		painter.setPen(haloPen);
		painter.drawLine(QPointF(13, 5), QPointF(13, 15));
		painter.drawLine(QPointF(27, 5), QPointF(27, 15));
		painter.drawLine(QPointF(13, 10), QPointF(27, 10));

		QPen darkPen(Qt::black, 1.2, Qt::SolidLine, Qt::SquareCap, Qt::MiterJoin);
		painter.setPen(darkPen);
		painter.drawLine(QPointF(13, 5), QPointF(13, 15));
		painter.drawLine(QPointF(27, 5), QPointF(27, 15));
		painter.drawLine(QPointF(13, 10), QPointF(27, 10));
		painter.drawLine(QPointF(13, 10), QPointF(16, 7.5));
		painter.drawLine(QPointF(13, 10), QPointF(16, 12.5));
		painter.drawLine(QPointF(27, 10), QPointF(24, 7.5));
		painter.drawLine(QPointF(27, 10), QPointF(24, 12.5));

		cursor = QCursor(pixmap, 0, 0);
		initialized = true;
	}
	return cursor;
}

void DiagramEventAddDimension::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
	if (Q_UNLIKELY(m_diagram->isReadOnly())) {
		return;
	}

	if (event->button() == Qt::RightButton) {
		if (m_state == WaitingForChainPoint) {
			finishChain();
		} else {
			abortTool();
		}
		event->setAccepted(true);
		return;
	}

	if (event->button() != Qt::LeftButton) {
		return;
	}

	QPointF pos = Diagram::snapToGrid(event->scenePos());

	switch (m_state) {
		case WaitingForP1: {
			m_p1 = pos;
			m_dimension_item = new DimensionItem(m_p1, m_p1, 20.0, m_mode);
			m_diagram->addItem(m_dimension_item);
			m_state = WaitingForP2;
			updateStatusHint();
			event->setAccepted(true);
			break;
		}

		case WaitingForP2: {
			if (pos == m_p1) {
				return;
			}
			m_p2 = pos;
			m_dimension_item->setP2(m_p2);
			m_state = WaitingForOffset;
			updateStatusHint();
			event->setAccepted(true);
			break;
		}

		case WaitingForOffset: {
			m_fixed_offset = m_dimension_item->offset();
			m_offset_determined = true;

			if (m_mode == DimensionItem::Chain) {
				m_active_chain_id = QUuid::createUuid();
				m_dimension_item->setChainId(m_active_chain_id);
			}

			commitCurrentDimension();

			if (m_mode == DimensionItem::Linear) {
				m_state = WaitingForP1;
				updateStatusHint();
			} else {
				// Chain dimensioning mode: keep P2 as new P1, reuse fixed offset
				m_p1 = m_p2;
				m_dimension_item = new DimensionItem(m_p1, m_p1, m_fixed_offset, DimensionItem::Chain);
				m_dimension_item->setChainId(m_active_chain_id);
				m_diagram->addItem(m_dimension_item);
				m_state = WaitingForChainPoint;
				updateStatusHint();
			}
			event->setAccepted(true);
			break;
		}

		case WaitingForChainPoint: {
			if (pos == m_p1) {
				return;
			}
			m_p2 = pos;
			m_dimension_item->setP2(m_p2);
			m_dimension_item->setOffset(m_fixed_offset);
			m_dimension_item->setChainId(m_active_chain_id);
			commitCurrentDimension();

			// Append next chain segment
			m_p1 = m_p2;
			m_dimension_item = new DimensionItem(m_p1, m_p1, m_fixed_offset, DimensionItem::Chain);
			m_dimension_item->setChainId(m_active_chain_id);
			m_diagram->addItem(m_dimension_item);
			updateStatusHint();
			event->setAccepted(true);
			break;
		}
	}
}

void DiagramEventAddDimension::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
	QPointF pos = Diagram::snapToGrid(event->scenePos());

	switch (m_state) {
		case WaitingForP1:
			break;

		case WaitingForP2: {
			if (m_dimension_item) {
				m_dimension_item->setP2(pos);
				updateStatusHint();
			}
			break;
		}

		case WaitingForOffset: {
			if (m_dimension_item) {
				QPointF diff = m_p2 - m_p1;
				qreal len = std::hypot(diff.x(), diff.y());
				if (len > 1e-4) {
					QPointF norm(-diff.y() / len, diff.x() / len);
					QPointF toCursor = event->scenePos() - m_p1;
					qreal offset = toCursor.x() * norm.x() + toCursor.y() * norm.y();
					offset = std::round(offset / 5.0) * 5.0;
					if (std::abs(offset) < 5.0) {
						offset = (offset >= 0 ? 5.0 : -5.0);
					}
					m_dimension_item->setOffset(offset);
					updateStatusHint();
				}
			}
			break;
		}

		case WaitingForChainPoint: {
			if (m_dimension_item) {
				m_dimension_item->setP2(pos);
				m_dimension_item->setOffset(m_fixed_offset);
				updateStatusHint();
			}
			break;
		}
	}
	event->setAccepted(true);
}

void DiagramEventAddDimension::keyPressEvent(QKeyEvent *event)
{
	if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
		finishChain();
		event->setAccepted(true);
	} else if (event->key() == Qt::Key_Escape) {
		abortTool();
		event->setAccepted(true);
	} else {
		DiagramEventInterface::keyPressEvent(event);
	}
}

void DiagramEventAddDimension::commitCurrentDimension()
{
	if (!m_dimension_item) return;

	// Push completed dimension to undo stack
	m_diagram->undoStack().push(new AddGraphicsObjectCommand(m_dimension_item, m_diagram));
	m_dimension_item = nullptr; // Item ownership transferred to diagram / undo stack
}

void DiagramEventAddDimension::finishChain()
{
	m_active_chain_id = QUuid();
	if (m_dimension_item) {
		m_diagram->removeItem(m_dimension_item);
		delete m_dimension_item;
		m_dimension_item = nullptr;
	}
	m_running = false;
	m_abort = false;
	emit finish();
}

void DiagramEventAddDimension::abortTool()
{
	m_active_chain_id = QUuid();
	if (m_dimension_item) {
		m_diagram->removeItem(m_dimension_item);
		delete m_dimension_item;
		m_dimension_item = nullptr;
	}
	m_running = false;
	m_abort = true;
	emit finish();
}

void DiagramEventAddDimension::updateStatusHint()
{
	if (!m_diagram || m_diagram->views().isEmpty()) return;
	auto *editor = QETApp::diagramEditorAncestorOf(m_diagram->views().constFirst());
	if (!editor) return;

	QString msg;
	switch (m_state) {
		case WaitingForP1:
			msg = (m_mode == DimensionItem::Linear)
				? tr("Linear Dimension: Click to set the first reference point (P1)")
				: tr("Chain Dimension: Click to set the start reference point (P1)");
			break;

		case WaitingForP2: {
			qreal dist = m_dimension_item ? m_dimension_item->dimensionValueMM() : 0.0;
			msg = tr("Dimension: Click to set the second reference point (P2) - Length: %1 mm")
				.arg(QString::number(dist, 'f', 1));
			break;
		}

		case WaitingForOffset: {
			qreal off = m_dimension_item ? std::abs(m_dimension_item->offset()) / 2.0 : 0.0;
			msg = tr("Dimension: Move mouse to set offset distance and side (%1 mm), click to fix")
				.arg(QString::number(off, 'f', 1));
			break;
		}

		case WaitingForChainPoint: {
			qreal dist = m_dimension_item ? m_dimension_item->dimensionValueMM() : 0.0;
			msg = tr("Chain Dimension: Click to place next segment (%1 mm) | Press Enter to finish, Esc to exit")
				.arg(QString::number(dist, 'f', 1));
			break;
		}
	}
	editor->statusBar()->showMessage(msg);
}

