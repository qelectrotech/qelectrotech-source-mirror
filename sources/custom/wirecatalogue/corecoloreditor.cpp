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

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>

WireColorComboBox::WireColorComboBox(QWidget *parent) :
	QComboBox(parent)
{
	setEditable(true);
	setInsertPolicy(QComboBox::NoInsert);
	const int sz = 14;
	setIconSize(QSize(sz, sz));
	for (const QString &name : Iec60757::standardNames())
		addItem(Iec60757::icon(name, sz), name);
	setCurrentIndex(-1);
}

QString WireColorComboBox::colorName() const
{
	return currentText().trimmed();
}

void WireColorComboBox::setColorName(const QString &name)
{
	const int idx = findText(name, Qt::MatchFixedString);
	if (idx >= 0)
		setCurrentIndex(idx);
	else
		setEditText(name);
}

CoreColorEditor::CoreColorEditor(QWidget *parent) :
	QWidget(parent)
{
	m_layout = new QVBoxLayout(this);
	m_layout->setContentsMargins(0, 0, 0, 0);
	m_layout->setSpacing(2);
	setCoreCount(1);
}

void CoreColorEditor::setCoreCount(int count)
{
	count = qMax(1, count);
	if (count == m_combos.size())
		return;

	// Grow: append "Core N" rows.
	while (m_combos.size() < count) {
		const int n = m_combos.size() + 1;
		auto *row = new QWidget(this);
		auto *h = new QHBoxLayout(row);
		h->setContentsMargins(0, 0, 0, 0);
		auto *lbl = new QLabel(tr("Core %1").arg(n), row);
		lbl->setMinimumWidth(48);
		auto *combo = new WireColorComboBox(row);
		connect(combo, &QComboBox::currentTextChanged,
				this, &CoreColorEditor::colorsChanged);
		h->addWidget(lbl);
		h->addWidget(combo, 1);
		m_layout->addWidget(row);
		m_combos.append(combo);
	}

	// Shrink: drop trailing rows (and their label container).
	while (m_combos.size() > count) {
		WireColorComboBox *combo = m_combos.takeLast();
		QWidget *row = combo->parentWidget();
		m_layout->removeWidget(row);
		row->deleteLater();
	}

	emit colorsChanged();
}

QStringList CoreColorEditor::colors() const
{
	QStringList out;
	for (WireColorComboBox *c : m_combos) {
		const QString name = c->colorName();
		if (!name.isEmpty())
			out << name;
	}
	return out;
}

void CoreColorEditor::setColors(const QStringList &colors)
{
	setCoreCount(qMax(1, colors.size()));
	for (int i = 0; i < m_combos.size(); ++i)
		m_combos.at(i)->setColorName(i < colors.size() ? colors.at(i) : QString());
}
