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
#include "devicenavigatordockwidget.h"
#include "ui_devicenavigatordockwidget.h"
#include "../qetproject.h"
#include "../diagram.h"
#include "../diagramview.h"
#include "../qetgraphicsitem/element.h"
#include "../qetgraphicsitem/masterelement.h"
#include "../qetgraphicsitem/slaveelement.h"
#include "../qetgraphicsitem/terminal.h"
#include "../qeticons.h"

#include <QTimer>
#include <QMenu>
#include <QHeaderView>
#include <QFont>
#include <QBrush>
#include <QColor>
#include <QRegularExpression>

/**
 * @brief DeviceNavigatorDockWidget::DeviceNavigatorDockWidget
 * @param parent
 */
DeviceNavigatorDockWidget::DeviceNavigatorDockWidget(QWidget *parent) :
	QDockWidget(parent),
	ui(new Ui::DeviceNavigatorDockWidget)
{
	ui->setupUi(this);
	setWindowTitle(tr("Device Navigator"));
	setObjectName(QStringLiteral("device_navigator_dock"));

	// Icons
	ui->m_refresh_btn->setIcon(QET::Icons::ViewRefresh);

	// Category Tabs
	ui->m_category_tabs->addTab(tr("Devices"));
	ui->m_category_tabs->addTab(tr("Cables"));
	ui->m_category_tabs->setCurrentIndex(0);

	// View mode Tabs
	ui->m_view_mode_tabs->addTab(tr("Tree"));
	ui->m_view_mode_tabs->addTab(tr("List"));
	ui->m_view_mode_tabs->setCurrentIndex(0);
	ui->m_view_stack->setCurrentIndex(0);

	// Filter schemes
	loadFilterSchemes();

	// Search placeholder
	ui->m_search_field->setPlaceholderText(tr("Search device, function, folio..."));

	// Tree widget setup
	ui->m_tree_widget->setContextMenuPolicy(Qt::CustomContextMenu);
	ui->m_tree_widget->header()->setVisible(false);

	// Table widget setup
	ui->m_table_widget->setContextMenuPolicy(Qt::CustomContextMenu);
	ui->m_table_widget->setColumnCount(7);
	QStringList headers;
	headers << tr("Device (BMK)")
			<< tr("Function / Description")
			<< tr("Terminals")
			<< tr("Structure (Plant / Location)")
			<< tr("Folio")
			<< tr("Position")
			<< tr("Type");
	ui->m_table_widget->setHorizontalHeaderLabels(headers);
	ui->m_table_widget->horizontalHeader()->setStretchLastSection(true);
	ui->m_table_widget->verticalHeader()->setVisible(false);

	// Debounce timer for non-blocking live updates
	m_debounce_timer = new QTimer(this);
	m_debounce_timer->setSingleShot(true);
	m_debounce_timer->setInterval(150);
	connect(m_debounce_timer, &QTimer::timeout, this, &DeviceNavigatorDockWidget::reload);

	// Signal connections
	connect(ui->m_tree_widget, &QTreeWidget::itemDoubleClicked,
			this, &DeviceNavigatorDockWidget::onItemDoubleClicked);
	connect(ui->m_tree_widget, &QTreeWidget::customContextMenuRequested,
			this, &DeviceNavigatorDockWidget::onCustomContextMenu);

	connect(ui->m_table_widget, &QTableWidget::cellDoubleClicked,
			this, &DeviceNavigatorDockWidget::onTableCellDoubleClicked);
	connect(ui->m_table_widget, &QTableWidget::customContextMenuRequested,
			this, &DeviceNavigatorDockWidget::onTableContextMenu);

	connect(ui->m_search_field, &QLineEdit::textChanged,
			this, &DeviceNavigatorDockWidget::onSearchTextChanged);
	connect(ui->m_filter_combo, qOverload<int>(&QComboBox::currentIndexChanged),
			this, &DeviceNavigatorDockWidget::onFilterComboChanged);
	connect(ui->m_filter_config_btn, &QToolButton::clicked,
			this, &DeviceNavigatorDockWidget::onFilterConfigClicked);
	connect(ui->m_category_tabs, &QTabBar::currentChanged,
			this, &DeviceNavigatorDockWidget::onCategoryTabChanged);
	connect(ui->m_view_mode_tabs, &QTabBar::currentChanged,
			this, &DeviceNavigatorDockWidget::onViewModeTabChanged);
	connect(ui->m_refresh_btn, &QToolButton::clicked,
			this, &DeviceNavigatorDockWidget::reload);
}

/**
 * @brief DeviceNavigatorDockWidget::~DeviceNavigatorDockWidget
 */
DeviceNavigatorDockWidget::~DeviceNavigatorDockWidget()
{
	disconnectProject();
	delete ui;
}

/**
 * @brief DeviceNavigatorDockWidget::setProject
 * @param project
 */
