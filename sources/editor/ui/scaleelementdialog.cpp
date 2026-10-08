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
#include "scaleelementdialog.h"

#include "../symbolscale.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLabel>
#include <QLocale>
#include <QPushButton>
#include <QVBoxLayout>

/**
	@brief ScaleElementDialog::ScaleElementDialog
	@param terminals : positions of the element's terminals, in scene
	coordinates (relative to the hotspot)
	@param parent
*/
ScaleElementDialog::ScaleElementDialog(const QList<QPointF> &terminals, QWidget *parent) :
	QDialog(parent)
{
	setWindowTitle(tr("Mettre l'élément à l'échelle"));

	auto layout = new QVBoxLayout(this);
	auto form = new QFormLayout();
	layout->addLayout(form);

	m_factor = new QComboBox(this);
	const auto factors = SymbolScale::safeFactors(terminals);
	for (const qreal f : factors) {
		m_factor->addItem(QStringLiteral("× %1").arg(QLocale().toString(f)), f);
	}
	const int two = m_factor->findData(2.0);
	if (two >= 0) {
		m_factor->setCurrentIndex(two);
	}
	form->addRow(tr("Facteur :"), m_factor);

	m_scale_text = new QCheckBox(tr("Mettre aussi les textes à l'échelle"), this);
	m_scale_text->setChecked(true);
	form->addRow(m_scale_text);

	QString explanation;
	const int off_grid = SymbolScale::offGridCount(terminals);
	if (off_grid && factors.isEmpty()) {
		explanation = tr("%n borne(s) de cet élément ne sont pas sur la grille, "
						 "et aucun facteur ne les y amène.", "", off_grid);
	} else if (off_grid) {
		explanation = tr("%n borne(s) de cet élément ne sont pas sur la grille. "
						 "Seuls les facteurs qui les y amènent sont proposés.", "", off_grid);
	} else {
		explanation = tr("Seuls les facteurs qui gardent les bornes sur la grille "
						 "sont proposés. L'élément est mis à l'échelle autour de son point de saisie.");
	}
	auto label = new QLabel(explanation, this);
	label->setWordWrap(true);
	layout->addWidget(label);

	auto buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
	buttons->button(QDialogButtonBox::Ok)->setEnabled(!factors.isEmpty());
	connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
	connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
	layout->addWidget(buttons);

	m_factor->setEnabled(!factors.isEmpty());
	m_scale_text->setEnabled(!factors.isEmpty());
}

/**
	@return the chosen factor, or 1 if none could be offered
*/
qreal ScaleElementDialog::factor() const
{
	return m_factor->count() ? m_factor->currentData().toReal() : 1.0;
}

bool ScaleElementDialog::scaleText() const
{
	return m_scale_text->isChecked();
}
