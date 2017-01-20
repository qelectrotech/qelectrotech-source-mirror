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
#include "arceditor.h"
#include "styleeditor.h"
#include "partarc.h"
#include "QPropertyUndoCommand/qpropertyundocommand.h"
#include "elementscene.h"

/**
	Constructeur
	@param editor L'editeur d'element concerne
	@param arc L'arc a editer
	@param parent le Widget parent
*/
ArcEditor::ArcEditor(QETElementEditor *editor, PartArc *arc, QWidget *parent) :
	ElementItemEditor(editor, parent),
	part(arc),
	m_locked(false)
{
	style_ = new StyleEditor(editor);
	x = new QDoubleSpinBox();
	y = new QDoubleSpinBox();
	h = new QDoubleSpinBox();
	v = new QDoubleSpinBox();
	start_angle = new QSpinBox();
	angle       = new QSpinBox();
	start_angle -> setRange(-360, 360);
	angle       -> setRange(-360, 360);
	
	x->setRange(-5000, 5000);
	y->setRange(-5000, 5000);
	h->setRange(-5000, 5000);
	v->setRange(-5000, 5000);
	
	QVBoxLayout *v_layout = new QVBoxLayout(this);
	
	QGridLayout *grid = new QGridLayout();
	grid -> addWidget(new QLabel(tr("Centre : ")),            0, 0);
	grid -> addWidget(new QLabel("x"),                        1, 0, Qt::AlignRight);
	grid -> addWidget(x,                                      1, 1);
	grid -> addWidget(new QLabel("y"),                        1, 2);
	grid -> addWidget(y,                                      1, 3);
	grid -> addWidget(new QLabel(tr("Diamètres : ")),      2, 0);
	grid -> addWidget(new QLabel(tr("horizontal :")),         3, 0);
	grid -> addWidget(h,                                      3, 1);
	grid -> addWidget(new QLabel(tr("vertical :")),           4, 0);
	grid -> addWidget(v,                                      4, 1);
	grid -> addWidget(new QLabel(tr("Angle de départ :")), 5, 0);
	grid -> addWidget(start_angle,                            5, 1);
	grid -> addWidget(new QLabel(tr("Angle :")),              6, 0);
	grid -> addWidget(angle,                                  6, 1);
	
	v_layout -> addWidget(style_);
	v_layout -> addLayout(grid);
	
	updateForm();
	
	activeConnections(true);
}

/// Destructeur
ArcEditor::~ArcEditor() {}

/**
 * @brief ArcEditor::setPart
 * Specifie to this editor the part to edit.
 * Note that an editor can accept or refuse to edit a part. This editor accept only partArc.
 * @param new_part
 * @return
 */
bool ArcEditor::setPart(CustomElementPart *new_part)
{
	if (!new_part)
	{
		if (part)
		{
			disconnect(part, &PartArc::rectChanged, this, &ArcEditor::updateForm);
			disconnect(part, &PartArc::spanAngleChanged, this, &ArcEditor::updateForm);
			disconnect(part, &PartArc::startAngleChanged, this, &ArcEditor::updateForm);
		}
		part = 0;
		style_ -> setPart(0);
		return(true);
	}

	if (PartArc *part_arc = dynamic_cast<PartArc *>(new_part))
	{
		if (part == part_arc) return true;
		if (part)
		{
			disconnect(part, &PartArc::rectChanged, this, &ArcEditor::updateForm);
			disconnect(part, &PartArc::spanAngleChanged, this, &ArcEditor::updateForm);
			disconnect(part, &PartArc::startAngleChanged, this, &ArcEditor::updateForm);
		}
		part = part_arc;
		style_ -> setPart(part);
		updateForm();
		connect(part, &PartArc::rectChanged, this, &ArcEditor::updateForm);
		connect(part, &PartArc::spanAngleChanged, this, &ArcEditor::updateForm);
		connect(part, &PartArc::startAngleChanged, this, &ArcEditor::updateForm);
		return(true);
	}

	return(false);
}

/**
 * @brief ArcEditor::currentPart
 * @return the curent edited part, or 0 if there is no edited part
 */
