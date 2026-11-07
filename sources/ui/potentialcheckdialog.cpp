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
#include "potentialcheckdialog.h"

#include <diagram.h>
#include <diagramcontent.h>
#include <diagramview.h>
#include <qet.h>
#include <qetgraphicsitem/conductor.h>
#include <qetproject.h>

#include <QApplication>
#include <QDialogButtonBox>
#include <QHeaderView>
#include <QLabel>
#include <QPushButton>
#include <QScopeGuard>
#include <QTreeWidget>
#include <QVBoxLayout>

PotentialCheckDialog::PotentialCheckDialog(QETProject *project, QWidget *parent) :
	QDialog(parent),
	m_project(project),
	m_tree(new QTreeWidget(this)),
	m_summary(new QLabel(this))
{
	setWindowTitle(tr("Check potential types"));
	resize(640, 420);
	auto *layout = new QVBoxLayout(this);
	m_summary->setWordWrap(true);
	m_summary->setObjectName(QStringLiteral("potentialCheckSummary"));
	layout->addWidget(m_summary);
	auto *help = new QLabel(
		tr("Each row is a potential whose wires have different types. "
		   "Click a row to select its wires. "
		   "Types joined on purpose can be declared in the project "
		   "properties and are not listed."),
		this);
	help->setWordWrap(true);
	layout->addWidget(help);
	m_tree->setObjectName(QStringLiteral("potentialCheckRows"));
	m_tree->setHeaderLabels({tr("Types on the potential"), tr("Typed wires")});
	m_tree->setRootIsDecorated(false);
	m_tree->setAlternatingRowColors(true);
	m_tree->header()->setSectionResizeMode(0, QHeaderView::Stretch);
	m_tree->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
	layout->addWidget(m_tree);
	connect(m_tree, &QTreeWidget::itemClicked, this,
		[this](QTreeWidgetItem *row, int) { showWires(row); });
	connect(m_tree, &QTreeWidget::itemActivated, this,
		[this](QTreeWidgetItem *row, int) { showWires(row); });
	auto *buttons = new QDialogButtonBox(QDialogButtonBox::Close, this);
	auto *refresh_button = buttons->addButton(tr("Refresh"), QDialogButtonBox::ActionRole);
	connect(refresh_button, &QPushButton::clicked, this, &PotentialCheckDialog::refresh);
	connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
	layout->addWidget(buttons);
	if (project) {
		connect(project, &QObject::destroyed, this, &QDialog::close);
	}
	QET::trackDialogGeometry(this);
	refresh();
}

void PotentialCheckDialog::refresh()
{
	m_wires.clear();
	m_tree->clear();
	if (!m_project) {
		return;
	}
	const auto mismatches = m_project->dataBase()->potentialTypeMismatches();
	for (const auto &mismatch : mismatches) {
		auto *row = new QTreeWidgetItem(m_tree);
		row->setText(0, mismatch.types.join(QStringLiteral(" / ")));
		row->setText(1, QString::number(mismatch.wires.size()));
		m_wires.insert(row, mismatch.wires);
	}
	if (mismatches.isEmpty()) {
		m_summary->setText(m_project->potentialTypes().isEnabled()
			? tr("No potential carries wires of different types.")
			: tr("Potential types are off for this project, so there is nothing to check."));
	} else {
		m_summary->setText(
			tr("%n potential(s) carry wires of different types.", nullptr, mismatches.size()));
	}
}

// The wires are looked up by uuid on the folios, once, when a row is
// clicked; the check itself reads the database only.
void PotentialCheckDialog::showWires(QTreeWidgetItem *row)
{
	if (!m_project || !m_wires.contains(row)) {
		return;
	}
	if (m_project->unloadedFolioCount()) {
		QApplication::setOverrideCursor(Qt::WaitCursor);
		const auto restore = qScopeGuard([]() { QApplication::restoreOverrideCursor(); });
		m_project->loadFolios();
	}
	const auto uuids = QSet<QUuid>(m_wires.value(row).begin(), m_wires.value(row).end());
	Diagram *first = nullptr;
	QRectF bounds;
	for (Diagram *diagram : m_project->diagrams()) {
		QList<Conductor *> found;
		for (Conductor *conductor : diagram->content().conductors()) {
			if (uuids.contains(conductor->uuid())) {
				found << conductor;
			}
		}
		if (found.isEmpty()) {
			continue;
		}
		if (!first) {
			first = diagram;
			diagram->showMe();
			diagram->clearSelection();
		}
		for (Conductor *conductor : found) {
			conductor->setSelected(true);
			if (diagram == first) {
				bounds |= conductor->sceneBoundingRect();
			}
		}
	}
	if (!first) {
		m_summary->setText(tr("These wires no longer exist. Refresh the results."));
		return;
	}
	bounds.adjust(-100, -100, 100, 100);
	for (QGraphicsView *view : first->views()) {
		if (auto *diagram_view = qobject_cast<DiagramView *>(view)) {
			diagram_view->zoomToRect(bounds);
		}
	}
}