void DeviceNavigatorDockWidget::setProject(QETProject *project)
{
	if (m_project == project) return;
	disconnectProject();
	m_project = project;
	connectProject(m_project);
	scheduleReload();
}

/**
 * @brief DeviceNavigatorDockWidget::project
 * @return
 */
QETProject *DeviceNavigatorDockWidget::project() const
{
	return m_project.data();
}

/**
 * @brief DeviceNavigatorDockWidget::scheduleReload
 */
void DeviceNavigatorDockWidget::scheduleReload()
{
	if (m_debounce_timer) {
		m_debounce_timer->start();
	}
}

/**
 * @brief DeviceNavigatorDockWidget::connectProject
 * @param project
 */
void DeviceNavigatorDockWidget::connectProject(QETProject *project)
{
	if (!project) return;

	m_connections.append(connect(project, &QETProject::projectModified,
								 this, &DeviceNavigatorDockWidget::scheduleReload));
	m_connections.append(connect(project, &QETProject::diagramAdded,
								 this, &DeviceNavigatorDockWidget::scheduleReload));
	m_connections.append(connect(project, &QETProject::diagramRemoved,
								 this, &DeviceNavigatorDockWidget::scheduleReload));
	m_connections.append(connect(project, &QETProject::projectDiagramsOrderChanged,
								 this, &DeviceNavigatorDockWidget::scheduleReload));
	m_connections.append(connect(project, &QETProject::projectTitleChanged,
								 this, &DeviceNavigatorDockWidget::scheduleReload));

	if (project->undoStack()) {
		m_connections.append(connect(project->undoStack(), &QUndoStack::indexChanged,
									 this, &DeviceNavigatorDockWidget::scheduleReload));
	}

	for (Diagram *diag : project->diagrams()) {
		if (diag) {
			m_connections.append(connect(diag, &Diagram::diagramInformationChanged,
										 this, &DeviceNavigatorDockWidget::scheduleReload));
		}
	}
}

/**
 * @brief DeviceNavigatorDockWidget::disconnectProject
 */
void DeviceNavigatorDockWidget::disconnectProject()
{
	for (auto &conn : m_connections) {
		if (conn) {
			QObject::disconnect(conn);
		}
	}
	m_connections.clear();
}

/**
 * @brief DeviceNavigatorDockWidget::elementFunctionDescription
 * @param elmt
 * @return
 */
QString DeviceNavigatorDockWidget::elementFunctionDescription(Element *elmt) const
{
	if (!elmt) return QString();
	const DiagramContext dc = elmt->elementInformations();
	const ElementData data = elmt->elementData();

	QString desc;
	if (data.m_type & ElementData::Master) {
		switch (data.m_master_type) {
			case ElementData::Coil: desc = tr("Coil"); break;
			case ElementData::Protection: desc = tr("Protection"); break;
			case ElementData::Commutator: desc = tr("Commutator"); break;
			case ElementData::PLC: desc = tr("PLC"); break;
			default: desc = tr("Master"); break;
		}
	} else if (data.m_type & ElementData::Slave) {
		if (data.m_slave_type == ElementData::Power) {
			desc = tr("Power contacts");
		} else if (data.m_slave_state == ElementData::NO) {
			desc = tr("NO contact");
		} else if (data.m_slave_state == ElementData::NC) {
			desc = tr("NC contact");
		} else if (data.m_slave_state == ElementData::SW) {
			desc = tr("Changeover contact");
		} else if (data.m_slave_type == ElementData::DelayOn) {
			desc = tr("Delay ON");
		} else if (data.m_slave_type == ElementData::DelayOff) {
			desc = tr("Delay OFF");
		} else {
			desc = tr("Contact");
		}
	} else if (data.m_type & ElementData::Terminal) {
		desc = tr("Terminal");
	} else if (data.m_type & ElementData::Thumbnail) {
		desc = tr("Part placement, general device");
	} else {
		QString desig = dc.value(QStringLiteral("designation")).toString().trimmed();
		if (desig.isEmpty()) desig = dc.value(QStringLiteral("function")).toString().trimmed();
		if (!desig.isEmpty()) {
			desc = desig;
		} else {
			desc = tr("Device");
		}
	}

	QString userComment = dc.value(QStringLiteral("comment")).toString().trimmed();
	if (userComment.isEmpty()) {
		userComment = dc.value(QStringLiteral("description")).toString().trimmed();
	}

	if (!userComment.isEmpty()) {
		if (desc == tr("Device")) {
			desc = userComment;
		} else if (desc != userComment) {
			desc += QStringLiteral(" - ") + userComment;
		}
	}

	return desc;
}

/**
 * @brief DeviceNavigatorDockWidget::buildData
 * @param items
 */
