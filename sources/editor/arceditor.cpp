/*
	Copyright 2006-2020 The QElectroTech Team
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
	v_layout->addStretch();

	updateForm();

	activeConnections(true);
}

/// Destructeur
ArcEditor::~ArcEditor()
{}

void ArcEditor::setUpChangeConnections()
{
    m_change_connections << connect(part, &PartArc::rectChanged, this, &ArcEditor::updateForm);
    m_change_connections << connect(part, &PartArc::spanAngleChanged, this, &ArcEditor::updateForm);
    m_change_connections << connect(part, &PartArc::startAngleChanged, this, &ArcEditor::updateForm);
#if TODO_LIST
#pragma message("@TODO implement position changes!")
#endif
    // TODO: implement position changes!
    //m_change_connections << connect(part, &PartArc::)
}

void ArcEditor::disconnectChangeConnections()
{
    for (QMetaObject::Connection c : m_change_connections) {
	disconnect(c);
    }
    m_change_connections.clear();
}

/**
	@brief ArcEditor::setPart
	Specifie to this editor the part to edit.
	Note that an editor can accept or refuse to edit a part. This editor accept only partArc.
	@param new_part
	@return
*/
bool ArcEditor::setPart(CustomElementPart *new_part)
{
	if (!new_part)
	{
		if (part)
			disconnectChangeConnections();

		part = nullptr;
		return(true);
	}

	if (PartArc *part_arc = dynamic_cast<PartArc *>(new_part))
	{
		if (part == part_arc) return true;
		if (part)
			disconnectChangeConnections();
		part = part_arc;
		updateForm();
		setUpChangeConnections();
		return(true);
	}

	return(false);
}

bool ArcEditor::setParts(QList <CustomElementPart *> parts)
{
	if (parts.isEmpty())
		return false;

	if (!setPart(parts.first()))
		return false;
	return style_->setParts(parts);
}

/**
	@brief ArcEditor::currentPart
	@return the curent edited part, or 0 if there is no edited part
*/
CustomElementPart *ArcEditor::currentPart() const
{
	return(part);
}

QList<CustomElementPart*> ArcEditor::currentParts() const
{
	return style_->currentParts();
}

/**
	@brief ArcEditor::updateArcS
	Update the start angle of the arc according to  the edited value.
*/
void ArcEditor::updateArcS()
{
	if (m_locked) return;
	m_locked = true;
	double value = start_angle->value() * 16;

	for (auto part: style_->currentParts()) {

		PartArc* arc = static_cast<PartArc*>(part);

		if (value != arc->property("startAngle"))
		{
			QPropertyUndoCommand *undo= new QPropertyUndoCommand(arc, "startAngle", arc->property("startAngle"), value);
			undo->setText("Modifier l'angle de depart d'un arc");
			undo->enableAnimation();
			elementScene()->undoStack().push(undo);
		}
	}

	m_locked = false;
}

/**
	@brief ArcEditor::updateArcA
	Update the span angle of the arc according to  the edited value.
*/
void ArcEditor::updateArcA()
{
	if (m_locked) return;
	m_locked = true;
	double value = angle->value() * 16;

	for (auto part: style_->currentParts()) {

		PartArc* arc = static_cast<PartArc*>(part);
		if (value != arc->property("spanAngle"))
		{
			QPropertyUndoCommand *undo= new QPropertyUndoCommand(arc, "spanAngle", arc->property("spanAngle"), value);
			undo->setText("Modifier l'angle d'un arc");
			undo->enableAnimation();
			elementScene()->undoStack().push(undo);
		}
	}

	m_locked = false;
}

