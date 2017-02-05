/*
	Copyright 2006-2017 The QElectroTech Team
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
#include "helpercell.h"

/**
	Constructor
	@param parent Parent QGraphicsItem
*/
HelperCell::HelperCell(QGraphicsItem *parent) :
	QGraphicsObject(parent),
	QGraphicsLayoutItem(),
	background_color(Qt::white),
	foreground_color(Qt::black),
	label(),
	orientation(Qt::Horizontal),
	index(-1)
{
	setGraphicsItem(this);
	setFlag(QGraphicsItem::ItemIsSelectable, false);
}

/**
	Destructor
*/
HelperCell::~HelperCell() {
}

/**
	Ensure geometry changes are handled for both QGraphicsObject and
	QGraphicsLayoutItem.
	@param g New geometry
*/
void HelperCell::setGeometry(const QRectF &g) {
	prepareGeometryChange();
	QGraphicsLayoutItem::setGeometry(g);
	setPos(g.topLeft());
}

/**
	@param which Size hint to be modified
	@param constraint New value for the size hint
	@return the size hint for \a which using the width or height of \a constraint
*/
QSizeF HelperCell::sizeHint(Qt::SizeHint which, const QSizeF &constraint) const {
	Q_UNUSED(which);
	return(constraint);
}

/**
	@return the bounding rect of this helper cell
*/
QRectF HelperCell::boundingRect() const {
	return QRectF(QPointF(0,0), geometry().size());
}

/**
	Handles the helper cell visual rendering
	@param painter QPainter to be used for the rendering
	@param option Rendering options
	@param widget QWidget being painted, if any
*/
void HelperCell::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) {
	Q_UNUSED(option);
	Q_UNUSED(widget);
	
	QRectF drawing_rectangle(QPointF(0, 0), geometry().size());
	
	painter -> setPen(Qt::black);
	painter -> setBrush(background_color);
	painter -> drawRect(drawing_rectangle);
	
	painter -> setPen(foreground_color);
	painter -> drawText(drawing_rectangle, Qt::AlignHCenter | Qt::AlignVCenter, label);
}

/**
	@param type new type of this helper cell -- @see QET::TitleBlockColumnLength
*/
void HelperCell::setType(QET::TitleBlockColumnLength type) {
	if (type == QET::Absolute) {
		background_color = QColor("#C0FFFF");
		foreground_color = Qt::black;
	} else if (type == QET::RelativeToTotalLength) {
		background_color = QColor("#FFA858");
		foreground_color = Qt::black;
	} else if (type == QET::RelativeToRemainingLength) {
		background_color = QColor("#FFC0C0");
		foreground_color = Qt::black;
	}
}

/**
	Set the list of actions displayed by the context menu of this helper cell.
*/
void HelperCell::setActions(const QList<QAction *> &actions) {
	actions_ = actions;
}

/**
	@return the list of actions displayed by the context menu of this helper cell.
*/
QList<QAction *> HelperCell::actions() const {
	return actions_;
}

/**
	@param text New label displayed by this helper cell
	@param set_as_tooltip If true, the text is also used as tooltip.
*/
void HelperCell::setLabel(const QString &text, bool set_as_tooltip) {
	label = text;
	if (set_as_tooltip) {
		setToolTip(text);
	}
}

/**
	Handle context menu events.
	@param event Context menu event.
*/
void HelperCell::contextMenuEvent(QGraphicsSceneContextMenuEvent *event) {
	if (actions_.isEmpty()) return;
	
	QMenu context_menu;
	foreach (QAction *action, actions_) {
		context_menu.addAction(action);
	}
	emit(contextMenuTriggered(this));
	context_menu.exec(event -> screenPos());
}

/**
	Handle double click events.
*/
void HelperCell::mouseDoubleClickEvent(QGraphicsSceneMouseEvent *) {
	
	emit(doubleClicked(this));
}
