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
#include "plantslocationsdialog.h"

#include "../reports/projectreports.h"

#include <qet.h>
#include <qetproject.h>

#include <QApplication>
#include <QDialogButtonBox>
#include <QHeaderView>
#include <QInputDialog>
#include <QLabel>
#include <QMessageBox>
#include <QProgressDialog>
#include <QPushButton>
#include <QScopeGuard>
#include <QTableWidget>
#include <QUndoStack>
#include <QVBoxLayout>

namespace
{
	enum Column {PlantColumn, LocationColumn, DesignationColumn,
				 SymbolsColumn, FoliosColumn, FirstFolioColumn};
}

PlantsLocationsDialog::PlantsLocationsDialog(QETProject *project, QWidget *parent) :
	QDialog(parent),
	m_project(project),
	m_summary(new QLabel(this)),
	m_table(new QTableWidget(this)),
	m_rename_plant(nullptr),
	m_rename_location(nullptr)
{
	setWindowTitle(tr("Plants and locations"));
	resize(760, 460);
	auto *layout = new QVBoxLayout(this);

	m_summary->setObjectName(QStringLiteral("placesSummary"));
	layout->addWidget(m_summary);

	m_table->setObjectName(QStringLiteral("placesTable"));
	m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
	m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
	m_table->setSelectionMode(QAbstractItemView::SingleSelection);
	m_table->setAlternatingRowColors(true);
	m_table->verticalHeader()->setVisible(false);
	m_table->setToolTip(tr("Double-click a row to go to its first folio"));
	layout->addWidget(m_table);
	connect(m_table, &QTableWidget::itemDoubleClicked, this, &PlantsLocationsDialog::rowActivated);

	auto *buttons = new QDialogButtonBox(QDialogButtonBox::Close, this);
	auto *refresh_button = buttons->addButton(tr("Refresh"), QDialogButtonBox::ActionRole);
	m_rename_plant = buttons->addButton(tr("Rename plant..."), QDialogButtonBox::ActionRole);
	m_rename_plant->setObjectName(QStringLiteral("renamePlant"));
	m_rename_plant->setToolTip(tr("Changes this plant's name everywhere in the project"));
	m_rename_location = buttons->addButton(tr("Rename location..."), QDialogButtonBox::ActionRole);
	m_rename_location->setObjectName(QStringLiteral("renameLocation"));
	m_rename_location->setToolTip(tr("Changes this location's name in its plant, everywhere in the project"));
	connect(m_rename_plant, &QPushButton::clicked, this, &PlantsLocationsDialog::renamePlant);
	connect(m_rename_location, &QPushButton::clicked, this, &PlantsLocationsDialog::renameLocation);
	connect(m_table, &QTableWidget::itemSelectionChanged, this, &PlantsLocationsDialog::updateButtons);
	connect(refresh_button, &QPushButton::clicked, this, &PlantsLocationsDialog::refresh);
	connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
	layout->addWidget(buttons);
	if (project) connect(project, &QObject::destroyed, this, &QDialog::close);
	QET::trackDialogGeometry(this);

	refresh();
}

/**
	@brief PlantsLocationsDialog::refresh
	Reads the project database again.
*/
void PlantsLocationsDialog::refresh()
{
	const ProjectReports::Table shown = ProjectReports::locations(m_project);
	m_table->clear();
	m_table->setColumnCount(int(shown.headers.size()));
	m_table->setRowCount(int(shown.rows.size()));
	m_table->setHorizontalHeaderLabels(shown.headers);
	for (int r = 0 ; r < shown.rows.size() ; ++r) {
		const QStringList &row = shown.rows.at(r);
		for (int c = 0 ; c < row.size() ; ++c) {
			QString text = row.at(c);
			if ((c == PlantColumn || c == LocationColumn) && text.trimmed().isEmpty())
				text = tr("(none)");
			auto *item = new QTableWidgetItem(text);
			if (c == SymbolsColumn || c == FoliosColumn || c == FirstFolioColumn) {
				item->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
			}
			if (c == PlantColumn) {
				item->setData(Qt::UserRole, row.at(FirstFolioColumn).toInt());
				item->setData(Qt::UserRole + 1, row.at(PlantColumn));
				item->setData(Qt::UserRole + 2, row.at(LocationColumn));
			}
			m_table->setItem(r, c, item);
		}
	}
	m_table->resizeColumnsToContents();
	updateButtons();
	m_table->horizontalHeader()->setSectionResizeMode(DesignationColumn, QHeaderView::Stretch);
	if (shown.error.isEmpty())
		m_summary->setText(tr("%n plant and location row(s).", nullptr, int(shown.rows.size())));
	else
		m_summary->setText(shown.error);
}

