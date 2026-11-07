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
#include <qet.h>

#include <QCheckBox>
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
	const QString kRepeatMoveKey = QStringLiteral("diagrameditor/duplicate_repeats_move");
		// In scene units, not grid steps: a copy may be moved off the grid
	const QString kLearnedXKey = QStringLiteral("diagrameditor/duplicate_learned_x");
	const QString kLearnedYKey = QStringLiteral("diagrameditor/duplicate_learned_y");
}

DuplicateOffsetDialog::DuplicateOffsetDialog(QWidget *parent) :
	QDialog(parent)
{
	setWindowTitle(tr("Duplicate"));

	auto *form = new QFormLayout;

	m_spacing = new QSpinBox(this);
	m_spacing->setRange(1, 1000);
	m_spacing->setSuffix(tr(" no grid"));
	form->addRow(tr("Spacing:"), m_spacing);

	m_direction = new QComboBox(this);
	// Order matches the Direction enum, so currentIndex() can be used
	// directly wherever Direction is needed.
	m_direction->addItem(tr("Top"));
	m_direction->addItem(tr("Bottom"));
	m_direction->addItem(tr("Left"));
	m_direction->addItem(tr("Right"));
	form->addRow(tr("Direction:"), m_direction);

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

	m_repeat_move = new QCheckBox(tr("Repeat the move of the copy"), this);
	m_repeat_move->setToolTip(tr("The first copy lands 4 grid steps right and "
								 "down; move it, and the next Duplicate "
								 "repeats that move. The spacing above is "
								 "then not used."));
	m_repeat_move->setChecked(savedRepeatsMove());
	form->addRow(m_repeat_move);
		//Set aside, not cleared: unchecking the box gives it back
	const auto setAside = [this](bool repeats) {
		m_spacing->setEnabled(!repeats);
		m_direction->setEnabled(!repeats);
	};
	setAside(m_repeat_move->isChecked());
	connect(m_repeat_move, &QCheckBox::toggled, this, setAside);

	auto *buttons = new QDialogButtonBox(
				QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
	connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
	connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

	auto *layout = new QVBoxLayout(this);
	layout->addLayout(form);
	layout->addWidget(buttons);

	QET::trackDialogGeometry(this);
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

bool DuplicateOffsetDialog::repeatsMove() const
{
	return m_repeat_move->isChecked();
}

bool DuplicateOffsetDialog::savedRepeatsMove()
{
	QSettings settings;
	return settings.value(kRepeatMoveKey, false).toBool();
}

void DuplicateOffsetDialog::saveRepeatsMove(bool repeats)
{
	QSettings settings;
		//Nothing written while it is off, as it is by default
	if (repeats)
		settings.setValue(kRepeatMoveKey, true);
	else
		settings.remove(kRepeatMoveKey);
}

bool DuplicateOffsetDialog::learnedOffset(QPointF *offset)
{
	QSettings settings;
	if (!settings.contains(kLearnedXKey) || !settings.contains(kLearnedYKey))
		return false;
	*offset = QPointF(settings.value(kLearnedXKey).toDouble(),
					  settings.value(kLearnedYKey).toDouble());
	return !offset->isNull();
}

void DuplicateOffsetDialog::saveLearnedOffset(const QPointF &offset)
{
	QSettings settings;
	settings.setValue(kLearnedXKey, offset.x());
	settings.setValue(kLearnedYKey, offset.y());
}

void DuplicateOffsetDialog::forgetLearnedOffset()
{
	QSettings settings;
	settings.remove(kLearnedXKey);
	settings.remove(kLearnedYKey);
}

void DuplicateOffsetDialog::save() const
{
	const bool switched_on = repeatsMove() && !savedRepeatsMove();
	saveStepOffset(stepOffset());
	saveRepeatsMove(repeatsMove());
	if (switched_on)
		forgetLearnedOffset();
}
