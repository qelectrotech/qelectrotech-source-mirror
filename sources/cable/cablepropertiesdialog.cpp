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
#include "cablepropertiesdialog.h"

#include "cablecreatedialog.h"
#include "cablepart.h"
#include "cablepropertieswidget.h"
#include "editcablecommand.h"
#include "../diagram.h"
#include "../qeticons.h"

#include <QAbstractItemView>
#include <QDialogButtonBox>
#include <QLabel>
#include <QListWidget>
#include <QMenu>
#include <QUndoStack>
#include <QVBoxLayout>

/**
	@brief CablePropertiesDialog::CablePropertiesDialog
	@param cable the cable being edited
	@param diagram the folio the line was double-clicked on, whose undo
	stack takes the edit back
	@param part the very line which was double-clicked: a core put down
	from this dialog lands on that line, never on the first line of a
	cable which stands on two folios
	@param parent
*/
CablePropertiesDialog::CablePropertiesDialog(Cable *cable,
											 Diagram *diagram,
											 CablePart *part,
											 QWidget *parent) :
	QDialog(parent),
	m_cable(cable),
	m_diagram(diagram),
	m_part(part)
{
	setWindowTitle(tr("Cable"));

	auto *layout = new QVBoxLayout(this);
	m_fields = new CablePropertiesWidget(this);
	layout->addWidget(m_fields);

		//The button beside the type field: it opens the window which
		//opens when a cable line has just been drawn, in the mode where
		//it changes the type of this very cable.
	connect(m_fields, &CablePropertiesWidget::typeChangeRequested,
			this, &CablePropertiesDialog::changeTypeRequested);

	m_cores = new QLabel(this);
	m_cores->setWordWrap(true);
	layout->addWidget(m_cores);

		//The cores the type offers which are not on the sheet yet.
		//Double-clicking one brings it in: the line it was asked from
		//takes the mouse and the next click puts the core down there.
	m_free_label = new QLabel(tr("Unplaced cores:"), this);
	m_free_cores = new QListWidget(this);
	m_free_cores->setSelectionMode(QAbstractItemView::SingleSelection);
	m_free_cores->setAlternatingRowColors(true);
	m_free_cores->setToolTip(tr("Double-click a core to place it on the "
								"line: the line follows the mouse and a "
								"click leaves it where you aimed."));
	m_free_cores->setContextMenuPolicy(Qt::CustomContextMenu);
	connect(m_free_cores, &QWidget::customContextMenuRequested,
			this, &CablePropertiesDialog::freeCoreMenu);
	connect(m_free_cores, &QListWidget::itemDoubleClicked,
			this, &CablePropertiesDialog::placeRequested);
	connect(m_free_cores, &QListWidget::itemActivated,
			this, &CablePropertiesDialog::placeRequested);
	layout->addWidget(m_free_label);
	layout->addWidget(m_free_cores);

	auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
	connect(buttons, &QDialogButtonBox::accepted, this, &CablePropertiesDialog::accept);
	connect(buttons, &QDialogButtonBox::rejected, this, &CablePropertiesDialog::reject);
	layout->addWidget(buttons);

	if (m_cable) {
		m_fields->setProperties(m_cable->properties());
	}
	updateCoreCount();

	fillFreeCores();

	setMinimumWidth(400);
}

/**
	@brief CablePropertiesDialog::updateCoreCount
	Say how many cores this cable has now and how many of them are wired
	to a conductor. It is said again after the type was changed, since
	the new type may hold more cores -- or fewer.
*/
void CablePropertiesDialog::updateCoreCount()
{
	if (!m_cores) return;

	if (!m_cable)
	{
		m_cores->setText(tr("No cable."));
		return;
	}

	m_cores->setText(tr("%1 of %2 cores wired to a conductor",
						"how many cores of this cable are wired",
						m_cable->usedCoreCount())
						.arg(m_cable->usedCoreCount())
						.arg(m_cable->coreCount()));
}

/**
	@brief CablePropertiesDialog::changeTypeRequested
	Open the window which opens when a cable line has just been drawn,
	in the mode where it changes the type of this very cable: he picks
	the new type there out of the cable type file, and the cable takes
	it -- cores reworked, marks redrawn -- as one single undo step.

	Whatever he typed into the form is written first, so the change and
	the typing cannot overwrite each other; and the form is filled in
	again afterwards, since pressing OK right after would otherwise
	write the old type back.
*/
void CablePropertiesDialog::changeTypeRequested()
{
	if (!m_cable) return;

	apply();

	CableCreateDialog dialog(m_diagram, m_cable, this);
	if (dialog.exec() != QDialog::Accepted) return;

	m_fields->setProperties(m_cable->properties());
	updateCoreCount();
	fillFreeCores();
}

