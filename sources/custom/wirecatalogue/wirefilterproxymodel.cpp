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
#include "wirefilterproxymodel.h"
#include "wirecataloguemodel.h"
#include "wirespec.h"

#include <QtMath>

WireFilterProxyModel::WireFilterProxyModel(QObject *parent) :
	QSortFilterProxyModel(parent)
{}

void WireFilterProxyModel::setColourFilter(const QString &colour)
{
	m_colour = colour.trimmed();
	invalidateFilter();
}

void WireFilterProxyModel::setSectionFilter(double section)
{
	m_section = section;
	invalidateFilter();
}

bool WireFilterProxyModel::filterAcceptsRow(int source_row,
											const QModelIndex &source_parent) const
{
	Q_UNUSED(source_parent)
	auto *model = qobject_cast<WireCatalogueModel*>(sourceModel());
	if (!model)
		return true;

	const WireSpec w = model->wireAt(source_row);

	if (!m_colour.isEmpty()) {
		bool match = false;
		const QStringList colours = w.allCoreColors() + QStringList{w.colorPrimary};
		for (const QString &c : colours) {
			if (c.compare(m_colour, Qt::CaseInsensitive) == 0) {
				match = true;
				break;
			}
		}
		if (!match)
			return false;
	}

	if (m_section >= 0.0 && !qFuzzyCompare(w.crossSectionMm2 + 1.0, m_section + 1.0))
		return false;

	return true;
}
