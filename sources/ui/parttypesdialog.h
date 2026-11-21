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
#ifndef PARTTYPESDIALOG_H
#define PARTTYPESDIALOG_H

#include "../materiallist/parttypes.h"

#include <QDialog>
#include <QPointer>
#include <QStringList>

class QCheckBox;
class QLabel;
class QLineEdit;
class QPushButton;
class QETProject;
class QTableWidget;

/**
	@brief The PartTypesDialog class
	Lists the parts of the material list without a device type, and takes
	the type of each: said once for a part, in the material list, and given
	to the symbols of the project which use it.
*/
class PartTypesDialog : public QDialog
{
	Q_OBJECT
	public:
		PartTypesDialog(QETProject *project, const QString &material_path, QWidget *parent = nullptr);

		enum Column {Part, Description, Manufacturer, Used, Suggestion, Type, ColumnCount};

	public slots:
		void applyFilter();
		int useSuggestions();
		void accept() override;

	private:
		void fillTable();

		QPointer<QETProject> m_project;
		QString m_path;
		MaterialListData m_data;
		QList<PartTypes::Row> m_rows;
		QStringList m_known_types;
		QTableWidget *m_table = nullptr;
		QLineEdit *m_filter = nullptr;
		QCheckBox *m_only_used = nullptr;
		QCheckBox *m_give_to_symbols = nullptr;
		QLabel *m_summary = nullptr;
		QPushButton *m_suggestions = nullptr;
};

#endif // PARTTYPESDIALOG_H
