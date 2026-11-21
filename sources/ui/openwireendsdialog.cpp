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
#include "openwireendsdialog.h"

#include <dataBase/projectdatabase.h>
#include <diagram.h>
#include <diagramcontent.h>
#include <diagramview.h>
#include <qet.h>
#include <qetgraphicsitem/conductor.h>
#include <qetgraphicsitem/element.h>
#include <qetproject.h>

#include <QApplication>
#include <QDialogButtonBox>
#include <QHeaderView>
#include <QLabel>
#include <QPushButton>
#include <QScopeGuard>
#include <QTreeWidget>
#include <QVBoxLayout>

OpenWireEndsDialog::OpenWireEndsDialog(QETProject *project, QWidget *parent) :
	QDialog(parent),
	m_project(project),
	m_tree(new QTreeWidget(this)),
	m_summary(new QLabel(this))
{
	setWindowTitle(tr("Check open wire ends"));
	resize(860, 420);
	auto *layout = new QVBoxLayout(this);
	m_summary->setWordWrap(true);
	m_summary->setObjectName(QStringLiteral("openWireEndsSummary"));
	layout->addWidget(m_summary);
	auto *help = new QLabel(
		tr("Each row is a wire that leads nowhere: to a folio report linked "
		   "to no other report, into a junction on one side only, or on a "
		   "potential that reaches one symbol terminal or none. "
		   "Click a row to select its wires."),
		this);
	help->setWordWrap(true);
	layout->addWidget(help);
	m_tree->setObjectName(QStringLiteral("openWireEndsRows"));
	m_tree->setHeaderLabels({tr("Problem"), tr("Folio"), tr("Position"), tr("Symbol"), tr("Wires")});
	m_tree->setRootIsDecorated(false);
	m_tree->setAlternatingRowColors(true);
	for (int column = 0 ; column < 5 ; ++column)
		m_tree->header()->setSectionResizeMode(column, QHeaderView::ResizeToContents);
	m_tree->header()->setStretchLastSection(true);
	layout->addWidget(m_tree);
		//Once per row picked, by the mouse or the arrow keys
	connect(m_tree, &QTreeWidget::currentItemChanged, this,
		[this](QTreeWidgetItem *row) { showFinding(row); });
	auto *buttons = new QDialogButtonBox(QDialogButtonBox::Close, this);
	auto *refresh_button = buttons->addButton(tr("Refresh"), QDialogButtonBox::ActionRole);
	connect(refresh_button, &QPushButton::clicked, this, &OpenWireEndsDialog::refresh);
	connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
	layout->addWidget(buttons);
	if (project) {
		connect(project, &QObject::destroyed, this, &QDialog::close);
	}
	QET::trackDialogGeometry(this);
	refresh();
}

QString OpenWireEndsDialog::problemText(const OpenWireEnds::Finding &finding)
{
	switch (finding.kind) {
		case OpenWireEnds::Kind::UnlinkedReport:
			return tr("Folio report linked to no other report");
		case OpenWireEnds::Kind::LoneJunction:
			return tr("Junction with one wire");
		case OpenWireEnds::Kind::OneEnd:
			break;
	}
	return finding.element.isNull()
		? tr("Wires that reach no symbol terminal")
		: tr("Wires that reach only one symbol terminal");
}

void OpenWireEndsDialog::refresh()
{
	m_findings.clear();
	m_tree->clear();
	if (!m_project) {
		return;
	}
	const auto findings = m_project->dataBase()->openWireEnds();
	for (const auto &finding : findings) {
		auto *row = new QTreeWidgetItem(m_tree);
		row->setText(0, problemText(finding));
		row->setText(1, finding.folioTitle.isEmpty()
			? QString::number(finding.folioNumber)
			: QStringLiteral("%1 - %2").arg(finding.folioNumber).arg(finding.folioTitle));
		row->setText(2, finding.position);
		row->setText(3, finding.terminal.isEmpty()
			? finding.label
			: QStringLiteral("%1:%2").arg(finding.label, finding.terminal));
		row->setText(4, QString::number(finding.wires.size()));
		m_findings.insert(row, finding);
	}
	if (findings.isEmpty()) {
		m_summary->setText(tr("Every wire leads somewhere."));
	} else {
		m_summary->setText(tr("%n place(s) where wires stop and go nowhere.", nullptr, findings.size()));
	}
}

// Only the folios of the row are built and searched, once, when it is
// picked; the check itself reads the database only.
void OpenWireEndsDialog::showFinding(QTreeWidgetItem *row)
{
	if (!m_project || !m_findings.contains(row)) {
		return;
	}
	const OpenWireEnds::Finding finding = m_findings.value(row);
	QList<QUuid> folio_uuids = finding.folios;
	if (!finding.folio.isNull() && !folio_uuids.contains(finding.folio)) {
		folio_uuids.prepend(finding.folio);
	}
	QList<Diagram *> folios;
	for (Diagram *diagram : m_project->diagrams()) {
			//What an earlier row selected, wherever it is
		if (diagram->isLoaded()) {
			diagram->clearSelection();
		}
		if (folio_uuids.contains(diagram->uuid())) {
			folios << diagram;
		}
	}
	{
		QApplication::setOverrideCursor(Qt::WaitCursor);
		const auto restore = qScopeGuard([]() { QApplication::restoreOverrideCursor(); });
		m_project->buildFolios(folios);
	}
	const auto uuids = QSet<QUuid>(finding.wires.begin(), finding.wires.end());
	QList<std::pair<Diagram *, QList<QGraphicsItem *>>> found;
	Diagram *shown = nullptr;
	for (Diagram *diagram : std::as_const(folios)) {
		QList<QGraphicsItem *> items;
		for (Conductor *conductor : diagram->content().conductors()) {
			if (uuids.contains(conductor->uuid())) {
				items << conductor;
			}
		}
		if (!finding.element.isNull()) {
			for (Element *element : diagram->elements()) {
				if (element->uuid() == finding.element) {
					items << element;
				}
			}
		}
		if (items.isEmpty()) {
			continue;
		}
		if (!shown || diagram->uuid() == finding.folio) {
			shown = diagram;
		}
		found.append({diagram, items});
	}
	if (!shown) {
		m_summary->setText(tr("These wires no longer exist. Refresh the results."));
		return;
	}
		//Shown first, so the window follows what is selected on it
	shown->showMe();
	QRectF bounds;
	for (const auto &[diagram, items] : std::as_const(found)) {
		for (QGraphicsItem *item : items) {
			item->setSelected(true);
			if (diagram == shown) {
				bounds |= item->sceneBoundingRect();
			}
		}
	}
	bounds.adjust(-100, -100, 100, 100);
	for (QGraphicsView *view : shown->views()) {
		if (auto *diagram_view = qobject_cast<DiagramView *>(view)) {
			diagram_view->zoomToRect(bounds);
		}
	}
}
