/*
	Copyright 2006-2020 The QElectroTech Team
	This file is part of QElectroTech.

	QElectroTech is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 2 of the License, or
	(at your option) any later version.

	QElectroTech is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with QElectroTech. If not, see <http://www.gnu.org/licenses/>.
*/
#ifndef ConductorNumExport_H
#define ConductorNumExport_H

#include <QHash>

class QETProject;
class QWidget;

/**
	@brief The ConductorNumExport class
	A class to export the num of conductors into a csv file.
*/
class ConductorNumExport
{
	public:
	ConductorNumExport(QETProject *project, QWidget *parent = nullptr);
	bool toCsv();
	QString wiresNum() const;
	
	private:
	void fillHash();
	
	private:
	QETProject *m_project = nullptr;
	QHash <QString, int> m_hash;
	QWidget *m_parent_widget = nullptr;
};

#endif // ConductorNumExport_H
