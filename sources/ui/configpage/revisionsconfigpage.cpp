// SPDX-License-Identifier: GPL-2.0-or-later
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
#include "revisionsconfigpage.h"

#include "../../properties/revisionlist.h"
#include "../../qeticons.h"
#include "../../qetproject.h"

#include <QDate>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QKeyEvent>
#include <QLabel>
#include <QPushButton>
#include <QTableWidget>
#include <QTimer>
#include <QVBoxLayout>

RevisionsConfigPage::RevisionsConfigPage(QETProject *project, QWidget *parent) :
	ProjectConfigPage(project, parent)
{
	init();
}

QString RevisionsConfigPage::title() const
{
	return tr("Revisions", "configuration page title");
}

QIcon RevisionsConfigPage::icon() const
{
	return QET::Icons::DocumentOpenRecent;
}

void RevisionsConfigPage::initWidgets()
{
	m_table = new QTableWidget(0, 5, this);
	m_table->setObjectName(QStringLiteral("revisionTable"));
	m_table->setHorizontalHeaderLabels({tr("Revision"), tr("Date"), tr("Description"),
										tr("Author"), tr("Approved by")});
	m_table->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
	m_table->verticalHeader()->setVisible(false);
	m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
	m_add = new QPushButton(tr("Add a revision"), this);
	m_add->setObjectName(QStringLiteral("addRevision"));
	m_remove = new QPushButton(tr("Remove"), this);
	m_remove->setObjectName(QStringLiteral("removeRevision"));
	connect(m_add, &QPushButton::clicked, this, &RevisionsConfigPage::addRevision);
	connect(m_remove, &QPushButton::clicked, this, &RevisionsConfigPage::removeRevision);
}

void RevisionsConfigPage::initLayout()
{
	auto *buttons = new QHBoxLayout;
	buttons->addWidget(m_add);
	buttons->addWidget(m_remove);
	buttons->addStretch();
	auto *layout = new QVBoxLayout(this);
	auto *help = new QLabel(tr("The project's revision history, the latest last. "
							   "A table on a folio can print it."), this);
	help->setWordWrap(true);
	layout->addWidget(help);
		//The buttons above the table: the page is as tall as the tallest
		//page of the dialog (a QStackedWidget), and the table stretches
		//to fill it, so buttons under it would be far out of view
	layout->addLayout(buttons);
	layout->addWidget(m_table);
}

void RevisionsConfigPage::readValuesFromProject()
{
	const QList<Revision> revisions = m_project->revisions().revisions();
	m_table->setRowCount(0);
	for (const Revision &r : revisions) {
		const int row = m_table->rowCount();
		m_table->insertRow(row);
		const QString values[] = {r.revision, r.date, r.description, r.author, r.approver};
		for (int column = 0 ; column < 5 ; ++column)
			m_table->setItem(row, column, new QTableWidgetItem(values[column]));
	}
}

void RevisionsConfigPage::adjustReadOnly()
{
	const bool read_only = m_project->isReadOnly();
	m_table->setEditTriggers(read_only ? QAbstractItemView::NoEditTriggers
									   : QAbstractItemView::AllEditTriggers);
	m_add->setEnabled(!read_only);
	m_remove->setEnabled(!read_only);
}

void RevisionsConfigPage::applyProjectConf()
{
	QList<Revision> revisions;
	for (int row = 0 ; row < m_table->rowCount() ; ++row) {
		const auto text = [this, row](int column) {
			const QTableWidgetItem *item = m_table->item(row, column);
			return item ? item->text().trimmed() : QString();
		};
		revisions << Revision{text(0), text(1), text(2), text(3), text(4)};
	}
	RevisionList list;
	list.setRevisions(revisions);
	m_project->setRevisions(list);
}

/**
	@brief RevisionsConfigPage::addRevision
	A new row at the end, dated today, its date typed in ISO 8601, and
	ready to be typed in
*/
void RevisionsConfigPage::addRevision()
{
	const int row = m_table->rowCount();
	m_table->insertRow(row);
	for (int column = 0 ; column < 5 ; ++column)
		m_table->setItem(row, column, new QTableWidgetItem(
			column == 1 ? QDate::currentDate().toString(Qt::ISODate) : QString()));
	m_table->setCurrentCell(row, 0);
	m_table->setFocus();
		//Once the new row is laid out: opened at once, the editor fails
	QTimer::singleShot(0, m_table, [this, row]() {
		if (QTableWidgetItem *item = m_table->item(row, 0))
			m_table->editItem(item);
	});
}

/**
	@brief RevisionsConfigPage::keyPressEvent
	Return or Enter typed in the table ends the edit of a cell (the cell's
	editor has already taken the text): it must not go on up to the
	dialog, whose default button would close it. Elsewhere on the page
	they reach the dialog as usual.
*/
void RevisionsConfigPage::keyPressEvent(QKeyEvent *event)
{
	if ((event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter)
			&& m_table && (m_table->hasFocus() || m_table->isAncestorOf(focusWidget()))) {
		event->accept();
		return;
	}
	ProjectConfigPage::keyPressEvent(event);
}

void RevisionsConfigPage::removeRevision()
{
	const int row = m_table->currentRow();
	if (row >= 0)
		m_table->removeRow(row);
}
