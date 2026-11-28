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
#ifndef TERMINALLABELDIALOG_H
#define TERMINALLABELDIALOG_H

#include <properties/terminallabel.h>

#include <QDialog>

#ifdef BUILD_WITHOUT_KF
#include <QPushButton>
#else
#include <KColorButton>
#endif

namespace Ui {
	class TerminalLabelDialog;
}

/**
	@brief The TerminalLabelDialog class
	How the name of a terminal is drawn: the group the element editor
	calls "Text properties", the font, its size, where the name sits, how
	turned it is, how aligned, whether it is framed and in which colour.
	The dialog changes nothing by itself: label() hands back what it was
	given, changed as the widgets were
	(@see ElementTerminalsWidget).
*/
class TerminalLabelDialog : public QDialog
{
	Q_OBJECT

	public:
		explicit TerminalLabelDialog(const TerminalLabel &label, QWidget *parent = nullptr);
		~TerminalLabelDialog() override;

			/// What the widgets say, whether the dialog was accepted or not
		TerminalLabel label() const {return m_label;}

	private:
		void fontClicked();
		void sizeEdited();
		void posEdited();
		void rotationEdited();
		void alignClicked();
		void colorClicked();

	private:
		Ui::TerminalLabelDialog *ui;
		TerminalLabel m_label;
#ifdef BUILD_WITHOUT_KF
		QPushButton *m_color_pb = nullptr;
#else
		KColorButton *m_color_pb = nullptr;
#endif
};

#endif // TERMINALLABELDIALOG_H
