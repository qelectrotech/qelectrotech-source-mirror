/*
        Copyright 2006-2021 The QElectroTech Team
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
#include "terminalstriptreewidget.h"
#include "../../qeticons.h"

#include <QDebug>
#include <QDrag>
#include <QMimeData>
#include <QDragMoveEvent>

int TerminalStripTreeWidget::UUID_USER_ROLE = Qt::UserRole + 1;

TerminalStripTreeWidget::TerminalStripTreeWidget(QWidget *parent) :
    QTreeWidget(parent)
{}

QStringList TerminalStripTreeWidget::mimeTypes() const
{
	QStringList strl(QStringLiteral("application/x-qet-terminal-strip-tree-item"));

	return strl;
}

void TerminalStripTreeWidget::startDrag(Qt::DropActions supportedActions)
{
	Q_UNUSED(supportedActions)

	auto item = currentItem();

	if (!item ||
		item->type() != TerminalStripTreeWidget::FreeTerminal) {
		return;
	}

	QDrag drag(this);
	auto mime_data = new QMimeData();
	mime_data->setData("application/x-qet-terminal-strip-tree-item", item->data(0, UUID_USER_ROLE).toString().toLatin1());

	drag.setMimeData(mime_data);
	drag.setPixmap(QET::Icons::ElementTerminal.pixmap(16,16));
	drag.exec(Qt::MoveAction);
}

void TerminalStripTreeWidget::dragMoveEvent(QDragMoveEvent *event)
{
	QTreeWidget::dragMoveEvent(event);

	auto item = itemAt(event->pos());

	if (item &&
		item->type() == TerminalStripTreeWidget::Strip) {
		event->accept();
	} else {
		event->ignore();
	}
}

void TerminalStripTreeWidget::dropEvent(QDropEvent *event)
{}

Qt::DropActions TerminalStripTreeWidget::supportedDropActions() const {
	return Qt::MoveAction;
}
