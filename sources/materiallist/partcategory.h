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
#ifndef PARTCATEGORY_H
#define PARTCATEGORY_H

#include "materiallist.h"

#include <QString>
#include <QStringList>

/**
	@brief The PartCategory namespace
	The fixed list of categories of a part, to filter a long material
	list. The "category" column holds the stable English key
	(power_cable, contactor...); what is shown is its translation.
*/
namespace PartCategory
{
		/// The keys, in the order they are offered; "other" is the last
	QStringList keys();
		/// The translated name of @p key; @p key itself when unknown
	QString translated(const QString &key);
		/// The key @p text names: a key, the English or the translated
		/// name, case ignored; empty when it is none of them
	QString keyOf(const QString &text);
		/// The category of @p record: its "category" column when that
		/// names a known one, else the one its device type and description
		/// suggest, else "other"
	QString categoryOf(const MaterialRecord &record);
}

#endif // PARTCATEGORY_H