void DeviceNavigatorDockWidget::buildData(QList<ElementInfo> &items)
{
	items.clear();
	if (!m_project) return;

	m_project->buildFolios(m_project->diagrams());

	bool anySchemeWantsReports = false;
	bool anySchemeWantsConductors = false;
	for (const auto &s : m_filter_schemes) {
		if (s.showReports) anySchemeWantsReports = true;
		if (s.showConductorDefs) anySchemeWantsConductors = true;
	}

	const QList<Diagram *> diag_list = m_project->diagrams();
	for (Diagram *diag : diag_list) {
		if (!diag) continue;
		const QList<Element *> elmts = diag->elements();
		for (Element *elmt : elmts) {
			if (!elmt) continue;

			const ElementData data = elmt->elementData();

			bool isReport = (data.m_type & ElementData::AllReport) || (elmt->linkType() & Element::AllReport);
			bool isConductorDef = (data.m_type & ElementData::ConductorDefinition) || (elmt->linkType() & Element::ConductorDefinition);
			if (isReport && !anySchemeWantsReports) continue;
			if (isConductorDef && !anySchemeWantsConductors) continue;

			ElementInfo info;
			info.element = elmt;
			info.diagram = diag;
			info.elementUuid = elmt->uuid();
			info.diagramUuid = diag->uuid();

			const DiagramContext dc = elmt->elementInformations();

			info.isMaster = (data.m_type & ElementData::Master);
			info.isSlave = (data.m_type & ElementData::Slave);
			info.isTerminal = (data.m_type & ElementData::Terminal);
			info.isThumbnail = (data.m_type & ElementData::Thumbnail);
			info.isReport = (data.m_type & ElementData::AllReport) || (elmt->linkType() & Element::AllReport);
			info.isConductorDef = (data.m_type & ElementData::ConductorDefinition) || (elmt->linkType() & Element::ConductorDefinition);

			// Plant & Location (Structure identifier)
			QString plant = dc.value(QStringLiteral("plant")).toString().trimmed();
			QString location = dc.value(QStringLiteral("location")).toString().trimmed();
			if (plant.isEmpty()) {
				plant = diag->border_and_titleblock.plant().trimmed();
			}
			if (location.isEmpty()) {
				location = diag->border_and_titleblock.locmach().trimmed();
			}
			info.plant = plant;
			info.location = location;

			if (plant.isEmpty() && location.isEmpty()) {
				info.structure = tr("No structure identifier");
			} else {
				QString structStr;
				if (!plant.isEmpty()) {
					if (!plant.startsWith(QLatin1Char('='))) structStr += QLatin1Char('=');
					structStr += plant;
				}
				if (!location.isEmpty()) {
					if (!structStr.isEmpty()) structStr += QLatin1Char(' ');
					if (!location.startsWith(QLatin1Char('+'))) structStr += QLatin1Char('+');
					structStr += location;
				}
				info.structure = structStr;
			}

			// Device Tag (BMK)
			QString bmk = elmt->actualLabel().trimmed();
			if (bmk.isEmpty()) {
				bmk = dc.value(QStringLiteral("label")).toString().trimmed();
			}
			if (bmk.isEmpty()) {
				bmk = tr("<unnamed>");
			} else if (!bmk.startsWith(QLatin1Char('-')) &&
					   !bmk.startsWith(QLatin1Char('=')) &&
					   !bmk.startsWith(QLatin1Char('+')) &&
					   bmk != tr("<unnamed>")) {
				bmk.prepend(QLatin1Char('-'));
			}
			info.bmk = bmk;
			info.isCable = bmk.startsWith(QStringLiteral("-W"), Qt::CaseInsensitive);

			// Terminals
			QStringList termList;
			for (Terminal *t : elmt->terminals()) {
				if (t) {
					QString tn = t->name().trimmed();
					if (!tn.isEmpty()) termList << tn;
				}
			}
			info.terminals = termList.join(QLatin1Char('|'));

			// Function description
			info.functionDesc = elementFunctionDescription(elmt);

			// Folio & Position
			QString folio = diag->border_and_titleblock.folio().trimmed();
			if (folio.isEmpty()) {
				folio = QString::number(diag->folioIndex() + 1);
			}
			info.folioStr = folio;
			info.posStr = diag->convertPosition(elmt->scenePos()).toString().trimmed();
			info.locStr = QStringLiteral("#") + folio;
			if (!info.posStr.isEmpty()) {
				info.locStr += QStringLiteral("/") + info.posStr;
			}

			// Full leaf representation line: e.g. 1|2|3|4|5|6 (Dreifachsicherungsautomat) #010/2.1
			QString full;
			if (!info.terminals.isEmpty()) {
				full += info.terminals + QStringLiteral(" ");
			}
			if (!info.functionDesc.isEmpty()) {
				full += QStringLiteral("(") + info.functionDesc + QStringLiteral(") ");
			}
			full += info.locStr;
			info.fullDisplay = full;

			items.append(info);
		}
	}
}

