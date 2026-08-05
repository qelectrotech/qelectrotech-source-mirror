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
#ifndef SHORTCUTMANAGER_H
#define SHORTCUTMANAGER_H

#include <QHash>
#include <QKeySequence>
#include <QList>
#include <QPointer>
#include <QString>
#include <QStringList>

class QObject;

/**
	@brief The ShortcutManager class
	App-wide registry of every rebindable shortcut, carried by either a
	QAction or a QAbstractButton -- both declare an identical "shortcut"
	Q_PROPERTY of type QKeySequence, which this class targets generically via
	QObject so it doesn't need a separate code path for the handful of
	QAbstractButton-based shortcuts (e.g. a QPushButton acting as a shortcut-
	only trigger).

	Every place in the code that used to call setShortcut() directly now
	calls registerAction() instead, giving each shortcut a stable string id, a
	category (for grouping in the UI) and a hardcoded default sequence.
	registerAction() applies the user's saved override for that id (if any),
	falling back to the default, and remembers the target so the Shortcuts
	configuration page can list, edit and persist it.

	Several targets can share the same id at once, since QET allows several
	windows of the same kind (diagram editor, element editor...) to be open
	simultaneously, each with its own QAction. setSequence() updates every
	live target for a given id in one call.
*/
class ShortcutManager
{
	public:
		static ShortcutManager &instance();

		struct ShortcutInfo {
			QString id;
			QString category;
			QString description;
			QKeySequence default_sequence;
			QKeySequence current_sequence;
		};

		void registerAction(QObject *target, const QString &id,
				     const QString &category,
				     const QKeySequence &default_sequence);

		QList<ShortcutInfo> allShortcuts() const;
		QKeySequence currentSequence(const QString &id) const;
		void setSequence(const QString &id, const QKeySequence &sequence);
		void resetToDefault(const QString &id);
		void resetAllToDefaults();

			/// Activate the first still-alive target registered under \a id
			/// -- QAction::trigger() or QAbstractButton::click(), whichever
			/// it turns out to be -- for an input source other than the
			/// keyboard (e.g. a 3D mouse button) that wants to invoke a
			/// named action without knowing or caring which of the two it
			/// is. Deliberately not disambiguated by the currently active
			/// window when an id has several live targets: a target's
			/// owning top-level window isn't reliably discoverable from a
			/// bare QAction. Correct in the overwhelming common case of one
			/// open editor window; a known simplification in the rarer
			/// multi-window case, not a guaranteed-correct dispatch.
			/// @return whether a live target was found and triggered.
		bool trigger(const QString &id) const;

	private:
		ShortcutManager() = default;
		ShortcutManager(const ShortcutManager &) = delete;
		void operator=(const ShortcutManager &) = delete;

		struct Entry {
			QString category;
			QString description;
			QKeySequence default_sequence;
			QList<QPointer<QObject>> targets;
		};

		QKeySequence savedSequence(const QString &id, const QKeySequence &default_sequence) const;

		QHash<QString, Entry> m_entries;
		QStringList m_order;
};

#endif // SHORTCUTMANAGER_H
