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
#ifndef PROJECTLANGUAGESDIALOG_H
#define PROJECTLANGUAGESDIALOG_H

#include <properties/projectlanguages.h>

#include <QDialog>

class QComboBox;
class QTableWidget;

/**
	@brief The ProjectLanguagesDialog class
	Edits the texts of a project in other languages: the languages, the
	words for each text, and the language the folios are shown in.
*/
class ProjectLanguagesDialog : public QDialog
{
	Q_OBJECT

	public:
		ProjectLanguagesDialog(const ProjectLanguages &languages,
							   const QStringList &project_texts,
							   QWidget *parent = nullptr);

		ProjectLanguages languages() const;

	private:
		void fillTable();
		void readTable();
		void refreshActive();
		void addLanguage();
		void removeLanguage();
		void collectTexts();
		void addText();
		void exportCsv();
		void importCsv();

		ProjectLanguages m_languages;
		QStringList m_project_texts;
		QComboBox *m_active;
		QTableWidget *m_table;
};

#endif // PROJECTLANGUAGESDIALOG_H
