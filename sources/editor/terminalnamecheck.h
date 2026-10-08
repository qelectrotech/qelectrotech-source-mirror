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
#ifndef TERMINALNAMECHECK_H
#define TERMINALNAMECHECK_H

#include <QHash>
#include <QList>
#include <QPair>
#include <QString>
#include <QStringList>

/**
	The terminal name check run when an element is saved, and by
	--check-elements. IEC 61666 requires every terminal to be identified
	unambiguously within its object: two terminals of one element must not
	share a name, and a terminal should have one.

	Names are compared after trimming spaces at both ends, and are case
	sensitive ("n" and "N" are different names).
*/
namespace TerminalNameCheck
{
		/// QSettings key: false turns the check off in the element editor.
	inline const QString settings_key{QStringLiteral("elementeditor/check-terminal-names")};

		/// The names used by more than one terminal, with how many terminals
		/// use each, in the order the names first appear. Unnamed terminals
		/// are not counted here.
	inline QList<QPair<QString, int>> repeatedNames(const QStringList &names)
	{
		QHash<QString, int> count;
		QStringList order;
		for (const QString &raw : names) {
			const QString name = raw.trimmed();
			if (name.isEmpty()) {
				continue;
			}
			if (!count.contains(name)) {
				order << name;
			}
			++count[name];
		}

		QList<QPair<QString, int>> repeated;
		for (const QString &name : order) {
			if (count.value(name) > 1) {
				repeated << qMakePair(name, count.value(name));
			}
		}
		return repeated;
	}

		/// The number of terminals without a name.
	inline int unnamedCount(const QStringList &names)
	{
		int unnamed = 0;
		for (const QString &name : names) {
			if (name.trimmed().isEmpty()) {
				++unnamed;
			}
		}
		return unnamed;
	}

		/// "N ×3, L ×2": the repeated names, for messages.
	inline QString describe(const QList<QPair<QString, int>> &repeated)
	{
		QStringList parts;
		for (const auto &entry : repeated) {
			parts << QStringLiteral("%1 ×%2").arg(entry.first, QString::number(entry.second));
		}
		return parts.join(QStringLiteral(", "));
	}
}

#endif // TERMINALNAMECHECK_H
