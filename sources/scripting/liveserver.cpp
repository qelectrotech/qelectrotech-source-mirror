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
#include "liveserver.h"

#include "assistantinfo.h"
#include "qetscripting.h"
#include "scriptlibrary.h"
#include "../diagram.h"
#include "../diagramview.h"
#include "../qetapp.h"
#include "../qetdiagrameditor.h"
#include "../qetproject.h"
#include "../utils/qetsettings.h"

#include <QApplication>
#include <QCoreApplication>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QLocalServer>
#include <QLocalSocket>
#include <QMessageBox>
#include <QPushButton>
#include <QRandomGenerator>
#include <QSaveFile>
#include <QTimer>
#include <QUndoStack>

namespace {
	QString randomHex(int bytes)
	{
		QByteArray raw(bytes, Qt::Uninitialized);
		for (char &c : raw) c = char(QRandomGenerator::system()->bounded(256));
		return QString::fromLatin1(raw.toHex());
	}

	QJsonObject failure(const QString &error)
	{
		return {{QStringLiteral("ok"), false}, {QStringLiteral("error"), error}};
	}
}

LiveServer &LiveServer::instance()
{
	static LiveServer server;
	return server;
}

LiveServer::LiveServer()
{
	connect(qApp, &QCoreApplication::aboutToQuit, this, &LiveServer::stop);
}

LiveServer::~LiveServer()
{
}

/**
	@brief LiveServer::askAndStart
	Called by every editor window as it opens; acts once per run. With the
	setting on, warn and let the user decide for this session before the
	channel exists.
*/
void LiveServer::askAndStart(QWidget *parent)
{
	if (m_asked) return;
	if (!QetSettings::liveAssistantEnabled()) {
		m_asked = true;
		return;
	}
		//Not on top of another start-up question (the backup copy, the
		//recovery files): two stacked boxes, and an answer meant for one
		//lands on the other. Ask once nothing else is waiting.
	if (QApplication::activeModalWidget()) {
		QPointer<QWidget> anchor(parent);
		QTimer::singleShot(300, this, [this, anchor]() { askAndStart(anchor); });
		return;
	}
	m_asked = true;

	QMessageBox box(QMessageBox::Warning, tr("Mode direct"),
			tr("Le mode direct est activé : un assistant IA connecté "
			   "pourra exécuter des scripts sur le projet ouvert.\n\n"
			   "Chaque action s'annule d'un Ctrl+Z, et le bouton "
			   "« Arrêter » de la barre d'état coupe la connexion.\n\n"
			   "Ce réglage se trouve dans Configurer QElectroTech > Général."),
			QMessageBox::NoButton, parent);
	QPushButton *go = box.addButton(tr("&Continuer"), QMessageBox::AcceptRole);
	box.addButton(tr("&Pas pour cette session"), QMessageBox::RejectRole);
	QPushButton *off = box.addButton(tr("&Désactiver"), QMessageBox::DestructiveRole);
	box.setDefaultButton(go);
	box.exec();

	if (box.clickedButton() == off) {
		QetSettings::setLiveAssistantEnabled(false);
	} else if (box.clickedButton() == go) {
		start();
	}
}

bool LiveServer::start()
{
	if (m_server) return true;
	m_server = new QLocalServer(this);
	m_server->setSocketOptions(QLocalServer::UserAccessOption);
	if (!m_server->listen(QStringLiteral("qet-live-") + randomHex(8))) {
		delete m_server;
		m_server = nullptr;
		return false;
	}
	m_token = randomHex(16);

		//Where the MCP server finds the channel: the "live" part of
		//qet-assistant.json, the one file it reads about this QElectroTech
	AssistantInfo::setLive(QJsonObject{
		{QStringLiteral("socket"), m_server->fullServerName()},
		{QStringLiteral("token"), m_token},
		{QStringLiteral("pid"), QCoreApplication::applicationPid()}});

	connect(m_server, &QLocalServer::newConnection, this, &LiveServer::newConnection);
	setState(Waiting);
	return true;
}

