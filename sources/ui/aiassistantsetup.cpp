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
#include "aiassistantsetup.h"

#include <QDir>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>

namespace {

/**
	The first of \a candidates that is a file, as a clean absolute path,
	or an empty string.
*/
QString firstFile(const QStringList &candidates)
{
	for (const QString &c : candidates) {
		const QFileInfo info(c);
		if (info.isFile())
			return QDir::cleanPath(info.absoluteFilePath());
	}
	return QString();
}

QJsonObject environment(const AiAssistantSetup::Paths &paths,
			const QString &workspace,
			bool allow_edit)
{
	QJsonObject env;
	env.insert(QStringLiteral("QET_MCP_WORKSPACE"), QDir::toNativeSeparators(workspace));
	env.insert(QStringLiteral("QET_BINARY"), QDir::toNativeSeparators(paths.qet_binary));
	if (allow_edit)
		env.insert(QStringLiteral("QET_ENABLE_SCRIPTING"), QStringLiteral("1"));
	return env;
}

/// A TOML basic string: backslashes and quotes escaped.
QString toml(const QString &s)
{
	QString out = s;
	out.replace(QLatin1Char('\\'), QStringLiteral("\\\\"));
	out.replace(QLatin1Char('"'), QStringLiteral("\\\""));
	return QLatin1Char('"') + out + QLatin1Char('"');
}

} // namespace

QList<AiAssistantSetup::Client> AiAssistantSetup::clients()
{
	return {Client::ClaudeDesktop, Client::ClaudeCode, Client::VsCode,
		Client::Cursor, Client::GeminiCli, Client::CodexCli,
		Client::LmStudio};
}

/**
	@brief AiAssistantSetup::detect
	Finds the server beside this QElectroTech, in the two places its
	packages put it: <root>/mcp/ beside <root>/bin/ (the Windows installers
	and portable folder) and <prefix>/share/qelectrotech/mcp/ beside
	<prefix>/bin/ (make install). On Windows the Python the installer can
	add is preferred to one on PATH.
	@param application_dir : QCoreApplication::applicationDirPath()
	@param application_file : QCoreApplication::applicationFilePath()
	@param windows : true on Windows, where Python is "python", not "python3"
	@param find_executable : looks a program up on PATH; empty means
	QStandardPaths::findExecutable(). Given in tests.
*/
AiAssistantSetup::Paths AiAssistantSetup::detect(const QString &application_dir,
						 const QString &application_file,
						 bool windows,
						 const ExecutableFinder &find_executable)
{
	const QDir bin(application_dir);
	Paths paths;
	paths.qet_binary = QDir::cleanPath(application_file);
	paths.server = firstFile({
		bin.filePath(QStringLiteral("../mcp/qet_mcp.py")),
		bin.filePath(QStringLiteral("../share/qelectrotech/mcp/qet_mcp.py"))});

	paths.python = windows ? QStringLiteral("python") : QStringLiteral("python3");
	if (windows) {
		const QString bundled = firstFile({
			bin.filePath(QStringLiteral("../mcp/python/python.exe"))});
		if (!bundled.isEmpty()) {
			paths.python = bundled;
			paths.python_status = PythonStatus::Found;
			return paths;
		}
	}

	const ExecutableFinder find = find_executable
			? find_executable
			: ExecutableFinder([](const QString &name) {
				  return QStandardPaths::findExecutable(name); });
	const QString found = find(paths.python);
	if (found.isEmpty())
		paths.python_status = PythonStatus::Missing;
	// Windows 10 and 11 put a python.exe in WindowsApps that only opens the
	// Microsoft Store. A Python installed from the Store answers from there
	// too, so this cannot tell the two apart: say which it may be.
	else if (windows && QString(found).replace(QLatin1Char('\\'), QLatin1Char('/'))
			 .contains(QLatin1String("/WindowsApps/"), Qt::CaseInsensitive))
		paths.python_status = PythonStatus::StoreShortcut;
	else
		paths.python_status = PythonStatus::Found;
	return paths;
}

/**
	@brief AiAssistantSetup::configuration
	@return the text to paste into \a client's configuration: JSON for all
	but Codex CLI, which reads TOML. Windows paths come out with native
	separators, escaped as each format requires.
*/
QString AiAssistantSetup::configuration(Client client,
					const Paths &paths,
					const QString &workspace,
					bool allow_edit)
{
	const QString python = QDir::toNativeSeparators(paths.python);
	const QString server = QDir::toNativeSeparators(paths.server);
	const QJsonObject env = environment(paths, workspace, allow_edit);

	if (client == Client::CodexCli) {
		QString text = QStringLiteral("[mcp_servers.qet]\n");
		text += QStringLiteral("command = ") + toml(python) + QLatin1Char('\n');
		text += QStringLiteral("args = [") + toml(server) + QStringLiteral("]\n\n");
		text += QStringLiteral("[mcp_servers.qet.env]\n");
		for (auto it = env.constBegin(); it != env.constEnd(); ++it)
			text += it.key() + QStringLiteral(" = ") + toml(it.value().toString()) + QLatin1Char('\n');
		return text;
	}

	QJsonObject server_entry;
	if (client == Client::VsCode || client == Client::Cursor)
		server_entry.insert(QStringLiteral("type"), QStringLiteral("stdio"));
	server_entry.insert(QStringLiteral("command"), python);
	server_entry.insert(QStringLiteral("args"), QJsonArray{server});
	server_entry.insert(QStringLiteral("env"), env);

	// VS Code names the list "servers"; every other client, "mcpServers".
	const QString key = client == Client::VsCode ? QStringLiteral("servers")
						      : QStringLiteral("mcpServers");
	const QJsonObject root{{key, QJsonObject{{QStringLiteral("qet"), server_entry}}}};
	return QString::fromUtf8(QJsonDocument(root).toJson(QJsonDocument::Indented));
}
