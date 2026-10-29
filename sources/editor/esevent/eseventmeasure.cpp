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
#include "eseventmeasure.h"
#include "elementmeasureitem.h"
#include "../elementscene.h"
#include "../ui/qetelementeditor.h"

#include <QGraphicsSceneMouseEvent>
#include <QKeyEvent>
#include <QStatusBar>
#include <QAction>
#include <QLineF>

ESEventMeasure::ESEventMeasure(ElementScene *scene, QAction *action) :
	ESEventInterface(scene),
	m_action(action),
	m_current_measure(nullptr),
	m_in_first_click(true)
{
	m_running = true;
	if (m_editor && m_editor->statusBar()) {
		m_editor->statusBar()->showMessage(
			tr("Tape measure: Click to set the first reference point (P1), Esc to exit"));
	}
}

ESEventMeasure::~ESEventMeasure()
{
	if (m_current_measure) {
		if (m_current_measure->scene()) {
			m_current_measure->scene()->removeItem(m_current_measure);
		}
		delete m_current_measure;
		m_current_measure = nullptr;
	}
}

void ESEventMeasure::updateStatusMessage(const QPointF &p1, const QPointF &p2)
{
	if (!m_editor || !m_editor->statusBar()) return;

	QLineF line(p1, p2);
	qreal lenMM = line.length() / 2.0;
	qreal dxMM = qAbs(p2.x() - p1.x()) / 2.0;
	qreal dyMM = qAbs(p2.y() - p1.y()) / 2.0;
	qreal angle = line.angle();

	QString msg = tr("Tape measure: %1 mm (%2 px) | ΔX: %3 mm | ΔY: %4 mm | Angle: %5°")
		.arg(QString::number(lenMM, 'f', 1).replace('.', ','))
		.arg(QString::number(line.length(), 'f', 0))
		.arg(QString::number(dxMM, 'f', 1).replace('.', ','))
		.arg(QString::number(dyMM, 'f', 1).replace('.', ','))
		.arg(QString::number(angle, 'f', 1).replace('.', ','));

	m_editor->statusBar()->showMessage(msg);
}

bool ESEventMeasure::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
	if (event->button() == Qt::LeftButton) {
		QPointF pos = m_scene->snapToGrid(event->scenePos());

		if (m_in_first_click) {
			m_current_measure = new ElementMeasureItem(pos, pos);
			m_scene->addItem(m_current_measure);
			m_in_first_click = false;

			if (m_editor && m_editor->statusBar()) {
				m_editor->statusBar()->showMessage(
					tr("Tape measure: Click to set the second reference point (P2)"));
			}
			return true;
		} else {
			if (m_current_measure) {
				m_current_measure->setP2(pos);
				m_current_measure->setFinished(true);
				updateStatusMessage(m_current_measure->p1(), pos);
				m_current_measure = nullptr;
			}
			m_in_first_click = true;
			return true;
		}
	} else if (event->button() == Qt::RightButton) {
		if (!m_in_first_click && m_current_measure) {
			m_scene->removeItem(m_current_measure);
			delete m_current_measure;
			m_current_measure = nullptr;
			m_in_first_click = true;

			if (m_editor && m_editor->statusBar()) {
				m_editor->statusBar()->showMessage(
					tr("Tape measure: Click to set the first reference point (P1), Esc to exit"));
			}
			return true;
		} else if (m_scene && m_scene->hasMeasureItems()) {
			m_scene->clearMeasureItems();
			if (m_editor && m_editor->statusBar()) {
				m_editor->statusBar()->showMessage(tr("Measurements cleared"));
			}
			return true;
		} else {
			if (m_editor && m_editor->statusBar()) {
				m_editor->statusBar()->clearMessage();
			}
			if (m_action) {
				m_action->setChecked(false);
			}
			m_running = false;
			m_abort = true;
			return true;
		}
	}

	return false;
}

bool ESEventMeasure::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
	updateHelpCross(event->scenePos());

	if (!m_in_first_click && m_current_measure) {
		QPointF pos = m_scene->snapToGrid(event->scenePos());
		m_current_measure->setP2(pos);
		updateStatusMessage(m_current_measure->p1(), pos);
		return true;
	}

	return false;
}

bool ESEventMeasure::keyPressEvent(QKeyEvent *event)
{
	if (event->key() == Qt::Key_Escape) {
		if (!m_in_first_click && m_current_measure) {
			m_scene->removeItem(m_current_measure);
			delete m_current_measure;
			m_current_measure = nullptr;
			m_in_first_click = true;

			if (m_editor && m_editor->statusBar()) {
				m_editor->statusBar()->showMessage(
					tr("Tape measure: Click to set the first reference point (P1), Esc to exit"));
			}
			return true;
		} else if (m_scene && m_scene->hasMeasureItems()) {
			m_scene->clearMeasureItems();
			if (m_editor && m_editor->statusBar()) {
				m_editor->statusBar()->showMessage(
					tr("Measurements cleared. Press Esc again to exit"));
			}
			return true;
		} else {
			if (m_editor && m_editor->statusBar()) {
				m_editor->statusBar()->clearMessage();
			}
			if (m_action) {
				m_action->setChecked(false);
			}
			m_running = false;
			m_abort = true;
			return true;
		}
	}

	return false;
}

bool ESEventMeasure::isFinish() const
{
	return !m_running || m_abort;
}
