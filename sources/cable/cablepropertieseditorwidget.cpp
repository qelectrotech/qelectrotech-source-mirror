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
#include "cablepropertieseditorwidget.h"

#include "cablecreatedialog.h"
#include "cablepart.h"
#include "cablepropertieswidget.h"
#include "editcablecommand.h"
#include "../qeticons.h"
#include "../qetproject.h"

#include <QAbstractItemView>
#include <QLabel>
#include <QListWidget>
#include <QMenu>
#include <QSizePolicy>
#include <QUndoStack>
#include <QVBoxLayout>

/**
	@brief CablePropertiesEditorWidget::CablePropertiesEditorWidget
	@param cable the cable being edited
	@param part the line of that cable this panel was opened by, which
	is the line a core picked here is put down on
	@param parent parent widget
*/
CablePropertiesEditorWidget::CablePropertiesEditorWidget(Cable *cable,
														 CablePart *part,
														 QWidget *parent) :
	PropertiesEditorWidget(parent),
	m_fields(new CablePropertiesWidget(this)),
	m_info(new QLabel(this)),
	m_free_label(new QLabel(this)),
	m_free_cores(new QListWidget(this))
{
	m_info->setWordWrap(true);

		//The cores which stand nowhere yet, listed under the count of
		//wired ones: double-clicking one puts it down on this very line
		//without any dialog having to be opened for it. The list takes
		//no keyboard focus, so clicking it never takes the drawing away
		//from the user -- startPlacingCore() hands the focus to the
		//sheet itself, which is where Escape has to be heard.
	m_free_label->setText(tr("Unplaced cores:"));
	m_free_label->setVisible(false);
	m_free_cores->setSelectionMode(QAbstractItemView::SingleSelection);
	m_free_cores->setAlternatingRowColors(true);
	m_free_cores->setFocusPolicy(Qt::NoFocus);
	m_free_cores->setToolTip(tr("Double-click a core to place it on the "
								"line: the line follows the mouse and a "
								"click leaves it where you aimed."));
	m_free_cores->setVisible(false);
	m_free_cores->setContextMenuPolicy(Qt::CustomContextMenu);
	connect(m_free_cores, &QWidget::customContextMenuRequested,
			this, &CablePropertiesEditorWidget::freeCoreMenu);
	connect(m_free_cores, &QListWidget::itemDoubleClicked,
			this, &CablePropertiesEditorWidget::placeRequested);
	connect(m_free_cores, &QListWidget::itemActivated,
			this, &CablePropertiesEditorWidget::placeRequested);

	auto *layout = new QVBoxLayout(this);
	layout->setContentsMargins(0, 0, 0, 0);
	layout->setSpacing(4);
	layout->addWidget(m_fields);

		//The button beside the type field: it opens the window which
		//opens when a cable line has just been drawn, in the mode where
		//it changes the type of this very cable.
	connect(m_fields, &CablePropertiesWidget::typeChangeRequested,
			this, &CablePropertiesEditorWidget::changeTypeRequested);

	layout->addWidget(m_info);
	layout->addWidget(m_free_label);
	layout->addWidget(m_free_cores);
	layout->addStretch(1);

		//Wide enough for the labels, the ticks, the fields, the buttons
		//setting how a text is written and the two distances nudging
		//the label and the row of colours -- they sit side by side.
	setMinimumWidth(250);
	setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);

	setDisabled(true);
	setCable(cable, part);
}

/**
	@brief CablePropertiesEditorWidget::setCable
	Set (or change) the cable whose properties are edited.
	@param cable
	@param part the line of that cable this panel was opened by: the one
	a core picked in the list is put down on, and null while no single
	line is selected -- then nothing can be put down and the list stays
	out of the way
*/
void CablePropertiesEditorWidget::setCable(Cable *cable, CablePart *part)
{
	if (!cable) return;
	m_part = part;
	if (m_cable && m_cable != cable)
		disconnect(m_cable, nullptr, this, nullptr);

	m_cable = cable;
		//Keep the panel in sync when the cable is edited elsewhere (the
		//dialog reached by double-clicking its line, for one): otherwise
		//the panel would write back what it held before that edit.
	connect(m_cable, &Cable::changed,
			this, &CablePropertiesEditorWidget::updateUi, Qt::UniqueConnection);
	setEnabled(true);
	updateUi();
}

