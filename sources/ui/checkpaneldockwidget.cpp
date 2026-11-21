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
#include "checkpaneldockwidget.h"

#include "../diagram.h"
#include "../diagramcontent.h"
#include "../diagramview.h"
#include "../duplicatelabels.h"
#include "../qetdiagrameditor.h"
#include "../qetgraphicsitem/conductor.h"
#include "../qetgraphicsitem/element.h"
#include "../qetproject.h"
#include "../dataBase/projectdatabase.h"
#include "../utils/qetsettings.h"

#include <QApplication>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QMenu>
#include <QProgressDialog>
#include <QPushButton>
#include <QScopeGuard>
#include <QSet>
#include <QSqlQuery>
#include <QStyle>
#include <QToolButton>
#include <QTreeWidget>
#include <QVBoxLayout>

CheckPanelDockWidget::CheckPanelDockWidget(QWidget *parent) :
	QDockWidget(tr("Check", "dock title"), parent)
{
	setObjectName(QStringLiteral("check_panel_dock"));
	auto *body = new QWidget(this);
	auto *layout = new QVBoxLayout(body);
	layout->setContentsMargins(4, 4, 4, 4);

	auto *top = new QHBoxLayout();
	m_run = new QPushButton(tr("Run checks"), body);
	m_run->setObjectName(QStringLiteral("checkPanelRun"));
	m_run->setToolTip(tr("Run every check on the current project"));
	top->addWidget(m_run);

	auto *options = new QToolButton(body);
	options->setObjectName(QStringLiteral("checkPanelOptions"));
	options->setText(tr("Options"));
	options->setPopupMode(QToolButton::InstantPopup);
	m_options = new QMenu(options);
	m_options->addSection(tr("Symbols with no wire"));
	const auto option = [this](const QString &name, const QString &text, bool checked,
							   void (*save)(bool)) {
		QAction *action = m_options->addAction(text);
		action->setObjectName(name);
		action->setCheckable(true);
		action->setChecked(checked);
		connect(action, &QAction::toggled, this, [this, save](bool on) {
			save(on);
			if (m_finding_count || m_tree->topLevelItemCount())
				m_summary->setText(tr("The options changed. Run the checks again."));
		});
	};
	option(QStringLiteral("unwiredSkipWirelessFolios"), tr("Leave out folios with no wire"),
		   QetSettings::unwiredSymbolsSkipWirelessFolios(),
		   &QetSettings::setUnwiredSymbolsSkipWirelessFolios);
	option(QStringLiteral("unwiredSkipStripTerminals"), tr("Leave out strip terminals"),
		   QetSettings::unwiredSymbolsSkipStripTerminals(),
		   &QetSettings::setUnwiredSymbolsSkipStripTerminals);
	option(QStringLiteral("unwiredSkipContacts"), tr("Leave out auxiliary contacts"),
		   QetSettings::unwiredSymbolsSkipContacts(),
		   &QetSettings::setUnwiredSymbolsSkipContacts);
	options->setMenu(m_options);
	top->addWidget(options);
	m_summary = new QLabel(body);
	m_summary->setWordWrap(true);
	top->addWidget(m_summary, 1);
	layout->addLayout(top);

	m_tree = new QTreeWidget(body);
	m_tree->setObjectName(QStringLiteral("checkPanelTree"));
	m_tree->setColumnCount(3);
	m_tree->setHeaderLabels({tr("Finding"), tr("Folio"), tr("Cell")});
	m_tree->setRootIsDecorated(true);
	m_tree->setUniformRowHeights(true);
	m_tree->header()->setStretchLastSection(false);
	m_tree->header()->setSectionResizeMode(0, QHeaderView::Stretch);
	m_tree->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
	m_tree->header()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
	m_tree->setToolTip(tr("Double-click a finding to show it on its folio."));
	layout->addWidget(m_tree);
	setWidget(body);

	connect(m_run, &QPushButton::clicked, this, &CheckPanelDockWidget::run);
	connect(m_tree, &QTreeWidget::itemActivated, this,
			[this](QTreeWidgetItem *row, int) { showRow(row); });
	clear(tr("No project is open."));
}

