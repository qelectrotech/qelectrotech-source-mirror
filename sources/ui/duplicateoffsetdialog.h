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
#ifndef DUPLICATEOFFSETDIALOG_H
#define DUPLICATEOFFSETDIALOG_H

#include <QDialog>
#include <QPoint>

class QSpinBox;
class QComboBox;

/**
	@brief The DuplicateOffsetDialog class
	Asks how far, and in which of the four cardinal directions, Ctrl+D
	(DiagramView::duplicate(), bugtracker #991) should offset a copy from
	its source. Shown once, then remembered: the answer is stored in
	QSettings and reused by every later Ctrl+D press without asking
	again, until this dialog is reopened deliberately.
*/
class DuplicateOffsetDialog : public QDialog
{
		Q_OBJECT

	public:
		enum Direction { Up, Down, Left, Right };

		explicit DuplicateOffsetDialog(QWidget *parent = nullptr);

			/// The offset in grid steps, positive along X to the right
			/// and positive along Y downward -- QET's own scene axes,
			/// matching the sign convention setPos() already uses
			/// everywhere else in this codebase.
		QPoint stepOffset() const;

			/// Reads the last-confirmed spacing/direction from QSettings,
			/// or the default (1 step, right) if none was ever set.
		static QPoint savedStepOffset();
			/// Writes @p steps to QSettings, in the same X/Y convention
			/// as stepOffset().
		static void saveStepOffset(const QPoint &steps);
			/// Whether a direction/spacing has already been confirmed
			/// once, i.e. whether Ctrl+D can skip the dialog.
		static bool hasSavedStepOffset();

	private:
		QSpinBox  *m_spacing = nullptr;
		QComboBox *m_direction = nullptr;
};

#endif // DUPLICATEOFFSETDIALOG_H
