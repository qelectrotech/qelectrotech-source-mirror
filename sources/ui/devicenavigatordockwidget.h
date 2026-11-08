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

#include "devicenavigatorfilterdialog.h"

#include <QDockWidget>
#include <QList>
#include <QMetaObject>
#include <QPointer>
#include <QSet>
#include <QUuid>

class QETProject;
class Diagram;
class Element;
class QTreeWidgetItem;
class QTimer;

namespace Ui
{
	class DeviceNavigatorDockWidget;
}

struct DeviceNavigatorTreeState
{
	QSet<QString> expandedStructures;
	QSet<QString> expandedDevices;
	int			  selectedNodeType = -1;
	QString		  selectedStructure;
	QString		  selectedDeviceKey;
	QString		  selectedElementUuid;
	int			  vScroll	   = 0;
	int			  hScroll	   = 0;
	bool		  hasSelection = false;
};

struct DeviceNavigatorTableState
{
	QString selectedElementUuid;
	int		vScroll		 = 0;
	int		hScroll		 = 0;
	bool	hasSelection = false;
};

/**
 * @brief The DeviceNavigatorDockWidget class
 * Provides a hierarchical device navigator tree and list
 * structured by plant / location (or "No structure identifier"), device tags,
 * and placed functions/contacts.
 */
class DeviceNavigatorDockWidget : public QDockWidget
{
	Q_OBJECT

  public:
	enum NodeType
	{
		ProjectNode	  = 1000,
		StructureNode = 1001,
		DeviceNode	  = 1002,
		FunctionNode  = 1003
	};

	enum Roles
	{
		NodeTypeRole	   = Qt::UserRole + 1,
		BmkRole			   = Qt::UserRole + 2,
		StructureRole	   = Qt::UserRole + 3,
		ElementUuidRole	   = Qt::UserRole + 4,
		DiagramUuidRole	   = Qt::UserRole + 5,
		IsMasterRole	   = Qt::UserRole + 6,
		IsSlaveRole		   = Qt::UserRole + 7,
		IsTerminalRole	   = Qt::UserRole + 8,
		IsCableRole		   = Qt::UserRole + 9,
		IsReportRole	   = Qt::UserRole + 10,
		IsConductorDefRole = Qt::UserRole + 11,
		IsThumbnailRole	   = Qt::UserRole + 12,
		HasStructureRole   = Qt::UserRole + 13
	};

	explicit DeviceNavigatorDockWidget(QWidget* parent = nullptr);
	~DeviceNavigatorDockWidget() override;

	void		setProject(QETProject* project);
	QETProject* project() const;

  protected:
	void showEvent(QShowEvent* event) override;

  public slots:
	void reload();
	void scheduleReload();

  private slots:
	void onItemDoubleClicked(QTreeWidgetItem* item, int column);
	void onTableCellDoubleClicked(int row, int column);
	void onSearchTextChanged(const QString& text);
	void onFilterComboChanged(int index);
	void onFilterConfigClicked();
	void onViewModeTabChanged(int index);
	void onCustomContextMenu(const QPoint& pos);
	void onTableContextMenu(const QPoint& pos);

  private:
	struct ElementInfo
	{
		QPointer<Element> element;
		QPointer<Diagram> diagram;
		QUuid			  elementUuid;
		QUuid			  diagramUuid;
		bool			  hasStructure = false;
		QString			  plant;
		QString			  location;
		QString			  structure;
		QString			  bmk;
		QString			  terminals;
		QString			  functionDesc;
		int				  folioIndex = 0;
		QString			  folioStr;
		QString			  posStr;
		QString			  locStr;
		QString			  fullDisplay;
		bool			  isMaster		 = false;
		bool			  isSlave		 = false;
		bool			  isTerminal	 = false;
		bool			  isCable		 = false;
		bool			  isThumbnail	 = false;
		bool			  isReport		 = false;
		bool			  isConductorDef = false;
	};

	void connectProject(QETProject* project);
	void disconnectProject();
	using TreeState	 = DeviceNavigatorTreeState;
	using TableState = DeviceNavigatorTableState;

	TreeState  saveTreeState() const;
	TableState saveTableState() const;
	void	   buildData(QList<ElementInfo>& items);
	void	   populateTree(
		const QList<ElementInfo>& items, const TreeState& state = TreeState());
	void populateTable(
		const QList<ElementInfo>& items,
		const TableState&		  state = TableState());
	void restoreTreeState(const TreeState& state);
	void restoreTableState(const TableState& state);
	void applyFilters();
	void jumpToElement(Element* element);
	void jumpToUuid(const QUuid& elementUuid, const QUuid& diagramUuid);
	void
	editElementProperties(const QUuid& elementUuid, const QUuid& diagramUuid);
	void addContextMenuActions(
		QMenu& menu, const QUuid& elementUuid, const QUuid& diagramUuid);
	QString elementFunctionDescription(Element* elmt) const;
	void	loadFilterSchemes();
	void	updateFilterCombo();

  private:
	Ui::DeviceNavigatorDockWidget*	   ui = nullptr;
	QPointer<QETProject>			   m_project;
	QTimer*							   m_debounce_timer = nullptr;
	QList<QMetaObject::Connection>	   m_connections;
	QList<ElementInfo>				   m_cached_items;
	QPointer<Element>				   m_last_highlighted_element;
	QList<DeviceNavigatorFilterScheme> m_filter_schemes;
	bool							   m_dirty = false;
};

#endif // DEVICENAVIGATORDOCKWIDGET_H