void CheckPanelDockWidget::setProject(QETProject *project)
{
	if (m_building) {
		m_pending = project;
		m_has_pending = true;
		return;
	}
	if (project == m_project) return;
	disconnect(m_destroyed);
	m_project = project;
	if (project)
		m_destroyed = connect(project, &QObject::destroyed, this,
							  [this]() { clear(tr("No project is open.")); });
	clear(project ? tr("Press Run checks to check the project.")
				  : tr("No project is open."));
}

void CheckPanelDockWidget::clear(const QString &summary)
{
	m_places.clear();
	m_tree->clear();
	m_finding_count = 0;
	m_summary->setText(summary);
	m_run->setEnabled(m_project);
}

/**
	@brief CheckPanelDockWidget::buildFolios
	Two checks read the built folios: those not built yet (lazy folios) are
	built first, with their progress shown.
*/
void CheckPanelDockWidget::buildFolios()
{
	if (!m_project || !m_project->unloadedFolioCount()) return;
	QProgressDialog progress(tr("Reading the folios..."), QString(), 0,
							 m_project->unloadedFolioCount(), window());
	progress.setWindowTitle(windowTitle());
	progress.setWindowModality(Qt::ApplicationModal);
	progress.setCancelButton(nullptr);
	progress.setMinimumDuration(0);
	QApplication::setOverrideCursor(Qt::WaitCursor);
	const auto restore = qScopeGuard([this]() {
		if (m_project) m_project->setFolioBuildObserver({});
		QApplication::restoreOverrideCursor();
	});
	m_project->setFolioBuildObserver([&progress](int built, int total) {
		progress.setMaximum(total);
		progress.setLabelText(tr("Reading folio %1 of %2...").arg(built).arg(total));
		progress.setValue(built);
		QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
	});
	m_project->loadFolios();
}

void CheckPanelDockWidget::run()
{
	clear(QString());
	if (!m_project) {
		m_summary->setText(tr("No project is open."));
		return;
	}
	if (m_building) return;
	m_building = true;
	buildFolios();
	m_building = false;
		// The current project changed while its folios were built: the
		// results would be the old one's
	if (m_has_pending) {
		m_has_pending = false;
		if (m_pending != m_project) {
			setProject(m_pending);
			return;
		}
	}
	if (!m_project) {
		clear(tr("No project is open."));
		return;
	}
	QApplication::setOverrideCursor(Qt::WaitCursor);
	const auto restore = qScopeGuard([]() { QApplication::restoreOverrideCursor(); });

	const auto setPlace = [this](QTreeWidgetItem *row, const ProjectCheck::Place &place) {
		row->setText(1, place.folio);
		row->setText(2, place.cell);
		if (place.canShow()) m_places.insert(row, place);
	};

	for (ProjectCheck::Kind kind : ProjectCheck::kinds()) {
		const auto result = ProjectCheck::run(m_project, kind);
		auto *root = new QTreeWidgetItem(m_tree);
		root->setToolTip(0, ProjectCheck::toolTip(kind));
		const int found = int(result.findings.size());
		m_finding_count += found;
		if (!result.note.isEmpty()) {
			//: %1 is the check, %2 why it found nothing to check
			root->setText(0, tr("%1: %2").arg(ProjectCheck::title(kind), result.note));
			root->setIcon(0, style()->standardIcon(QStyle::SP_MessageBoxInformation));
			root->setForeground(0, palette().brush(QPalette::Disabled, QPalette::Text));
			continue;
		}
		if (!found) {
			//: %1 is the check
			root->setText(0, tr("%1: nothing found").arg(ProjectCheck::title(kind)));
			root->setIcon(0, style()->standardIcon(QStyle::SP_DialogApplyButton));
			continue;
		}
		//: %1 is the check, %2 how many findings it has
		root->setText(0, tr("%1 (%2)").arg(ProjectCheck::title(kind)).arg(found));
		root->setIcon(0, style()->standardIcon(QStyle::SP_MessageBoxWarning));
		for (const auto &finding : result.findings) {
			auto *row = new QTreeWidgetItem(root);
			row->setText(0, finding.text);
			row->setToolTip(0, finding.text);
				// One place: on the finding's own row. Several: a row each,
				// and the finding shows the first that can be shown.
			if (finding.places.size() == 1) {
				setPlace(row, finding.places.first());
				continue;
			}
			for (const auto &place : finding.places) {
				auto *child = new QTreeWidgetItem(row);
				child->setText(0, place.text);
				setPlace(child, place);
				if (!m_places.contains(row) && place.canShow())
					m_places.insert(row, place);
			}
			if (m_places.contains(row)) {
				row->setText(1, m_places.value(row).folio);
				row->setText(2, m_places.value(row).cell);
			}
		}
		root->setExpanded(true);
	}
	m_summary->setText(m_finding_count
		? tr("%n finding(s). Run the checks again after a change.", nullptr, m_finding_count)
		: tr("Nothing found."));
}