/**
 * @brief DeviceNavigatorDockWidget::reload
 */
void DeviceNavigatorDockWidget::reload()
{
	if (m_last_highlighted_element && m_last_highlighted_element->diagram()) {
		m_last_highlighted_element->setHighlighted(false);
	}
	m_last_highlighted_element = nullptr;

	buildData(m_cached_items);
	populateTree(m_cached_items);
	populateTable(m_cached_items);
	applyFilters();
}

/**
 * @brief DeviceNavigatorDockWidget::populateTree
 * @param items
 */
void DeviceNavigatorDockWidget::populateTree(const QList<ElementInfo> &items)
{
	ui->m_tree_widget->clear();
	if (!m_project) return;

	// Project Root Node
	QString prjTitle = m_project->title().trimmed();
	if (prjTitle.isEmpty()) {
		prjTitle = m_project->filePath();
		if (prjTitle.isEmpty()) {
			prjTitle = tr("Unnamed Project");
		} else {
			prjTitle = QFileInfo(prjTitle).fileName();
		}
	}

	auto *projectItem = new QTreeWidgetItem(ui->m_tree_widget);
	projectItem->setText(0, prjTitle);
	projectItem->setIcon(0, QET::Icons::Project);
	projectItem->setData(0, NodeTypeRole, ProjectNode);
	projectItem->setData(0, ProjectPointerRole, QVariant::fromValue(static_cast<void*>(m_project.data())));
	QFont pFont = projectItem->font(0);
	pFont.setBold(true);
	projectItem->setFont(0, pFont);

	// Group by structure: QMap keeps sorted order
	// Place "Ohne Strukturkennzeichen" explicitly at the end if desired
	const QString noStructureName = tr("No structure identifier");
	QMap<QString, QMap<QString, QList<ElementInfo>>> structureMap;

	for (const ElementInfo &info : items) {
		structureMap[info.structure][info.bmk].append(info);
	}

	// Order structures: named ones first, "Ohne Strukturkennzeichen" last
	QStringList structKeys = structureMap.keys();
	bool hasNoStructure = structKeys.removeAll(noStructureName);
	structKeys.sort();
	if (hasNoStructure) {
		structKeys.append(noStructureName);
	}

	for (const QString &structName : structKeys) {
		auto *structItem = new QTreeWidgetItem(projectItem);
		structItem->setText(0, structName);
		structItem->setIcon(0, QET::Icons::Folder);
		structItem->setData(0, NodeTypeRole, StructureNode);
		structItem->setData(0, StructureRole, structName);

		const QMap<QString, QList<ElementInfo>> &devices = structureMap[structName];
		QStringList bmkKeys = devices.keys();
		bmkKeys.sort();

		for (const QString &bmk : bmkKeys) {
			QList<ElementInfo> deviceElements = devices[bmk];
			if (deviceElements.isEmpty()) continue;

			// Sort device functions: Master/main function first, then Slaves, then others; then by folio/pos
			std::stable_sort(deviceElements.begin(), deviceElements.end(), [](const ElementInfo &a, const ElementInfo &b) {
				int rankA = a.isMaster ? 0 : (a.isSlave ? 1 : 2);
				int rankB = b.isMaster ? 0 : (b.isSlave ? 1 : 2);
				if (rankA != rankB) return rankA < rankB;
				if (a.folioStr != b.folioStr) return a.folioStr < b.folioStr;
				return a.posStr < b.posStr;
			});

			auto *deviceItem = new QTreeWidgetItem(structItem);
			deviceItem->setText(0, bmk);
			deviceItem->setData(0, NodeTypeRole, DeviceNode);
			deviceItem->setData(0, BmkRole, bmk);
			deviceItem->setData(0, StructureRole, structName);

			// Choose icon for device
			if (bmk.startsWith(QStringLiteral("-K"))) {
				deviceItem->setIcon(0, QET::Icons::ElementMaster);
			} else if (bmk.startsWith(QStringLiteral("-X"))) {
				deviceItem->setIcon(0, QET::Icons::TerminalStrip);
			} else if (bmk.startsWith(QStringLiteral("-W"))) {
				deviceItem->setIcon(0, QET::Icons::Cable);
			} else {
				deviceItem->setIcon(0, QET::Icons::GenericDevice);
			}

			// Store master / first element as jump target for double clicking the device node
			if (!deviceElements.isEmpty()) {
				const ElementInfo &firstInfo = deviceElements.first();
				deviceItem->setData(0, ElementUuidRole, firstInfo.elementUuid.toString());
				deviceItem->setData(0, DiagramUuidRole, firstInfo.diagramUuid.toString());
				if (firstInfo.element) {
					deviceItem->setData(0, ElementPointerRole,
										QVariant::fromValue(static_cast<void*>(firstInfo.element.data())));
				}
				if (firstInfo.diagram) {
					deviceItem->setData(0, DiagramPointerRole,
										QVariant::fromValue(static_cast<void*>(firstInfo.diagram.data())));
				}
			}

			// Add leaf function items
			for (const ElementInfo &info : deviceElements) {
				auto *funcItem = new QTreeWidgetItem(deviceItem);
				funcItem->setText(0, info.fullDisplay);
				funcItem->setData(0, NodeTypeRole, FunctionNode);
				funcItem->setData(0, BmkRole, info.bmk);
				funcItem->setData(0, StructureRole, info.structure);
				funcItem->setData(0, ElementUuidRole, info.elementUuid.toString());
				funcItem->setData(0, DiagramUuidRole, info.diagramUuid.toString());
				funcItem->setData(0, IsMasterRole, info.isMaster);
				funcItem->setData(0, IsSlaveRole, info.isSlave);
				funcItem->setData(0, IsTerminalRole, info.isTerminal);
				funcItem->setData(0, IsCableRole, info.isCable);
				funcItem->setData(0, IsReportRole, info.isReport);
				funcItem->setData(0, IsConductorDefRole, info.isConductorDef);
				funcItem->setData(0, IsThumbnailRole, info.isThumbnail);
				if (info.element) {
					funcItem->setData(0, ElementPointerRole,
									  QVariant::fromValue(static_cast<void*>(info.element.data())));
				}
				if (info.diagram) {
					funcItem->setData(0, DiagramPointerRole,
									  QVariant::fromValue(static_cast<void*>(info.diagram.data())));
				}

				// Function icon
				if (info.isMaster) {
					funcItem->setIcon(0, QET::Icons::ElementMaster);
				} else if (info.isSlave) {
					funcItem->setIcon(0, QET::Icons::ElementSlave);
				} else if (info.isTerminal) {
					funcItem->setIcon(0, QET::Icons::Terminal);
				} else if (info.isCable) {
					funcItem->setIcon(0, QET::Icons::Cable);
				} else {
					funcItem->setIcon(0, QET::Icons::GenericDevice);
				}
			}
		}
	}

	projectItem->setExpanded(true);
}