/**
	@brief CablePropertiesEditorWidget::setLiveEdit
	In live-edit mode (how the panel uses every editor) an edit is
	applied as soon as a field is finished, rather than waiting for a
	call to apply(). On top of that the text itself is written onto the
	cable with every character, so what he types shows up on the drawing
	while he is still typing it -- the whole editing of one field stays
	one single undo step, which is worked out when the field is left.
	@param live_edit true to enable live edit
	@return always true
*/
bool CablePropertiesEditorWidget::setLiveEdit(bool live_edit)
{
	if (m_live_edit == live_edit) return true;
	m_live_edit = live_edit;

	if (m_live_edit) {
		m_live_connection = connect(m_fields, &CablePropertiesWidget::propertiesEdited,
									this, &CablePropertiesEditorWidget::apply);
		connect(m_fields, &CablePropertiesWidget::propertiesTyped,
				this, &CablePropertiesEditorWidget::applyLive);
	} else {
		disconnect(m_live_connection);
		disconnect(m_fields, &CablePropertiesWidget::propertiesTyped,
				   this, &CablePropertiesEditorWidget::applyLive);
	}
	return true;
}

/**
	@brief CablePropertiesEditorWidget::applyLive
	Put what is being typed straight onto the cable, without pushing an
	undo step: the whole editing of one field is one step, which
	apply() works out from what the panel showed when the field is left.
*/
void CablePropertiesEditorWidget::applyLive()
{
		//Ignore the signals sent while the form is being filled in: at
		//that moment it holds a half loaded state.
	if (m_updating) return;
	if (!m_cable) return;

	m_live_cable = m_cable;
	m_cable->setProperties(m_fields->properties());
}

/**
	@brief CablePropertiesEditorWidget::changeTypeRequested
	Open the window which opens when a cable line has just been drawn,
	in the mode where it changes the type of this very cable: he picks
	the new type there out of the cable type file, and the cable takes
	it -- cores reworked, marks redrawn -- as one single undo step.

	The form is filled in again afterwards rather than through the usual
	path: the type has just changed under it, and it may not go on
	offering the one which was left.
*/
void CablePropertiesEditorWidget::changeTypeRequested()
{
	if (!m_cable) return;

	CableCreateDialog dialog(m_part ? m_part->diagram() : nullptr,
							 m_cable,
							 this);
	if (dialog.exec() != QDialog::Accepted) return;

	m_updating = true;
	m_fields->setProperties(m_cable->properties());
	m_shown = m_cable->properties();
	m_updating = false;
	updateInfo();
}

/**
	@brief CablePropertiesEditorWidget::apply
	Put what was typed onto the cable as one undo step.
*/
void CablePropertiesEditorWidget::apply()
{
		//Ignore the signals sent while the form is being filled in: at
		//that moment it holds a half loaded state.
	if (m_updating) return;
	if (!m_cable) return;

	if (QUndoCommand *undo = associatedUndo())
	{
		auto *project = qobject_cast<QETProject *>(m_cable->parent());
		if (project && project->undoStack()) {
			project->undoStack()->push(undo);
		} else {
			m_cable->setProperties(m_fields->properties());
			delete undo;
		}
	}
	m_shown = m_fields->properties();
}

/**
	@brief CablePropertiesEditorWidget::associatedUndo
	@return the edit as one undo step, or nullptr when nothing changed
*/
QUndoCommand *CablePropertiesEditorWidget::associatedUndo() const
{
	if (!m_cable) return nullptr;

	const CableProperties after = m_fields->properties();
	if (after == m_shown) return nullptr;
	return new ChangeCablePropertiesCommand(m_cable, m_shown, after);
}

/**
	@brief CablePropertiesEditorWidget::reset
	Discard the edit in progress, putting back what the panel showed.
*/
void CablePropertiesEditorWidget::reset()
{
	if (!m_cable) return;
	m_updating = true;
	m_fields->setProperties(m_shown);
	m_updating = false;
}

