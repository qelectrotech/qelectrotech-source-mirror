/*
	Copyright 2006-2014 The QElectroTech Team
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
#ifndef POTENTIALTEXTSDIALOG_H
#define POTENTIALTEXTSDIALOG_H

#include <QDialog>
#include <QMultiMap>
class Conductor;
class QSignalMapper;

namespace Ui {
	class PotentialTextsDialog;
}

/**
 * @brief The PotentialTextsDialog class
 * This dialog show all differents conductors texts at the same
 * potential of @conductor.
 * The user can select a text in the list.
 */
class PotentialTextsDialog : public QDialog
{
		Q_OBJECT

	public:
		explicit PotentialTextsDialog(Conductor *conductor, QWidget *parent = 0);
		~PotentialTextsDialog();

		QString selectedText () const;

	private:
		void buildRadioList();
		void conductorsTextToMap();

	private slots:
		void setSelectedText (QString text);

	private:
		Ui::PotentialTextsDialog *ui;
		Conductor *m_conductor;
		QSignalMapper *m_signal_mapper;
		QString m_selected_text;
		QMultiMap <int, QString> m_texts;
};

#endif // POTENTIALTEXTSDIALOG_H
