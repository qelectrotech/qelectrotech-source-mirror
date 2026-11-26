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
#ifndef LISTQUERYWIDGET_H
#define LISTQUERYWIDGET_H

#include <QCoreApplication>
#include <QLabel>
#include <QVBoxLayout>
#include <QWidget>

/**
	@brief The ListQueryWidget class
	What a list table shows, in the "Add a table" dialog: a list the
	project database already answers (qet_device_list, ...), as a table on
	a folio that follows it (#1878 B9-B11). Nothing to choose: the table's
	look is set by the dialog around it, and a table too long for its
	folio goes on to further linked tables as the parts list does.
	Saved as a database table: its identifier and its query.
*/
class ListQueryWidget : public QWidget
{
	public:
		enum Kind {
			Device ///< one row per device (qet_device_list)
		};

		explicit ListQueryWidget(Kind kind, QWidget *parent = nullptr) :
			QWidget(parent), m_kind(kind)
		{
			auto *layout = new QVBoxLayout(this);
			auto *text = new QLabel(description(kind), this);
			text->setWordWrap(true);
			layout->addWidget(text);
			layout->addStretch();
		}

		Kind kind() const {return m_kind;}

		static QString modelIdentifier(Kind kind)
		{
			switch (kind) {
				case Device: return QStringLiteral("device_list");
			}
			return QString();
		}

		static QString description(Kind kind)
		{
			switch (kind) {
				case Device: return QCoreApplication::translate("ListQueryWidget",
					"The table shows the project's devices, one row each: reference, "
					"description, designation, article, number of symbols and first "
					"sheet. It follows the labels and the information of the symbols.");
			}
			return QString();
		}

		static QString queryStr(Kind kind)
		{
			switch (kind) {
				case Device:
					return QStringLiteral(
						"SELECT COALESCE(structure_id, '') AS device, "
						"COALESCE(description, '') AS description, "
						"COALESCE(designation, '') AS designation, "
						"COALESCE(articles, '') AS articles, "
						"COALESCE(symbols, 0) AS symbols, "
						"COALESCE(folio_number, '') AS folio_number "
						"FROM qet_device_list ORDER BY structure_id");
			}
			return QString();
		}

	private:
		Kind m_kind;
};

#endif // LISTQUERYWIDGET_H
