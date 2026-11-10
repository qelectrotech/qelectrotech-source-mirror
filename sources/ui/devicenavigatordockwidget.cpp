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
#include "../cable/cable.h"
#include "../cable/cablepart.h"
#include "../qeticons.h"

#include <QTimer>
#include <QMenu>
#include <QHeaderView>
#include <QFont>
#include <QBrush>
#include <QColor>
#include <QRegularExpression>
#include <QScrollBar>
#include <QSqlQuery>

namespace {
	class FolioTableWidgetItem : public QTableWidgetItem {
	public:
		FolioTableWidgetItem(const QString &text, int folioIndex)
			: QTableWidgetItem(text), m_folioIndex(folioIndex) {}

		bool operator<(const QTableWidgetItem &other) const override {
			if (auto *o = dynamic_cast<const FolioTableWidgetItem *>(&other)) {
				return m_folioIndex < o->m_folioIndex;
			}
			return QTableWidgetItem::operator<(other);
		}

	private:
		int m_folioIndex = 0;
	};
}

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
	headers << tr("Device tag")
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

	// Reload when becoming visible if data was dirtied while hidden
	connect(this, &QDockWidget::visibilityChanged, this, [this](bool visible) {
		if (visible && m_dirty) {
			m_dirty = false;
			reload();
		}
	});

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
 * @brief DeviceNavigatorDockWidget::showEvent
 * @param event
 */
