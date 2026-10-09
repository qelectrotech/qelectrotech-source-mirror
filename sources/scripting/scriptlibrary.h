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
#ifndef SCRIPTLIBRARY_H
#define SCRIPTLIBRARY_H

#include "scriptheader.h"

#include <QFileSystemWatcher>
#include <QIcon>
#include <QList>
#include <QObject>
#include <QTimer>

/**
	@brief The ScriptLibrary class
	The user's stored scripts: every .js file with a ScriptHeader in
	folder(). The folder is the whole interface. A script written by hand
	in any editor, one saved by the script manager and one installed by an
	assistant through the qet MCP server all arrive the same way, as a file,
	and the library notices it with a QFileSystemWatcher and emits changed().

	One instance for the application, shared by every editor window.
*/
class ScriptLibrary : public QObject
{
	Q_OBJECT

	public:
		struct Script
		{
			ScriptHeader header;
			QString path;
		};

		static ScriptLibrary &instance();
		static QString folder();
		static QString actionId(const QString &script_id);
		static QIcon icon(const Script &script);

		QList<Script> scripts() const;
		QStringList errors() const;

	signals:
		void changed();

	private:
		ScriptLibrary();
		void rescan();
		void watch();

		QFileSystemWatcher m_watcher;
		QTimer m_rescan_timer;
		QList<Script> m_scripts;
		QStringList m_errors;
};

#endif // SCRIPTLIBRARY_H
