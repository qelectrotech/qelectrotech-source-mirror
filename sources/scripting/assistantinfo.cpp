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
#include "assistantinfo.h"

#include "qetscriptapi.h"
#include "scriptlibrary.h"
#include "macrorecorder.h"
#include "../qetapp.h"
#include "../qetversion.h"
#include "../utils/qetsettings.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QSaveFile>
#include <QStandardPaths>

namespace {
	QJsonObject s_live;	///< empty while live mode is closed
	bool s_running = true;
	bool s_watching = false;	///< this QElectroTech writes the file (an editor opened)
}

namespace AssistantInfo {

QString path()
{
	return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
			+ QStringLiteral("/qet-assistant.json");
}

/**
	@brief watch
	Write the file now, again whenever the stored scripts change, and mark
	it stopped when QElectroTech quits. Called by every editor window as it
	opens; acts once.
*/
void watch()
{
	if (s_watching) return;
	s_watching = true;
	QObject::connect(&ScriptLibrary::instance(), &ScriptLibrary::changed, &write);
	QObject::connect(qApp, &QCoreApplication::aboutToQuit, &markStopped);
	write();
}

/**
	@brief refresh
	Write the file again after a setting it carries changed, but only in a
	QElectroTech that writes it at all: a headless --run never opened an
	editor, and writing from it would replace the open QElectroTech's live
	channel with its own empty one.
*/
void refresh()
{
	if (s_watching) write();
}

void write()
{
	QJsonArray scripts, refused;
	for (const ScriptLibrary::Script &s : ScriptLibrary::instance().scripts()) {
		const ScriptHeader &h = s.header;
		scripts.append(QJsonObject{
			{QStringLiteral("id"), h.id},
			{QStringLiteral("name"), h.name},
			{QStringLiteral("file"), s.path},
			{QStringLiteral("icon"), h.icon},
			{QStringLiteral("tooltip"), h.tooltip},
			{QStringLiteral("shortcut"), h.shortcut},
			{QStringLiteral("context"), h.context},
			{QStringLiteral("action_id"), ScriptLibrary::actionId(h.id)}});
	}
	for (const QString &e : ScriptLibrary::instance().errors()) refused.append(e);

	const QJsonObject info{
		{QStringLiteral("format"), 1},
		{QStringLiteral("qelectrotech_version"), QetVersion::displayedVersion()},
		{QStringLiteral("program"), QCoreApplication::applicationFilePath()},
		{QStringLiteral("pid"), QCoreApplication::applicationPid()},
		{QStringLiteral("running"), s_running},
		{QStringLiteral("written"), QDateTime::currentDateTimeUtc().toString(Qt::ISODate)},
		{QStringLiteral("folders"), QJsonObject{
			{QStringLiteral("data"), QETApp::dataDir()},
			{QStringLiteral("config"), QETApp::configDir()},
			{QStringLiteral("scripts"), ScriptLibrary::folder()},
			{QStringLiteral("recordings"), MacroRecorder::folder()},
			{QStringLiteral("elements_common"), QETApp::commonElementsDir()},
			{QStringLiteral("elements_company"), QETApp::companyElementsDir()},
			{QStringLiteral("elements_custom"), QETApp::customElementsDir()},
			{QStringLiteral("titleblocks_common"), QETApp::commonTitleBlockTemplatesDir()},
			{QStringLiteral("titleblocks_company"), QETApp::companyTitleBlockTemplatesDir()},
			{QStringLiteral("titleblocks_custom"), QETApp::customTitleBlockTemplatesDir()}}},
		{QStringLiteral("features"), QJsonObject{
#ifdef QET_HAS_SCRIPTING
			{QStringLiteral("scripting_available"), true},
#else
			{QStringLiteral("scripting_available"), false},
#endif
			{QStringLiteral("scripting_enabled"), QetSettings::scriptingEnabled()},
			{QStringLiteral("live_mode_setting"), QetSettings::liveAssistantEnabled()},
			{QStringLiteral("live_mode_open"), !s_live.isEmpty()},
			{QStringLiteral("macro_recording"), MacroRecorder::instance().isRecording()}}},
		{QStringLiteral("script_api"), QJsonArray::fromStringList(QetScriptApi::signatures())},
		{QStringLiteral("stored_scripts"), scripts},
		{QStringLiteral("refused_scripts"), refused},
		{QStringLiteral("recordings"), MacroRecorder::recordings()},
		{QStringLiteral("live"), s_live.isEmpty() ? QJsonValue() : QJsonValue(s_live)},
		// Free-text drawing conventions, set once by the user
		// (QetSettings::setHouseStyle(), qet.setHouseStyle()) and read
		// by any assistant that connects, live or headless, without it
		// having to ask the user or guess. Null, not "", when unset --
		// the two mean different things to a reader.
		{QStringLiteral("house_style"), QetSettings::houseStyle().isEmpty()
			? QJsonValue() : QJsonValue(QetSettings::houseStyle())}};

	const QString file_path = path();
	QDir().mkpath(QFileInfo(file_path).path());
	QSaveFile file(file_path);
	if (!file.open(QIODevice::WriteOnly)) return;
	file.write(QJsonDocument(info).toJson(QJsonDocument::Indented));
	if (file.commit())
		QFile::setPermissions(file_path, QFile::ReadOwner | QFile::WriteOwner);
}

/**
	@brief setLive
	The live channel's details while it is open (socket, token), or an
	empty object once it closes.
*/
void setLive(const QJsonObject &live)
{
	s_live = live;
	write();
}

/**
	@brief markStopped
	QElectroTech is quitting: the file stays, as a record of where things
	are, but says nothing is listening any more.
*/
void markStopped()
{
	s_running = false;
	s_live = QJsonObject();
	write();
}

}
