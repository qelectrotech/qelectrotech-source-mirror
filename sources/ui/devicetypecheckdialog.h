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
#ifndef DEVICETYPECHECKDIALOG_H
#define DEVICETYPECHECKDIALOG_H

#include <QDialog>
#include <QHash>
#include <QPointer>

class Element;
class QCheckBox;
class QLabel;
class QETProject;
class QTreeWidget;
class QTreeWidgetItem;

/**
	@brief The DeviceTypeCheckDialog class
	Lists the devices which are not drawn the way their device type says
	(DeviceTypeCheck): the parts of the type not drawn, the symbols beyond
	it, the models the project has no type for. A click shows the symbol.
*/
class DeviceTypeCheckDialog : public QDialog
{
	Q_OBJECT
	public:
		explicit DeviceTypeCheckDialog(QETProject *project, QWidget *parent = nullptr);

	public slots:
		void refresh();
		void showSymbol(QTreeWidgetItem *row);

	private:
		void buildFolios();

		QPointer<QETProject> m_project;
		QTreeWidget *m_tree;
		QCheckBox *m_show_complete;
		QLabel *m_summary;
		QHash<QTreeWidgetItem *, QPointer<Element>> m_targets;
};

#endif // DEVICETYPECHECKDIALOG_H
