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
#ifndef CONFIGPROFILE_H
#define CONFIGPROFILE_H

#include <QSettings>
#include <QString>
#include <QStringList>

/**
	Configuration profiles (discussion #610): the settings of QElectroTech
	saved to a file, and loaded back from one.

	Two kinds of keys stay out of a profile, because they describe this
	computer rather than how the user wants QElectroTech to behave: window
	sizes, positions and dock layouts (".../geometry", ".../state"), and
	the recent-files lists ("...-recentfiles/..."). They are not written to
	the file, and loading a profile keeps the current ones.
*/
namespace ConfigProfile
{
		/// Key written to every exported file, so that loading refuses a
		/// file that was not exported by QElectroTech. Never copied into
		/// the live settings.
	inline const QString marker_key{QStringLiteral("qelectrotech-configuration/format")};

		/// True for a key that stays out of a profile (see above).
	inline bool isLocalKey(const QString &key)
	{
		return key.endsWith(QLatin1String("/geometry"))
			|| key.endsWith(QLatin1String("/state"))
			|| key.contains(QLatin1String("-recentfiles/"))
			|| key == marker_key;
	}

		/// True if @a file was exported by exportTo().
	inline bool isProfile(const QSettings &file)
	{
		return file.value(marker_key).toInt() == 1;
	}

		/// Replace the contents of @a file with every key of @a live,
		/// except the local ones. Returns the number of keys written.
	inline int exportTo(const QSettings &live, QSettings &file)
	{
		file.clear();
		int written = 0;
		const QStringList keys = live.allKeys();
		for (const QString &key : keys) {
			if (isLocalKey(key)) {
				continue;
			}
			file.setValue(key, live.value(key));
			++written;
		}
		file.setValue(marker_key, 1);
		file.sync();
		return written;
	}

		/// Replace every key of @a live, except the local ones, with the
		/// keys of @a file. A key that is not in @a file is removed rather
		/// than kept: loading a profile must not leave behind a setting
		/// the previous profile made. Local keys in @a file are ignored.
	inline void importFrom(const QSettings &file, QSettings &live)
	{
		const QStringList live_keys = live.allKeys();
		for (const QString &key : live_keys) {
			if (!isLocalKey(key)) {
				live.remove(key);
			}
		}
		const QStringList file_keys = file.allKeys();
		for (const QString &key : file_keys) {
			if (!isLocalKey(key)) {
				live.setValue(key, file.value(key));
			}
		}
		live.sync();
	}
}

#endif // CONFIGPROFILE_H
