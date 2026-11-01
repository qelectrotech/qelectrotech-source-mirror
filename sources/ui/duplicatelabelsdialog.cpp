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
// SPDX-License-Identifier: GPL-2.0-or-later
#include "duplicatelabelsdialog.h"

#include "../diagram.h"
#include "../diagramview.h"
#include "../qet.h"
#include "../qetdiagrameditor.h"
#include "../qetproject.h"
#include "../utils/qetsettings.h"

#include <QApplication>
#include <QCheckBox>
#include <QDialogButtonBox>
#include <QHeaderView>
#include <QLabel>
#include <QProgressDialog>
#include <QPushButton>
#include <QScopeGuard>
#include <QStyle>
#include <QTreeWidget>
#include <QVBoxLayout>

DuplicateLabelsDialog::DuplicateLabelsDialog(
	QETProject* project, QWidget* parent) :
	QDialog(parent),
	m_project(project),
	m_tree(new QTreeWidget(this)),
	m_case_sensitive(new QCheckBox(this)),
	m_summary(new QLabel(this))
{
	setWindowTitle(tr("Duplicate signals and components"));
	resize(820, 520);
	auto* layout = new QVBoxLayout(this);
	m_summary->setWordWrap(true);
	m_summary->setObjectName(QStringLiteral("duplicateSummary"));
	layout->addWidget(m_summary);
	auto* help = new QLabel(
		tr("Click an occurrence to select and show it on its sheet. "
		   "Refresh after editing the project."),
		this);
	help->setWordWrap(true);
	layout->addWidget(help);
	m_case_sensitive->setObjectName(QStringLiteral("duplicateCaseSensitive"));
	m_case_sensitive->setText(tr("Case-sensitive comparison"));
	m_case_sensitive->setToolTip(
		tr("When unchecked, labels differing only in letter case, such as K1 "
		   "and k1, are duplicates."));
	m_case_sensitive->setChecked(QetSettings::duplicateLabelsCaseSensitive());
	connect(
		m_case_sensitive,
		&QCheckBox::toggled,
		this,
		[this](bool checked)
		{
			QetSettings::setDuplicateLabelsCaseSensitive(checked);
			refresh();
		});
	layout->addWidget(m_case_sensitive);
	m_tree->setObjectName(QStringLiteral("duplicateOccurrences"));
	m_tree->setHeaderLabels(
		{tr("Error / occurrence"), tr("Sheet"), tr("Position")});
	m_tree->setRootIsDecorated(true);
	m_tree->setAlternatingRowColors(true);
	m_tree->header()->setSectionResizeMode(0, QHeaderView::Stretch);
	for (int column = 1; column < 3; ++column)
		m_tree->header()->setSectionResizeMode(
			column,
			QHeaderView::ResizeToContents);
	layout->addWidget(m_tree);
	connect(
		m_tree,
		&QTreeWidget::itemClicked,
		this,
		[this](QTreeWidgetItem* row, int) { showOccurrence(row); });
	connect(
		m_tree,
		&QTreeWidget::itemActivated,
		this,
		[this](QTreeWidgetItem* row, int) { showOccurrence(row); });
	auto* buttons = new QDialogButtonBox(QDialogButtonBox::Close, this);
	auto* refresh_button =
		buttons->addButton(tr("Refresh"), QDialogButtonBox::ActionRole);
	refresh_button->setObjectName(QStringLiteral("refreshDuplicates"));
	connect(
		refresh_button,
		&QPushButton::clicked,
		this,
		&DuplicateLabelsDialog::refresh);
	connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
	layout->addWidget(buttons);
	if (project) connect(project, &QObject::destroyed, this, &QDialog::close);
	QET::trackDialogGeometry(this);
	refresh();
}

// The search follows signals and devices across folios, so it reads
// every folio: those not built yet (lazy folios) are built first, with
// their progress shown, as the terminal block manager does.
void DuplicateLabelsDialog::buildFolios()
{
	if (! m_project || ! m_project->unloadedFolioCount()) return;
	QProgressDialog progress(
		tr("Reading the folios..."),
		QString(),
		0,
		m_project->unloadedFolioCount(),
		parentWidget() ? parentWidget() : this);
	progress.setWindowTitle(windowTitle());
	progress.setWindowModality(Qt::ApplicationModal);
	progress.setCancelButton(nullptr);
	progress.setMinimumDuration(0);
	QApplication::setOverrideCursor(Qt::WaitCursor);
	const auto restore = qScopeGuard(
		[this]()
		{
			if (m_project) m_project->setFolioBuildObserver({});
			QApplication::restoreOverrideCursor();
		});
	// Between two folios: the progress, and the event loop without the
	// user's input, so that the window system still gets answers.
	m_project->setFolioBuildObserver(
		[&progress](int built, int total)
		{
			progress.setMaximum(total);
			progress.setLabelText(
				tr("Reading folio %1 of %2...").arg(built).arg(total));
			progress.setValue(built);
			QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
		});
	m_project->loadFolios();
}

