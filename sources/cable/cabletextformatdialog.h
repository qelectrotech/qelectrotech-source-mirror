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
#ifndef CABLETEXTFORMATDIALOG_H
#define CABLETEXTFORMATDIALOG_H

#include "cable.h"

#include <QDialog>

class QComboBox;
class QFontComboBox;
class QDoubleSpinBox;
class QLabel;
class QPushButton;

/**
	@brief The small window where one text of one cable is set: the font
	it is written with -- family, size and style -- and, for the texts
	written next to the trunk line, the way it lines up at the line.

	The colours of the cores come here as well, all of them together
	because they are drawn together: they have no alignment, being
	turned by a quarter along their own line.

	Saying nothing is the normal case: the cable then follows what the
	preferences offer on the "Textes" page, and OK keeps an answer of
	its own only when he really changed something -- so that picking
	another default later on still reaches this cable.
*/
class CableTextFormatDialog : public QDialog
{
		Q_OBJECT

	public:
		/**
			@param format what this cable writes that text with right
			now, an empty format when it merely follows the preferences
			@param cores true for the colours of the cores: they have
			no alignment and follow QETApp::cableCoreFont()
			@param parent
		*/
		CableTextFormatDialog(const CableTextFormat &format,
							  bool cores,
							  QWidget *parent = nullptr);

			///What the cable is to write that text with from now on
		CableTextFormat format() const;

	private:
		void buildUi();
		void updatePreview();
		QFont chosenFont() const;

		bool m_cores = false;
			///True when he asked to go back to following the preferences
		bool m_reset_chosen = false;
			///The alignment this text has right now, 0 for the cores
		int m_initial_alignment = int(Qt::AlignRight);

		QFontComboBox *m_family = nullptr;
		QDoubleSpinBox *m_size = nullptr;
		QComboBox *m_style = nullptr;
		QComboBox *m_alignment = nullptr;
		QLabel *m_preview = nullptr;
		QPushButton *m_reset = nullptr;
};

#endif // CABLETEXTFORMATDIALOG_H
