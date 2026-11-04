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
#ifndef DEVICENAVIGATORDOCKWIDGET_H
#define DEVICENAVIGATORDOCKWIDGET_H

#include <QDockWidget>
#include <QPointer>
#include <QList>
#include <QMetaObject>

class QETProject;
class Diagram;
class Element;
class QTreeWidgetItem;
class QTimer;

namespace Ui {
	class DeviceNavigatorDockWidget;
}

/**
 * @brief The DeviceNavigatorDockWidget class
 * Provides an EPLAN-style hierarchical device navigator tree and list
 * structured by plant / location (or "Ohne Strukturkennzeichen"), device tags (BMK),
 * and placed functions/contacts.
 */
class DeviceNavigatorDockWidget : public QDockWidget
{
	Q_OBJECT

	public:
		enum NodeType {
			ProjectNode = 1000,
			StructureNode = 1001,
			DeviceNode = 1002,
			FunctionNode = 1003
		};

		enum Roles {
			NodeTypeRole = Qt::UserRole + 1,
			ElementPointerRole = Qt::UserRole + 2,
			DiagramPointerRole = Qt::UserRole + 3,
			BmkRole = Qt::UserRole + 4,
			StructureRole = Qt::UserRole + 5,
			ProjectPointerRole = Qt::UserRole + 6
		};

		explicit DeviceNavigatorDockWidget(QWidget *parent = nullptr);
		~DeviceNavigatorDockWidget() override;

		void setProject(QETProject *project);
		QETProject *project() const;

	public slots:
		void reload();
		void scheduleReload();

	private slots:
		void onItemDoubleClicked(QTreeWidgetItem *item, int column);
		void onTableCellDoubleClicked(int row, int column);
		void onSearchTextChanged(const QString &text);
		void onFilterComboChanged(int index);
		void onCategoryTabChanged(int index);
		void onViewModeTabChanged(int index);
		void onCustomContextMenu(const QPoint &pos);
		void onTableContextMenu(const QPoint &pos);

	private:
		struct ElementInfo {
			QPointer<Element> element;
			QPointer<Diagram> diagram;
			QString plant;
			QString location;
			QString structure;
			QString bmk;
			QString terminals;
			QString functionDesc;
			QString folioStr;
			QString posStr;
			QString locStr;
			QString fullDisplay;
			bool isMaster = false;
			bool isSlave = false;
			bool isTerminal = false;
			bool isCable = false;
			bool isThumbnail = false;
		};

		void connectProject(QETProject *project);
		void disconnectProject();
		void buildData(QList<ElementInfo> &items);
		void populateTree(const QList<ElementInfo> &items);
		void populateTable(const QList<ElementInfo> &items);
		void applyFilters();
		void jumpToElement(Element *element);
		QString elementFunctionDescription(Element *elmt) const;

	private:
		Ui::DeviceNavigatorDockWidget *ui = nullptr;
		QPointer<QETProject> m_project;
		QTimer *m_debounce_timer = nullptr;
		QList<QMetaObject::Connection> m_connections;
		QList<ElementInfo> m_cached_items;
		QPointer<Element> m_last_highlighted_element;
};

#endif // DEVICENAVIGATORDOCKWIDGET_H