void DeviceNavigatorDockWidget::showEvent(QShowEvent *event)
{
	QDockWidget::showEvent(event);
	if (m_dirty) {
		m_dirty = false;
		reload();
	}
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
	if (!isVisible()) {
		m_dirty = true;
		return;
	}
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
	if (!isVisible()) {
		m_dirty = true;
		return;
	}
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
								 this, [this](QETProject *, Diagram *diagram) {
		if (diagram) {
			m_connections.append(connect(diagram, &Diagram::diagramInformationChanged,
										 this, &DeviceNavigatorDockWidget::scheduleReload));
		}
		scheduleReload();
	}));
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

	for (Diagram *diag : project->folios()) {
		if (diag) {
			m_connections.append(connect(diag, &Diagram::diagramInformationChanged,
										 this, &DeviceNavigatorDockWidget::scheduleReload));
		}
	}

	for (Cable *cable : project->cables()) {
		if (cable) {
			m_connections.append(connect(cable, &Cable::changed,
										 this, &DeviceNavigatorDockWidget::scheduleReload));
		}
	}

	m_connections.append(connect(project, &QETProject::cableAdded, this, [this](Cable *c) {
		if (c) {
			m_connections.append(connect(c, &Cable::changed,
										 this, &DeviceNavigatorDockWidget::scheduleReload));
		}
		scheduleReload();
	}));
	m_connections.append(connect(project, &QETProject::cableRemoved,
								 this, &DeviceNavigatorDockWidget::scheduleReload));
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

	bool anySchemeWantsReports = false;
	bool anySchemeWantsConductors = false;
	bool anySchemeWantsCables = false;
	for (const auto &s : m_filter_schemes) {
		if (s.showReports) anySchemeWantsReports = true;
		if (s.showConductorDefs) anySchemeWantsConductors = true;
		if (s.showCables) anySchemeWantsCables = true;
	}

	// 1. Process loaded (built) folios
	const QList<Diagram *> diag_list = m_project->folios();
	QSet<QUuid> unbuiltFolioUuids;

	for (Diagram *diag : diag_list) {
		if (!diag) continue;
		if (!diag->isLoaded()) {
			unbuiltFolioUuids.insert(diag->uuid());
			continue;
		}

		for (Element *elmt : diag->elements()) {
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
			info.isReport = isReport;
			info.isConductorDef = isConductorDef;
			info.isCable = false;

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
			info.hasStructure = (!plant.isEmpty() || !location.isEmpty());

			if (!info.hasStructure) {
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

			// Device Tag
			QString bmk = elmt->actualLabel().trimmed();
			if (bmk.isEmpty()) {
				bmk = dc.value(QStringLiteral("label")).toString().trimmed();
			}
			if (bmk.isEmpty()) {
				bmk = tr("<unnamed>");
			}
			info.bmk = bmk;

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

			// Folio & Position (Rely on existing canonical folio evaluation)
			info.folioIndex = diag->folioIndex();
			QString folio = diag->border_and_titleblock.finalfolio().trimmed();
			if (folio.isEmpty()) {
				folio = QString::number(diag->folioIndex() + 1);
			}
			info.folioStr = folio;
			info.posStr = diag->convertPosition(elmt->scenePos()).toString().trimmed();
			info.locStr = QStringLiteral("#") + folio;
			if (!info.posStr.isEmpty()) {
				info.locStr += QStringLiteral("/") + info.posStr;
			}

			// Full leaf representation line
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

	// 2. Process unbuilt (lazy) folios from project database
	if (!unbuiltFolioUuids.isEmpty() && m_project->dataBase()) {
		projectDataBase *db = m_project->dataBase();

		// Map terminals by element_uuid
		QHash<QUuid, QStringList> elementTerminals;
		QSqlQuery termQuery = db->newQuery(QStringLiteral("SELECT element_uuid, name FROM terminal ORDER BY terminal_index"));
		while (termQuery.next()) {
			QUuid elmtUuid(termQuery.value(0).toString());
			QString name = termQuery.value(1).toString().trimmed();
			if (!name.isEmpty()) {
				elementTerminals[elmtUuid].append(name);
			}
		}

		QString sql = QStringLiteral(
			"SELECT e.uuid, e.diagram_uuid, e.pos, e.type, e.sub_type, "
			"ei.label, ei.plant, ei.location, ei.comment, ei.description, ei.designation, ei.function "
			"FROM element e "
			"LEFT JOIN element_info ei ON e.uuid = ei.element_uuid"
		);
		QSqlQuery q = db->newQuery(sql);
		while (q.next()) {
			QUuid diagUuid(q.value(1).toString());
			if (!unbuiltFolioUuids.contains(diagUuid)) {
				continue; // already processed from loaded diagram
			}

			QUuid elmtUuid(q.value(0).toString());
			QString pos = q.value(2).toString().trimmed();
			QString typeStr = q.value(3).toString().trimmed();
			QString subTypeStr = q.value(4).toString().trimmed();

			QString label = q.value(5).toString().trimmed();
			QString plant = q.value(6).toString().trimmed();
			QString location = q.value(7).toString().trimmed();
			QString comment = q.value(8).toString().trimmed();
			QString description = q.value(9).toString().trimmed();
			QString designation = q.value(10).toString().trimmed();
			QString function = q.value(11).toString().trimmed();

			bool isReport = (typeStr == QStringLiteral("next_report") || typeStr == QStringLiteral("previous_report"));
			bool isConductorDef = (typeStr == QStringLiteral("conductor_definition"));
			if (isReport && !anySchemeWantsReports) continue;
			if (isConductorDef && !anySchemeWantsConductors) continue;

			ElementInfo info;
			info.elementUuid = elmtUuid;
			info.diagramUuid = diagUuid;
			info.isMaster = (typeStr == QStringLiteral("master"));
			info.isSlave = (typeStr == QStringLiteral("slave"));
			info.isTerminal = (typeStr == QStringLiteral("terminal"));
			info.isThumbnail = (typeStr == QStringLiteral("thumbnail"));
			info.isReport = isReport;
			info.isConductorDef = isConductorDef;
			info.isCable = false;

			Diagram *diag = m_project->diagramByUuid(diagUuid);
			if (diag) {
				info.diagram = diag;
				if (plant.isEmpty()) {
					plant = diag->border_and_titleblock.plant().trimmed();
				}
				if (location.isEmpty()) {
					location = diag->border_and_titleblock.locmach().trimmed();
				}
				info.folioIndex = diag->folioIndex();
				QString folio = diag->border_and_titleblock.finalfolio().trimmed();
				if (folio.isEmpty()) {
					folio = QString::number(diag->folioIndex() + 1);
				}
				info.folioStr = folio;
				info.posStr = pos;
				info.locStr = QStringLiteral("#") + folio;
				if (!info.posStr.isEmpty()) {
					info.locStr += QStringLiteral("/") + info.posStr;
				}
			} else {
				info.posStr = pos;
			}

			info.plant = plant;
			info.location = location;
			info.hasStructure = (!plant.isEmpty() || !location.isEmpty());

			if (!info.hasStructure) {
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

			// Device Tag
			QString bmk = label;
			if (bmk.isEmpty()) {
				bmk = tr("<unnamed>");
			}
			info.bmk = bmk;

			// Terminals
			info.terminals = elementTerminals.value(elmtUuid).join(QLatin1Char('|'));

			// Function description
			QString desc;
			if (info.isMaster) {
				if (subTypeStr.contains(QStringLiteral("coil"), Qt::CaseInsensitive)) desc = tr("Coil");
				else if (subTypeStr.contains(QStringLiteral("protection"), Qt::CaseInsensitive)) desc = tr("Protection");
				else if (subTypeStr.contains(QStringLiteral("commutator"), Qt::CaseInsensitive)) desc = tr("Commutator");
				else if (subTypeStr.contains(QStringLiteral("plc"), Qt::CaseInsensitive)) desc = tr("PLC");
				else desc = tr("Master");
			} else if (info.isSlave) {
				if (subTypeStr.contains(QStringLiteral("power"), Qt::CaseInsensitive)) desc = tr("Power contacts");
				else if (subTypeStr.contains(QStringLiteral("no"), Qt::CaseInsensitive)) desc = tr("NO contact");
				else if (subTypeStr.contains(QStringLiteral("nc"), Qt::CaseInsensitive)) desc = tr("NC contact");
				else if (subTypeStr.contains(QStringLiteral("sw"), Qt::CaseInsensitive)) desc = tr("Changeover contact");
				else desc = tr("Contact");
			} else if (info.isTerminal) {
				desc = tr("Terminal");
			} else if (info.isThumbnail) {
				desc = tr("Part placement, general device");
			} else {
				if (!designation.isEmpty()) desc = designation;
				else if (!function.isEmpty()) desc = function;
				else desc = tr("Device");
			}

			QString userComment = comment;
			if (userComment.isEmpty()) userComment = description;
			if (!userComment.isEmpty()) {
				if (desc == tr("Device")) desc = userComment;
				else if (desc != userComment) desc += QStringLiteral(" - ") + userComment;
			}
			info.functionDesc = desc;

			// Full display
			QString full;
			if (!info.terminals.isEmpty()) full += info.terminals + QStringLiteral(" ");
			if (!info.functionDesc.isEmpty()) full += QStringLiteral("(") + info.functionDesc + QStringLiteral(") ");
			if (!info.locStr.isEmpty()) full += info.locStr;
			info.fullDisplay = full.trimmed();

			items.append(info);
		}
	}

	// Project cables (from cable management)
	if (anySchemeWantsCables) {
		for (Cable *cable : m_project->cables()) {
			if (!cable) continue;
			QString bmk = cable->bmk().trimmed();
			if (bmk.isEmpty()) bmk = cable->designation().trimmed();
			if (bmk.isEmpty()) bmk = tr("<unnamed>");

			QString plant = cable->installation().trimmed();
			QString location = cable->location().trimmed();
			bool hasStructure = (!plant.isEmpty() || !location.isEmpty());
			QString structStr;
			if (hasStructure) {
				if (!plant.isEmpty()) {
					if (!plant.startsWith(QLatin1Char('='))) structStr += QLatin1Char('=');
					structStr += plant;
				}
				if (!location.isEmpty()) {
					if (!structStr.isEmpty()) structStr += QLatin1Char(' ');
					if (!location.startsWith(QLatin1Char('+'))) structStr += QLatin1Char('+');
					structStr += location;
				}
			} else {
				structStr = tr("No structure identifier");
			}

			const QList<CablePartData> parts = cable->parts();
			if (parts.isEmpty()) {
				ElementInfo info;
				info.isCable = true;
				info.elementUuid = cable->uuid();
				info.hasStructure = hasStructure;
				info.plant = plant;
				info.location = location;
				info.structure = structStr;
				info.bmk = bmk;
				info.functionDesc = cable->type();
				if (info.functionDesc.isEmpty()) info.functionDesc = tr("Cable");
				info.terminals = tr("%1 cores").arg(cable->coreCount());
				info.folioIndex = 0;
				info.folioStr = QString();
				info.fullDisplay = info.terminals + QStringLiteral(" (") + info.functionDesc + QStringLiteral(")");
				items.append(info);
			} else {
				for (const CablePartData &part : parts) {
					ElementInfo info;
					info.isCable = true;
					info.elementUuid = part.uuid;
					info.diagramUuid = part.diagram;
					info.hasStructure = hasStructure;
					info.plant = plant;
					info.location = location;
					info.structure = structStr;
					info.bmk = bmk;
					info.functionDesc = cable->type();
					if (info.functionDesc.isEmpty()) info.functionDesc = tr("Cable");
					info.terminals = tr("%1 cores").arg(cable->coreCount());

					Diagram *diag = m_project->diagramByUuid(part.diagram);
					if (diag) {
						info.diagram = diag;
						info.folioIndex = diag->folioIndex();
						QString folio = diag->border_and_titleblock.finalfolio().trimmed();
						if (folio.isEmpty()) {
							folio = QString::number(diag->folioIndex() + 1);
						}
						info.folioStr = folio;
						info.posStr = diag->convertPosition(part.p1).toString().trimmed();
						info.locStr = QStringLiteral("#") + folio;
						if (!info.posStr.isEmpty()) {
							info.locStr += QStringLiteral("/") + info.posStr;
						}
					}
					QString full;
					if (!info.terminals.isEmpty()) full += info.terminals + QStringLiteral(" ");
					if (!info.functionDesc.isEmpty()) full += QStringLiteral("(") + info.functionDesc + QStringLiteral(") ");
					if (!info.locStr.isEmpty()) full += info.locStr;
					info.fullDisplay = full.trimmed();
					items.append(info);
				}
			}
		}
	}
}

/**
 * @brief DeviceNavigatorDockWidget::saveTreeState
 * Collects current expansion states, selection, and scroll positions.
 */
DeviceNavigatorDockWidget::TreeState DeviceNavigatorDockWidget::saveTreeState() const
{
	TreeState state;
	state.vScroll = ui->m_tree_widget->verticalScrollBar()->value();
	state.hScroll = ui->m_tree_widget->horizontalScrollBar()->value();

	QTreeWidgetItem *curItem = ui->m_tree_widget->currentItem();
	if (curItem) {
		state.hasSelection = true;
		state.selectedNodeType = curItem->data(0, NodeTypeRole).toInt();
		state.selectedStructure = curItem->data(0, StructureRole).toString();
		QString bmk = curItem->data(0, BmkRole).toString();
		state.selectedDeviceKey = state.selectedStructure + QStringLiteral("///") + bmk;
		state.selectedElementUuid = curItem->data(0, ElementUuidRole).toString();
	}

	std::function<void(QTreeWidgetItem *)> collectExpanded = [&](QTreeWidgetItem *item) {
		if (!item) return;
		int type = item->data(0, NodeTypeRole).toInt();
		if (type == StructureNode) {
			if (item->isExpanded()) {
				state.expandedStructures.insert(item->data(0, StructureRole).toString());
			}
		} else if (type == DeviceNode) {
			if (item->isExpanded()) {
				QString structName = item->data(0, StructureRole).toString();
				QString bmk = item->data(0, BmkRole).toString();
				state.expandedDevices.insert(structName + QStringLiteral("///") + bmk);
			}
		}

		for (int i = 0; i < item->childCount(); ++i) {
			collectExpanded(item->child(i));
		}
	};

	for (int i = 0; i < ui->m_tree_widget->topLevelItemCount(); ++i) {
		collectExpanded(ui->m_tree_widget->topLevelItem(i));
	}

	return state;
}

/**
 * @brief DeviceNavigatorDockWidget::saveTableState
 * Collects current selected element and scroll positions for table view.
 */
DeviceNavigatorDockWidget::TableState DeviceNavigatorDockWidget::saveTableState() const
{
	TableState state;
	state.vScroll = ui->m_table_widget->verticalScrollBar()->value();
	state.hScroll = ui->m_table_widget->horizontalScrollBar()->value();

	QTableWidgetItem *curItem = ui->m_table_widget->currentItem();
	if (curItem) {
		int row = curItem->row();
		QTableWidgetItem *bmkItem = ui->m_table_widget->item(row, 0);
		if (bmkItem) {
			state.hasSelection = true;
			state.selectedElementUuid = bmkItem->data(ElementUuidRole).toString();
		}
	}

	return state;
}

/**
 * @brief DeviceNavigatorDockWidget::restoreTreeState
 * Restores selection, ensures ancestor visibility, and restores scroll offset.
 */
void DeviceNavigatorDockWidget::restoreTreeState(const TreeState &state)
{
	QTreeWidgetItem *itemToSelect = nullptr;

	if (state.hasSelection) {
		std::function<bool(QTreeWidgetItem *)> findItem = [&](QTreeWidgetItem *item) -> bool {
			if (!item) return false;
			int type = item->data(0, NodeTypeRole).toInt();

			if (state.selectedNodeType == FunctionNode && type == FunctionNode) {
				if (!state.selectedElementUuid.isEmpty() &&
					item->data(0, ElementUuidRole).toString() == state.selectedElementUuid) {
					itemToSelect = item;
					return true;
				}
			} else if (state.selectedNodeType == DeviceNode && type == DeviceNode) {
				QString structName = item->data(0, StructureRole).toString();
				QString bmk = item->data(0, BmkRole).toString();
				if ((structName + QStringLiteral("///") + bmk) == state.selectedDeviceKey) {
					itemToSelect = item;
					return true;
				}
			} else if (state.selectedNodeType == StructureNode && type == StructureNode) {
				if (item->data(0, StructureRole).toString() == state.selectedStructure) {
					itemToSelect = item;
					return true;
				}
			} else if (state.selectedNodeType == ProjectNode && type == ProjectNode) {
				itemToSelect = item;
				return true;
			}

			for (int i = 0; i < item->childCount(); ++i) {
				if (findItem(item->child(i))) return true;
			}
			return false;
		};

		for (int i = 0; i < ui->m_tree_widget->topLevelItemCount(); ++i) {
			if (findItem(ui->m_tree_widget->topLevelItem(i))) break;
		}
	}

	if (itemToSelect) {
		// Ensure ancestor path is expanded so the item is visible
		QTreeWidgetItem *p = itemToSelect->parent();
		while (p) {
			p->setExpanded(true);
			p = p->parent();
		}
		ui->m_tree_widget->setCurrentItem(itemToSelect);
		ui->m_tree_widget->scrollToItem(itemToSelect);
	} else {
		ui->m_tree_widget->verticalScrollBar()->setValue(state.vScroll);
		ui->m_tree_widget->horizontalScrollBar()->setValue(state.hScroll);
	}
}

/**
 * @brief DeviceNavigatorDockWidget::restoreTableState
 */
void DeviceNavigatorDockWidget::restoreTableState(const TableState &state)
{
	if (state.hasSelection && !state.selectedElementUuid.isEmpty()) {
		for (int r = 0; r < ui->m_table_widget->rowCount(); ++r) {
			QTableWidgetItem *bmkItem = ui->m_table_widget->item(r, 0);
			if (bmkItem && bmkItem->data(ElementUuidRole).toString() == state.selectedElementUuid) {
				ui->m_table_widget->setCurrentCell(r, 0);
				ui->m_table_widget->scrollToItem(bmkItem);
				return;
			}
		}
	}

	ui->m_table_widget->verticalScrollBar()->setValue(state.vScroll);
	ui->m_table_widget->horizontalScrollBar()->setValue(state.hScroll);
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

	TreeState treeState = saveTreeState();
	TableState tableState = saveTableState();

	ui->m_tree_widget->setUpdatesEnabled(false);
	ui->m_table_widget->setUpdatesEnabled(false);
	ui->m_tree_widget->blockSignals(true);
	ui->m_table_widget->blockSignals(true);

	buildData(m_cached_items);
	populateTree(m_cached_items, treeState);
	populateTable(m_cached_items, tableState);
	applyFilters();

	ui->m_tree_widget->blockSignals(false);
	ui->m_table_widget->blockSignals(false);

	restoreTreeState(treeState);
	restoreTableState(tableState);

	ui->m_tree_widget->setUpdatesEnabled(true);
	ui->m_table_widget->setUpdatesEnabled(true);
}

/**
 * @brief DeviceNavigatorDockWidget::populateTree
 * @param items
 */
void DeviceNavigatorDockWidget::populateTree(const QList<ElementInfo> &items, const TreeState &state)
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
	QFont pFont = projectItem->font(0);
	pFont.setBold(true);
	projectItem->setFont(0, pFont);

	// Group by structure: QMap keeps sorted order
	const QString noStructureName = tr("No structure identifier");
	QMap<QString, QMap<QString, QList<ElementInfo>>> structureMap;

	for (const ElementInfo &info : items) {
		structureMap[info.structure][info.bmk].append(info);
	}

	// Order structures: named ones first, "No structure identifier" last
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
		if (state.expandedStructures.contains(structName)) {
			structItem->setExpanded(true);
		}

		const QMap<QString, QList<ElementInfo>> &devices = structureMap[structName];
		QStringList bmkKeys = devices.keys();
		bmkKeys.sort();

		for (const QString &bmk : bmkKeys) {
			QList<ElementInfo> deviceElements = devices[bmk];
			if (deviceElements.isEmpty()) continue;

			// Sort device functions: Master first, then Slaves, then others; then by integer folioIndex / pos
			std::stable_sort(deviceElements.begin(), deviceElements.end(), [](const ElementInfo &a, const ElementInfo &b) {
				int rankA = a.isMaster ? 0 : (a.isSlave ? 1 : 2);
				int rankB = b.isMaster ? 0 : (b.isSlave ? 1 : 2);
				if (rankA != rankB) return rankA < rankB;
				if (a.folioIndex != b.folioIndex) return a.folioIndex < b.folioIndex;
				return a.posStr < b.posStr;
			});

			auto *deviceItem = new QTreeWidgetItem(structItem);
			deviceItem->setText(0, bmk);
			deviceItem->setData(0, NodeTypeRole, DeviceNode);
			deviceItem->setData(0, BmkRole, bmk);
			deviceItem->setData(0, StructureRole, structName);

			// Choose icon for device based on actual element types, not prefix
			bool hasCable = false;
			bool hasMaster = false;
			bool hasTerminal = false;
			for (const ElementInfo &info : deviceElements) {
				if (info.isCable) hasCable = true;
				if (info.isMaster) hasMaster = true;
				if (info.isTerminal) hasTerminal = true;
			}
			if (hasCable) {
				deviceItem->setIcon(0, QET::Icons::Cable);
			} else if (hasTerminal) {
				deviceItem->setIcon(0, QET::Icons::TerminalStrip);
			} else if (hasMaster) {
				deviceItem->setIcon(0, QET::Icons::ElementMaster);
			} else {
				deviceItem->setIcon(0, QET::Icons::GenericDevice);
			}

			// Store master / first element UUID as jump target for double clicking the device node
			if (!deviceElements.isEmpty()) {
				const ElementInfo &firstInfo = deviceElements.first();
				deviceItem->setData(0, ElementUuidRole, firstInfo.elementUuid.toString());
				deviceItem->setData(0, DiagramUuidRole, firstInfo.diagramUuid.toString());
			}

			QString devKey = structName + QStringLiteral("///") + bmk;
			if (state.expandedDevices.contains(devKey)) {
				deviceItem->setExpanded(true);
			}

			// Add leaf function items
			for (const ElementInfo &info : deviceElements) {
				auto *funcItem = new QTreeWidgetItem(deviceItem);
				funcItem->setText(0, info.fullDisplay);
				funcItem->setData(0, NodeTypeRole, FunctionNode);
				funcItem->setData(0, BmkRole, info.bmk);
				funcItem->setData(0, StructureRole, info.structure);
				funcItem->setData(0, HasStructureRole, info.hasStructure);
				funcItem->setData(0, ElementUuidRole, info.elementUuid.toString());
				funcItem->setData(0, DiagramUuidRole, info.diagramUuid.toString());
				funcItem->setData(0, IsMasterRole, info.isMaster);
				funcItem->setData(0, IsSlaveRole, info.isSlave);
				funcItem->setData(0, IsTerminalRole, info.isTerminal);
				funcItem->setData(0, IsCableRole, info.isCable);
				funcItem->setData(0, IsReportRole, info.isReport);
				funcItem->setData(0, IsConductorDefRole, info.isConductorDef);
				funcItem->setData(0, IsThumbnailRole, info.isThumbnail);

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
void DeviceNavigatorDockWidget::populateTable(const QList<ElementInfo> &items, const TableState &state)
{
	Q_UNUSED(state);
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
		itemBmk->setData(HasStructureRole, info.hasStructure);
		itemBmk->setData(IsMasterRole, info.isMaster);
		itemBmk->setData(IsSlaveRole, info.isSlave);
		itemBmk->setData(IsTerminalRole, info.isTerminal);
		itemBmk->setData(IsCableRole, info.isCable);
		itemBmk->setData(IsReportRole, info.isReport);
		itemBmk->setData(IsConductorDefRole, info.isConductorDef);
		itemBmk->setData(IsThumbnailRole, info.isThumbnail);

		auto *itemFunc = new QTableWidgetItem(info.functionDesc);
		auto *itemTerm = new QTableWidgetItem(info.terminals);
		auto *itemStruct = new QTableWidgetItem(info.structure);

		auto *itemFolio = new FolioTableWidgetItem(info.folioStr, info.folioIndex);

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

	DeviceNavigatorFilterScheme activeScheme;
	if (filterIdx >= 0 && filterIdx < m_filter_schemes.size()) {
		activeScheme = m_filter_schemes.at(filterIdx);
	}

	// Helper lambda for element filter matching
	auto matchesFilter = [&](const QString &bmk,
							 const QString &fullText,
							 const QString &structure,
							 bool hasStructure,
							 bool isMaster,
							 bool isSlave,
							 bool isTerminal,
							 bool isCable,
							 bool isReport,
							 bool isConductorDef,
							 bool isThumbnail) -> bool {
		if (!activeScheme.matches(bmk, structure, hasStructure, isMaster, isSlave, isTerminal, isCable, isReport, isConductorDef, isThumbnail)) {
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
			bool hasStructure = item->data(0, HasStructureRole).toBool();
			bool isMaster = item->data(0, IsMasterRole).toBool();
			bool isSlave = item->data(0, IsSlaveRole).toBool();
			bool isTerminal = item->data(0, IsTerminalRole).toBool();
			bool isCable = item->data(0, IsCableRole).toBool();
			bool isReport = item->data(0, IsReportRole).toBool();
			bool isConductorDef = item->data(0, IsConductorDefRole).toBool();
			bool isThumbnail = item->data(0, IsThumbnailRole).toBool();

			bool visible = matchesFilter(bmk, full, structure, hasStructure, isMaster, isSlave, isTerminal, isCable, isReport, isConductorDef, isThumbnail);
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
		bool hasStructure = bmkItem->data(HasStructureRole).toBool();
		bool isMaster = bmkItem->data(IsMasterRole).toBool();
		bool isSlave = bmkItem->data(IsSlaveRole).toBool();
		bool isTerminal = bmkItem->data(IsTerminalRole).toBool();
		bool isCable = bmkItem->data(IsCableRole).toBool();
		bool isReport = bmkItem->data(IsReportRole).toBool();
		bool isConductorDef = bmkItem->data(IsConductorDefRole).toBool();
		bool isThumbnail = bmkItem->data(IsThumbnailRole).toBool();

		bool visible = matchesFilter(bmk, full, structStr, hasStructure, isMaster, isSlave, isTerminal, isCable, isReport, isConductorDef, isThumbnail);
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
	if (!diag && m_project->dataBase()) {
		QSqlQuery q = m_project->dataBase()->newQuery(
			QStringLiteral("SELECT diagram_uuid FROM element WHERE uuid = '%1'").arg(elementUuid.toString())
		);
		if (q.next()) {
			diag = m_project->diagramByUuid(QUuid(q.value(0).toString()));
		}
	}

	if (!diag) {
		scheduleReload();
		return;
	}

	m_project->buildFolioToShow(diag);

	Element *targetElement = nullptr;
	for (Element *e : diag->elements()) {
		if (e && e->uuid() == elementUuid) {
			targetElement = e;
			break;
		}
	}

	if (targetElement) {
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
		return;
	}

	// Check if the UUID belongs to a CablePart
	for (QGraphicsItem *item : diag->items()) {
		if (auto *part = qgraphicsitem_cast<CablePart *>(item)) {
			if (part->partUuid() == elementUuid) {
				diag->showMe();
				diag->clearSelection();
				part->setSelected(true);
				if (!diag->views().isEmpty()) {
					if (auto *view = qobject_cast<DiagramView *>(diag->views().first())) {
						const QPointF centerPos = part->boundingRect().center();
						QMetaObject::invokeMethod(view, [view, centerPos]() {
							view->centerOn(centerPos);
						}, Qt::QueuedConnection);
					}
				}
				return;
			}
		}
	}

	scheduleReload();
}

/**
 * @brief DeviceNavigatorDockWidget::editElementProperties
 * Opens property editor for an element or cable part.
 * @param elementUuid
 * @param diagramUuid
 */
void DeviceNavigatorDockWidget::editElementProperties(const QUuid &elementUuid, const QUuid &diagramUuid)
{
	if (!m_project || elementUuid.isNull()) return;
	Diagram *diag = m_project->diagramByUuid(diagramUuid);
	if (!diag) return;

	m_project->buildFolioToShow(diag);
	for (Element *e : diag->elements()) {
		if (e && e->uuid() == elementUuid) {
			e->editProperty();
			return;
		}
	}

	for (QGraphicsItem *item : diag->items()) {
		if (auto *part = qgraphicsitem_cast<CablePart *>(item)) {
			if (part->partUuid() == elementUuid) {
				part->editProperty();
				return;
			}
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
 * @brief DeviceNavigatorDockWidget::onViewModeTabChanged
 * @param index
 */
void DeviceNavigatorDockWidget::onViewModeTabChanged(int index)
{
	ui->m_view_stack->setCurrentIndex(index);
}

/**
 * @brief DeviceNavigatorDockWidget::addContextMenuActions
 * Populates context menu actions for a device or function node.
 * @param menu
 * @param elementUuid
 * @param diagramUuid
 */
void DeviceNavigatorDockWidget::addContextMenuActions(QMenu &menu, const QUuid &elementUuid, const QUuid &diagramUuid)
{
	if (!elementUuid.isNull()) {
		menu.addAction(tr("Show in diagram"), this, [this, elementUuid, diagramUuid]() {
			jumpToUuid(elementUuid, diagramUuid);
		});
		menu.addAction(tr("Edit properties..."), this, [this, elementUuid, diagramUuid]() {
			editElementProperties(elementUuid, diagramUuid);
		});
		menu.addSeparator();
	}
	menu.addAction(tr("Refresh"), this, &DeviceNavigatorDockWidget::reload);
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
	addContextMenuActions(menu, elmtUuid, diagUuid);
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
	addContextMenuActions(menu, elmtUuid, diagUuid);
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
		const QString dispName = m_filter_schemes.at(i).displayName();
		ui->m_filter_combo->addItem(dispName);
		if (dispName == currentName) {
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
		reload();
	}
}