void DuplicateLabelsDialog::refresh()
{
	m_targets.clear();
	m_tree->clear();
	buildFolios();
	const auto groups = DuplicateLabels::find(
		m_project,
		m_case_sensitive->isChecked() ? Qt::CaseSensitive :
										Qt::CaseInsensitive);
	for (const auto& group : groups)
	{
		auto* root = new QTreeWidgetItem(m_tree);
		root->setIcon(0, style()->standardIcon(QStyle::SP_MessageBoxCritical));
		if (group.kind == DuplicateLabels::Kind::Signal)
			root->setText(0, tr("Duplicate signal: %1").arg(group.label));
		else if (group.place.isEmpty())
			root->setText(0, tr("Duplicate component: %1").arg(group.label));
		else
			root->setText(
				0,
				//: %2 is the plant and location, such as "=P1+UV1".
				tr("Duplicate component: %1 in %2")
					.arg(group.label, group.place));
		// One collapsed row per signal or linked device: a group holding a
		// single occurrence, the usual mistake, stands out from the others.
		QHash<int, QTreeWidgetItem*> entities;
		QHash<int, QStringList>		 sheets;
		for (const auto& occurrence : group.occurrences)
		{
			auto*	 item	 = occurrence.item.data();
			Diagram* diagram = DuplicateLabels::diagramOf(item);
			if (! diagram) continue;
			QTreeWidgetItem* entity = entities.value(occurrence.entity);
			if (! entity)
			{
				entity = new QTreeWidgetItem(root);
				//: %1 numbers one electrical signal or one linked device
				//: among those sharing a label.
				entity->setText(0, tr("Group %1").arg(entities.size() + 1));
				entity->setToolTip(
					0,
					tr("Occurrences in the same group belong to one signal "
					   "or linked device."));
				entities.insert(occurrence.entity, entity);
				// Clicking a group shows its first occurrence.
				m_targets.insert(entity, item);
			}
			const QString sheet =
				QString::number(m_project->folioIndex(diagram) + 1);
			QStringList& entity_sheets = sheets[occurrence.entity];
			if (! entity_sheets.contains(sheet)) entity_sheets.append(sheet);
			auto* row = new QTreeWidgetItem(entity);
			row->setText(0, occurrence.label);
			row->setText(1, tr("%1 — %2").arg(sheet, diagram->title()));
			const QPointF position = item->sceneBoundingRect().center();
			row->setText(2, diagram->convertPosition(position).toString());
			row->setToolTip(
				2,
				tr("x: %1, y: %2").arg(position.x()).arg(position.y()));
			m_targets.insert(row, item);
		}
		for (auto it = entities.cbegin(); it != entities.cend(); ++it)
		{
			it.value()->setText(
				1,
				sheets.value(it.key()).join(QStringLiteral(", ")));
			it.value()->setText(
				2,
				tr("%n occurrences", nullptr, it.value()->childCount()));
		}
		root->setExpanded(true);
	}
	if (groups.isEmpty())
	{
		m_summary->setText(tr("No duplicate signals or components found."));
	}
	else
	{
		//: %n counts conflicting identifiers, not wire segments or symbols.
		m_summary->setText(
			tr("%n duplicate labels found.", nullptr, groups.size()));
	}
}

void DuplicateLabelsDialog::showOccurrence(QTreeWidgetItem* row)
{
	if (! m_targets.contains(row)) return;
	const auto target  = m_targets.value(row);
	Diagram*   diagram = DuplicateLabels::diagramOf(target);
	if (! m_project || ! diagram || m_project->folioIndex(diagram) < 0)
	{
		m_summary->setText(
			tr("This occurrence no longer exists. Refresh the results."));
		return;
	}
	if (auto* editor = qobject_cast<QETDiagramEditor*>(parentWidget()))
		editor->activateProject(m_project);
	diagram->showMe();
	diagram->clearSelection();
	target->setSelected(true);
	const QRectF bounds =
		target->sceneBoundingRect().adjusted(-100, -100, 100, 100);
	for (QGraphicsView* view : diagram->views())
	{
		if (auto* diagram_view = qobject_cast<DiagramView*>(view))
			diagram_view->zoomToRect(bounds);
		else
		{
			view->fitInView(bounds, Qt::KeepAspectRatio);
			view->centerOn(bounds.center());
		}
	}
}
