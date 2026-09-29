/*
	Copyright 2006-2026 The QElectroTech Team
	This file is part of QElectroTech.

	QElectroTech is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 2 of the License, or
	(at your option) any later version.

	QElectroTech is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with QElectroTech. If not, see <http://www.gnu.org/licenses/>.
*/
#ifndef AIASSISTANTSETUP_H
#define AIASSISTANTSETUP_H

#include <QString>
#include <QStringList>

#include <functional>

/**
	@brief What an AI assistant needs to start QElectroTech's MCP server
	(misc/qet-mcp), and the configuration text each assistant expects.

	No widgets and no application object, so it can be tested alone. It
	reads the file system and nothing else: it writes nothing and starts
	nothing. AiAssistantDialog shows its result.
*/
namespace AiAssistantSetup
{
	/// The assistants a configuration can be written for.
	enum class Client {
		ClaudeDesktop,
		ClaudeCode,
		VsCode,
		Cursor,
		GeminiCli,
		CodexCli,
		LmStudio
	};

	QList<Client> clients();

	/// Whether the Python the configuration names can actually run.
	enum class PythonStatus {
		Found,
		Missing,
		/// Only Windows' "python.exe" shortcut in WindowsApps, which opens
		/// the Microsoft Store when Python is not installed.
		StoreShortcut
	};

	/// Where the pieces are. An empty server means none was found.
	struct Paths {
		QString server;     ///< qet_mcp.py
		QString python;     ///< the Python to run it with
		QString qet_binary; ///< this QElectroTech
		PythonStatus python_status = PythonStatus::Missing;
	};

	/// Finds a program on PATH; QStandardPaths::findExecutable() by default.
	using ExecutableFinder = std::function<QString(const QString &)>;

	Paths detect(const QString &application_dir,
		     const QString &application_file,
		     bool windows,
		     const ExecutableFinder &find_executable = ExecutableFinder());

	QString configuration(Client client,
			      const Paths &paths,
			      const QString &workspace,
			      bool allow_edit);
}

#endif // AIASSISTANTSETUP_H
