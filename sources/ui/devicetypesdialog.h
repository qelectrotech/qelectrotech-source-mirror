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
#ifndef DEVICETYPESDIALOG_H
#define DEVICETYPESDIALOG_H

#include "../devicetype/devicetypelist.h"

#include <QDialog>
#include <QPointer>

class QETProject;
class QPushButton;
class QTabWidget;
class QTreeWidget;

/**
	@brief The DeviceTypesDialog class
	Project > Device types: the project's device types and the user's
	library (DeviceTypeLibrary), side by side in two tabs. New, Duplicate,
	Edit and Delete work on the tab shown; a type is copied from one to the
	other. A change to the project's types is one undo step of the project;
	the library is a folder of files, changed at once.
*/
class DeviceTypesDialog : public QDialog
{
		Q_OBJECT

	public:
		DeviceTypesDialog(QETProject *project, QWidget *parent = nullptr,
						  const QString &library_folder = QString());

		bool inLibrary() const;
		void showLibrary(bool library);
		QString selectedName() const;
		void select(const QString &name);

			/// What the buttons do, without asking (tests; the buttons ask first)
		bool keep(const DeviceType &type, bool library);
		bool remove(const QString &name, bool library);
		bool copy(const QString &name, bool to_library);
		static QString freeName(const QString &name, const QStringList &taken);

	public slots:
		void refresh();
		QString listCsv(DeviceTypeList::Kind kind) const;

	private:
		QStringList names(bool library) const;
		DeviceType typeNamed(const QString &name, bool library) const;
		void newType();
		void duplicate();
		void edit();
		void removeSelected();
		void copySelected();
		void updateButtons();
		void exportList(DeviceTypeList::Kind kind);

		QPointer<QETProject> m_project;
		QString m_folder;
		QTabWidget *m_tabs = nullptr;
		QTreeWidget *m_project_list = nullptr;
		QTreeWidget *m_library_list = nullptr;
		QPushButton *m_edit = nullptr;
		QPushButton *m_duplicate = nullptr;
		QPushButton *m_remove = nullptr;
		QPushButton *m_copy = nullptr;
};

#endif // DEVICETYPESDIALOG_H
