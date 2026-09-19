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
#ifndef WIRECATALOGUEUI_H
#define WIRECATALOGUEUI_H

#include <QDockWidget>

class WireCatalogueDb;
class WireCatalogueModel;
class QTableView;
class QLineEdit;
class QAction;

/**
	@brief The WireCatalogueWidget class
	Dockable sidebar panel for the wire / cable catalogue: a search field, a
	table of catalogue entries, and Add / Edit / Delete actions. Owns the
	WireCatalogueDb (opened on a per-user catalogue file) and the table model.
*/
class WireCatalogueWidget : public QDockWidget
{
	Q_OBJECT

	public:
		explicit WireCatalogueWidget(QWidget *parent = nullptr);
		~WireCatalogueWidget() override;

		WireCatalogueDb *database() const { return m_db; }

	private slots:
		void addWire();
		void editWire();
		void removeWire();
		void filterChanged(const QString &text);
		void selectionChanged();
		void importCsv();
		void exportCsv();

	private:
		void buildUi();
		/// Default catalogue file under the user's QET config location.
		static QString defaultCataloguePath();

	private:
		WireCatalogueDb    *m_db    = nullptr;
		WireCatalogueModel *m_model = nullptr;
		QTableView         *m_view  = nullptr;
		QLineEdit          *m_search = nullptr;
		QAction            *m_add_action    = nullptr;
		QAction            *m_edit_action   = nullptr;
		QAction            *m_remove_action = nullptr;
};

#endif // WIRECATALOGUEUI_H
