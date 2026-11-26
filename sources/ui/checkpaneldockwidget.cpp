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
#include "../qetgraphicsitem/independenttextitem.h"
#include "../qetproject.h"
#include "../dataBase/projectdatabase.h"
#include "../projectcheck/checksettings.h"

#include <QApplication>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QInputDialog>
#include <QLabel>
#include <QMenu>
#include <QProgressDialog>
#include <QPushButton>
#include <QScopeGuard>
#include <QSet>
#include <QSignalBlocker>
#include <QSqlQuery>
#include <QStyle>
#include <QToolButton>
#include <QTreeWidget>
#include <QVBoxLayout>

#include <functional>

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
		// The project's own options when it has them, else the
		// application's (CheckSettings): read again each time the menu opens
	const auto option = [this](const QString &name, const QString &text,
							   CheckSettings::Option which) {
		QAction *action = m_options->addAction(text);
		action->setObjectName(name);
		action->setCheckable(true);
		action->setChecked(CheckSettings::value(m_project, which));
		connect(m_options, &QMenu::aboutToShow, action, [this, action, which]() {
			const QSignalBlocker blocker(action);
			action->setChecked(CheckSettings::value(m_project, which));
		});
		connect(action, &QAction::toggled, this, [this, which](bool on) {
			CheckSettings::setValue(m_project, which, on);
			if (m_finding_count || m_tree->topLevelItemCount())
				m_summary->setText(tr("The options changed. Run the checks again."));
		});
	};
	option(QStringLiteral("unwiredSkipWirelessFolios"), tr("Leave out folios with no wire"),
		   CheckSettings::Option::UnwiredSkipWirelessFolios);
	option(QStringLiteral("unwiredSkipStripTerminals"), tr("Leave out strip terminals"),
		   CheckSettings::Option::UnwiredSkipStripTerminals);
	option(QStringLiteral("unwiredSkipContacts"), tr("Leave out auxiliary contacts"),
		   CheckSettings::Option::UnwiredSkipContacts);
	m_options->addSection(tr("Accepted findings"));
	m_show_accepted = m_options->addAction(tr("Show accepted findings"));
	m_show_accepted->setObjectName(QStringLiteral("checkPanelShowAccepted"));
	m_show_accepted->setCheckable(true);
	connect(m_show_accepted, &QAction::toggled, this, [this]() {
		for (int i = 0 ; i < m_tree->topLevelItemCount() ; ++i)
			updateCheck(m_tree->topLevelItem(i));
	});
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
	m_tree->setToolTip(tr("Double-click a finding to show it on its folio. Right-click "
						  "to accept it, or to switch its check off for this project."));
	m_tree->setContextMenuPolicy(Qt::CustomContextMenu);
	connect(m_tree, &QTreeWidget::customContextMenuRequested,
			this, &CheckPanelDockWidget::showMenu);
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
	for (ProjectCheck::Kind kind : ProjectCheck::kinds())
		fillCheck(new QTreeWidgetItem(m_tree), kind);
	updateSummary();
}

namespace {
		/// On a check's row: its ProjectCheck::Kind; its note, if any
	constexpr int kind_role = Qt::UserRole;
	constexpr int note_role = Qt::UserRole + 1;
		/// On a finding's row: CheckSettings::findingId(), its text, and
		/// whether it is accepted and with what note
	constexpr int id_role = Qt::UserRole;
	constexpr int text_role = Qt::UserRole + 1;
	constexpr int accepted_role = Qt::UserRole + 2;
	constexpr int accepted_note_role = Qt::UserRole + 3;
}

