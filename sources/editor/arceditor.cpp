/*
	Copyright 2006-2025 The QElectroTech Team
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

#include "../QPropertyUndoCommand/qpropertyundocommand.h"
#include "elementscene.h"
#include "graphicspart/partarc.h"
#include "styleeditor.h"

/**
	Constructeur
	@param editor L'editeur d'element concerne
	@param arc L'arc a editer
	@param parent le Widget parent
*/
ArcEditor::ArcEditor(QETElementEditor *editor, PartArc *arc, QWidget *parent) :
	ElementItemEditor(editor, parent),
	m_part(arc),
	m_locked(false)
{
	m_style = new StyleEditor(editor);
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

	v_layout -> addWidget(m_style);
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
	m_change_connections << connect(m_part, &PartArc::rectChanged, this, &ArcEditor::updateForm);
	m_change_connections << connect(m_part, &PartArc::spanAngleChanged, this, &ArcEditor::updateForm);
	m_change_connections << connect(m_part, &PartArc::startAngleChanged, this, &ArcEditor::updateForm);
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
		if (m_part)
			disconnectChangeConnections();

		m_part = nullptr;
		return(true);
	}

	if (PartArc *part_arc = dynamic_cast<PartArc *>(new_part))
	{
		if (m_part == part_arc) return true;
		if (m_part)
			disconnectChangeConnections();
		m_part = part_arc;
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
	return m_style->setParts(parts);
}

/**
	@brief ArcEditor::currentPart
	@return the current edited part, or 0 if there is no edited part
*/
CustomElementPart *ArcEditor::currentPart() const
{
	return(m_part);
}

QList<CustomElementPart*> ArcEditor::currentParts() const
{
	return m_style->currentParts();
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

	for (auto part: m_style->currentParts()) {

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

	for (auto part: m_style->currentParts()) {

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
	@brief ArcEditor::updateForm
	Update the value of the widgets
*/
void ArcEditor::updateForm()
{
	if (!m_part) return;
	activeConnections(false);
	QRectF rect = m_part->property("rect").toRectF();
	x->setValue(m_part->mapToScene(rect.topLeft()).x() + (rect.width()/2));
	y->setValue(m_part->mapToScene(rect.topLeft()).y() + (rect.height()/2));
	h->setValue(rect.width());
	v->setValue(rect.height());
	start_angle->setValue(m_part->property("startAngle").toInt()/16);
	angle->setValue(m_part->property("spanAngle").toInt()/16);
	activeConnections(true);
}

void ArcEditor::updateRect()
{
	QRectF rect;

	auto pos_ = m_part->mapFromScene(QPointF(x->value(), y->value()));
	rect.setX(pos_.x() - h->value()/2);
	rect.setY(pos_.y() - v->value()/2);
	rect.setWidth(h->value());
	rect.setHeight(v->value());

	for (auto part: m_style->currentParts())
	{
		auto ellipse = static_cast<PartArc*>(part);
		if (rect != part->property("rect"))
		{
			auto undo= new QPropertyUndoCommand(ellipse, "rect", ellipse->property("rect"), rect);
			undo->setText("Modifier un arc");
			undo->enableAnimation();
			elementScene()->undoStack().push(undo);
		}
	}
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
		connect(x,           &QDoubleSpinBox::editingFinished, this, &ArcEditor::updateRect);
		connect(y,           &QDoubleSpinBox::editingFinished, this, &ArcEditor::updateRect);
		connect(h,           &QDoubleSpinBox::editingFinished, this, &ArcEditor::updateRect);
		connect(v,           &QDoubleSpinBox::editingFinished, this, &ArcEditor::updateRect);
		connect(start_angle, &QDoubleSpinBox::editingFinished, this, &ArcEditor::updateArcS);
		connect(angle,       &QDoubleSpinBox::editingFinished, this, &ArcEditor::updateArcA);
	}
	else
	{
		disconnect(x,           &QDoubleSpinBox::editingFinished, this, &ArcEditor::updateRect);
		disconnect(y,           &QDoubleSpinBox::editingFinished, this, &ArcEditor::updateRect);
		disconnect(h,           &QDoubleSpinBox::editingFinished, this, &ArcEditor::updateRect);
		disconnect(v,           &QDoubleSpinBox::editingFinished, this, &ArcEditor::updateRect);
		disconnect(start_angle, &QDoubleSpinBox::editingFinished, this, &ArcEditor::updateArcS);
		disconnect(angle,       &QDoubleSpinBox::editingFinished, this, &ArcEditor::updateArcA);
	}
}
