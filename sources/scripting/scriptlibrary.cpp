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
#include "scriptlibrary.h"

#include "../qetapp.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QGuiApplication>
#include <QPainter>
#include <QPalette>
#include <QPixmap>

ScriptLibrary &ScriptLibrary::instance()
{
	static ScriptLibrary library;
	return library;
}

/**
	@brief ScriptLibrary::folder
	Where stored scripts live: "scripts" in the user's data folder, next to
	the user's own element and title block collections.
*/
QString ScriptLibrary::folder()
{
	return QETApp::dataDir() + QStringLiteral("/scripts");
}

/**
	@brief ScriptLibrary::actionId
	The ShortcutManager id of a script's action. The "diagrameditor."
	prefix is what puts it among the commands the shortcut bar can show.
*/
QString ScriptLibrary::actionId(const QString &script_id)
{
	return QStringLiteral("diagrameditor.script.") + script_id;
}

ScriptLibrary::ScriptLibrary()
{
		//An editor saving a file often writes it more than once, or
		//replaces it through a rename: wait for it to settle.
	m_rescan_timer.setSingleShot(true);
	m_rescan_timer.setInterval(250);
	connect(&m_rescan_timer, &QTimer::timeout, this, &ScriptLibrary::rescan);
	connect(&m_watcher, &QFileSystemWatcher::directoryChanged,
		&m_rescan_timer, qOverload<>(&QTimer::start));
	connect(&m_watcher, &QFileSystemWatcher::fileChanged,
		&m_rescan_timer, qOverload<>(&QTimer::start));
	rescan();
}

/**
	@brief ScriptLibrary::watch
	Watch the folder and each script in it, or, while the folder does not
	exist yet, the data folder it will be created in.
*/
void ScriptLibrary::watch()
{
	if (!m_watcher.files().isEmpty()) m_watcher.removePaths(m_watcher.files());
	if (!m_watcher.directories().isEmpty()) m_watcher.removePaths(m_watcher.directories());

	const QDir dir(folder());
	if (!dir.exists()) {
		if (QFileInfo::exists(QETApp::dataDir())) {
			m_watcher.addPath(QETApp::dataDir());
		}
		return;
	}
	m_watcher.addPath(dir.path());
	QStringList files;
	for (const Script &s : std::as_const(m_scripts)) files << s.path;
	for (const QString &e : std::as_const(m_errors)) files << dir.filePath(e.section(QLatin1Char(':'), 0, 0));
	if (!files.isEmpty()) m_watcher.addPaths(files);
}

void ScriptLibrary::rescan()
{
	QList<Script> scripts;
	QStringList errors;
	const QDir dir(folder());
	const QFileInfoList infos = dir.entryInfoList({QStringLiteral("*.js")},
						      QDir::Files, QDir::Name);
	for (const QFileInfo &info : infos) {
		QFile file(info.filePath());
		if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) continue;
		const ScriptHeader header = ScriptHeader::parse(
			QString::fromUtf8(file.readAll()), info.completeBaseName());
		if (header.isValid()) {
			scripts << Script{header, info.filePath()};
		} else {
			errors << info.fileName() + QStringLiteral(": ") + header.error;
		}
	}

	auto signature = [](const QList<Script> &list) {
		QStringList parts;
		for (const Script &s : list) {
			const ScriptHeader &h = s.header;
			parts << QStringList{s.path, h.name, h.icon, h.tooltip, h.shortcut,
					     h.context}.join(QLatin1Char('\x1f'));
		}
		return parts;
	};
	const bool same = signature(scripts) == signature(m_scripts) && errors == m_errors;
	m_scripts = scripts;
	m_errors = errors;
	watch();
		//The script's own text is read again on every run, so only a
		//change to what its button shows needs the actions rebuilt --
		//except an icon file edited in place, which this cannot see; the
		//next change to the folder picks it up.
	if (!same) emit changed();
}

QList<ScriptLibrary::Script> ScriptLibrary::scripts() const
{
	return m_scripts;
}

/**
	@brief ScriptLibrary::errors
	One "file.js: reason" per script whose header could not be used, so
	whoever wrote it can be told why its button is missing.
*/
QStringList ScriptLibrary::errors() const
{
	return m_errors;
}

/**
	@brief ScriptLibrary::icon
	The icon a script's header names -- "builtin:<name>" from the icon
	theme, or an image file, relative to the script -- or, failing that, a
	tile with the script's initials, so every button can be told apart.
*/
QIcon ScriptLibrary::icon(const Script &script)
{
	const QString spec = script.header.icon;
	if (spec.startsWith(QLatin1String("builtin:"))) {
		const QIcon themed = QIcon::fromTheme(spec.mid(8));
		if (!themed.isNull()) return themed;
	} else if (!spec.isEmpty()) {
		const QString path = QFileInfo(script.path).dir().absoluteFilePath(spec);
		if (QFileInfo::exists(path)) {
			const QIcon file_icon(path);
			if (!file_icon.availableSizes().isEmpty()
			    || !file_icon.pixmap(32).isNull()) {
				return file_icon;
			}
		}
	}

	QString initials;
	for (const QString &word : script.header.name.split(QLatin1Char(' '), Qt::SkipEmptyParts)) {
		if (word.at(0).isLetterOrNumber()) initials += word.at(0).toUpper();
		if (initials.size() == 2) break;
	}
	if (initials.isEmpty()) initials = QStringLiteral("JS");

	const QPalette palette = QGuiApplication::palette();
	QPixmap pixmap(64, 64);
	pixmap.fill(Qt::transparent);
	QPainter painter(&pixmap);
	painter.setRenderHint(QPainter::Antialiasing);
	painter.setPen(Qt::NoPen);
	painter.setBrush(palette.color(QPalette::Highlight));
	painter.drawRoundedRect(QRectF(4, 4, 56, 56), 10, 10);
	QFont font = painter.font();
	font.setBold(true);
	font.setPixelSize(initials.size() == 1 ? 34 : 26);
	painter.setFont(font);
	painter.setPen(palette.color(QPalette::HighlightedText));
	painter.drawText(QRectF(4, 4, 56, 56), Qt::AlignCenter, initials);
	painter.end();
	return QIcon(pixmap);
}