/**
	@brief CheckPanelDockWidget::fillCheck
	Run the check @p kind and list its findings under @p root, unless the
	project switched it off. The findings the project accepted are hidden
	unless Show accepted findings is on.
*/
void CheckPanelDockWidget::fillCheck(QTreeWidgetItem *root, ProjectCheck::Kind kind)
{
	removeFindings(root);
	root->setData(0, kind_role, int(kind));
	root->setData(0, note_role, QVariant());
	root->setToolTip(0, ProjectCheck::toolTip(kind));
	if (!m_project || !CheckSettings::ruleEnabled(m_project, kind)) {
		updateCheck(root);
		return;
	}

	const auto setPlace = [this](QTreeWidgetItem *row, const ProjectCheck::Place &place) {
		row->setText(1, place.folio);
		row->setText(2, place.cell);
		if (place.canShow()) m_places.insert(row, place);
	};

	const auto result = ProjectCheck::run(m_project, kind);
	if (!result.note.isEmpty()) root->setData(0, note_role, result.note);
	const auto accepted = CheckSettings::acceptedFindings(m_project);
	for (const auto &finding : result.findings) {
		auto *row = new QTreeWidgetItem(root);
		const QString id = CheckSettings::findingId(kind, finding);
		row->setData(0, id_role, id);
		row->setData(0, text_role, finding.text);
		row->setData(0, accepted_role, accepted.contains(id));
		row->setData(0, accepted_note_role, accepted.value(id));
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
	updateCheck(root);
	root->setExpanded(true);
}

/**
	@brief CheckPanelDockWidget::updateCheck
	The text of a check's row and of its findings' rows, from what they
	hold: switched off, a note, or how many findings are open and accepted.
*/
void CheckPanelDockWidget::updateCheck(QTreeWidgetItem *root)
{
	const auto kind = ProjectCheck::Kind(root->data(0, kind_role).toInt());
	const QString title = ProjectCheck::title(kind);
	root->setData(0, Qt::ForegroundRole, QVariant());
	if (!m_project || !CheckSettings::ruleEnabled(m_project, kind)) {
		//: %1 is the check
		root->setText(0, tr("%1: switched off for this project").arg(title));
		root->setIcon(0, style()->standardIcon(QStyle::SP_MessageBoxInformation));
		root->setForeground(0, palette().brush(QPalette::Disabled, QPalette::Text));
		return;
	}
	const QString note = root->data(0, note_role).toString();
	if (!note.isEmpty() && !root->childCount()) {
		//: %1 is the check, %2 why it found nothing to check
		root->setText(0, tr("%1: %2").arg(title, note));
		root->setIcon(0, style()->standardIcon(QStyle::SP_MessageBoxInformation));
		root->setForeground(0, palette().brush(QPalette::Disabled, QPalette::Text));
		return;
	}
	int open = 0, accepted = 0;
	const bool show_accepted = m_show_accepted->isChecked();
	for (int i = 0 ; i < root->childCount() ; ++i) {
		QTreeWidgetItem *row = root->child(i);
		const QString text = row->data(0, text_role).toString();
		if (row->data(0, accepted_role).toBool()) {
			++accepted;
			const QString why = row->data(0, accepted_note_role).toString();
			row->setText(0, why.isEmpty()
				//: %1 is a finding the project accepted
				? tr("%1 (accepted)").arg(text)
				//: %1 is a finding the project accepted, %2 the note given then
				: tr("%1 (accepted: %2)").arg(text, why));
			row->setForeground(0, palette().brush(QPalette::Disabled, QPalette::Text));
			row->setHidden(!show_accepted);
		} else {
			++open;
			row->setText(0, text);
			row->setData(0, Qt::ForegroundRole, QVariant());
			row->setHidden(false);
		}
		row->setToolTip(0, row->text(0));
	}
	if (!open) {
		root->setText(0, accepted
			//: %1 is the check; %n counts the findings the project accepted
			? tr("%1: nothing found, %n accepted", nullptr, accepted).arg(title)
			//: %1 is the check
			: tr("%1: nothing found").arg(title));
		root->setIcon(0, style()->standardIcon(QStyle::SP_DialogApplyButton));
		return;
	}
	root->setText(0, accepted
		//: %1 is the check, %2 how many findings it has; %n counts those accepted
		? tr("%1 (%2, %n accepted)", nullptr, accepted).arg(title).arg(open)
		//: %1 is the check, %2 how many findings it has
		: tr("%1 (%2)").arg(title).arg(open));
	root->setIcon(0, style()->standardIcon(QStyle::SP_MessageBoxWarning));
}

/// The findings of the last run which are not accepted, in the summary
void CheckPanelDockWidget::updateSummary()
{
	m_finding_count = 0;
	for (int i = 0 ; i < m_tree->topLevelItemCount() ; ++i) {
		QTreeWidgetItem *root = m_tree->topLevelItem(i);
		for (int j = 0 ; j < root->childCount() ; ++j)
			if (!root->child(j)->data(0, accepted_role).toBool()) ++m_finding_count;
	}
	m_summary->setText(m_finding_count
		? tr("%n finding(s). Run the checks again after a change.", nullptr, m_finding_count)
		: tr("Nothing found."));
}

/// Take away the findings listed under @p root
void CheckPanelDockWidget::removeFindings(QTreeWidgetItem *root)
{
	const std::function<void(QTreeWidgetItem *)> forget = [&](QTreeWidgetItem *row) {
		m_places.remove(row);
		for (int i = 0 ; i < row->childCount() ; ++i) forget(row->child(i));
	};
	for (int i = 0 ; i < root->childCount() ; ++i) forget(root->child(i));
	qDeleteAll(root->takeChildren());
}

QTreeWidgetItem *CheckPanelDockWidget::checkRow(ProjectCheck::Kind kind) const
{
	for (int i = 0 ; i < m_tree->topLevelItemCount() ; ++i) {
		QTreeWidgetItem *root = m_tree->topLevelItem(i);
		if (root->data(0, kind_role).toInt() == int(kind)) return root;
	}
	return nullptr;
}

/// The finding's row @p row is, or is a place of; null for a check's row
QTreeWidgetItem *CheckPanelDockWidget::findingRow(QTreeWidgetItem *row) const
{
	if (!row || !row->parent()) return nullptr;
	while (row->parent()->parent()) row = row->parent();
	return row;
}

/**
	@brief CheckPanelDockWidget::setAccepted
	Accept the finding of @p row with @p note in the project, or (@p
	accept false) list it again. An accepted finding stays hidden after
	the project is saved and opened again.
	@return false when @p row is no finding or the project cannot change
*/
bool CheckPanelDockWidget::setAccepted(QTreeWidgetItem *row, bool accept, const QString &note)
{
	row = findingRow(row);
	if (!row || !m_project || m_project->isReadOnly()) return false;
	const QString id = row->data(0, id_role).toString();
	if (accept)
		CheckSettings::acceptFinding(m_project, id, note);
	else
		CheckSettings::unacceptFinding(m_project, id);
		// Every row of that finding: one finding may be listed twice
		// (two places of one id)
	const auto accepted = CheckSettings::acceptedFindings(m_project);
	QTreeWidgetItem *root = row->parent();
	for (int i = 0 ; i < root->childCount() ; ++i) {
		QTreeWidgetItem *other = root->child(i);
		if (other->data(0, id_role).toString() != id) continue;
		other->setData(0, accepted_role, accepted.contains(id));
		other->setData(0, accepted_note_role, accepted.value(id));
	}
	updateCheck(root);
	updateSummary();
	return true;
}

/**
	@brief CheckPanelDockWidget::setCheckEnabled
	Switch the check @p kind on or off in the project, and run it again or
	take its findings away.
*/
bool CheckPanelDockWidget::setCheckEnabled(ProjectCheck::Kind kind, bool enabled)
{
	if (!m_project || m_project->isReadOnly() || m_building) return false;
	CheckSettings::setRuleEnabled(m_project, kind, enabled);
	if (!checkRow(kind)) return true;
	if (enabled) {
		m_building = true;
		buildFolios();
		m_building = false;
		if (m_has_pending) {
			m_has_pending = false;
			if (m_pending != m_project) {
				setProject(m_pending);
				return true;
			}
		}
		if (!m_project) return true;
	}
		// Found again: building the folios processes events
	QTreeWidgetItem *root = checkRow(kind);
	if (!root) return true;
	QApplication::setOverrideCursor(Qt::WaitCursor);
	const auto restore = qScopeGuard([]() { QApplication::restoreOverrideCursor(); });
	fillCheck(root, kind);
	updateSummary();
	return true;
}

void CheckPanelDockWidget::showMenu(const QPoint &pos)
{
	QTreeWidgetItem *item = m_tree->itemAt(pos);
	if (!item || !m_project) return;
	QTreeWidgetItem *row = findingRow(item);
	QTreeWidgetItem *root = row ? row->parent() : item;
	const auto kind = ProjectCheck::Kind(root->data(0, kind_role).toInt());
	const bool read_only = m_project->isReadOnly();
	QMenu menu(this);
	if (row) {
			// The rows may be listed again while the note is asked: the
			// finding is found again by its id
		const QString id = row->data(0, id_role).toString();
		const auto findAgain = [this, kind, id]() -> QTreeWidgetItem * {
			QTreeWidgetItem *check = checkRow(kind);
			for (int i = 0 ; check && i < check->childCount() ; ++i)
				if (check->child(i)->data(0, id_role).toString() == id) return check->child(i);
			return nullptr;
		};
		if (row->data(0, accepted_role).toBool()) {
			QAction *action = menu.addAction(tr("List this finding again"));
			action->setEnabled(!read_only);
			connect(action, &QAction::triggered, this, [this, findAgain]() {
				setAccepted(findAgain(), false);
			});
		} else {
			QAction *action = menu.addAction(tr("Accept this finding..."));
			action->setToolTip(tr("The finding is not listed any more in this project, "
								  "until Options > Show accepted findings."));
			action->setEnabled(!read_only);
			connect(action, &QAction::triggered, this, [this, findAgain]() {
				bool ok = false;
				const QString note = QInputDialog::getText(
					this, tr("Accept this finding"),
					tr("Why it is right as it is (kept in the project):"),
					QLineEdit::Normal, QString(), &ok);
				if (ok) setAccepted(findAgain(), true, note.trimmed());
			});
		}
		menu.addSeparator();
	}
	const bool enabled = CheckSettings::ruleEnabled(m_project, kind);
	QAction *toggle = menu.addAction(enabled ? tr("Switch this check off for this project")
											 : tr("Switch this check on for this project"));
	toggle->setEnabled(!read_only);
	connect(toggle, &QAction::triggered, this, [this, kind, enabled]() {
		setCheckEnabled(kind, !enabled);
	});
	menu.exec(m_tree->viewport()->mapToGlobal(pos));
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
			if (!place.drawing.isNull())
				for (QGraphicsItem *item : diagram->items())
					if (auto *text = dynamic_cast<IndependentTextItem *>(item))
						if (text->uuid() == place.drawing) {
							targets << text;
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