CustomElementPart *ArcEditor::currentPart() const {
	return(part);
}

/**
 * @brief ArcEditor::updateArcS
 * Update the start angle of the arc according to  the edited value.
 */
void ArcEditor::updateArcS()
{
	if (m_locked) return;
	m_locked = true;
	double value = start_angle->value() * 16;

	if (value != part->property("startAngle"))
	{
		QPropertyUndoCommand *undo= new QPropertyUndoCommand(part, "startAngle", part->property("startAngle"), value);
		undo->setText("Modifier l'angle de depart d'un arc");
		undo->enableAnimation();
		elementScene()->undoStack().push(undo);
	}

	m_locked = false;
}

/**
 * @brief ArcEditor::updateArcA
 * Update the span angle of the arc according to  the edited value.
 */
void ArcEditor::updateArcA()
{
	if (m_locked) return;
	m_locked = true;
	double value = angle->value() * 16;

	if (value != part->property("spanAngle"))
	{
		QPropertyUndoCommand *undo= new QPropertyUndoCommand(part, "spanAngle", part->property("spanAngle"), value);
		undo->setText("Modifier l'angle d'un arc");
		undo->enableAnimation();
		elementScene()->undoStack().push(undo);
	}

	m_locked = false;
}

/**
 * @brief ArcEditor::updateArcRect
 * Update the geometrie of the rect that define this arc according the the edited values
 */
void ArcEditor::updateArcRect()
{
	if (m_locked) return;
	m_locked = true;
	QPointF point = part->mapFromScene(x->value() - h->value()/2, y->value() - v->value()/2);
	QRectF rect(point, QSizeF(h->value(), v->value()));

	if (rect != part->property("rect"))
	{
		QPropertyUndoCommand *undo= new QPropertyUndoCommand(part, "rect", part->property("rect"), rect);
		undo->setText("Modifier un arc");
		undo->enableAnimation();
		elementScene()->undoStack().push(undo);
	}

	m_locked = false;
}

/**
 * @brief ArcEditor::updateForm
 * Update the value of the widgets
 */
void ArcEditor::updateForm()
{
	if (!part) return;
	activeConnections(false);
	QRectF rect = part->property("rect").toRectF();
	x->setValue(part->mapToScene(rect.topLeft()).x() + (rect.width()/2));
	y->setValue(part->mapToScene(rect.topLeft()).y() + (rect.height()/2));
	h->setValue(rect.width());
	v->setValue(rect.height());
	start_angle->setValue(part->property("startAngle").toInt()/16);
	angle->setValue(part->property("spanAngle").toInt()/16);
	activeConnections(true);
}

/**
 * @brief ArcEditor::activeConnections
 * Enable/disable connection between editor widget and slot editingFinished
 * True == enable | false == disable
 * @param active
 */
void ArcEditor::activeConnections(bool active)
{
	if (active)
	{
		connect(x,           SIGNAL(editingFinished()), this, SLOT(updateArcRect()));
		connect(y,           SIGNAL(editingFinished()), this, SLOT(updateArcRect()));
		connect(h,           SIGNAL(editingFinished()), this, SLOT(updateArcRect()));
		connect(v,           SIGNAL(editingFinished()), this, SLOT(updateArcRect()));
		connect(start_angle, SIGNAL(editingFinished()), this, SLOT(updateArcS()));
		connect(angle,       SIGNAL(editingFinished()), this, SLOT(updateArcA()));
	}
	else
	{
		disconnect(x,           SIGNAL(editingFinished()), this, SLOT(updateArcRect()));
		disconnect(y,           SIGNAL(editingFinished()), this, SLOT(updateArcRect()));
		disconnect(h,           SIGNAL(editingFinished()), this, SLOT(updateArcRect()));
		disconnect(v,           SIGNAL(editingFinished()), this, SLOT(updateArcRect()));
		disconnect(start_angle, SIGNAL(editingFinished()), this, SLOT(updateArcS()));
		disconnect(angle,       SIGNAL(editingFinished()), this, SLOT(updateArcA()));
	}
}
