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
#ifndef ASSISTANTINFO_H
#define ASSISTANTINFO_H

#include <QJsonObject>
#include <QString>

/**
	@brief qet-assistant.json: what an AI assistant (through the qet MCP
	server) needs to know about this QElectroTech, in one file.

	Where its folders are -- data, settings, scripts, the element and title
	block collections --, which features are switched on, every call a
	script can make, the stored scripts, and, while live mode is open, how
	to reach it. QElectroTech rewrites it when it starts and whenever one of
	those changes, so the MCP server reads facts instead of guessing them
	per platform -- wrong as soon as QElectroTech is started with
	--data-dir or --config-dir.

	It always lives in the standard data folder for the platform, even
	when --data-dir moves the data elsewhere: that is the one place the
	server can look without being told. Readable by the user only: while
	live mode is open it holds the channel's token.
*/
namespace AssistantInfo
{
	QString path();
	void watch();
	void write();
	void setLive(const QJsonObject &live);
	void markStopped();
}

#endif // ASSISTANTINFO_H
