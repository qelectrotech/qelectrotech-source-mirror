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
#include "textclearancedialog.h"

#include "../diagram.h"

#include <QComboBox>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QLabel>
#include <QPushButton>

/**
	@brief TextClearanceDialog::TextClearanceDialog
	@param diagram the folio whose texts are moved
	@param parent
*/
TextClearanceDialog::TextClearanceDialog(Diagram *diagram, QWidget *parent) :
	QDialog(parent),
	m_diagram(diagram)
{
	setWindowTitle(tr("Keep texts clear of wires"));
	setMinimumWidth(400);
	auto *form = new QFormLayout(this);

	auto *note = new QLabel(tr("Texts that sit on a wire are moved the shortest way that leaves "
							   "a gap. Texts of wires and texts in a group stay where they are."));
	note->setWordWrap(true);
	form->addRow(note);

	m_scope = new QComboBox;
	const bool has_selection = diagram && !diagram->selectedItems().isEmpty();
	m_scope->addItem(tr("Selected texts and symbols"), true);
	m_scope->addItem(tr("This folio"), false);
	m_scope->setCurrentIndex(has_selection ? 0 : 1);
	m_scope->model()->setData(m_scope->model()->index(0, 0),
							  has_selection ? QVariant(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsUserCheckable)
											: QVariant(Qt::NoItemFlags), Qt::UserRole - 1);
	form->addRow(tr("Look at"), m_scope);

	const auto spin = [form](const QString &label, qreal value, qreal low, qreal high) {
		auto *box = new QDoubleSpinBox;
		box->setRange(low, high);
		box->setDecimals(1);
		box->setValue(value);
		form->addRow(label, box);
		return box;
	};
	const TextClearance::Settings defaults;
	m_distance = spin(tr("Gap to keep"), defaults.distance, 0, 100);
	m_max_move = spin(tr("Move a text at most"), defaults.maxMove, 1, 300);

	m_direction = new QComboBox;
	m_direction->addItem(tr("Shortest way"), TextClearance::Any);
	m_direction->addItem(tr("Sideways only"), TextClearance::Horizontal);
	m_direction->addItem(tr("Up or down only"), TextClearance::Vertical);
	form->addRow(tr("Direction"), m_direction);

	m_summary = new QLabel;
	m_summary->setWordWrap(true);
	form->addRow(m_summary);

	m_buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
	m_buttons->button(QDialogButtonBox::Ok)->setText(tr("Move texts"));
	form->addRow(m_buttons);
	connect(m_buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
	connect(m_buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

	connect(m_scope, &QComboBox::currentIndexChanged, this, &TextClearanceDialog::update);
	connect(m_direction, &QComboBox::currentIndexChanged, this, &TextClearanceDialog::update);
	connect(m_distance, &QDoubleSpinBox::valueChanged, this, &TextClearanceDialog::update);
	connect(m_max_move, &QDoubleSpinBox::valueChanged, this, &TextClearanceDialog::update);
	update();
}

/**
	@brief TextClearanceDialog::update
	Make the plan again and say what it would do.
*/
void TextClearanceDialog::update()
{
	TextClearance::Settings settings;
	settings.distance = m_distance->value();
	settings.maxMove = m_max_move->value();
	settings.direction = static_cast<TextClearance::Direction>(m_direction->currentData().toInt());
	m_plan = TextClearance::plan(m_diagram, settings, m_scope->currentData().toBool());

	QString text = tr("%n text(s) would move.", nullptr, m_plan.moves.size());
	if (m_plan.unresolved)
		text += QLatin1Char(' ') + tr("%n more sit on a wire with no free place in reach.", nullptr, m_plan.unresolved);
	if (!m_plan.considered)
		text = tr("There is no text to look at.");
	m_summary->setText(text);
	m_buttons->button(QDialogButtonBox::Ok)->setEnabled(!m_plan.moves.isEmpty());
}
