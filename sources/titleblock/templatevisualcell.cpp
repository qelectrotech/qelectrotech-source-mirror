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
#include "templatevisualcell.h"
#include "titleblocktemplate.h"
#include "diagramcontext.h"

/**
	Constructor
	@param parent Parent QGraphicsItem
*/
TitleBlockTemplateVisualCell::TitleBlockTemplateVisualCell(QGraphicsItem *parent) :
	QGraphicsLayoutItem(),
	QGraphicsItem(parent),
	template_(0),
	cell_(0)
{
	setGraphicsItem(this);
	setFlag(QGraphicsItem::ItemIsSelectable, true);
	
}

/**
	Destructor
*/
TitleBlockTemplateVisualCell::~TitleBlockTemplateVisualCell() {
}

/**
	Ensure geometry changes are handled for both QGraphicsObject and
	QGraphicsLayoutItem.
	@param g New geometry
*/
void TitleBlockTemplateVisualCell::setGeometry(const QRectF &g) {
	prepareGeometryChange();
	QGraphicsLayoutItem::setGeometry(g);
	setPos(g.topLeft());
}

/**
	@param which Size hint to be modified
	@param constraint New value for the size hint
	@return the size hint for \a which using the width or height of \a constraint
*/
QSizeF TitleBlockTemplateVisualCell::sizeHint(Qt::SizeHint which, const QSizeF &constraint) const {
	Q_UNUSED(which);
	return constraint;
}

/**
	@return the bounding rect of this helper cell
*/
QRectF TitleBlockTemplateVisualCell::boundingRect() const {
	return QRectF(QPointF(0,0), geometry().size());
}

/**
	Handles the helper cell visual rendering
	@param painter QPainter to be used for the rendering
	@param option Rendering options
	@param widget QWidget being painted, if any
*/
void TitleBlockTemplateVisualCell::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) {
	Q_UNUSED(option);
	Q_UNUSED(widget);
	
	QRectF drawing_rectangle(QPointF(0, 0), geometry().size() /*- QSizeF(1, 1)*/);
	
	if (template_ && cell_) {
		template_ -> renderCell(*painter, *cell_, DiagramContext(), drawing_rectangle.toRect());
	}
	if (isSelected()) {
		QBrush selection_brush = QApplication::palette().highlight();
		QColor selection_color = selection_brush.color();
		selection_color.setAlpha(127);
		selection_brush.setColor(selection_color);
		painter -> setPen(Qt::NoPen);
		painter -> setBrush(selection_brush);
		painter -> drawRect(drawing_rectangle/*.adjusted(1, 1, -1, -1)*/);
	}
}

/**
	Set the previewed title block cell.
	@param tbt Parent title block template of the previewed cell
	@param cell Previewed cell
*/
void TitleBlockTemplateVisualCell::setTemplateCell(TitleBlockTemplate *tbt, TitleBlockCell *cell) {
	template_ = tbt;
	cell_     = cell;
}

/**
	@return the parent title block template of the previewed cell
*/
TitleBlockTemplate *TitleBlockTemplateVisualCell::titleBlockTemplate() const {
	return(template_);
}

/**
	@return the previewed title block cell
*/
TitleBlockCell *TitleBlockTemplateVisualCell::cell() const {
	return(cell_);
}

/**
	@return the title block cell previewed by this object, plus the cells it
	spans over, if any
*/
QSet<TitleBlockCell *> TitleBlockTemplateVisualCell::cells() const {
	QSet<TitleBlockCell *> set;
	if (cell_) {
		if (template_) {
			set = template_ -> spannedCells(cell_);
		}
		
		// the TitleBlockCell rendered by this object
		set << cell_;
	}
	return(set);
}
