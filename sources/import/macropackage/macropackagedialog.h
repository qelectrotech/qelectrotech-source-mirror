/*
	Copyright 2006-2026 The QElectroTech Team
	This file is part of QElectroTech.

	QElectroTech is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 2 of the License, or
	(at your option) any later version.

	QElectroTech is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with QElectroTech. If not, see <http://www.gnu.org/licenses/>.
*/
#ifndef MACROPACKAGEDIALOG_H
#define MACROPACKAGEDIALOG_H

#include "macropackage.h"

#include <QDialog>
#include <QStringList>

class QCheckBox;
class QComboBox;
class QLabel;
class QLineEdit;
class QPushButton;
class QTreeWidget;

/**
	@brief Dialog of the "Part import" menu for a manufacturer's macro
	package (all EPLAN macros of a manufacturer in one ZIP, as Beckhoff
	offers it): choose the package once, search its articles, import the
	chosen ones into manufacturer / category / type folders.

	The package last used is remembered and opened again, so that later
	parts take a search and a click.
*/
class MacroPackageDialog : public QDialog
{
	Q_OBJECT

	public:
		explicit MacroPackageDialog(QWidget *parent = nullptr);

		/// the elements written while the dialog was open
		QStringList writtenPaths() const { return m_written; }

	private:
		void choosePackage(bool folder);
		void openPackage(const QString &path);
		void showArticles();
		void runImport(bool all);
		void updateButtons();

		MacroPackage m_package;
		QLineEdit *m_path = nullptr;
		QLineEdit *m_search = nullptr;
		QLabel *m_count = nullptr;
		QTreeWidget *m_results = nullptr;
		QComboBox *m_target = nullptr;
		QCheckBox *m_terms = nullptr;
		QPushButton *m_import = nullptr;
		QPushButton *m_import_all = nullptr;
		QStringList m_written;
};

#endif // MACROPACKAGEDIALOG_H
