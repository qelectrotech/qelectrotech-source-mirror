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
#include "xrefdisplaydialog.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QLabel>
#include <QPushButton>

/**
	@brief XRefDisplayDialog::XRefDisplayDialog
	@param display the master's display now: not enabled when it follows the
	project's setting, in which case the fields start from the defaults
	@param suggested_manual the offset from the symbol where "Manual position" starts, when the master
	has no position of its own yet
	@param parent
*/
XRefDisplayDialog::XRefDisplayDialog(const ElementData::XRefDisplay &display,
									 const QPointF &suggested_manual,
									 QWidget *parent) :
	QDialog(parent)
{
	setWindowTitle(tr("Relay contact display"));
	setMinimumWidth(420);
	auto *form = new QFormLayout(this);

	auto *note = new QLabel(tr("The contact display keeps its electrical links. "
							   "For a manual position, Shift-drag the display on the folio, "
							   "or enter the offset of its top left corner from the symbol."));
	note->setWordWrap(true);
	form->addRow(note);

	m_appearance = new QComboBox;
	m_appearance->addItem(tr("Cross"), false);
	m_appearance->addItem(tr("Contact symbols"), true);
	m_appearance->setCurrentIndex(display.contacts ? 1 : 0);
	form->addRow(tr("Appearance"), m_appearance);

	m_position = new QComboBox;
	m_position->addItem(tr("Under device label"), ElementData::XRefDisplay::UnderLabel);
	m_position->addItem(tr("Below symbol"), ElementData::XRefDisplay::BelowSymbol);
	m_position->addItem(tr("Folio footer"), ElementData::XRefDisplay::Footer);
	m_position->addItem(tr("Manual position"), ElementData::XRefDisplay::Manual);
	m_position->setCurrentIndex(m_position->findData(display.position));
	form->addRow(tr("Position"), m_position);

	const auto spin = [this, form](const QString &label, qreal value, qreal low, qreal high) {
		auto *box = new QDoubleSpinBox;
		box->setRange(low, high);
		box->setDecimals(2);
		box->setValue(value);
		form->addRow(label, box);
		return box;
	};
	m_width = spin(tr("Line thickness"), display.width, 0.1, 10);
	m_gap = spin(tr("Gap below symbol"), display.gap, 0, 500);
	const QPointF manual = display.enabled && display.position == ElementData::XRefDisplay::Manual
			? display.manual : suggested_manual;
	m_x = spin(tr("Manual offset X"), manual.x(), -10000, 10000);
	m_y = spin(tr("Manual offset Y"), manual.y(), -10000, 10000);

	m_show_unlinked = new QCheckBox(tr("Show defined contacts even when not linked"));
	m_show_unlinked->setChecked(display.showUnlinked);
	form->addRow(m_show_unlinked);

	auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
	auto *restore = buttons->addButton(tr("Use the project setting"), QDialogButtonBox::ResetRole);
	form->addRow(buttons);
	connect(restore, &QPushButton::clicked, this, [this]() {
		m_restore = true;
		accept();
	});
	connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
	connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
	connect(m_position, &QComboBox::currentIndexChanged, this, &XRefDisplayDialog::updateEnabled);
	updateEnabled();
}

/**
	@brief XRefDisplayDialog::display
	@return what the user chose: not enabled if they asked for the project's
	setting
*/
ElementData::XRefDisplay XRefDisplayDialog::display() const
{
	ElementData::XRefDisplay display;
	if (m_restore)
		return display;

	display.enabled = true;
	display.contacts = m_appearance->currentData().toBool();
	display.position = static_cast<ElementData::XRefDisplay::Position>(m_position->currentData().toInt());
	display.width = m_width->value();
	display.gap = m_gap->value();
	display.manual = QPointF(m_x->value(), m_y->value());
	display.showUnlinked = m_show_unlinked->isChecked();
	return display;
}

/**
	@brief XRefDisplayDialog::updateEnabled
	The gap and the coordinates only matter for their own position.
*/
void XRefDisplayDialog::updateEnabled()
{
	const int position = m_position->currentData().toInt();
	m_gap->setEnabled(position == ElementData::XRefDisplay::BelowSymbol);
	m_x->setEnabled(position == ElementData::XRefDisplay::Manual);
	m_y->setEnabled(position == ElementData::XRefDisplay::Manual);
}