/**
 * @brief DeviceNavigatorDockWidget::populateTable
 * @param items
 */
void DeviceNavigatorDockWidget::populateTable(const QList<ElementInfo> &items)
{
	ui->m_table_widget->setSortingEnabled(false);
	ui->m_table_widget->setRowCount(items.size());

	for (int row = 0; row < items.size(); ++row) {
		const ElementInfo &info = items.at(row);

		auto *itemBmk = new QTableWidgetItem(info.bmk);
		if (info.isMaster) itemBmk->setIcon(QET::Icons::ElementMaster);
		else if (info.isSlave) itemBmk->setIcon(QET::Icons::ElementSlave);
		else if (info.isTerminal) itemBmk->setIcon(QET::Icons::Terminal);
		else if (info.isCable) itemBmk->setIcon(QET::Icons::Cable);
		else itemBmk->setIcon(QET::Icons::GenericDevice);

		itemBmk->setData(ElementUuidRole, info.elementUuid.toString());
		itemBmk->setData(DiagramUuidRole, info.diagramUuid.toString());
		itemBmk->setData(IsMasterRole, info.isMaster);
		itemBmk->setData(IsSlaveRole, info.isSlave);
		itemBmk->setData(IsTerminalRole, info.isTerminal);
		itemBmk->setData(IsCableRole, info.isCable);
		itemBmk->setData(IsReportRole, info.isReport);
		itemBmk->setData(IsConductorDefRole, info.isConductorDef);
		itemBmk->setData(IsThumbnailRole, info.isThumbnail);
		if (info.element) {
			itemBmk->setData(ElementPointerRole, QVariant::fromValue(static_cast<void*>(info.element.data())));
		}
		if (info.diagram) {
			itemBmk->setData(DiagramPointerRole, QVariant::fromValue(static_cast<void*>(info.diagram.data())));
		}

		auto *itemFunc = new QTableWidgetItem(info.functionDesc);
		auto *itemTerm = new QTableWidgetItem(info.terminals);
		auto *itemStruct = new QTableWidgetItem(info.structure);
		auto *itemFolio = new QTableWidgetItem(info.folioStr);
		auto *itemPos = new QTableWidgetItem(info.posStr);

		QString typeStr;
		if (info.isMaster) typeStr = tr("Master");
		else if (info.isSlave) typeStr = tr("Slave");
		else if (info.isTerminal) typeStr = tr("Terminal");
		else if (info.isCable) typeStr = tr("Cable");
		else if (info.isThumbnail) typeStr = tr("Cabinet Layout");
		else typeStr = tr("Symbol");
		auto *itemType = new QTableWidgetItem(typeStr);

		ui->m_table_widget->setItem(row, 0, itemBmk);
		ui->m_table_widget->setItem(row, 1, itemFunc);
		ui->m_table_widget->setItem(row, 2, itemTerm);
		ui->m_table_widget->setItem(row, 3, itemStruct);
		ui->m_table_widget->setItem(row, 4, itemFolio);
		ui->m_table_widget->setItem(row, 5, itemPos);
		ui->m_table_widget->setItem(row, 6, itemType);
	}

	ui->m_table_widget->resizeColumnsToContents();
	ui->m_table_widget->setSortingEnabled(true);
}

