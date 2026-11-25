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
#ifndef REVISIONSCONFIGPAGE_H
#define REVISIONSCONFIGPAGE_H

#include "projectconfigpages.h"

class QTableWidget;
class QPushButton;

/**
	@brief The RevisionsConfigPage class
	The project's revision history (#1295, rule 2; #1878 item C8): one
	row per revision -- revision, date, description, author, approver --
	added, removed and edited in place.
*/
class RevisionsConfigPage : public ProjectConfigPage
{
	Q_OBJECT
	public:
		RevisionsConfigPage(QETProject *project, QWidget *parent = nullptr);

		QString title() const override;
		QIcon icon() const override;
		void applyProjectConf() override;

	protected:
		void keyPressEvent(QKeyEvent *event) override;
		void initWidgets() override;
		void initLayout() override;
		void readValuesFromProject() override;
		void adjustReadOnly() override;

	private:
		void addRevision();
		void removeRevision();

		QTableWidget *m_table = nullptr;
		QPushButton *m_add = nullptr;
		QPushButton *m_remove = nullptr;
};

#endif // REVISIONSCONFIGPAGE_H
