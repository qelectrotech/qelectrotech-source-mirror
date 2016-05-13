/*
	Copyright 2006-2016 The QElectroTech Team
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
#include "customelementpart.h"
#include "qetgraphicsitem/customelement.h"
#include "qetelementeditor.h"

/// @return le QETElementEditor auquel cet editeur appartient
QETElementEditor *CustomElementPart::elementEditor() const {
	return(element_editor);
}

/**
	Appelle le slot updateCurrentPartEditor de l'editeur
	@see QETElementEditor::updateCurrentPartEditor()
*/
void CustomElementPart::updateCurrentPartEditor() const {
	if (element_editor) {
		element_editor -> updateCurrentPartEditor();
	}
}

/// @return l'ElementScene contenant les parties editees par cet editeur
ElementScene *CustomElementPart::elementScene() const {
	return(element_editor -> elementScene());
}

/// @return la QUndoStack a utiliser pour les annulations
QUndoStack &CustomElementPart::undoStack() const {
	return(elementScene() -> undoStack());
}

/// @return this primitive as a QGraphicsItem
QGraphicsItem *CustomElementPart::toItem() {
	return(dynamic_cast<QGraphicsItem *>(this));
}

/**
	This method is called by the decorator when it manages only a single
	primitive. This brings the possibility to implement custom behaviour, such
	as text edition, points edition or specific resizing.
	The default implementation does nothing.
*/
void CustomElementPart::setDecorator(ElementPrimitiveDecorator *decorator) {
	Q_UNUSED(decorator)
}

/**
	This method is called by the decorator when it needs to determine the best
	way to interactively scale a primitive. It is typically called when only a
	single primitive is being scaled.
	The default implementation systematically returns
	QET::SnapScalingPointToGrid
*/
QET::ScalingMethod CustomElementPart::preferredScalingMethod() const {
	return(QET::SnapScalingPointToGrid);
}

/**
	This method is called by the decorator when it manages only a single
	primitive and it received a mouse press event.
	The implementation should return true if the primitive accepts the event, false otherwise.
	The default implementation returns false.
*/
bool CustomElementPart::singleItemPressEvent(ElementPrimitiveDecorator *, QGraphicsSceneMouseEvent *) {
	return(false);
}

/**
	This method is called by the decorator when it manages only a single
	primitive and it received a mouse move event.
	The implementation should return true if the primitive accepts the event, false otherwise.
	The default implementation returns false.
*/
bool CustomElementPart::singleItemMoveEvent(ElementPrimitiveDecorator *, QGraphicsSceneMouseEvent *) {
	return(false);
}

/**
	This method is called by the decorator when it manages only a single
	primitive and it received a mouse release event.
	The implementation should return true if the primitive accepts the event, false otherwise.
	The default implementation returns false.
*/
bool CustomElementPart::singleItemReleaseEvent(ElementPrimitiveDecorator *, QGraphicsSceneMouseEvent *) {
	return(false);
}

/**
	This method is called by the decorator when it manages only a single
	primitive and it received a mouse double click event.
	The implementation should return true if the primitive accepts the event, false otherwise.
	The default implementation returns false.
*/
bool CustomElementPart::singleItemDoubleClickEvent(ElementPrimitiveDecorator *, QGraphicsSceneMouseEvent *) {
	return(false);
}

/**
	Helper method to map points in CustomElementPart::handleUserTransformation()
	@param initial_selection_rect Selection rectangle when the movement started, in scene coordinates
	@param new_selection_rect New selection rectangle, in scene coordinates
	@param points List of points when the movement started, in scene coordinates.
	@return The list of points mapped from initial_selection_rect to new_selection_rect
*/
QList<QPointF> CustomElementPart::mapPoints(const QRectF &initial_selection_rect, const QRectF &new_selection_rect, const QList<QPointF> &points) {
	QList<QPointF> new_points;
	if (!points.count()) return(new_points);
	
	// compare the new selection rectangle with the stored one to get the scaling ratio
	qreal sx = new_selection_rect.width() / initial_selection_rect.width();
	qreal sy = new_selection_rect.height() / initial_selection_rect.height();
	
	QPointF initial_top_left = initial_selection_rect.topLeft();
	qreal new_top_left_x = new_selection_rect.x();
	qreal new_top_left_y = new_selection_rect.y();
	
	foreach (QPointF point, points) {
		QPointF point_offset = point - initial_top_left;
		new_points << QPointF(
			new_top_left_x + (point_offset.rx() * sx),
			new_top_left_y + (point_offset.y() * sy)
		);
	}
	
	return(new_points);
}