/**
 * @brief DeviceNavigatorDockWidget::applyFilters
 */
void DeviceNavigatorDockWidget::applyFilters()
{
	const QString searchText = ui->m_search_field->text().trimmed();
	const int filterIdx = ui->m_filter_combo->currentIndex();
	const int catTab = ui->m_category_tabs->currentIndex();
	const bool cablesOnly = (catTab == 1);

	DeviceNavigatorFilterScheme activeScheme;
	if (filterIdx >= 0 && filterIdx < m_filter_schemes.size()) {
		activeScheme = m_filter_schemes.at(filterIdx);
	}

	// Helper lambda for element filter matching
	auto matchesFilter = [&](const QString &bmk,
							 const QString &fullText,
							 const QString &structure,
							 bool isMaster,
							 bool isSlave,
							 bool isTerminal,
							 bool isCable,
							 bool isReport,
							 bool isConductorDef,
							 bool isThumbnail) -> bool {
		if (cablesOnly && !isCable) return false;

		if (!activeScheme.matches(bmk, structure, isMaster, isSlave, isTerminal, isCable, isReport, isConductorDef, isThumbnail)) {
			return false;
		}

		if (!searchText.isEmpty()) {
			bool hit = bmk.contains(searchText, Qt::CaseInsensitive)
					|| fullText.contains(searchText, Qt::CaseInsensitive)
					|| structure.contains(searchText, Qt::CaseInsensitive);
			if (!hit) return false;
		}
		return true;
	};

	// 1. Filter Tree Widget
	std::function<bool(QTreeWidgetItem*)> filterItem = [&](QTreeWidgetItem *item) -> bool {
		int type = item->data(0, NodeTypeRole).toInt();
		if (type == FunctionNode) {
			QString bmk = item->data(0, BmkRole).toString();
			QString full = item->text(0);
			QString structure = item->data(0, StructureRole).toString();
			bool isMaster = item->data(0, IsMasterRole).toBool();
			bool isSlave = item->data(0, IsSlaveRole).toBool();
			bool isTerminal = item->data(0, IsTerminalRole).toBool();
			bool isCable = item->data(0, IsCableRole).toBool();
			bool isReport = item->data(0, IsReportRole).toBool();
			bool isConductorDef = item->data(0, IsConductorDefRole).toBool();
			bool isThumbnail = item->data(0, IsThumbnailRole).toBool();

			bool visible = matchesFilter(bmk, full, structure, isMaster, isSlave, isTerminal, isCable, isReport, isConductorDef, isThumbnail);
			item->setHidden(!visible);
			return visible;
		}

		bool anyChildVisible = false;
		for (int i = 0; i < item->childCount(); ++i) {
			if (filterItem(item->child(i))) {
				anyChildVisible = true;
			}
		}

		if (type == ProjectNode) {
			item->setHidden(false);
			return true;
		}

		item->setHidden(!anyChildVisible);
		if (anyChildVisible && (!searchText.isEmpty() || filterIdx > 0)) {
			item->setExpanded(true);
		}
		return anyChildVisible;
	};

	for (int i = 0; i < ui->m_tree_widget->topLevelItemCount(); ++i) {
		filterItem(ui->m_tree_widget->topLevelItem(i));
	}

	// 2. Filter Table Widget
	for (int r = 0; r < ui->m_table_widget->rowCount(); ++r) {
		QTableWidgetItem *bmkItem = ui->m_table_widget->item(r, 0);
		QTableWidgetItem *funcItem = ui->m_table_widget->item(r, 1);
		QTableWidgetItem *structItem = ui->m_table_widget->item(r, 3);
		if (!bmkItem || !funcItem || !structItem) continue;

		QString bmk = bmkItem->text();
		QString full = funcItem->text();
		QString structStr = structItem->text();
		bool isMaster = bmkItem->data(IsMasterRole).toBool();
		bool isSlave = bmkItem->data(IsSlaveRole).toBool();
		bool isTerminal = bmkItem->data(IsTerminalRole).toBool();
		bool isCable = bmkItem->data(IsCableRole).toBool();
		bool isReport = bmkItem->data(IsReportRole).toBool();
		bool isConductorDef = bmkItem->data(IsConductorDefRole).toBool();
		bool isThumbnail = bmkItem->data(IsThumbnailRole).toBool();

		bool visible = matchesFilter(bmk, full, structStr, isMaster, isSlave, isTerminal, isCable, isReport, isConductorDef, isThumbnail);
		ui->m_table_widget->setRowHidden(r, !visible);
	}
}