/**
	@brief CablePropertiesDialog::fillFreeCores
	List every core the type of this cable offers and which stands
	nowhere on the sheet: those are the ones he can still put down. The
	list is of no use when there is none, so it takes no room then.
*/
void CablePropertiesDialog::fillFreeCores()
{
	if (!m_free_cores) return;

	m_free_cores->clear();
	for (int i = 0; m_cable && i < m_cable->coreCount(); ++i)
	{
		if (m_cable->hasCore(i)) continue;

		const QString color = m_cable->colorOfCore(i);
		auto *item = new QListWidgetItem(
			color.isEmpty() ? tr("Core %1").arg(i + 1)
							: tr("Core %1 — %2").arg(i + 1).arg(color),
			m_free_cores);
		item->setData(Qt::UserRole, i);
	}

	const bool any = m_part && m_free_cores->count() > 0;
	m_free_label->setVisible(any);
	m_free_cores->setVisible(any);
	if (!any) return;

		//As high as the entries it really shows, six of them at most:
		//a handful of free cores must not turn the dialog into a wall.
	const int row = qMax(m_free_cores->sizeHintForRow(0), 18);
	m_free_cores->setFixedHeight(row * qMin(6, m_free_cores->count())
								 + 2 * m_free_cores->frameWidth() + 4);
}

/**
	@brief CablePropertiesDialog::placeRequested
	He picked one of the cores which stand nowhere: whatever is typed in
	the form is written first, the dialog closes, and the line he asked
	it from takes the mouse -- so the core goes onto that very line, and
	the next click leaves it standing there.
	@param item the entry he double-clicked
*/
void CablePropertiesDialog::placeRequested(QListWidgetItem *item)
{
		//A double click reports itself twice on some platforms (once as
		//activated, once as double-clicked): the first one takes the
		//mouse away, the second must not ask for it again.
	if (!item || !m_part || m_placing) return;

	m_placing = true;
	const int core = item->data(Qt::UserRole).toInt();
	QPointer<CablePart> part = m_part;

	apply();
	QDialog::accept();

	part->startPlacingCore(core);
}

/**
	@brief CablePropertiesDialog::freeCoreMenu
	The small menu a right click on a core which stands nowhere opens:
	one entry, doing what a double click does -- this dialog writes
	whatever he typed, closes itself, the line he asked it from takes
	the mouse, the mark shows on that line at once and the mouse is put
	down there, so that the very next click leaves the core standing on
	that line.

	@param pos where he right-clicked, in list coordinates
*/
void CablePropertiesDialog::freeCoreMenu(const QPoint &pos)
{
	QListWidgetItem *item = m_free_cores ? m_free_cores->itemAt(pos) : nullptr;
	if (!item || !m_part || m_placing) return;

	m_free_cores->setCurrentItem(item);

	QMenu menu(m_free_cores);
	QAction *place = menu.addAction(tr("Place this core on the line"));
	place->setIcon(QET::Icons::Cable);

	if (menu.exec(m_free_cores->viewport()->mapToGlobal(pos)) == place) {
			//Gone before he is sent off: this dialog is closing, the
			//mouse goes to the line and nothing may be left standing
			//over the place it is sent to.
		menu.hide();
		placeRequested(item);
	}
}

/**
	@brief CablePropertiesDialog::accept
	Whatever was typed here is written into the cable as one undo step,
	and the cable works out the cable field of every conductor it feeds
	again on its own: rename the cable and every drawing which shows it
	follows.
*/
void CablePropertiesDialog::accept()
{
	apply();
	QDialog::accept();
}

/**
	@brief CablePropertiesDialog::apply
	Everything the form holds goes into the cable as one single undo
	step -- and only when something really changed, so that closing the
	dialog by any means leaves no step of undo to walk back through.
*/
void CablePropertiesDialog::apply()
{
	if (!m_cable) return;

	const CableProperties before = m_cable->properties();
	const CableProperties after = m_fields->properties();
	if (after != before)
	{
		if (m_diagram) {
			m_diagram->undoStack().push(
				new ChangeCablePropertiesCommand(m_cable, before, after));
		} else {
			m_cable->setProperties(after);
		}
	}
}