/**
	@brief CheckPanelDockWidget::showRow
	Show the place of @p row on its folio: select it and zoom on it. A wire
	or a symbol read from the database is looked up by uuid now, building
	its folio when it is not built yet.
	@return false when the row has no place or the place no longer exists
*/
bool CheckPanelDockWidget::showRow(QTreeWidgetItem *row)
{
	if (!m_project || !m_places.contains(row)) return false;
	const ProjectCheck::Place place = m_places.value(row);

	QList<QGraphicsItem *> targets;
	Diagram *diagram = nullptr;
	if (place.item) {
		diagram = DuplicateLabels::diagramOf(place.item);
		if (diagram && m_project->folioIndex(diagram) >= 0)
			targets << place.item.data();
	} else if (place.canShow()) {
			// The place's folio, else the folio of its first wire, from
			// the database: only that folio is built and looked at
		if (!place.diagram.isNull())
			diagram = m_project->diagramByUuid(place.diagram);
		if (!diagram && !place.wires.isEmpty()) {
			QSqlQuery query = m_project->dataBase()->newQuery();
			query.prepare(QStringLiteral("SELECT diagram_uuid FROM conductor WHERE uuid = :uuid"));
			for (const QUuid &wire : place.wires) {
				query.bindValue(QStringLiteral(":uuid"), wire.toString());
				if (query.exec() && query.next()) {
					diagram = m_project->diagramByUuid(QUuid(query.value(0).toString()));
					if (diagram) break;
				}
			}
		}
		if (diagram) {
			m_project->buildFolioToShow(diagram);
			if (!place.element.isNull())
				for (Element *element : diagram->elements())
					if (element && element->uuid() == place.element) {
						targets << element;
						break;
					}
			const QSet<QUuid> uuids(place.wires.begin(), place.wires.end());
			if (!uuids.isEmpty())
				for (Conductor *conductor : diagram->content().conductors())
					if (uuids.contains(conductor->uuid()))
						targets << conductor;
		}
	}
	if (!diagram || targets.isEmpty()) {
		m_summary->setText(tr("This is no longer in the project. Run the checks again."));
		return false;
	}

	if (auto *editor = qobject_cast<QETDiagramEditor *>(window()))
		editor->activateProject(m_project.data());
	diagram->showMe();
	diagram->clearSelection();
	QRectF bounds;
	for (QGraphicsItem *item : targets) {
		item->setSelected(true);
		bounds |= item->sceneBoundingRect();
	}
	bounds.adjust(-100, -100, 100, 100);
	for (QGraphicsView *view : diagram->views()) {
		if (auto *diagram_view = qobject_cast<DiagramView *>(view))
			diagram_view->zoomToRect(bounds);
		else {
			view->fitInView(bounds, Qt::KeepAspectRatio);
			view->centerOn(bounds.center());
		}
	}
	return true;
}
