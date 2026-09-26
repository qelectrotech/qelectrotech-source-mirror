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
#include "duplicateoffsetdialog.h"

#include <QComboBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QSettings>
#include <QSpinBox>
#include <QVBoxLayout>

namespace {
	// Stored as one integer per axis, in grid steps -- not pixels, so the
	// remembered offset still makes sense if the grid size ever changes.
	const QString kOffsetXKey = QStringLiteral("diagrameditor/duplicate_offset_x");
	const QString kOffsetYKey = QStringLiteral("diagrameditor/duplicate_offset_y");
}

DuplicateOffsetDialog::DuplicateOffsetDialog(QWidget *parent) :
	QDialog(parent)
{
	setWindowTitle(tr("Dupliquer"));

	auto *form = new QFormLayout;

	m_spacing = new QSpinBox(this);
	m_spacing->setRange(1, 1000);
	m_spacing->setSuffix(tr(" pas de grille"));
	form->addRow(tr("Espacement :"), m_spacing);

	m_direction = new QComboBox(this);
	// Order matches the Direction enum, so currentIndex() can be used
	// directly wherever Direction is needed.
	m_direction->addItem(tr("Haut"));
	m_direction->addItem(tr("Bas"));
	m_direction->addItem(tr("Gauche"));
	m_direction->addItem(tr("Droite"));
	form->addRow(tr("Direction :"), m_direction);

	const QPoint saved = savedStepOffset();
	// The saved value is a signed (dx, dy) pair, not itself a
	// spacing+direction pair, so it has to be decomposed back into the
	// two the dialog shows. Exactly one axis is ever non-zero (see
	// stepOffset()), so whichever one is picks the direction; a value
	// that somehow has neither (only possible if QSettings was hand-
	// edited) falls back to the same default stepOffset() would.
	int spacing = 1;
	Direction direction = Right;
	if (saved.x() > 0)      { direction = Right; spacing = saved.x(); }
	else if (saved.x() < 0) { direction = Left;  spacing = -saved.x(); }
	else if (saved.y() > 0) { direction = Down;  spacing = saved.y(); }
	else if (saved.y() < 0) { direction = Up;    spacing = -saved.y(); }
	m_spacing->setValue(spacing);
	m_direction->setCurrentIndex(static_cast<int>(direction));

	auto *buttons = new QDialogButtonBox(
				QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
	connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
	connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

	auto *layout = new QVBoxLayout(this);
	layout->addLayout(form);
	layout->addWidget(buttons);
}

QPoint DuplicateOffsetDialog::stepOffset() const
{
	const int spacing = m_spacing->value();
	switch (static_cast<Direction>(m_direction->currentIndex())) {
		case Up:    return QPoint(0, -spacing);
		case Down:  return QPoint(0,  spacing);
		case Left:  return QPoint(-spacing, 0);
		case Right: return QPoint(spacing,  0);
	}
	return QPoint(spacing, 0); // unreachable; keeps -Wreturn-type quiet
}

QPoint DuplicateOffsetDialog::savedStepOffset()
{
	QSettings settings;
	if (!hasSavedStepOffset()) {
		return QPoint(1, 0); // default: one grid step to the right
	}
	return QPoint(settings.value(kOffsetXKey).toInt(),
				 settings.value(kOffsetYKey).toInt());
}

void DuplicateOffsetDialog::saveStepOffset(const QPoint &steps)
{
	QSettings settings;
	settings.setValue(kOffsetXKey, steps.x());
	settings.setValue(kOffsetYKey, steps.y());
}

bool DuplicateOffsetDialog::hasSavedStepOffset()
{
	QSettings settings;
	return settings.contains(kOffsetXKey) && settings.contains(kOffsetYKey);
}
