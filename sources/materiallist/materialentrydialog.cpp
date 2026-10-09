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
#include "materialentrydialog.h"
#include "../qet.h"

#include "../qetmessagebox.h"

#include <QDialogButtonBox>
#include <QGridLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QScrollArea>
#include <QVBoxLayout>

/**
	@brief MaterialEntryDialog::MaterialEntryDialog
	Build one field per column of the material file.
	@param columns the columns of the file, in file order
	@param parent parent widget
*/
MaterialEntryDialog::MaterialEntryDialog(const QStringList &columns, QWidget *parent) :
	QDialog(parent),
	m_columns(columns)
{
	setWindowTitle(tr("New entry"));

		//Two fields per row : the form shows the whole article at once
		//instead of stacking twenty lines in a narrow column.
	setMinimumSize(780, 560);
	resize(1120, 800);

	auto *main_layout = new QVBoxLayout(this);

	auto *intro = new QLabel(
		tr("Fill in the item to add. Fields left empty stay empty in "
		   "the file."), this);
	intro->setWordWrap(true);
	main_layout->addWidget(intro);

	auto *scroll_area = new QScrollArea(this);
	scroll_area->setWidgetResizable(true);
	scroll_area->setFrameShape(QFrame::NoFrame);

	auto *container = new QWidget(scroll_area);
	auto *grid = new QGridLayout(container);

	for (int i = 0; i < m_columns.size(); ++i)
	{
		const QString column = m_columns.at(i);

		auto *label = new QLabel(MaterialList::translatedColumn(column) + QLatin1Char(' '), container);
		label->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

		auto *edit = new QLineEdit(container);
		edit->setClearButtonEnabled(true);

		label->setBuddy(edit);
		const int row = i / 2;
		const int pair = i % 2;
		grid->addWidget(label, row, pair * 2);
		grid->addWidget(edit, row, pair * 2 + 1);
		m_edits.append(edit);
	}
	grid->setColumnStretch(1, 1);
	grid->setColumnStretch(3, 1);

	scroll_area->setWidget(container);
	main_layout->addWidget(scroll_area, 1);

	auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
	buttons->button(QDialogButtonBox::Ok)->setText(tr("Save"));
	buttons->button(QDialogButtonBox::Cancel)->setText(tr("Cancel"));
	connect(buttons, &QDialogButtonBox::accepted, this, &MaterialEntryDialog::accept);
	connect(buttons, &QDialogButtonBox::rejected, this, &MaterialEntryDialog::reject);
	main_layout->addWidget(buttons);

	if (!m_edits.isEmpty()) {
		m_edits.first()->setFocus();
	}

	QET::trackDialogGeometry(this);
}

/**
	@brief MaterialEntryDialog::~MaterialEntryDialog
	Destructor
*/
MaterialEntryDialog::~MaterialEntryDialog() = default;

/**
	@brief MaterialEntryDialog::record
	@return the article described by the form. Empty fields are left out :
	an empty column never means anything for an article which is being
	created.
*/
MaterialRecord MaterialEntryDialog::record() const
{
	MaterialRecord record;

	for (int i = 0; i < m_columns.size() && i < m_edits.size(); ++i)
	{
		QString value = m_edits.at(i)->text();
		value.remove(QLatin1Char('\r'));
		value.remove(QLatin1Char('\n'));
		value = value.trimmed();
		if (!value.isEmpty()) {
			record.setValue(m_columns.at(i), value);
		}
	}

	return record;
}

/**
	@brief MaterialEntryDialog::accept
	Refuse an empty form: an article without any information is not worth
	a line in the file.
*/
void MaterialEntryDialog::accept()
{
	if (record().values.isEmpty())
	{
		QET::QetMessageBox::warning(this,
									tr("Nothing filled in"),
									tr("Fill in at least one field to create "
									   "an entry."));
		return;
	}

	QDialog::accept();
}