/**
	@brief LiveServer::stop
	Drop the assistant and close the channel for the rest of the session.
*/
void LiveServer::stop()
{
	if (m_client) {
		m_client->disconnectFromServer();
		m_client->deleteLater();
	}
	if (m_server) {
		m_server->close();
		m_server->deleteLater();
		m_server = nullptr;
	}
	if (!m_token.isEmpty()) AssistantInfo::setLive(QJsonObject());
	m_token.clear();
	m_buffer.clear();
	setState(Off);
}

void LiveServer::setState(State state)
{
	if (state == m_state) return;
	m_state = state;
	emit stateChanged(state);
}

void LiveServer::newConnection()
{
	while (QLocalSocket *socket = m_server ? m_server->nextPendingConnection() : nullptr) {
		if (m_client) {
			socket->write(QJsonDocument(failure(QStringLiteral(
				"another assistant is already connected"))).toJson(QJsonDocument::Compact) + '\n');
			socket->disconnectFromServer();
			socket->deleteLater();
			continue;
		}
		m_client = socket;
		m_buffer.clear();
		connect(socket, &QLocalSocket::readyRead, this, &LiveServer::readClient);
			//Connected stays on once an assistant has used the channel:
			//the MCP server connects per request, so following the socket
			//would flicker the indicator off between two actions.
		connect(socket, &QLocalSocket::disconnected, socket, &QObject::deleteLater);
	}
}

void LiveServer::readClient()
{
	if (!m_client) return;
	m_buffer += m_client->readAll();
	int newline;
	while ((newline = m_buffer.indexOf('\n')) >= 0) {
		const QByteArray line = m_buffer.left(newline).trimmed();
		m_buffer.remove(0, newline + 1);
		if (line.isEmpty()) continue;
		const QJsonObject request = QJsonDocument::fromJson(line).object();
		if (request.value(QStringLiteral("token")).toString() != m_token || m_token.isEmpty()) {
			send(failure(QStringLiteral("bad token")));
			m_client->disconnectFromServer();
			return;
		}
		setState(Connected);
			//Out of the socket handler before anything runs (see class doc)
		QTimer::singleShot(0, this, [this, request]() { handle(request); });
	}
}

void LiveServer::send(const QJsonObject &answer)
{
	if (m_client) {
		m_client->write(QJsonDocument(answer).toJson(QJsonDocument::Compact) + '\n');
		m_client->flush();
	}
}

void LiveServer::handle(const QJsonObject &request)
{
	const QString cmd = request.value(QStringLiteral("cmd")).toString();
	QJsonObject answer;
	if (m_busy) {
		answer = failure(QStringLiteral("busy: the previous request is still running"));
	} else {
		m_busy = true;
		if (cmd == QLatin1String("status")) {
			answer = status();
		} else if (cmd == QLatin1String("run_script")) {
			answer = runScript(request.value(QStringLiteral("name")).toString(),
					   request.value(QStringLiteral("source")).toString());
		} else if (cmd == QLatin1String("run_stored")) {
			answer = runStored(request.value(QStringLiteral("script")).toString());
		} else {
			answer = failure(QStringLiteral("unknown command: %1").arg(cmd));
		}
		m_busy = false;
	}
	if (request.contains(QStringLiteral("id")))
		answer.insert(QStringLiteral("id"), request.value(QStringLiteral("id")));
	send(answer);
		//One request per connection: close it from this side once it is
		//answered. Waiting for the client to hang up raced the next
		//request on Windows, where a named pipe's disconnection reaches
		//QLocalSocket late -- the second call of a quick pair was turned
		//away as "another assistant" (found under Wine, 2026-10-02).
	if (m_client) {
		m_client->disconnectFromServer();
		m_client = nullptr;
	}
	emit handled(request, answer);
}

