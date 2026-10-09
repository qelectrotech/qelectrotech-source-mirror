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

#include <QList>
#include <QSettings>
#include <QString>
#include <QStringList>

/**
	Configuration profiles (discussion #610): the settings of QElectroTech
	saved to a file, and loaded back from one.

	Two kinds of keys stay out of a profile, because they describe this
	computer rather than how the user wants QElectroTech to behave: window
	sizes, positions and dock layouts (".../geometry", ".../state",
	"dialoggeometry/..."), and the recent-files lists ("...-recentfiles/..."). They are not written to
	the file, and loading a profile keeps the current ones.

	A file can hold every other setting (a complete profile), or only some
	parts of them (discussion #1405), for example the defaults for new
	projects alone, to hand to someone else. Loading a file replaces only
	the parts it holds and keeps every other setting.
*/
namespace ConfigProfile
{
		/// Key written to every exported file, so that loading refuses a
		/// file that was not exported by QElectroTech. Never copied into
		/// the live settings. 1 = a complete profile, 2 = some parts only.
		/// A QElectroTech that only knows 1 refuses a file of some parts:
		/// it would load it as a complete one and lose every other setting.
	inline const QString marker_key{QStringLiteral("qelectrotech-configuration/format")};

		/// Key listing the parts a file of format 2 holds, see partName().
	inline const QString parts_key{QStringLiteral("qelectrotech-configuration/parts")};

		/// The parts the settings are split into.
	enum class Part {
		Controls,	///< toolbars, keyboard shortcuts, shortcut bar, mouse and trackpad gestures
		NewProject,	///< defaults for new projects: folio, title block, wires, numbering...
		Other,		///< everything else: appearance, grid, language...
		Folders		///< folders of the collections, title blocks and macros
	};

		/// The parts a user can choose to save. Folders are not one of
		/// them: they are only saved in a complete profile, because they
		/// rarely exist on another computer.
	inline QList<Part> choosableParts()
	{
		return {Part::Controls, Part::NewProject, Part::Other};
	}

		/// True for a key that stays out of a profile (see above).
	inline bool isLocalKey(const QString &key)
	{
		return key.endsWith(QLatin1String("/geometry"))
			|| key.endsWith(QLatin1String("/state"))
			|| key.startsWith(QLatin1String("dialoggeometry/"))
			|| key.contains(QLatin1String("-recentfiles/"))
			|| key == marker_key
			|| key == parts_key;
	}

		/// The part @a key belongs to. Every key belongs to exactly one
		/// part: a key not listed here is Other.
	inline Part partOf(const QString &key)
	{
		static const QStringList controls{
			QStringLiteral("shortcuts/"),
			QStringLiteral("toolbars/"),
			QStringLiteral("diagrameditor/toolbars/"),
			QStringLiteral("diagrameditor/custom_toolbars/"),
			QStringLiteral("diagrameditor/shortcut_bar/"),
			QStringLiteral("diagrameditor/gestures/"),
			QStringLiteral("diagrameditor/mouse_gestures"),
			QStringLiteral("diagrameditor/context_toolbar"),
			QStringLiteral("diagramview/gestures")};
		static const QStringList new_project{
				//BorderProperties, TitleBlockProperties, ConductorProperties,
				//report, cross-reference and guide defaults all start so
			QStringLiteral("diagrameditor/default"),
			QStringLiteral("autonum/")};

		for (const QString &prefix : controls) {
			if (key.startsWith(prefix)) return Part::Controls;
		}
		for (const QString &prefix : new_project) {
			if (key.startsWith(prefix)) return Part::NewProject;
		}
		if (key.startsWith(QLatin1String("elements-collections/"))
			&& key.endsWith(QLatin1String("-path"))) {
			return Part::Folders;
		}
		return Part::Other;
	}

		/// Name of @a part in the parts_key of a file.
	inline QString partName(Part part)
	{
		switch (part) {
			case Part::Controls:   return QStringLiteral("controls");
			case Part::NewProject: return QStringLiteral("newproject");
			case Part::Other:      return QStringLiteral("other");
			case Part::Folders:    return QStringLiteral("folders");
		}
		return QString();
	}

		/// True if @a file was exported by exportTo().
	inline bool isProfile(const QSettings &file)
	{
		const int format = file.value(marker_key).toInt();
		return format == 1 || format == 2;
	}

		/// The parts @a file holds: every part for a complete profile.
	inline QList<Part> partsOf(const QSettings &file)
	{
		const QList<Part> all{Part::Controls, Part::NewProject,
							  Part::Other, Part::Folders};
		if (file.value(marker_key).toInt() != 2) {
			return all;
		}
		const QStringList names = file.value(parts_key).toString()
								  .split(QLatin1Char(','), Qt::SkipEmptyParts);
		QList<Part> parts;
		for (Part part : all) {
			if (names.contains(partName(part))) {
				parts << part;
			}
		}
		return parts;
	}

		/// Replace the contents of @a file with the keys of @a live that
		/// belong to @a parts, except the local ones. With every choosable
		/// part, the file is a complete profile, folders included. Returns
		/// the number of keys written.
	inline int exportTo(const QSettings &live, QSettings &file,
						QList<Part> parts = choosableParts())
	{
		const bool complete = parts.contains(Part::Controls)
							  && parts.contains(Part::NewProject)
							  && parts.contains(Part::Other);
		if (complete) {
			parts << Part::Folders;
		} else {
			parts.removeAll(Part::Folders);
		}

		file.clear();
		int written = 0;
		const QStringList keys = live.allKeys();
		for (const QString &key : keys) {
			if (isLocalKey(key) || !parts.contains(partOf(key))) {
				continue;
			}
			file.setValue(key, live.value(key));
			++written;
		}
		if (complete) {
			file.setValue(marker_key, 1);
		} else {
			QStringList names;
			for (Part part : std::as_const(parts)) {
				names << partName(part);
			}
			file.setValue(marker_key, 2);
			file.setValue(parts_key, names.join(QLatin1Char(',')));
		}
		file.sync();
		return written;
	}

		/// Replace every key of @a live that belongs to a part @a file
		/// holds, except the local ones, with the keys of @a file. A key
		/// of such a part that is not in @a file is removed rather than
		/// kept: loading a profile must not leave behind a setting the
		/// previous profile made. Keys of the other parts are kept. Local
		/// keys in @a file are ignored.
	inline void importFrom(const QSettings &file, QSettings &live)
	{
		const QList<Part> parts = partsOf(file);
		const QStringList live_keys = live.allKeys();
		for (const QString &key : live_keys) {
			if (!isLocalKey(key) && parts.contains(partOf(key))) {
				live.remove(key);
			}
		}
		const QStringList file_keys = file.allKeys();
		for (const QString &key : file_keys) {
			if (!isLocalKey(key) && parts.contains(partOf(key))) {
				live.setValue(key, file.value(key));
			}
		}
		live.sync();
	}

		/// True if loading @a parts needs QElectroTech to start again.
		/// The defaults for new projects are read again for every new
		/// project; every other part is read once, at start.
	inline bool needsRestart(const QList<Part> &parts)
	{
		for (Part part : parts) {
			if (part != Part::NewProject) {
				return true;
			}
		}
		return false;
	}
}

#endif // CONFIGPROFILE_H
