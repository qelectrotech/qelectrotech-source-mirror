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
#ifndef WIREFILTERPROXYMODEL_H
#define WIREFILTERPROXYMODEL_H

#include <QSortFilterProxyModel>

/**
	@brief Filters a WireCatalogueModel by colour and/or cross-section.
	Used by the conductor "Assign wires" tab so an engineer can narrow the
	catalogue to, e.g., all Red 10 mm² wires before assigning one. An empty
	colour or a negative section means "any".
*/
class WireFilterProxyModel : public QSortFilterProxyModel
{
	Q_OBJECT
	public:
		explicit WireFilterProxyModel(QObject *parent = nullptr);

		void setColourFilter(const QString &colour); // "" = any
		void setSectionFilter(double section);        // < 0 = any

	protected:
		bool filterAcceptsRow(int source_row,
							  const QModelIndex &source_parent) const override;

	private:
		QString m_colour;
		double  m_section = -1.0;
};

#endif // WIREFILTERPROXYMODEL_H
