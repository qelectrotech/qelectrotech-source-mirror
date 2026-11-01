// SPDX-License-Identifier: GPL-2.0-or-later
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
#include "textdestinationdialog.h"

#include "../diagram.h"
#include "../qetproject.h"

#include <QComboBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLabel>
#include <QPushButton>

/**
	@brief TextDestinationDialog::TextDestinationDialog
	@param project the project whose folios are offered
	@param current the folio the text leads to now, null for none
	@param parent
*/
TextDestinationDialog::TextDestinationDialog(QETProject *project, const QUuid &current, QWidget *parent) :
	QDialog(parent)
{
	setWindowTitle(tr("Reference destination"));
	setMinimumWidth(420);
	auto *form = new QFormLayout(this);

	auto *note = new QLabel(tr("Navigation only: this does not make an electrical link. "
							   "Ctrl-click the text to go to the folio; in a PDF export the text becomes a link to it."));
	note->setWordWrap(true);
	form->addRow(note);

	m_folios = new QComboBox;
	const auto folios = project ? project->diagrams() : QList<Diagram *>();
	for (Diagram *folio : folios) {
		const QString name = folio->border_and_titleblock.finalfolio() + QStringLiteral(" - ")
				+ folio->border_and_titleblock.title();
		m_folios->addItem(name, folio->uuid());
	}
	m_folios->setCurrentIndex(qMax(0, m_folios->findData(current)));
	form->addRow(tr("Folio"), m_folios);

	auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
	auto *remove = buttons->addButton(tr("Remove the link"), QDialogButtonBox::DestructiveRole);
	remove->setEnabled(!current.isNull());
	form->addRow(buttons);
	connect(remove, &QPushButton::clicked, this, [this]() {
		m_remove = true;
		accept();
	});
	connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
	connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

/**
	@brief TextDestinationDialog::destination
	@return the folio chosen, null if the user asked to remove the link
*/
QUuid TextDestinationDialog::destination() const
{
	return m_remove ? QUuid() : m_folios->currentData().toUuid();
}
