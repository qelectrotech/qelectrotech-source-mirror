/*
	Copyright 2026 Trovo Tech Solutions
	This file is part of a custom feature set built on QElectroTech.

	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 2 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#include "wirecatalogueui.h"
#include "wirecataloguedb.h"
#include "wirecataloguemodel.h"
#include "wirespecdialog.h"

#include <QTableView>
#include <QLineEdit>
#include <QToolBar>
#include <QVBoxLayout>
#include <QHeaderView>
#include <QAction>
#include <QMessageBox>
#include <QStandardPaths>
#include <QDir>
#include <QFileDialog>

WireCatalogueWidget::WireCatalogueWidget(QWidget *parent) :
	QDockWidget(tr("Catalogue de fils et câbles"), parent)
{
	setObjectName(QStringLiteral("wire_catalogue_dock"));

	m_db = new WireCatalogueDb(this);
	if (!m_db->open(defaultCataloguePath())) {
		QMessageBox::warning(this, tr("Catalogue de fils"),
			tr("Impossible d'ouvrir la base de données du catalogue :\n%1")
				.arg(m_db->lastError()));
	}

	buildUi();
}

WireCatalogueWidget::~WireCatalogueWidget() = default;

QString WireCatalogueWidget::defaultCataloguePath()
{
	return WireCatalogueDb::defaultPath();
}

void WireCatalogueWidget::buildUi()
{
	auto *container = new QWidget(this);
	auto *layout = new QVBoxLayout(container);
	layout->setContentsMargins(2, 2, 2, 2);

	// --- toolbar ---
	// Note: actions are created and connected explicitly rather than via the
	// addAction(text, receiver, &slot) convenience overloads, which only exist
	// on QToolBar from Qt 6.3 onwards (this project targets Qt5).
	auto *toolbar = new QToolBar(container);
	toolbar->setStyleSheet(QStringLiteral(
		"QToolBar { background: qlineargradient(x1:0, y1:0, x2:1, y2:0,"
		" stop:0 #e8f0fe, stop:1 #e6f7ee); border: none; spacing: 4px; }"
		"QToolButton { color: #0a4a8f; font-weight: bold; padding: 3px 8px;"
		" border-radius: 3px; }"
		"QToolButton:hover { background: #d0e3ff; }"
		"QToolButton:disabled { color: #a0a0a0; }"));
	m_add_action    = new QAction(tr("Ajouter"),     this);
	m_edit_action   = new QAction(tr("Éditer"),    this);
	m_remove_action = new QAction(tr("Supprimer"),  this);
	auto *refresh_action = new QAction(tr("Actualiser"), this);
	auto *import_action  = new QAction(tr("Importer…"), this);
	auto *export_action  = new QAction(tr("Exporter…"), this);
	toolbar->addAction(m_add_action);
	toolbar->addAction(m_edit_action);
	toolbar->addAction(m_remove_action);
	toolbar->addAction(refresh_action);
	toolbar->addSeparator();
	toolbar->addAction(import_action);
	toolbar->addAction(export_action);
	connect(import_action,   &QAction::triggered, this, &WireCatalogueWidget::importCsv);
	connect(export_action,   &QAction::triggered, this, &WireCatalogueWidget::exportCsv);
	connect(m_add_action,    &QAction::triggered, this, &WireCatalogueWidget::addWire);
	connect(m_edit_action,   &QAction::triggered, this, &WireCatalogueWidget::editWire);
	connect(m_remove_action, &QAction::triggered, this, &WireCatalogueWidget::removeWire);
	connect(refresh_action,  &QAction::triggered, this, [this]() { m_model->refresh(); });
	layout->addWidget(toolbar);

	// --- search ---
	m_search = new QLineEdit(container);
	m_search->setClearButtonEnabled(true);
	m_search->setPlaceholderText(tr("Rechercher un identifiant, un fabricant, un fournisseur, une couleur…"));
	connect(m_search, &QLineEdit::textChanged,
			this, &WireCatalogueWidget::filterChanged);
	layout->addWidget(m_search);

	// --- table ---
	m_model = new WireCatalogueModel(m_db, this);
	m_view = new QTableView(container);
	m_view->setModel(m_model);
	m_view->setSelectionBehavior(QAbstractItemView::SelectRows);
	m_view->setSelectionMode(QAbstractItemView::SingleSelection);
	m_view->setEditTriggers(QAbstractItemView::NoEditTriggers);
	m_view->verticalHeader()->setVisible(false);
	m_view->horizontalHeader()->setStretchLastSection(true);
	m_view->setSortingEnabled(false);
	layout->addWidget(m_view);

	connect(m_view->selectionModel(), &QItemSelectionModel::selectionChanged,
			this, &WireCatalogueWidget::selectionChanged);
	connect(m_view, &QTableView::doubleClicked,
			this, &WireCatalogueWidget::editWire);

	setWidget(container);
	selectionChanged(); // set initial enabled state
}

void WireCatalogueWidget::filterChanged(const QString &text)
{
	m_model->setFilter(text);
}

void WireCatalogueWidget::selectionChanged()
{
	const bool has = m_view->selectionModel()->hasSelection();
	m_edit_action->setEnabled(has);
	m_remove_action->setEnabled(has);
}

void WireCatalogueWidget::addWire()
{
	WireSpecDialog dlg(this);
	dlg.setWireSpec(WireSpec(), false);
	if (dlg.exec() != QDialog::Accepted)
		return;

	const WireSpec spec = dlg.wireSpec();
	if (!spec.isValid()) {
		QMessageBox::warning(this, tr("Catalogue de fils"),
			tr("Un identifiant de fil est obligatoire."));
		return;
	}
	if (m_db->contains(spec.wireId)) {
		QMessageBox::warning(this, tr("Catalogue de fils"),
			tr("Un fil portant l'identifiant « %1 » existe déjà.").arg(spec.wireId));
		return;
	}
	if (!m_db->addWire(spec)) {
		QMessageBox::warning(this, tr("Catalogue de fils"),
			tr("Impossible d'ajouter le fil :\n%1").arg(m_db->lastError()));
		return;
	}
	m_model->refresh();
}

void WireCatalogueWidget::editWire()
{
	const QModelIndex idx = m_view->currentIndex();
	if (!idx.isValid())
		return;

	const WireSpec current = m_model->wireForIndex(idx);
	if (!current.isValid())
		return;

	WireSpecDialog dlg(this);
	dlg.setWireSpec(current, true);
	if (dlg.exec() != QDialog::Accepted)
		return;

	if (!m_db->updateWire(dlg.wireSpec())) {
		QMessageBox::warning(this, tr("Catalogue de fils"),
			tr("Impossible de mettre à jour le fil :\n%1").arg(m_db->lastError()));
		return;
	}
	m_model->refresh();
}

void WireCatalogueWidget::removeWire()
{
	const QModelIndex idx = m_view->currentIndex();
	if (!idx.isValid())
		return;

	const WireSpec current = m_model->wireForIndex(idx);
	if (!current.isValid())
		return;

	if (QMessageBox::question(this, tr("Catalogue de fils"),
			tr("Supprimer le fil « %1 » du catalogue ?").arg(current.wireId))
		!= QMessageBox::Yes)
		return;

	if (!m_db->removeWire(current.wireId)) {
		QMessageBox::warning(this, tr("Catalogue de fils"),
			tr("Impossible de supprimer le fil :\n%1").arg(m_db->lastError()));
		return;
	}
	m_model->refresh();
}

void WireCatalogueWidget::exportCsv()
{
	const QString path = QFileDialog::getSaveFileName(this,
		tr("Exporter le catalogue de fils"), QStringLiteral("wirecatalogue.csv"),
		tr("Fichiers CSV (*.csv)"));
	if (path.isEmpty())
		return;

	const int n = m_db->exportCsv(path);
	if (n < 0)
		QMessageBox::warning(this, tr("Exporter"), m_db->lastError());
	else
		QMessageBox::information(this, tr("Exporter"),
			tr("%1 fil(s) exporté(s) vers :\n%2").arg(n).arg(path));
}

void WireCatalogueWidget::importCsv()
{
	const QString path = QFileDialog::getOpenFileName(this,
		tr("Importer un catalogue de fils"), QString(), tr("Fichiers CSV (*.csv)"));
	if (path.isEmpty())
		return;

	const int n = m_db->importCsv(path);
	if (n < 0) {
		QMessageBox::warning(this, tr("Importer"), m_db->lastError());
		return;
	}
	m_model->refresh();
	QMessageBox::information(this, tr("Importer"),
		tr("%1 fil(s) importé(s).").arg(n));
}
