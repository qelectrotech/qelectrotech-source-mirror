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
#ifndef REVISIONQUERYWIDGET_H
#define REVISIONQUERYWIDGET_H

#include <QCoreApplication>
#include <QLabel>
#include <QVBoxLayout>
#include <QWidget>

/**
	@brief The RevisionQueryWidget class
	What a revision table shows, in the "Add a table" dialog: the
	project's revision history (Project properties > Revisions, #1295
	rule 2), every revision, the latest last. Nothing to choose: the
	table's look is set by the dialog around it.
*/
class RevisionQueryWidget : public QWidget
{
	public:
		explicit RevisionQueryWidget(QWidget *parent = nullptr) : QWidget(parent)
		{
			auto *layout = new QVBoxLayout(this);
			auto *text = new QLabel(QCoreApplication::translate("RevisionQueryWidget",
				"The table shows the project's revision history: revision, date, "
				"description, author and approver, the latest last. Edit it in "
				"Project properties > Revisions."), this);
			text->setWordWrap(true);
			layout->addWidget(text);
			layout->addStretch();
		}

		static QString modelIdentifier() {return QStringLiteral("revisions");}
		static QString queryStr()
		{
			return QStringLiteral("SELECT revision, date, description, author, approver "
								  "FROM qet_revisions ORDER BY position");
		}
};

#endif // REVISIONQUERYWIDGET_H
