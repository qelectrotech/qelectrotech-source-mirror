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

#include "../conductormultiedit.h"
#include "../diagram.h"
#include "../qetgraphicsitem/conductor.h"
#include "conductorpropertieswidget.h"
#include "../qtextorientationspinboxwidget.h"

#ifdef BUILD_WITHOUT_KF
#	include "nokde/kcolorbutton.h"
#else
#	include <KColorButton>
#endif

#include <QAbstractButton>
#include <QAbstractSpinBox>
#include <QCheckBox>
#include <QComboBox>
#include <QGroupBox>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QScrollArea>
#include <QSettings>
#include <QSizePolicy>
#include <QSlider>
#include <QVBoxLayout>

/**
	@brief ConductorPropertiesEditorWidget::ConductorPropertiesEditorWidget
	@param conductors : conductors to edit
	@param parent : parent widget
*/
ConductorPropertiesEditorWidget::ConductorPropertiesEditorWidget(
		const QList<Conductor *> &conductors, QWidget *parent) :
	PropertiesEditorWidget(parent),
	m_cpw(new ConductorPropertiesWidget(this))
{
	// The conductor widget is dialog-sized (min width ~600px), far too wide for
	// a dock. Host it in a scroll area with a small minimum width so the dock
	// can be dragged to any width (a scrollbar appears when narrower than the
	// content). QET already persists dock geometry across restarts via
	// QETDiagramEditor save/restoreState, so the chosen width is remembered.
	// "Apply to all conductors of the potential": same semantics as the modal
	// dialog's checkbox (ConductorPropertiesDialog::applyAll), pinned at the top
	// of the panel so it stays visible above the scrolling tabs (#500). Unlike
	// the dialog the choice is persisted, so a user who always wants it on (or
	// off) sets it once. It is a child of this editor, not of m_cpw, so it is
	// deliberately outside the live-edit signal wiring in connectChangeSignals()
	// (toggling it must not push an edit, only change how the next edit applies).
	// Reuse the modal dialog's exact wording for consistency (and so the
	// existing translation applies).
	m_apply_all_cb = new QCheckBox(
		tr("Apply properties to all conductors of this potential"),
		this);
	m_apply_all_cb->setChecked(QSettings().value(
		QStringLiteral("diagrameditor/conductor_apply_all"), true).toBool());
	connect(m_apply_all_cb, &QCheckBox::toggled, this, [](bool on) {
		QSettings().setValue(
			QStringLiteral("diagrameditor/conductor_apply_all"), on);
	});

		//Shown only when several conductors are selected: the fields hold
		//the first one's values, and only the edited ones reach the others.
	m_count_label = new QLabel(this);
	m_count_label->setWordWrap(true);
	m_count_label->hide();

	auto *scroll = new QScrollArea(this);
	scroll->setWidgetResizable(true);
	scroll->setFrameShape(QFrame::NoFrame);
	scroll->setWidget(m_cpw);
	auto *layout = new QVBoxLayout(this);
	layout->setContentsMargins(0, 0, 0, 0);
	layout->addWidget(m_count_label);
	layout->addWidget(m_apply_all_cb);
	layout->addWidget(scroll);
	setMinimumWidth(120);
	// Expand vertically to fill the dock like the other editors do (otherwise
	// the panel sits at its small size hint with empty space below it, #500),
	// while keeping a minimum height so it stays usable when the dock is short.
	setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
	setMinimumHeight(200);
		//Enter in a field (Function, Section...) is not used by the field: it
		//goes on to the checkable "Multifilaire" or "Unifilaire" group box
		//around it, which takes it as a click and switches the wire between
		//multi-line and single-line. The modal dialog never shows this, its
		//OK button takes Enter; here nothing does, so stop it at the box.
	for (auto *gb : m_cpw->findChildren<QGroupBox *>())
		if (gb->isCheckable())
			gb->installEventFilter(this);

	setDisabled(true);
	setConductors(conductors);
}

/**
	@brief ConductorPropertiesEditorWidget::eventFilter
	Swallow Enter on the checkable group boxes, see the constructor.
*/
bool ConductorPropertiesEditorWidget::eventFilter(QObject *watched, QEvent *event)
{
	if (event->type() == QEvent::KeyPress
		|| event->type() == QEvent::KeyRelease)
	{
		const int key = static_cast<QKeyEvent *>(event)->key();
		if (key == Qt::Key_Return || key == Qt::Key_Enter)
			return true;
	}
	return PropertiesEditorWidget::eventFilter(watched, event);
}

ConductorPropertiesEditorWidget::~ConductorPropertiesEditorWidget()
{}

