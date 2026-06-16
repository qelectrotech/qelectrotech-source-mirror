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
#include "../qtextorientationspinboxwidget.h"

#include <KColorButton>

#include <QAbstractButton>
#include <QAbstractSpinBox>
#include <QComboBox>
#include <QGroupBox>
#include <QLineEdit>
#include <QScrollArea>
#include <QSizePolicy>
#include <QSlider>
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
	// The conductor widget is dialog-sized (min width ~600px), far too wide for
	// a dock. Host it in a scroll area with a small minimum width so the dock
	// can be dragged to any width (a scrollbar appears when narrower than the
	// content). QET already persists dock geometry across restarts via
	// QETDiagramEditor save/restoreState, so the chosen width is remembered.
	auto *scroll = new QScrollArea(this);
	scroll->setWidgetResizable(true);
	scroll->setFrameShape(QFrame::NoFrame);
	scroll->setWidget(m_cpw);
	auto *layout = new QVBoxLayout(this);
	layout->setContentsMargins(0, 0, 0, 0);
	layout->addWidget(scroll);
	setMinimumWidth(120);
	// Expand vertically to fill the dock like the other editors do (otherwise
	// the panel sits at its small size hint with empty space below it, #500),
	// while keeping a minimum height so it stays usable when the dock is short.
	setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
	setMinimumHeight(200);
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
	// Ignore the field-change signals emitted while the widget is being loaded
	// programmatically (updateUi/reset): mid-load the widget holds a partial
	// state that must not be committed onto the conductor.
	if (m_updating) return;
	if (!m_conductor || !m_conductor->diagram()) return;
	if (QUndoCommand *undo = associatedUndo())
		m_conductor->diagram()->undoStack().push(undo);
	m_initial = m_conductor->properties();
}

/**
	@brief ConductorPropertiesEditorWidget::setLiveEdit
	In live-edit mode (how the dock uses every editor), each field change is
	applied immediately instead of via an explicit apply() call. Without this
	override the base class is a no-op and edits in the dock were never applied
	(issue #500).
	@param live_edit true to enable live edit
	@return always true
*/
bool ConductorPropertiesEditorWidget::setLiveEdit(bool live_edit)
{
	if (m_live_edit == live_edit) return true;
	m_live_edit = live_edit;

	if (m_live_edit) connectChangeSignals();
	else             disconnectChangeSignals();

	return true;
}

/**
	@brief ConductorPropertiesEditorWidget::connectChangeSignals
	Wire every editable control of the hosted ConductorPropertiesWidget to
	apply(). Commit-style signals (editingFinished / activated / toggled /
	sliderReleased) are used rather than per-keystroke ones so each edit yields
	a single, clean undo step. Loading the widget programmatically (updateUi)
	also fires some of these, but apply() is a no-op then because
	associatedUndo() returns nullptr when the properties are unchanged.
*/
void ConductorPropertiesEditorWidget::connectChangeSignals()
{
	if (!m_cpw) return;

	const auto add = [this](QMetaObject::Connection c) {
		m_live_connections << c;
	};

	for (auto *w : m_cpw->findChildren<QLineEdit *>())
		add(connect(w, &QLineEdit::editingFinished,
					this, &ConductorPropertiesEditorWidget::apply));
	for (auto *w : m_cpw->findChildren<QAbstractSpinBox *>())
		add(connect(w, &QAbstractSpinBox::editingFinished,
					this, &ConductorPropertiesEditorWidget::apply));
	for (auto *w : m_cpw->findChildren<QComboBox *>())
		add(connect(w, QOverload<int>::of(&QComboBox::activated),
					this, &ConductorPropertiesEditorWidget::apply));
	for (auto *w : m_cpw->findChildren<QSlider *>())
		add(connect(w, &QSlider::sliderReleased,
					this, &ConductorPropertiesEditorWidget::apply));
	for (auto *w : m_cpw->findChildren<KColorButton *>())
		add(connect(w, &KColorButton::changed,
					this, &ConductorPropertiesEditorWidget::apply));
	for (auto *w : m_cpw->findChildren<QTextOrientationSpinBoxWidget *>())
		add(connect(w, QOverload<>::of(&QTextOrientationSpinBoxWidget::editingFinished),
					this, &ConductorPropertiesEditorWidget::apply));
	// Checkboxes and the checkable group boxes (single/multi wire, bicolor…).
	for (auto *w : m_cpw->findChildren<QAbstractButton *>())
		if (w->isCheckable())
			add(connect(w, &QAbstractButton::toggled,
						this, &ConductorPropertiesEditorWidget::apply));
	for (auto *w : m_cpw->findChildren<QGroupBox *>())
		if (w->isCheckable())
			add(connect(w, &QGroupBox::toggled,
						this, &ConductorPropertiesEditorWidget::apply));
}

/**
	@brief ConductorPropertiesEditorWidget::disconnectChangeSignals
	Tear down the live-edit connections made by connectChangeSignals().
*/
void ConductorPropertiesEditorWidget::disconnectChangeSignals()
{
	for (const QMetaObject::Connection &c : m_live_connections)
		disconnect(c);
	m_live_connections.clear();
}

/**
	@brief ConductorPropertiesEditorWidget::reset
	Discard the in-progress edit, restoring the conductor's current properties.
*/
void ConductorPropertiesEditorWidget::reset()
{
	if (!m_conductor) return;
	m_updating = true;
	m_cpw->setProperties(m_initial);
	m_updating = false;
}

/**
	@brief ConductorPropertiesEditorWidget::updateUi
	Reload the widget from the conductor (e.g. when the selection changes).
*/
void ConductorPropertiesEditorWidget::updateUi()
{
	if (!m_conductor) return;
	m_updating = true;
	m_initial = m_conductor->properties();
	m_cpw->setProperties(m_initial);
	m_updating = false;
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
