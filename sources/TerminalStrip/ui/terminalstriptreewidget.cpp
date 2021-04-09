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
	QStringList strl(QStringLiteral("application/x-qet-terminal-strip-tree-terminal-uuid"));

	return strl;
}

void TerminalStripTreeWidget::startDrag(Qt::DropActions supportedActions)
{
	Q_UNUSED(supportedActions)

	auto item = currentItem();

	if (!item ||
		item->type() != TerminalStripTreeWidget::Terminal) {
		return;
	}

	QDrag drag(this);
	auto mime_data = new QMimeData();
	mime_data->setData("application/x-qet-terminal-strip-tree-terminal-uuid", item->data(0, UUID_USER_ROLE).toString().toLatin1());

	drag.setMimeData(mime_data);
	drag.setPixmap(QET::Icons::ElementTerminal.pixmap(16,16));
	drag.exec(Qt::MoveAction);
}

void TerminalStripTreeWidget::dragMoveEvent(QDragMoveEvent *event)
{
	auto strl = event->mimeData()->formats();
	if (strl.size() != 1 ||
		strl.first() != "application/x-qet-terminal-strip-tree-terminal-uuid") {
		event->ignore();
		return;
	}
		//Accepted move are :
		//free terminal to terminal strip
		//terminal strip to another terminal strip
		//terminal strip to free terminal
		//All other other move is ignored
	QTreeWidget::dragMoveEvent(event);

	auto overred_item = itemAt(event->pos());
	auto dragged_item = currentItem();
	if (!overred_item ||
		!dragged_item ||
		!dragged_item->parent()) {
		return;
	}
		//Ignore the event by default, we confirm it bellow if needed.
	event->ignore();

		//Move terminal
	if (dragged_item->parent()->type() == FreeTerminal && //From free to strip
		overred_item->type() == Strip) {
		event->accept();
	}
	else if (dragged_item->parent()->type() == Strip) //From strip to ...
	{
		if (overred_item->type() == FreeTerminal) { //Free terminal
			event->accept();
		} else if (overred_item->type() == Strip && //Another strip
				   dragged_item->parent() != overred_item) {
			event->accept();
		}
	}
}

void TerminalStripTreeWidget::dropEvent(QDropEvent *event)
{
	auto overred_item = itemAt(event->pos());
	auto dragged_item = currentItem();
	if (!overred_item ||
		!dragged_item ||
		!dragged_item->parent()) {
		return;
	}

	dragged_item->parent()->removeChild(dragged_item);
	overred_item->addChild(dragged_item);

		//Move terminal
	if (dragged_item->parent()->type() == FreeTerminal && //From free to strip
		overred_item->type() == Strip) {
		emit terminalAddedToStrip(QUuid::fromString(dragged_item->data(0, UUID_USER_ROLE).toString()),
								  QUuid::fromString(overred_item->data(0, UUID_USER_ROLE).toString()));
	}
	else if (dragged_item->parent()->type() == Strip) //From strip to ...
	{
		if (overred_item->type() == FreeTerminal) //Free terminal
		{
			emit terminalRemovedFromStrip(QUuid::fromString(dragged_item->data(0, UUID_USER_ROLE).toString()),
										  QUuid::fromString(overred_item->data(0, UUID_USER_ROLE).toString()));
		}
		else if (overred_item->type() == Strip && //Another strip
				 dragged_item->parent() != overred_item)
		{
			emit terminalMovedFromStripToStrip(QUuid::fromString(dragged_item->data(0, UUID_USER_ROLE).toString()),
											   QUuid::fromString(dragged_item->parent()->data(0, UUID_USER_ROLE).toString()),
											   QUuid::fromString(overred_item->data(0, UUID_USER_ROLE).toString()));
		}
	}

}

Qt::DropActions TerminalStripTreeWidget::supportedDropActions() const {
	return Qt::MoveAction;
}
