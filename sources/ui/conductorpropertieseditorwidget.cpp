/*
	Copyright 2006-2026 The QElectroTech Team
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
#include "conductorpropertieseditorwidget.h"

#include "../QPropertyUndoCommand/qpropertyundocommand.h"
#include "../diagram.h"
#include "../qetgraphicsitem/conductor.h"
#include "conductorpropertieswidget.h"

#include <QVBoxLayout>

/**
	@brief ConductorPropertiesEditorWidget::ConductorPropertiesEditorWidget
	@param conductor : conductor to edit
	@param parent : parent widget
*/
ConductorPropertiesEditorWidget::ConductorPropertiesEditorWidget(
		Conductor *conductor, QWidget *parent) :
	PropertiesEditorWidget(parent),
	m_cpw(new ConductorPropertiesWidget(this))
{
	auto *layout = new QVBoxLayout(this);
	layout->setContentsMargins(0, 0, 0, 0);
	layout->addWidget(m_cpw);
	setDisabled(true);
	setConductor(conductor);
}

ConductorPropertiesEditorWidget::~ConductorPropertiesEditorWidget()
{}

/**
	@brief ConductorPropertiesEditorWidget::setConductor
	Set (or change) the conductor whose properties are edited.
	@param conductor
*/
void ConductorPropertiesEditorWidget::setConductor(Conductor *conductor)
{
	if (!conductor) return;
	m_conductor = conductor;
	setEnabled(true);
	updateUi();
}

/**
	@brief ConductorPropertiesEditorWidget::apply
	Push the edit onto the diagram's undo stack.
*/
void ConductorPropertiesEditorWidget::apply()
{
	if (!m_conductor || !m_conductor->diagram()) return;
	if (QUndoCommand *undo = associatedUndo())
		m_conductor->diagram()->undoStack().push(undo);
	m_initial = m_conductor->properties();
}

/**
	@brief ConductorPropertiesEditorWidget::reset
	Discard the in-progress edit, restoring the conductor's current properties.
*/
void ConductorPropertiesEditorWidget::reset()
{
	if (!m_conductor) return;
	m_cpw->setProperties(m_initial);
}

/**
	@brief ConductorPropertiesEditorWidget::updateUi
	Reload the widget from the conductor (e.g. when the selection changes).
*/
void ConductorPropertiesEditorWidget::updateUi()
{
	if (!m_conductor) return;
	m_initial = m_conductor->properties();
	m_cpw->setProperties(m_initial);
}

/**
	@brief ConductorPropertiesEditorWidget::associatedUndo
	@return the edit as a QPropertyUndoCommand, or nullptr if unchanged.

	Prototype note: applies only to the selected conductor. The modal dialog
	additionally offers to propagate to every conductor on the same potential
	(relatedPotentialConductors()); whether/how to expose that in the dock is
	the open design decision for #500.
*/
QUndoCommand *ConductorPropertiesEditorWidget::associatedUndo() const
{
	if (!m_conductor) return nullptr;

	const ConductorProperties new_properties = m_cpw->properties();
	if (new_properties == m_conductor->properties()) return nullptr;

	QVariant old_value, new_value;
	old_value.setValue(m_conductor->properties());
	new_value.setValue(new_properties);

	auto *undo = new QPropertyUndoCommand(
		m_conductor, "properties", old_value, new_value);
	undo->setText(tr("Modifier les propriétés d'un conducteur", "undo caption"));
	return undo;
}

/**
	@brief ConductorPropertiesEditorWidget::title
	@return the panel title.
*/
QString ConductorPropertiesEditorWidget::title() const
{
	return tr("Conducteur");
}
