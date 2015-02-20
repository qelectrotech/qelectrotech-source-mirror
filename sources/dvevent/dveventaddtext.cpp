/*
	Copyright 2006-2015 The QElectroTech Team
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
#include "dveventaddtext.h"
#include "independenttextitem.h"
#include "diagram.h"
#include "diagramcommands.h"
#include "diagramview.h"
#include <QMouseEvent>

DVEventAddText::DVEventAddText(DiagramView *dv) :
	DVEventInterface (dv)
{}

DVEventAddText::~DVEventAddText() {}

bool DVEventAddText::mousePressEvent(QMouseEvent *event) {
	if (event->button() == Qt::LeftButton) {
		m_diagram -> undoStack().push(new AddItemCommand<IndependentTextItem *>(new IndependentTextItem("_"),
																				m_diagram,
																				m_dv -> mapToScene(event -> pos())));
		return true;
	}
	return false;
}