void PlantsLocationsDialog::rowActivated(QTableWidgetItem *item)
{
	if (!item) return;
	const QTableWidgetItem *first = m_table->item(item->row(), PlantColumn);
	const int position = first ? first->data(Qt::UserRole).toInt() : 0;
	if (position > 0)
		emit showFolio(position);
}

/**
	@brief PlantsLocationsDialog::updateButtons
	A place with no plant, or no location, has no name to change.
*/
void PlantsLocationsDialog::updateButtons()
{
	const int row = m_table->currentRow();
	const QTableWidgetItem *first = row >= 0 ? m_table->item(row, PlantColumn) : nullptr;
	const bool plant = first && !first->data(Qt::UserRole + 1).toString().isEmpty();
	const bool location = plant && !first->data(Qt::UserRole + 2).toString().isEmpty();
	m_rename_plant->setEnabled(plant);
	m_rename_location->setEnabled(location);
}

/**
	@brief PlantsLocationsDialog::buildFolios
	A rename reaches every folio: those not built yet are built first
	(structure boxes exist only on built folios).
*/
void PlantsLocationsDialog::buildFolios()
{
	if (!m_project || !m_project->unloadedFolioCount()) return;
	QProgressDialog progress(tr("Reading the folios..."), QString(), 0,
							 m_project->unloadedFolioCount(), this);
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

/**
	@brief PlantsLocationsDialog::applyRename
	Renames in one undo step of the project.
	@return false when the project stores the old name nowhere
*/
bool PlantsLocationsDialog::applyRename(const PlaceRename::Request &request,
										PlaceRename::CounterPolicy policy)
{
	if (!m_project || !PlaceRename::isValid(request)) return false;
	buildFolios();
	QUndoCommand *command = PlaceRename::command(m_project, request, policy);
	if (!command) return false;
	m_project->undoStack()->push(command);
	refresh();
	return true;
}

/**
	@brief PlantsLocationsDialog::renameSelected
	Asks for the new name, and for the two choices a rename can need: going
	onto a name that exists, and which wire counters the plant keeps.
	@param location : rename the selected row's location, not its plant
*/
void PlantsLocationsDialog::renameSelected(bool location)
{
	const int row = m_table->currentRow();
	const QTableWidgetItem *first = row >= 0 ? m_table->item(row, PlantColumn) : nullptr;
	if (!m_project || !first) return;
	PlaceRename::Request request;
	request.plant = first->data(Qt::UserRole + 1).toString();
	request.location = first->data(Qt::UserRole + 2).toString();
	request.rename_location = location;
	const QString old_name = location ? request.location : request.plant;

	bool ok = false;
	const QString text = QInputDialog::getText(
				this, location ? tr("Rename location") : tr("Rename plant"),
				location ? tr("New name for the location %1 of plant %2:").arg(old_name, request.plant)
						 : tr("New name for the plant %1:").arg(old_name),
				QLineEdit::Normal, old_name, &ok);
	if (!ok) return;
	request.new_name = text.trimmed();
	QString why;
	if (!PlaceRename::isValid(request, &why)) {
		QMessageBox::warning(this, windowTitle(), why);
		return;
	}
	if (PlaceRename::targetExists(m_project, request)
			&& QMessageBox::question(this, windowTitle(),
				tr("\"%1\" already exists. The two will become one. Continue?")
					.arg(request.new_name)) != QMessageBox::Yes)
		return;

	PlaceRename::CounterPolicy policy = PlaceRename::CounterPolicy::MergeHigher;
	if (PlaceRename::counterClash(m_project, request)) {
		QMessageBox box(QMessageBox::Question, windowTitle(),
				tr("Both plants have wire numbers counted. Which counters should \"%1\" keep?")
					.arg(request.new_name), QMessageBox::NoButton, this);
		auto *merge = box.addButton(tr("The higher number of each"), QMessageBox::AcceptRole);
		auto *keep_new = box.addButton(tr("Those of %1").arg(request.new_name), QMessageBox::AcceptRole);
		auto *keep_old = box.addButton(tr("Those of %1").arg(request.plant), QMessageBox::AcceptRole);
		box.addButton(QMessageBox::Cancel);
		box.setDefaultButton(merge);
		box.exec();
		if (box.clickedButton() == keep_new) policy = PlaceRename::CounterPolicy::KeepNew;
		else if (box.clickedButton() == keep_old) policy = PlaceRename::CounterPolicy::KeepOld;
		else if (box.clickedButton() != merge) return;
	}
	if (!applyRename(request, policy)) {
		QMessageBox::information(this, windowTitle(),
				m_project->unloadedFolioCount()
				? tr("Not every folio could be read, so nothing was renamed.")
				: tr("The project does not store \"%1\" anywhere that can be renamed.").arg(old_name));
	}
}