/**
 * @brief DeviceNavigatorDockWidget::jumpToUuid
 * Safely resolves the diagram and element by UUID without risking dangling pointer dereferences.
 * @param elementUuid
 * @param diagramUuid
 */
void DeviceNavigatorDockWidget::jumpToUuid(const QUuid &elementUuid, const QUuid &diagramUuid)
{
	if (!m_project || elementUuid.isNull()) return;

	Diagram *diag = diagramUuid.isNull() ? nullptr : m_project->diagramByUuid(diagramUuid);
	if (!diag) {
		for (Diagram *d : m_project->diagrams()) {
			if (d) {
				m_project->buildFolioToShow(d);
				for (Element *e : d->elements()) {
					if (e && e->uuid() == elementUuid) {
						diag = d;
						break;
					}
				}
				if (diag) break;
			}
		}
	} else {
		m_project->buildFolioToShow(diag);
	}

	if (!diag) {
		scheduleReload();
		return;
	}

	Element *targetElement = nullptr;
	for (Element *e : diag->elements()) {
		if (e && e->uuid() == elementUuid) {
			targetElement = e;
			break;
		}
	}

	if (!targetElement) {
		scheduleReload();
		return;
	}

	if (m_last_highlighted_element && m_last_highlighted_element != targetElement) {
		if (m_last_highlighted_element->diagram()) {
			m_last_highlighted_element->setHighlighted(false);
		}
	}

	diag->showMe();
	diag->clearSelection();
	targetElement->setSelected(true);
	targetElement->setHighlighted(true);
	m_last_highlighted_element = targetElement;

	if (!diag->views().isEmpty()) {
		if (auto *view = qobject_cast<DiagramView *>(diag->views().first())) {
			const QPointF centerPos = targetElement->sceneBoundingRect().center();
			QMetaObject::invokeMethod(view, [view, centerPos]() {
				view->centerOn(centerPos);
			}, Qt::QueuedConnection);
		}
	}
}

/**
 * @brief DeviceNavigatorDockWidget::jumpToElement
 * @param element
 */
void DeviceNavigatorDockWidget::jumpToElement(Element *element)
{
	if (!element) return;
	jumpToUuid(element->uuid(), element->diagram() ? element->diagram()->uuid() : QUuid());
}

/**
 * @brief DeviceNavigatorDockWidget::onItemDoubleClicked
 * @param item
 * @param column
 */
void DeviceNavigatorDockWidget::onItemDoubleClicked(QTreeWidgetItem *item, int column)
{
	Q_UNUSED(column);
	if (!item) return;

	int type = item->data(0, NodeTypeRole).toInt();
	if (type == FunctionNode || type == DeviceNode) {
		QString uuidStr = item->data(0, ElementUuidRole).toString();
		QString diagUuidStr = item->data(0, DiagramUuidRole).toString();
		if (!uuidStr.isEmpty()) {
			jumpToUuid(QUuid(uuidStr), QUuid(diagUuidStr));
		}
	}
}

/**
 * @brief DeviceNavigatorDockWidget::onTableCellDoubleClicked
 * @param row
 * @param column
 */
void DeviceNavigatorDockWidget::onTableCellDoubleClicked(int row, int column)
{
	Q_UNUSED(column);
	QTableWidgetItem *item = ui->m_table_widget->item(row, 0);
	if (!item) return;

	QString uuidStr = item->data(ElementUuidRole).toString();
	QString diagUuidStr = item->data(DiagramUuidRole).toString();
	if (!uuidStr.isEmpty()) {
		jumpToUuid(QUuid(uuidStr), QUuid(diagUuidStr));
	}
}

/**
 * @brief DeviceNavigatorDockWidget::onSearchTextChanged
 * @param text
 */
void DeviceNavigatorDockWidget::onSearchTextChanged(const QString &text)
{
	Q_UNUSED(text);
	applyFilters();
}

/**
 * @brief DeviceNavigatorDockWidget::onFilterComboChanged
 * @param index
 */
void DeviceNavigatorDockWidget::onFilterComboChanged(int index)
{
	Q_UNUSED(index);
	applyFilters();
}

/**
 * @brief DeviceNavigatorDockWidget::onCategoryTabChanged
 * @param index
 */
void DeviceNavigatorDockWidget::onCategoryTabChanged(int index)
{
	Q_UNUSED(index);
	applyFilters();
}

/**
 * @brief DeviceNavigatorDockWidget::onViewModeTabChanged
 * @param index
 */
void DeviceNavigatorDockWidget::onViewModeTabChanged(int index)
{
	ui->m_view_stack->setCurrentIndex(index);
}