/**
	@brief CablePropertiesEditorWidget::updateInfo
	Say how many cores of this cable are wired to a conductor.
*/
void CablePropertiesEditorWidget::updateInfo()
{
	if (!m_cable) return;

	m_info->setText(tr("%1 of %2 cores wired to a conductor",
					   "how many cores of this cable are wired",
					   m_cable->usedCoreCount())
						.arg(m_cable->usedCoreCount())
						.arg(m_cable->coreCount()));

	updateFreeCores();
}

/**
	@brief CablePropertiesEditorWidget::updateFreeCores
	List the cores the type of this cable offers which stand nowhere on
	the sheet yet: those are the ones he can still put down, and they
	come down straight from here, on the line this panel was opened by.

	The list is of no use when there is none -- and it cannot work at
	all when no single line of the cable is the selected one, since a
	core then has no line to be put down on -- so it takes no room then.
*/
void CablePropertiesEditorWidget::updateFreeCores()
{
	if (!m_free_cores) return;

	const bool usable = m_cable && m_part && m_part->cable() == m_cable;

	m_free_cores->clear();
	if (usable)
	{
		for (int i = 0; i < m_cable->coreCount(); ++i)
		{
			if (m_cable->hasCore(i)) continue;

			const QString color = m_cable->colorOfCore(i);
			auto *item = new QListWidgetItem(
				color.isEmpty() ? tr("Core %1").arg(i + 1)
								: tr("Core %1 — %2").arg(i + 1).arg(color),
				m_free_cores);
			item->setData(Qt::UserRole, i);
		}
	}

	const bool any = usable && m_free_cores->count() > 0;
	m_free_label->setVisible(any);
	m_free_cores->setVisible(any);
	if (!any) return;

		//As high as the entries it really shows, four of them at most:
		//the dock is small, and a handful of free cores must not turn
		//it into a wall.
	const int row = qMax(m_free_cores->sizeHintForRow(0), 18);
	m_free_cores->setFixedHeight(row * qMin(4, m_free_cores->count())
								 + 2 * m_free_cores->frameWidth() + 4);
}

/**
	@brief CablePropertiesEditorWidget::placeRequested
	He double-clicked one of the cores which stand nowhere: this very
	line takes the mouse and waits for the click which puts the core
	down. There is no dialog to close here -- the panel stays where it
	is, and it reads the cable back as soon as the core lands.

	@param item the entry he double-clicked
*/
void CablePropertiesEditorWidget::placeRequested(QListWidgetItem *item)
{
	if (!item || !m_part) return;

	m_part->startPlacingCore(item->data(Qt::UserRole).toInt());
}

/**
	@brief CablePropertiesEditorWidget::freeCoreMenu
	The small menu a right click on a core which stands nowhere opens:
	one entry, doing what a double click does -- the line takes the
	mouse, the mark shows on it at once and the mouse is put down there,
	so the very next click leaves the core standing on that line.

	@param pos where he right-clicked, in list coordinates
*/
void CablePropertiesEditorWidget::freeCoreMenu(const QPoint &pos)
{
	QListWidgetItem *item = m_free_cores ? m_free_cores->itemAt(pos) : nullptr;
	if (!item || !m_part) return;

	m_free_cores->setCurrentItem(item);

	QMenu menu(m_free_cores);
	QAction *place = menu.addAction(tr("Place this core on the line"));
	place->setIcon(QET::Icons::Cable);

	if (menu.exec(m_free_cores->viewport()->mapToGlobal(pos)) == place) {
			//Gone before he is sent off: the mouse goes to the line
			//and nothing may be left standing over the place it is
			//sent to.
		menu.hide();
		placeRequested(item);
	}
}

/**
	@brief CablePropertiesEditorWidget::updateUi
	Read the cable back into the panel.

	Not while a field is being typed in: writing what was just typed
	back into that field would take the cursor away from the user with
	every key, and it would take back the state the whole editing of
	that field is measured against when the undo step is worked out.
*/
void CablePropertiesEditorWidget::updateUi()
{
	if (!m_cable) return;

	if (m_fields->editing() && m_live_cable == m_cable) {
		updateInfo();
		return;
	}

	m_updating = true;
	m_fields->setProperties(m_cable->properties());
	m_shown = m_cable->properties();
	updateInfo();
	m_updating = false;
}

/**
	@brief CablePropertiesEditorWidget::title
	@return the panel title
*/
QString CablePropertiesEditorWidget::title() const
{
	return tr("Cable");
}
