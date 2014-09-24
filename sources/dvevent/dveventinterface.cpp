/*
	Copyright 2006-2014 The QElectroTech Team
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
#include "dveventinterface.h"
#include "diagramview.h"
#include <QMouseEvent>

DVEventInterface::DVEventInterface(DiagramView *dv) :
	m_dv(dv),
	m_diagram(dv->diagram()),
	m_running(false)
{
}

DVEventInterface::~DVEventInterface() {};

bool DVEventInterface::mouseDoubleClickEvent(QMouseEvent *event) {
	Q_UNUSED (event);
	return false;
}

bool DVEventInterface::mousePressEvent(QMouseEvent *event) {
	Q_UNUSED (event);
	return false;
}

bool DVEventInterface::mouseMoveEvent(QMouseEvent *event) {
	Q_UNUSED (event);
	return false;
}

bool DVEventInterface::mouseReleaseEvent(QMouseEvent *event) {
	Q_UNUSED (event);
	return false;
}

bool DVEventInterface::wheelEvent(QWheelEvent *event) {
	Q_UNUSED (event);
	return false;
}

bool DVEventInterface::isRunning() const {
	return m_running;
}

bool DVEventInterface::isFinish() const {
	return !m_running;
}
