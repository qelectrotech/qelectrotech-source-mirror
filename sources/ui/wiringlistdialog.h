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
#ifndef WIRINGLISTDIALOG_H
#define WIRINGLISTDIALOG_H

#include <QDialog>

class QETProject;

/**
	@brief The WiringListDialog class
	Read-only view of the project's from-to wiring list, read from the
	wiring_list_view of projectDataBase.

	Deliberately not an exporter: QET already ships a wiring-list CSV export
	(Projet > Exporter le plan de câblage, and --export-cables), which walks
	the project XML and covers that need. This dialog exists to make the
	database view inspectable, and above all to state how many conductors
	are missing from it and why -- a count the CSV export cannot give,
	because it never excludes anything in the first place.
*/
class WiringListDialog : public QDialog
{
	Q_OBJECT

	public:
		explicit WiringListDialog(QETProject *project, QWidget *parent = nullptr);

	private:
		QETProject *m_project = nullptr;
};

#endif // WIRINGLISTDIALOG_H
