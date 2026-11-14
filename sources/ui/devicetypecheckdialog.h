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

#include <devicetype/devicetypecheck.h>

#include <QDialog>
#include <QHash>
#include <QPair>
#include <QPointer>
#include <QVector>

class Diagram;
class Element;
class QCheckBox;
class QLabel;
class QPushButton;
class QETProject;
class QTreeWidget;
class QTreeWidgetItem;

/**
	@brief The DeviceTypeCheckDialog class
	Lists the devices which are not drawn the way their device type says
	(DeviceTypeCheck): the parts of the type not drawn, the symbols beyond
	it, the models the project has no type for. A click shows the symbol;
	a part not drawn can be placed on the current folio.
*/
class DeviceTypeCheckDialog : public QDialog
{
	Q_OBJECT
	public:
		explicit DeviceTypeCheckDialog(QETProject *project, QWidget *parent = nullptr);

	public slots:
		void refresh();
		void showSymbol(QTreeWidgetItem *row);
		void placeRow(QTreeWidgetItem *row);
		bool startPlacing(QTreeWidgetItem *row, int way, Diagram *folio = nullptr);

	private:
		void buildFolios();
		void updatePlaceButton();

		QPointer<QETProject> m_project;
		QTreeWidget *m_tree;
		QCheckBox *m_show_complete;
		QLabel *m_summary;
		QPushButton *m_place;
		QVector<DeviceTypeCheck::Device> m_devices;
			/// A part not drawn: its device in m_devices, its part
		QHash<QTreeWidgetItem *, QPair<int, int>> m_parts;
		QHash<QTreeWidgetItem *, QPointer<Element>> m_targets;
};

#endif // DEVICETYPECHECKDIALOG_H
