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
#include "rectangleeditor.h"
#include "partrectangle.h"
#include "styleeditor.h"
#include "QPropertyUndoCommand/qpropertyundocommand.h"
#include "elementscene.h"

/**
	Constructeur
	@param editor L'editeur d'element concerne
	@param rect Le rectangle a editer
	@param parent le Widget parent
*/
RectangleEditor::RectangleEditor(QETElementEditor *editor, PartRectangle *rect, QWidget *parent) :
	ElementItemEditor(editor, parent),
	part(rect),
	m_locked(false)
{
	style_ = new StyleEditor(editor);
	
	x = new QDoubleSpinBox();
	y = new QDoubleSpinBox();
	w = new QDoubleSpinBox();
	h = new QDoubleSpinBox();
	
	x->setRange(-1000, 1000);
	y->setRange(-1000, 1000);
	w->setRange(-1000, 1000);
	h->setRange(-1000, 1000);
	
	QVBoxLayout *v_layout = new QVBoxLayout(this);
	
	QGridLayout *grid = new QGridLayout();
	grid -> addWidget(new QLabel(tr("Coin supérieur gauche : ")), 0, 0, 1, 4);
	grid -> addWidget(new QLabel("x"),                                  1, 0, Qt::AlignRight);
	grid -> addWidget(x,                                                1, 1);
	grid -> addWidget(new QLabel("y"),                                  1, 2);
	grid -> addWidget(y,                                                1, 3);
	grid -> addWidget(new QLabel(tr("Dimensions : ")),               2, 0, 1, 4);
	grid -> addWidget(new QLabel(tr("Largeur :")),                   3, 0);
	grid -> addWidget(w,                                                3, 1);
	grid -> addWidget(new QLabel(tr("Hauteur :")),                   4, 0);
	grid -> addWidget(h,                                                4, 1);
	
	v_layout -> addWidget(style_);
	v_layout -> addLayout(grid);
	
	activeConnections(true);
	updateForm();
}

/// Destructeur
RectangleEditor::~RectangleEditor() {
}

/**
 * @brief RectangleEditor::setPart
 * Specifie to this editor the part to edit.
 * Note that an editor can accept or refuse to edit a part. This editor accept only partRectangle.
 * @param new_part
 * @return
 */
bool RectangleEditor::setPart(CustomElementPart *new_part)
{
	if (!new_part)
	{
		if (part)
			disconnect(part, &PartRectangle::rectChanged, this, &RectangleEditor::updateForm);
		part = 0;
		style_ -> setPart(0);
		return(true);
	}

	if (PartRectangle *part_rectangle = dynamic_cast<PartRectangle *>(new_part))
	{
		if (part == part_rectangle) return true;
		if (part)
			disconnect(part, &PartRectangle::rectChanged, this, &RectangleEditor::updateForm);
		part = part_rectangle;
		style_ -> setPart(part);
		updateForm();
		connect(part, &PartRectangle::rectChanged, this, &RectangleEditor::updateForm);
		return(true);
	}

	return(false);
}

/**
 * @brief RectangleEditor::currentPart
 * @return the curent edited part, or 0 if there is no edited part
 */
CustomElementPart *RectangleEditor::currentPart() const {
	return(part);
}

/**
 * @brief RectangleEditor::topLeft
 * @return The edited topLeft already mapped to part coordinate
 */
QPointF RectangleEditor::editedTopLeft() const {
	return part -> mapFromScene(x->value(), y->value());
}

/**
 * @brief RectangleEditor::updateForm
 * Update the values displayed by this widget
 */
void RectangleEditor::updateForm()
{
	if (!part) return;
	activeConnections(false);

	QRectF rect = part->property("rect").toRectF();
	QPointF p = part->mapToScene(rect.topLeft());
	x->setValue(p.x());
	y->setValue(p.y());
	w->setValue(rect.width());
	h->setValue(rect.height());

	activeConnections(true);
}

/**
 * @brief RectangleEditor::editingFinished
 * Slot called when a editor widget is finish to be edited
 * Update the geometry of the rectangle according to value of editing widget.
 */
void RectangleEditor::editingFinished()
{
	if (m_locked) return;
	m_locked = true;

	QRectF rect(editedTopLeft(), QSizeF(w->value(), h->value()));
	QPropertyUndoCommand *undo = new QPropertyUndoCommand(part, "rect", part->property("rect"), rect);
	undo->setText(tr("Modifier un rectangle"));
	undo->enableAnimation();
	elementScene()->undoStack().push(undo);

	m_locked = false;
}

/**
 * @brief RectangleEditor::activeConnections
 * Enable/disable connection between editor widget and slot editingFinished
 * True == enable | false == disable
 * @param active
 */
void RectangleEditor::activeConnections(bool active)
{
	if (active)
	{
		connect(x, &QDoubleSpinBox::editingFinished, this, &RectangleEditor::editingFinished);
		connect(y, &QDoubleSpinBox::editingFinished, this, &RectangleEditor::editingFinished);
		connect(w, &QDoubleSpinBox::editingFinished, this, &RectangleEditor::editingFinished);
		connect(h, &QDoubleSpinBox::editingFinished, this, &RectangleEditor::editingFinished);
	}
	else
	{
		disconnect(x, &QDoubleSpinBox::editingFinished, this, &RectangleEditor::editingFinished);
		disconnect(y, &QDoubleSpinBox::editingFinished, this, &RectangleEditor::editingFinished);
		disconnect(w, &QDoubleSpinBox::editingFinished, this, &RectangleEditor::editingFinished);
		disconnect(h, &QDoubleSpinBox::editingFinished, this, &RectangleEditor::editingFinished);
	}
}
