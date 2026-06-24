/*
	Copyright 2026 Trovo Tech Solutions
	This file is part of a custom feature set built on QElectroTech.

	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 2 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#include "corecoloreditor.h"
#include "iec60757.h"

#include <QGridLayout>
#include <QLabel>

namespace { const QString kNoColor = QStringLiteral("<No color>"); }

WireColorComboBox::WireColorComboBox(QWidget *parent) :
	QComboBox(parent)
{
	setEditable(true);
	setInsertPolicy(QComboBox::NoInsert);
	const int sz = 14;
	setIconSize(QSize(sz, sz));
	addItem(kNoColor);                       // index 0 == empty
	for (const QString &name : Iec60757::standardNames())
		addItem(Iec60757::icon(name, sz), name);
	setCurrentIndex(0);
}

QString WireColorComboBox::colorName() const
{
	const QString t = currentText().trimmed();
	return (t == kNoColor) ? QString() : t;
}

void WireColorComboBox::setColorName(const QString &name)
{
	if (name.isEmpty()) {
		setCurrentIndex(0);
		return;
	}
	const int idx = findText(name, Qt::MatchFixedString);
	if (idx >= 0)
		setCurrentIndex(idx);
	else
		setEditText(name);
}

CoreColorEditor::CoreColorEditor(QWidget *parent) :
	QWidget(parent)
{
	// Outer vbox keeps the grid packed at the top (a trailing stretch absorbs
	// extra height) instead of the grid spreading its rows apart.
	auto *outer = new QVBoxLayout(this);
	outer->setContentsMargins(0, 0, 0, 0);
	m_grid = new QGridLayout;
	m_grid->setContentsMargins(0, 0, 0, 0);
	m_grid->setHorizontalSpacing(6);
	m_grid->setVerticalSpacing(3);
	outer->addLayout(m_grid);
	outer->addStretch(1);

	// Header row.
	m_grid->addWidget(new QLabel(tr("Core"), this),     0, 0);
	m_grid->addWidget(new QLabel(tr("Colour 1"), this), 0, 1);
	m_grid->addWidget(new QLabel(tr("Colour 2"), this), 0, 2);
	m_grid->addWidget(new QLabel(tr("Colour 3"), this), 0, 3);
	m_grid->setColumnStretch(1, 1);
	m_grid->setColumnStretch(2, 1);
	m_grid->setColumnStretch(3, 1);

	addCore(); // start with a single core
}

void CoreColorEditor::addCore()
{
	const int r = m_rows.size() + 1; // +1 for header row in the grid

	Row row;
	row.number = new QLabel(QString::number(m_rows.size() + 1), this);
	row.c1 = new WireColorComboBox(this);
	row.c2 = new WireColorComboBox(this);
	row.c3 = new WireColorComboBox(this);

	for (WireColorComboBox *c : {row.c1, row.c2, row.c3})
		connect(c, &QComboBox::currentTextChanged, this, &CoreColorEditor::coresChanged);

	m_grid->addWidget(row.number, r, 0);
	m_grid->addWidget(row.c1,     r, 1);
	m_grid->addWidget(row.c2,     r, 2);
	m_grid->addWidget(row.c3,     r, 3);

	m_rows.append(row);
	emit coresChanged();
}

void CoreColorEditor::removeSelectedCore()
{
	if (m_rows.size() <= 1) // always keep at least one core
		return;

	Row row = m_rows.takeLast();
	for (QWidget *w : {static_cast<QWidget*>(row.number),
					   static_cast<QWidget*>(row.c1),
					   static_cast<QWidget*>(row.c2),
					   static_cast<QWidget*>(row.c3)}) {
		m_grid->removeWidget(w);
		w->deleteLater();
	}
	renumber();
	emit coresChanged();
}

void CoreColorEditor::renumber()
{
	for (int i = 0; i < m_rows.size(); ++i)
		m_rows.at(i).number->setText(QString::number(i + 1));
}

QVector<QStringList> CoreColorEditor::colors() const
{
	QVector<QStringList> out;
	for (const Row &row : m_rows) {
		QStringList core;
		for (WireColorComboBox *c : {row.c1, row.c2, row.c3}) {
			const QString name = c->colorName();
			if (!name.isEmpty())
				core << name;
		}
		out << core; // may be empty if no colour chosen for that core
	}
	return out;
}

void CoreColorEditor::setColors(const QVector<QStringList> &cores)
{
	const int target = qMax(1, cores.size());
	while (m_rows.size() < target)
		addCore();
	while (m_rows.size() > target)
		removeSelectedCore();

	for (int i = 0; i < m_rows.size(); ++i) {
		const QStringList core = (i < cores.size()) ? cores.at(i) : QStringList();
		m_rows.at(i).c1->setColorName(core.value(0));
		m_rows.at(i).c2->setColorName(core.value(1));
		m_rows.at(i).c3->setColorName(core.value(2));
	}
}