/**
	@brief ArcEditor::updateArcRect
	Update the geometrie of the rect that define this arc according the the edited values
*/
void ArcEditor::updateArcRectX()
{
	if (m_locked) return;
	m_locked = true;

	for (auto part: style_->currentParts()) {

		PartArc* arc = static_cast<PartArc*>(part);
		QRectF rect = arc->property("rect").toRectF();
		QPointF point = arc->mapFromScene(x->value() - h->value()/2, y->value() - v->value()/2); // does not matter which value y is, because only the x value is used
		rect.setX(point.x()); // change only the x value

		if (rect != part->property("rect"))
		{
			QPropertyUndoCommand *undo= new QPropertyUndoCommand(arc, "rect", arc->property("rect"), rect);
			undo->setText("Modifier un arc");
			undo->enableAnimation();
			elementScene()->undoStack().push(undo);
		}
	}

	m_locked = false;
}

void ArcEditor::updateArcRectY()
{
	if (m_locked) return;
	m_locked = true;

	for (auto part: style_->currentParts()) {

		PartArc* arc = static_cast<PartArc*>(part);
		QRectF rect = arc->property("rect").toRectF();

		QPointF point = arc->mapFromScene(x->value() - h->value()/2, y->value() - v->value()/2);
		rect.setY(point.y());

		if (rect != arc->property("rect"))
		{
			QPropertyUndoCommand *undo= new QPropertyUndoCommand(arc, "rect", arc->property("rect"), rect);
			undo->setText("Modifier un arc");
			undo->enableAnimation();
			elementScene()->undoStack().push(undo);
		}

	}

	m_locked = false;
}

void ArcEditor::updateArcRectH()
{
	if (m_locked) return;
	m_locked = true;

	for (auto part: style_->currentParts()) {

		PartArc* arc = static_cast<PartArc*>(part);
		QRectF rect = arc->property("rect").toRectF();

		if (rect.width() != h->value())
		{
			rect.setWidth(h->value());
			QPropertyUndoCommand *undo= new QPropertyUndoCommand(arc, "rect", arc->property("rect"), rect);
			undo->setText("Modifier un arc");
			undo->enableAnimation();
			elementScene()->undoStack().push(undo);
		}
	}

	m_locked = false;
}

void ArcEditor::updateArcRectV()
{
	if (m_locked) return;
	m_locked = true;

	for (auto part: style_->currentParts()) {

		PartArc* arc = static_cast<PartArc*>(part);
		QRectF rect = arc->property("rect").toRectF();

		if (rect.height() != v->value())
		{
			rect.setHeight(v->value());
			QPropertyUndoCommand *undo= new QPropertyUndoCommand(arc, "rect", arc->property("rect"), rect);
			undo->setText("Modifier un arc");
			undo->enableAnimation();
			elementScene()->undoStack().push(undo);
		}
	}

	m_locked = false;
}

/**
	@brief ArcEditor::updateForm
	Update the value of the widgets
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
	@brief ArcEditor::activeConnections
	Enable/disable connection between editor widget and slot editingFinished
	True == enable | false == disable
	@param active
*/
void ArcEditor::activeConnections(bool active)
{
	if (active)
	{
		connect(x,           SIGNAL(editingFinished()), this, SLOT(updateArcRectX()));
		connect(y,           SIGNAL(editingFinished()), this, SLOT(updateArcRectY()));
		connect(h,           SIGNAL(editingFinished()), this, SLOT(updateArcRectH()));
		connect(v,           SIGNAL(editingFinished()), this, SLOT(updateArcRectV()));
		connect(start_angle, SIGNAL(editingFinished()), this, SLOT(updateArcS()));
		connect(angle,       SIGNAL(editingFinished()), this, SLOT(updateArcA()));
	}
	else
	{
		disconnect(x,           SIGNAL(editingFinished()), this, SLOT(updateArcRectX()));
		disconnect(y,           SIGNAL(editingFinished()), this, SLOT(updateArcRectY()));
		disconnect(h,           SIGNAL(editingFinished()), this, SLOT(updateArcRectH()));
		disconnect(v,           SIGNAL(editingFinished()), this, SLOT(updateArcRectV()));
		disconnect(start_angle, SIGNAL(editingFinished()), this, SLOT(updateArcS()));
		disconnect(angle,       SIGNAL(editingFinished()), this, SLOT(updateArcA()));
	}
}