/**
	@brief ConductorPropertiesEditorWidget::setConductors
	Set (or change) the conductors whose properties are edited.
	@param conductors
*/
void ConductorPropertiesEditorWidget::setConductors(
		const QList<Conductor *> &conductors)
{
	if (conductors.isEmpty()) return;

	for (const QPointer<Conductor> &c : std::as_const(m_conductors))
		if (c)
			disconnect(c, &Conductor::propertiesChange,
					   this, &ConductorPropertiesEditorWidget::scheduleUpdateUi);
	m_conductors.clear();

		//The scene gives no order for its selection: sort, so the same
		//wires always show the same "first" one.
	QList<Conductor *> sorted = conductors;
	ConductorMultiEdit::sortByPosition(sorted, [](Conductor *c) {
		return c->sceneBoundingRect().topLeft();
	});

	for (Conductor *c : std::as_const(sorted))
	{
		m_conductors << c;
			//Keep the dock in sync when a conductor is edited elsewhere (e.g.
			//the modal "Edit conductor" dialog); otherwise a stale snapshot
			//would be written back on the next apply() and overwrite that
			//change (issue #500). One edit of N conductors emits N signals:
			//they are gathered into one reload.
		connect(c, &Conductor::propertiesChange,
				this, &ConductorPropertiesEditorWidget::scheduleUpdateUi,
				Qt::UniqueConnection);
	}

	const int count = conductors.size();
	m_count_label->setText(
		tr("%n conductors selected: only the fields you change are applied to them.",
		   "selection properties panel", count));
	m_count_label->setVisible(count > 1);

	setEnabled(true);
	updateUi();
}

/**
	@brief ConductorPropertiesEditorWidget::firstConductor
	@return the first edited conductor still alive, its values are the ones
	shown, or nullptr
*/
Conductor *ConductorPropertiesEditorWidget::firstConductor() const
{
	for (const QPointer<Conductor> &c : m_conductors)
		if (c)
			return c;
	return nullptr;
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
	Conductor *first = firstConductor();
	if (!first || !first->diagram()) return;
	if (QUndoCommand *undo = associatedUndo())
		first->diagram()->undoStack().push(undo);
	updateUi();
}

/**
	@brief ConductorPropertiesEditorWidget::scheduleUpdateUi
	Reload the widget once the current event is done, however many
	conductors changed in it.
*/
void ConductorPropertiesEditorWidget::scheduleUpdateUi()
{
	if (m_update_pending) return;
	m_update_pending = true;
	QMetaObject::invokeMethod(this, [this]() {
		if (m_update_pending) updateUi();
	}, Qt::QueuedConnection);
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
	Discard the in-progress edit, restoring what the widget showed.
*/
void ConductorPropertiesEditorWidget::reset()
{
	if (!firstConductor()) return;
	m_updating = true;
	m_cpw->setProperties(m_shown);
	m_updating = false;
}

/**
	@brief ConductorPropertiesEditorWidget::updateUi
	Reload the widget from the conductors (e.g. when the selection
	changes): the first one's values, with the text fields they do not
	agree on left blank.
*/
void ConductorPropertiesEditorWidget::updateUi()
{
	m_update_pending = false;
	QList<ConductorProperties> list;
	for (const QPointer<Conductor> &c : std::as_const(m_conductors))
		if (c)
			list << c->properties();
	if (list.isEmpty()) return;

	const auto mixed = ConductorMultiEdit::mixedTextFields(list);
	m_updating = true;
	m_cpw->setProperties(ConductorMultiEdit::shown(list, mixed));
	m_cpw->setMixedTextFields(mixed);
	m_cpw->setTextLocked(list.size() > 1);
		//Read back rather than keep the conductor's own values: a value the
		//widget cannot show exactly must not count as an edit.
	m_shown = m_cpw->properties();
	m_updating = false;
}

/**
	@brief ConductorPropertiesEditorWidget::associatedUndo
	@return the edit as one undo step, or nullptr if nothing changes.

	Only the fields the user changed are applied, to each edited conductor:
	with several conductors selected, each keeps its own text, function,
	cable... unless that is the field being edited.

	When "apply to all" is ticked, every conductor on the same potential as
	an edited one is updated too, in the same undo step (one undo reverts
	them all), as the modal dialog does
	(ConductorPropertiesDialog::PropertiesDialog).
*/
QUndoCommand *ConductorPropertiesEditorWidget::associatedUndo() const
{
		//Most calls are a field losing focus with nothing edited: answer
		//before walking the potentials.
	const ConductorProperties edited = m_cpw->properties();
	if (edited == m_shown) return nullptr;

	QList<Conductor *> selected;
	for (const QPointer<Conductor> &c : m_conductors)
		if (c)
			selected << c.data();

	return ConductorMultiEdit::undo(
		ConductorMultiEdit::targets(
			selected, m_apply_all_cb && m_apply_all_cb->isChecked()),
		m_shown, edited);
}

/**
	@brief ConductorPropertiesEditorWidget::title
	@return the panel title.
*/
QString ConductorPropertiesEditorWidget::title() const
{
	return tr("Conductor");
}
