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
#ifndef LEGACYELEMENTTYPES_H
#define LEGACYELEMENTTYPES_H

#include <QRegularExpression>
#include <QString>

/**
	Element type names in queries saved by versions before June 2022.
	The project database then held Element::linkTypeToString() ("Simple",
	"Terminale", ...); since it holds ElementData::typeToString()
	("simple", "terminal", ...). SQLite compares text case-sensitively, so
	a nomenclature table saved with element_type = 'Simple' silently lost
	every simple element, and its continuation tables were deleted as
	empty.
*/
namespace LegacyElementTypes
{
	/**
		@return query with every element_type = '<old name>' comparison
		rewritten to the current name. Anything else is left as it is,
		so a query saved by a current version comes back unchanged.
	*/
	inline QString upgradeQuery(const QString &query)
	{
		static const QRegularExpression re(QStringLiteral(
			"element_type\\s*=\\s*'(Simple|NextReport|PreviousReport|Master"
			"|Slave|Terminale|Thumbnail)'"));

		QString upgraded = query;
		auto it = re.globalMatch(query);
		int shift = 0;
		while (it.hasNext())
		{
			const auto match = it.next();
			const QString old_name = match.captured(1);
			QString new_name;
			if      (old_name == QLatin1String("Simple"))         new_name = QStringLiteral("simple");
			else if (old_name == QLatin1String("NextReport"))     new_name = QStringLiteral("next_report");
			else if (old_name == QLatin1String("PreviousReport")) new_name = QStringLiteral("previous_report");
			else if (old_name == QLatin1String("Master"))         new_name = QStringLiteral("master");
			else if (old_name == QLatin1String("Slave"))          new_name = QStringLiteral("slave");
			else if (old_name == QLatin1String("Terminale"))      new_name = QStringLiteral("terminal");
			else                                                  new_name = QStringLiteral("thumbnail");

			upgraded.replace(match.capturedStart(1) + shift, old_name.size(), new_name);
			shift += new_name.size() - old_name.size();
		}
		return upgraded;
	}
}

#endif // LEGACYELEMENTTYPES_H