/**
	@brief LiveServer::editor
	The editor window the user is working in: the active one, or the first
	shown.
*/
QETDiagramEditor *LiveServer::editor() const
{
	QETDiagramEditor *chosen = nullptr;
	for (QETDiagramEditor *e : QETApp::diagramEditors()) {
		if (e->isActiveWindow()) {
			chosen = e;
			break;
		}
		if (!chosen && e->isVisible()) chosen = e;
	}
		//While the user is typing to the assistant, QElectroTech is not
		//the active application, and QMdiArea then has no active
		//sub-window: every "current project" in the editor reads as none.
		//The one it remembers is the project the user was looking at.
	if (chosen && !chosen->m_workspace.activeSubWindow()
	    && chosen->m_workspace.currentSubWindow()) {
		chosen->m_workspace.setActiveSubWindow(chosen->m_workspace.currentSubWindow());
	}
	return chosen;
}

QJsonObject LiveServer::status()
{
	QJsonObject answer{{QStringLiteral("ok"), true},
			   {QStringLiteral("scripting"), QetSettings::scriptingEnabled()}};
	QJsonArray scripts;
	for (const ScriptLibrary::Script &s : ScriptLibrary::instance().scripts())
		scripts.append(QJsonObject{{QStringLiteral("id"), s.header.id},
					   {QStringLiteral("name"), s.header.name}});
	answer.insert(QStringLiteral("stored_scripts"), scripts);

	QETDiagramEditor *e = editor();
	QETProject *project = e ? e->currentProject() : nullptr;
	if (!project) {
		answer.insert(QStringLiteral("project"), QJsonValue());
		return answer;
	}
	DiagramView *view = e->currentDiagramView();
	Diagram *diagram = view ? view->diagram() : nullptr;
	QJsonArray selected;
	if (diagram) {
		for (QGraphicsItem *item : diagram->selectedItems())
			if (item->type() == Element::Type)
				selected.append(static_cast<Element *>(item)->uuid().toString());
	}
	answer.insert(QStringLiteral("project"), QJsonObject{
		{QStringLiteral("title"), project->title()},
		{QStringLiteral("path"), project->filePath()},
		{QStringLiteral("folios"), int(project->diagrams().count())},
		{QStringLiteral("folio"), diagram ? int(project->diagrams().indexOf(diagram)) : -1},
		{QStringLiteral("folio_title"), diagram ? diagram->title() : QString()},
		{QStringLiteral("selected_elements"), selected},
		{QStringLiteral("read_only"), project->isReadOnly()},
		{QStringLiteral("last_undo"), project->undoStack()->text(project->undoStack()->index() - 1)}});
	return answer;
}

QJsonObject LiveServer::runScript(const QString &name, const QString &source)
{
	if (source.trimmed().isEmpty()) return failure(QStringLiteral("no script given"));
	QETDiagramEditor *e = editor();
	QETProject *project = e ? e->currentProject() : nullptr;
	if (!project) return failure(QStringLiteral("no project is open in QElectroTech"));

	const QString title = name.trimmed().isEmpty() ? tr("script") : name.trimmed();
	QetScripting::LiveRun run;
	const bool ok = QetScripting::runSource(source, QStringLiteral("assistant:") + title, title,
						project, e->currentDiagramView(), &run);
	QJsonObject answer{{QStringLiteral("ok"), ok},
			   {QStringLiteral("log"), QJsonArray::fromStringList(run.log)},
			   {QStringLiteral("undo"), run.undoText}};
	if (!run.error.isEmpty()) answer.insert(QStringLiteral("error"), run.error);
	return answer;
}

QJsonObject LiveServer::runStored(const QString &id)
{
	for (const ScriptLibrary::Script &s : ScriptLibrary::instance().scripts()) {
		if (s.header.id != id) continue;
		QFile file(s.path);
		if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
			return failure(QStringLiteral("cannot read %1").arg(s.path));
		QJsonObject answer = runScript(s.header.name, QString::fromUtf8(file.readAll()));
		answer.insert(QStringLiteral("name"), s.header.name);
		return answer;
	}
	return failure(QStringLiteral("no stored script with id %1").arg(id));
}
