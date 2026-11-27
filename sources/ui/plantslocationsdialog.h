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
#ifndef PLANTSLOCATIONSDIALOG_H
#define PLANTSLOCATIONSDIALOG_H

#include "placerename.h"

#include <QDialog>
#include <QPointer>

class QETProject;
class QLabel;
class QPushButton;
class QTableWidget;
class QTableWidgetItem;

/**
	@brief The PlantsLocationsDialog class
	Project > Plants and locations...: the project's plants and locations
	with how many symbols and folios each holds. It reads the project
	database's qet_locations view and nothing else. Double-clicking a row
	asks for the first folio of that place.
*/
class PlantsLocationsDialog : public QDialog
{
	Q_OBJECT
	public:
		explicit PlantsLocationsDialog(QETProject *project, QWidget *parent = nullptr);

	signals:
			/// @p position is the folio's 1-based position in the project
		void showFolio(int position);

	public slots:
		void refresh();
		void renamePlant() {renameSelected(false);}
		void renameLocation() {renameSelected(true);}
		bool applyRename(const PlaceRename::Request &request,
						 PlaceRename::CounterPolicy policy = PlaceRename::CounterPolicy::MergeHigher);
		void copyPlant();
		bool applyCopy(const QString &plant, const QString &new_name);

	private:
		void rowActivated(QTableWidgetItem *item);
		void renameSelected(bool location);
		void updateButtons();
		void buildFolios();

		QPointer<QETProject> m_project;
		QLabel *m_summary;
		QTableWidget *m_table;
		QPushButton *m_rename_plant;
		QPushButton *m_rename_location;
		QPushButton *m_copy_plant;
};

#endif // PLANTSLOCATIONSDIALOG_H
