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
#ifndef LIVESERVER_H
#define LIVESERVER_H

#include <QJsonObject>
#include <QObject>
#include <QPointer>

class QLocalServer;
class QLocalSocket;
class QETDiagramEditor;
class QWidget;

/**
	@brief The LiveServer class
	Live mode: an AI assistant, through the qet MCP server, runs scripts on
	the project open in this QElectroTech while the user watches.

	Three things must all be true before anything can connect: scripting is
	allowed, the "mode direct" setting is on (off by default), and the user
	accepted the warning shown at this start. With the setting off this
	class opens nothing and QElectroTech behaves as if it did not exist.

	The channel is a QLocalServer only the user's own account can open,
	with a random name and token written to the "live" part of
	qet-assistant.json (AssistantInfo), which the MCP server reads, and
	cleared when the channel closes. One JSON request per connection, one JSON answer back, then
	the server closes the connection; every request carries the token.

	Requests are never handled inside the socket's readyRead: each is
	queued to the event loop first. QETApp::receiveMessage() learnt why --
	a modal dialog opened from inside a socket handler crashed QElectroTech
	once the socket went away (PR #861).
*/
class LiveServer : public QObject
{
	Q_OBJECT

	public:
		enum State { Off, Waiting, Connected };

		static LiveServer &instance();

		void askAndStart(QWidget *parent);
		void stop();
		State state() const { return m_state; }
		bool askFirst() const { return m_ask_first; }
		void setAskFirst(bool ask);
		static QStringList allowedCommands();

	signals:
		void stateChanged(LiveServer::State state);
			/// A request was handled: what was asked, and the answer sent
		void handled(const QJsonObject &request, const QJsonObject &answer);
		void askFirstChanged(bool ask);

	private:
		LiveServer();
		~LiveServer() override;
		bool start();
		void setState(State state);
		void newConnection();
		void readClient();
		void handle(const QJsonObject &request);
		void send(const QJsonObject &answer);
		QJsonObject status();
		QJsonObject runScript(const QString &name, const QString &source);
		QJsonObject runStored(const QString &id);
		QJsonObject command(const QString &id);
		QJsonObject showFolio(int folio);
		QJsonObject undoLast();
		QJsonObject screenshot();
		bool confirm(const QString &name, const QString &source);
		QETDiagramEditor *editor() const;

		QLocalServer *m_server = nullptr;
		QPointer<QLocalSocket> m_client;
		QByteArray m_buffer;
		QString m_token;
		State m_state = Off;
		bool m_asked = false;
		bool m_busy = false;
		bool m_ask_first = true;	///< per session, never saved: every start asks again
};

#endif // LIVESERVER_H