/**
 * @brief DeviceNavigatorDockWidget::onCustomContextMenu
 * @param pos
 */
void DeviceNavigatorDockWidget::onCustomContextMenu(const QPoint &pos)
{
	QTreeWidgetItem *item = ui->m_tree_widget->itemAt(pos);
	if (!item) return;

	QString uuidStr = item->data(0, ElementUuidRole).toString();
	QString diagUuidStr = item->data(0, DiagramUuidRole).toString();
	QUuid elmtUuid(uuidStr);
	QUuid diagUuid(diagUuidStr);

	QMenu menu(this);
	if (!elmtUuid.isNull()) {
		menu.addAction(tr("Show in diagram"), this, [this, elmtUuid, diagUuid]() {
			jumpToUuid(elmtUuid, diagUuid);
		});
		menu.addAction(tr("Edit properties..."), this, [this, elmtUuid, diagUuid]() {
			if (!m_project) return;
			Diagram *diag = m_project->diagramByUuid(diagUuid);
			if (diag) {
				m_project->buildFolioToShow(diag);
				for (Element *e : diag->elements()) {
					if (e && e->uuid() == elmtUuid) {
						e->editProperty();
						break;
					}
				}
			}
		});
		menu.addSeparator();
	}
	menu.addAction(tr("Refresh"), this, &DeviceNavigatorDockWidget::reload);
	menu.exec(ui->m_tree_widget->viewport()->mapToGlobal(pos));
}

/**
 * @brief DeviceNavigatorDockWidget::onTableContextMenu
 * @param pos
 */
void DeviceNavigatorDockWidget::onTableContextMenu(const QPoint &pos)
{
	QTableWidgetItem *item = ui->m_table_widget->itemAt(pos);
	if (!item) return;

	QTableWidgetItem *bmkItem = ui->m_table_widget->item(item->row(), 0);
	if (!bmkItem) return;

	QString uuidStr = bmkItem->data(ElementUuidRole).toString();
	QString diagUuidStr = bmkItem->data(DiagramUuidRole).toString();
	QUuid elmtUuid(uuidStr);
	QUuid diagUuid(diagUuidStr);

	QMenu menu(this);
	if (!elmtUuid.isNull()) {
		menu.addAction(tr("Show in diagram"), this, [this, elmtUuid, diagUuid]() {
			jumpToUuid(elmtUuid, diagUuid);
		});
		menu.addAction(tr("Edit properties..."), this, [this, elmtUuid, diagUuid]() {
			if (!m_project) return;
			Diagram *diag = m_project->diagramByUuid(diagUuid);
			if (diag) {
				m_project->buildFolioToShow(diag);
				for (Element *e : diag->elements()) {
					if (e && e->uuid() == elmtUuid) {
						e->editProperty();
						break;
					}
				}
			}
		});
		menu.addSeparator();
	}
	menu.addAction(tr("Refresh"), this, &DeviceNavigatorDockWidget::reload);
	menu.exec(ui->m_table_widget->viewport()->mapToGlobal(pos));
}


/**
 * @brief DeviceNavigatorDockWidget::loadFilterSchemes
 */
void DeviceNavigatorDockWidget::loadFilterSchemes()
{
	m_filter_schemes = DeviceNavigatorFilterScheme::loadSchemes();
	updateFilterCombo();
}

/**
 * @brief DeviceNavigatorDockWidget::updateFilterCombo
 */
void DeviceNavigatorDockWidget::updateFilterCombo()
{
	const QString currentName = ui->m_filter_combo->currentText();
	ui->m_filter_combo->blockSignals(true);
	ui->m_filter_combo->clear();

	int selectIndex = 0;
	for (int i = 0; i < m_filter_schemes.size(); ++i) {
		ui->m_filter_combo->addItem(m_filter_schemes.at(i).name);
		if (m_filter_schemes.at(i).name == currentName) {
			selectIndex = i;
		}
	}
	ui->m_filter_combo->setCurrentIndex(selectIndex);
	ui->m_filter_combo->blockSignals(false);
}

/**
 * @brief DeviceNavigatorDockWidget::onFilterConfigClicked
 */
void DeviceNavigatorDockWidget::onFilterConfigClicked()
{
	DeviceNavigatorFilterDialog dlg(this);
	dlg.setSchemes(m_filter_schemes, ui->m_filter_combo->currentIndex());
	if (dlg.exec() == QDialog::Accepted) {
		m_filter_schemes = dlg.schemes();
		DeviceNavigatorFilterScheme::saveSchemes(m_filter_schemes);
		updateFilterCombo();
		int newIdx = dlg.currentSchemeIndex();
		if (newIdx >= 0 && newIdx < ui->m_filter_combo->count()) {
			ui->m_filter_combo->setCurrentIndex(newIdx);
		}
		applyFilters();
	}
}
