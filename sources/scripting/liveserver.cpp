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
#include "macrorecorder.h"
#include "qetscripting.h"
#include "scriptlibrary.h"
#include "../diagram.h"
#include "../diagramview.h"
#include "../qet.h"
#include "../qetapp.h"
#include "../qetdiagrameditor.h"
#include "../qetproject.h"
#include "../utils/qetsettings.h"

#include <QApplication>
#include <QFontDatabase>
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
#include <QAction>
#include <QBuffer>
#include <QPixmap>
#include <QPlainTextEdit>
#include <QVBoxLayout>
#include <QCheckBox>
#include <QDialog>
#include <QElapsedTimer>
#include <QDateTime>
#include <functional>
#include <QJsonValue>
#include <QDir>
#include <QFileInfo>
#include <QMdiSubWindow>
#include <QPageLayout>
#include <QPainter>
#include <QPrinterInfo>
#include <QPrinter>
#include <QDialogButtonBox>
#include <QLabel>
#include "../projectview.h"
#include "../shortcutmanager.h"

namespace {
	/**
		Timing of the live channel, for finding where an assistant's
		request spends its time. Nothing is measured unless the request
		asks for it ("timing": true, answered in "timing") or the
		environment names a log file (QET_LIVE_PERF_LOG: one JSON line per
		request, with the time until the folio is next painted).
	*/
	qint64 nowNs()
	{
		static QElapsedTimer clock;
		if (!clock.isValid()) clock.start();
		return clock.nsecsElapsed();
	}

	double ms(qint64 ns) { return qRound(ns / 1e4) / 100.0; }

	QString perfLogPath()
	{
		static const QString path = qEnvironmentVariable("QET_LIVE_PERF_LOG");
		return path;
	}

	void appendPerfLog(const QJsonObject &line)
	{
		QFile file(perfLogPath());
		if (file.open(QIODevice::WriteOnly | QIODevice::Append))
			file.write(QJsonDocument(line).toJson(QJsonDocument::Compact) + '\n');
	}

	/**
		Waits for the next paint of a widget (the folio on screen) and
		calls done with the time it started and the time the event loop
		came back, or with -1 when nothing was painted within 2 s (a
		request that changed nothing visible).
	*/
	class PaintProbe : public QObject
	{
		public:
			PaintProbe(QWidget *widget, std::function<void(qint64, qint64)> done) :
				QObject(widget), m_done(std::move(done))
			{
				widget->installEventFilter(this);
				QTimer::singleShot(2000, this, [this]() { finish(-1, -1); });
			}

		protected:
			bool eventFilter(QObject *, QEvent *event) override
			{
				if (event->type() == QEvent::Paint && m_start < 0) {
					m_start = nowNs();
						//Runs once the paint event has been handled
					QTimer::singleShot(0, this, [this]() { finish(m_start, nowNs()); });
				}
				return false;
			}

		private:
			void finish(qint64 start, qint64 end)
			{
				if (!m_done) return;
				auto done = std::move(m_done);
				m_done = nullptr;
				done(start, end);
				deleteLater();
			}

			std::function<void(qint64, qint64)> m_done;
			qint64 m_start = -1;
	};

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

LiveServer::LiveServer() :
	m_ask_first(QetSettings::liveAskFirst())
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
	if (QetSettings::liveSkipStartWarning()) {
		start();
		return;
	}

	QMessageBox box(QMessageBox::Warning, tr("Live mode"),
			tr("Live mode is on: a connected AI assistant will be able to run "
			   "scripts on the open project.\n"
			   "\n"
			   "Each action is undone with one Ctrl+Z, and the “Stop” button "
			   "in the status bar closes the connection.\n"
			   "\n"
			   "This setting is in Configure QElectroTech > General > "
			   "Projects."),
			QMessageBox::NoButton, parent);
	QPushButton *go = box.addButton(tr("&Continue"), QMessageBox::AcceptRole);
	box.addButton(tr("&Not this session"), QMessageBox::RejectRole);
	QPushButton *off = box.addButton(tr("&Turn off"), QMessageBox::DestructiveRole);
	box.setDefaultButton(go);
	auto *remember = new QCheckBox(tr("Don't ask again at start"), &box);
	remember->setToolTip(tr("Live mode will open at every start. To be warned again, "
				"turn it off and back on in Configure QElectroTech > "
				"General > Projects."));
	box.setCheckBox(remember);
	box.exec();

	if (box.clickedButton() == off) {
		QetSettings::setLiveAssistantEnabled(false);
	} else if (box.clickedButton() == go) {
		if (remember->isChecked()) QetSettings::setLiveSkipStartWarning(true);
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
		const qint64 received = nowNs();
			//Out of the socket handler before anything runs (see class doc)
		QTimer::singleShot(0, this, [this, request, received]() { handle(request, received); });
	}
}

void LiveServer::send(const QJsonObject &answer)
{
	if (m_client) {
		m_client->write(QJsonDocument(answer).toJson(QJsonDocument::Compact) + '\n');
		m_client->flush();
	}
}

void LiveServer::handle(const QJsonObject &request, qint64 received_ns)
{
	const qint64 started = nowNs();
	qint64 confirm_ns = 0;
	const QString cmd = request.value(QStringLiteral("cmd")).toString();
	QJsonObject answer;
	if (m_busy) {
		answer = failure(QStringLiteral("busy: the previous request is still running"));
	} else {
		m_busy = true;
		if (cmd == QLatin1String("status")) {
			answer = status();
		} else if (cmd == QLatin1String("run_script")) {
			const QString name = request.value(QStringLiteral("name")).toString();
			const QString source = request.value(QStringLiteral("source")).toString();
				//A script the assistant just wrote: the user sees it first,
				//unless they chose "always" (remembered). A stored script
				//is one the user already has, so it runs as a click would.
			const qint64 confirm_start = nowNs();
			const bool refused = m_ask_first && !confirm(name, source);
			confirm_ns = nowNs() - confirm_start;
			if (refused)
				answer = failure(QStringLiteral("refused by the user"));
			else
				answer = runScript(name, source);
		} else if (cmd == QLatin1String("run_stored")) {
			answer = runStored(request.value(QStringLiteral("script")).toString());
		} else if (cmd == QLatin1String("command")) {
			answer = command(request.value(QStringLiteral("action")).toString());
		} else if (cmd == QLatin1String("new_project")) {
			answer = newProject(request);
		} else if (cmd == QLatin1String("open_project")) {
			answer = openProject(request.value(QStringLiteral("path")).toString());
		} else if (cmd == QLatin1String("switch_project")) {
			answer = switchProject(request);
		} else if (cmd == QLatin1String("save_project")) {
			answer = saveProject(request);
		} else if (cmd == QLatin1String("close_project")) {
			answer = closeProject(request);
		} else if (cmd == QLatin1String("print")) {
			answer = print(request);
		} else if (cmd == QLatin1String("snapshot")) {
			answer = snapshot(request.value(QStringLiteral("path")).toString());
		} else if (cmd == QLatin1String("changes")) {
			answer = changes(request);
		} else if (cmd == QLatin1String("show_folio")) {
			answer = showFolio(request.value(QStringLiteral("folio")).toInt(-1));
		} else if (cmd == QLatin1String("undo_last")) {
			answer = undoLast();
		} else if (cmd == QLatin1String("screenshot")) {
			answer = screenshot();
		} else {
			answer = failure(QStringLiteral("unknown command: %1").arg(cmd));
		}
		m_busy = false;
	}
	if (request.contains(QStringLiteral("id")))
		answer.insert(QStringLiteral("id"), request.value(QStringLiteral("id")));
	const qint64 done = nowNs();
	QJsonObject timing{{QStringLiteral("queue_ms"), ms(started - received_ns)},
			   {QStringLiteral("confirm_ms"), ms(confirm_ns)},
			   {QStringLiteral("exec_ms"), ms(done - started - confirm_ns)}};
	if (request.value(QStringLiteral("timing")).toBool())
		answer.insert(QStringLiteral("timing"), timing);
	send(answer);
	timing.insert(QStringLiteral("reply_ms"), ms(nowNs() - received_ns));
		//One request per connection: close it from this side once it is
		//answered. Waiting for the client to hang up raced the next
		//request on Windows, where a named pipe's disconnection reaches
		//QLocalSocket late -- the second call of a quick pair was turned
		//away as "another assistant" (found under Wine, 2026-10-02).
	if (m_client) {
		m_client->disconnectFromServer();
		m_client = nullptr;
	}
	if (!perfLogPath().isEmpty()) {
		timing.insert(QStringLiteral("src"), QStringLiteral("qet"));
		timing.insert(QStringLiteral("id"), request.value(QStringLiteral("id")));
		timing.insert(QStringLiteral("cmd"), cmd);
		timing.insert(QStringLiteral("ok"), answer.value(QStringLiteral("ok")));
		timing.insert(QStringLiteral("t"), QDateTime::currentMSecsSinceEpoch() / 1000.0);
		QETDiagramEditor *e = editor();
		DiagramView *view = e ? e->currentDiagramView() : nullptr;
		if (!view) {
			appendPerfLog(timing);
		} else {
			new PaintProbe(view->viewport(), [timing, received_ns](qint64 start, qint64 end) mutable {
				timing.insert(QStringLiteral("paint_start_ms"),
					      start < 0 ? QJsonValue() : QJsonValue(ms(start - received_ns)));
				timing.insert(QStringLiteral("paint_ms"),
					      start < 0 ? QJsonValue() : QJsonValue(ms(end - start)));
				appendPerfLog(timing);
			});
		}
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
		//So an assistant can notice a recording the user just made
	answer.insert(QStringLiteral("projects"), openProjects());
	answer.insert(QStringLiteral("macro_recorder"), QJsonObject{
		{QStringLiteral("recording"), MacroRecorder::instance().isRecording()},
		{QStringLiteral("steps"), MacroRecorder::instance().stepCount()},
		{QStringLiteral("last_recording"), MacroRecorder::instance().lastId()}});

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

void LiveServer::setAskFirst(bool ask)
{
	if (ask == m_ask_first) return;
	m_ask_first = ask;
	QetSettings::setLiveAskFirst(ask);
	emit askFirstChanged(ask);
}

/**
	@brief LiveServer::allowedCommands
	The editor commands an assistant may trigger: ones that open no dialog
	(a dialog would wait for an answer nobody is there to give the
	assistant) and change nothing that undo cannot take back. Saving,
	printing, exporting, deleting, opening and closing are not in it: a
	script can delete, as one undo step, and the rest stay the user's.
*/
QStringList LiveServer::allowedCommands()
{
	static const QStringList ids{
		QStringLiteral("diagrameditor.select_all"),
		QStringLiteral("diagrameditor.select_nothing"),
		QStringLiteral("diagrameditor.select_invert"),
		QStringLiteral("diagrameditor.select_all_conductors"),
		QStringLiteral("diagrameditor.select_all_text_fields"),
		QStringLiteral("diagrameditor.zoom_in"),
		QStringLiteral("diagrameditor.zoom_out"),
		QStringLiteral("diagrameditor.zoom_content"),
		QStringLiteral("diagrameditor.zoom_fit"),
		QStringLiteral("diagrameditor.zoom_reset"),
		QStringLiteral("diagrameditor.rotate_selection"),
		QStringLiteral("diagrameditor.mirror_horizontal"),
		QStringLiteral("diagrameditor.mirror_vertical"),
		QStringLiteral("diagrameditor.rotate_texts"),
		QStringLiteral("diagrameditor.snap_selection_to_grid"),
		QStringLiteral("diagrameditor.group_selection"),
		QStringLiteral("diagrameditor.ungroup_selection"),
		QStringLiteral("diagrameditor.conductor_reset")};
	return ids;
}

QJsonObject LiveServer::command(const QString &id)
{
	if (!allowedCommands().contains(id))
		return failure(QStringLiteral("%1 is not a command an assistant may use; allowed: %2")
			       .arg(id, allowedCommands().join(QStringLiteral(", "))));
	QETDiagramEditor *e = editor();
	QAction *action = e ? ShortcutManager::instance().action(id, e) : nullptr;
	if (!action) return failure(QStringLiteral("no editor window has %1").arg(id));
	if (!action->isEnabled())
		return failure(QStringLiteral("%1 is not available right now (nothing "
					      "selected, or no project open)").arg(id));
	action->trigger();
	return {{QStringLiteral("ok"), true}, {QStringLiteral("action"), id}};
}

QJsonObject LiveServer::showFolio(int folio)
{
	QETDiagramEditor *e = editor();
	ProjectView *pv = e ? e->currentProjectView() : nullptr;
	if (!pv) return failure(QStringLiteral("no project is open in QElectroTech"));
	const QList<Diagram *> diagrams = pv->project()->diagrams();
	if (folio < 0 || folio >= diagrams.count())
		return failure(QStringLiteral("no folio %1: the project has %2, counted from 0")
			       .arg(folio).arg(diagrams.count()));
	pv->showDiagram(diagrams.at(folio));
	return {{QStringLiteral("ok"), true}, {QStringLiteral("folio"), folio}};
}

/**
	@brief LiveServer::newProject
	A new project, as File > New makes it: the new-folio defaults of this
	QElectroTech, opened and made the current project, so the next script
	works on it. Optional: "title", "folios" (how many empty folios, 1 to
	100, default 1), and "path" to save it to at once -- never over an
	existing file, so an assistant cannot replace the user's work.
*/
QJsonObject LiveServer::newProject(const QJsonObject &request)
{
	QETDiagramEditor *e = editor();
	if (!e) return failure(QStringLiteral("no QElectroTech editor window is open"));

	const int folios = request.value(QStringLiteral("folios")).toInt(1);
	if (folios < 1 || folios > 100)
		return failure(QStringLiteral("folios must be between 1 and 100, not %1").arg(folios));
	const QString path = request.value(QStringLiteral("path")).toString().trimmed();
	if (!path.isEmpty()) {
		const QFileInfo info(path);
		if (info.isRelative())
			return failure(QStringLiteral("path must be absolute: %1").arg(path));
		if (info.exists())
			return failure(QStringLiteral("%1 already exists; a new project is never "
						      "saved over a file").arg(path));
		if (!info.dir().exists())
			return failure(QStringLiteral("the folder %1 does not exist").arg(info.absolutePath()));
	}

	auto project = new QETProject(e);
	for (int i = 0; i < folios; ++i) project->addNewDiagram();
	const QString title = request.value(QStringLiteral("title")).toString().trimmed();
	if (!title.isEmpty()) project->setTitle(title);
	if (!e->addProject(project)) return failure(QStringLiteral("QElectroTech refused the new project"));

		//Current at once, whether or not QElectroTech is the active
		//application: the next request runs on it (see editor())
	for (ProjectView *pv : e->openedProjects()) {
		if (pv->project() == project) {
			for (QMdiSubWindow *w : e->m_workspace.subWindowList())
				if (w->widget() == pv) e->m_workspace.setActiveSubWindow(w);
		}
	}

	if (!path.isEmpty()) {
		project->setFilePath(path);
		const QETResult result = project->write();
		if (!result.isOk())
			return failure(QStringLiteral("the project was created but not saved to %1: %2")
				       .arg(path, result.errorMessage()));
	}

	QJsonObject answer = status();
	answer.insert(QStringLiteral("created"), true);
	return answer;
}

/**
	@brief LiveServer::openProjects
	The projects open in the editor window, in tab order: the index
	switch_project takes, title, file, folios, unsaved changes, and which
	one is current.
*/
QJsonArray LiveServer::openProjects() const
{
	QJsonArray list;
	QETDiagramEditor *e = editor();
	if (!e) return list;
	const QETProject *current = e->currentProject();
	const QList<ProjectView *> views = e->openedProjects();
	for (int i = 0; i < views.count(); ++i) {
		QETProject *p = views.at(i)->project();
		list.append(QJsonObject{
			{QStringLiteral("index"), i},
			{QStringLiteral("title"), p->title()},
			{QStringLiteral("path"), p->filePath()},
			{QStringLiteral("folios"), int(p->diagrams().count())},
			{QStringLiteral("modified"), p->projectWasModified()},
			{QStringLiteral("read_only"), p->isReadOnly()},
			{QStringLiteral("current"), p == current}});
	}
	return list;
}

/**
	@brief LiveServer::openProject
	Open a saved project and make it current, as File > Open does but with
	no file dialog and no error windows: what is wrong comes back in the
	answer. A project already open is only made current.
*/
QJsonObject LiveServer::openProject(const QString &raw_path)
{
	QETDiagramEditor *e = editor();
	if (!e) return failure(QStringLiteral("no QElectroTech editor window is open"));
	const QString path = raw_path.trimmed();
	if (path.isEmpty()) return failure(QStringLiteral("no path given"));
	const QFileInfo info(path);
	if (info.isRelative()) return failure(QStringLiteral("path must be absolute: %1").arg(path));
	if (!info.isFile()) return failure(QStringLiteral("%1 does not exist").arg(path));
	if (!info.isReadable()) return failure(QStringLiteral("%1 cannot be read (permissions)").arg(path));

	QJsonObject extra{{QStringLiteral("opened"), true}};
	if (ProjectView *pv = e->viewForFile(info.absoluteFilePath())) {
		e->activateProject(pv);
		extra = {{QStringLiteral("opened"), false}, {QStringLiteral("already_open"), true}};
	} else if (QETApp::diagramEditorForFile(info.absoluteFilePath())) {
		return failure(QStringLiteral("%1 is open in another QElectroTech window; "
					      "switch to it there").arg(path));
	} else {
		if (!e->openAndAddProject(info.absoluteFilePath(), false))
			return failure(QStringLiteral("QElectroTech could not open %1 (not a "
						      "QElectroTech project, or a newer format)").arg(path));
		if (ProjectView *pv = e->viewForFile(info.absoluteFilePath())) e->activateProject(pv);
	}
	QJsonObject answer = status();
	for (auto it = extra.begin(); it != extra.end(); ++it) answer.insert(it.key(), it.value());
	return answer;
}

/**
	@brief LiveServer::switchProject
	Make another open project current, by its index in "projects" (status)
	or by its file path.
*/
QJsonObject LiveServer::switchProject(const QJsonObject &request)
{
	QETDiagramEditor *e = editor();
	if (!e) return failure(QStringLiteral("no QElectroTech editor window is open"));
	const QList<ProjectView *> views = e->openedProjects();
	ProjectView *target = nullptr;
	if (request.contains(QStringLiteral("index"))) {
		const int index = request.value(QStringLiteral("index")).toInt(-1);
		if (index < 0 || index >= views.count())
			return failure(QStringLiteral("no open project %1: %2 are open, counted from 0")
				       .arg(index).arg(views.count()));
		target = views.at(index);
	} else {
		const QString path = request.value(QStringLiteral("path")).toString().trimmed();
		if (path.isEmpty()) return failure(QStringLiteral("give \"index\" (from \"projects\") or \"path\""));
		target = e->viewForFile(QFileInfo(path).absoluteFilePath());
		if (!target) return failure(QStringLiteral("%1 is not open; open_project opens it").arg(path));
	}
	e->activateProject(target);
	return status();
}

/**
	@brief LiveServer::saveProject
	Save the current project to its own file, or, with "path", to a new
	file that becomes its file (Save As) -- never over an existing one.
*/
QJsonObject LiveServer::saveProject(const QJsonObject &request)
{
	QETDiagramEditor *e = editor();
	QETProject *project = e ? e->currentProject() : nullptr;
	if (!project) return failure(QStringLiteral("no project is open in QElectroTech"));
	if (project->isReadOnly()) return failure(QStringLiteral("the project is read-only"));

	const QString path = request.value(QStringLiteral("path")).toString().trimmed();
	if (path.isEmpty()) {
		if (project->filePath().isEmpty())
			return failure(QStringLiteral("the project has no file yet: give \"path\" to save it as one"));
	} else {
		const QFileInfo info(path);
		if (info.isRelative()) return failure(QStringLiteral("path must be absolute: %1").arg(path));
		if (info.exists())
			return failure(QStringLiteral("%1 already exists; save as never replaces a file").arg(path));
		if (!info.dir().exists())
			return failure(QStringLiteral("the folder %1 does not exist").arg(info.absolutePath()));
		project->setFilePath(info.absoluteFilePath());
	}
	const QETResult result = project->write();
	if (!result.isOk()) return failure(QStringLiteral("not saved: %1").arg(result.errorMessage()));
	QJsonObject answer = status();
	answer.insert(QStringLiteral("saved"), project->filePath());
	return answer;
}

/**
	@brief LiveServer::closeProject
	Close an open project (the current one, or "index"), only when it has
	no unsaved changes: closing never discards the user's work, and never
	asks them a question on the assistant's behalf.
*/
QJsonObject LiveServer::closeProject(const QJsonObject &request)
{
	QETDiagramEditor *e = editor();
	if (!e) return failure(QStringLiteral("no QElectroTech editor window is open"));
	const QList<ProjectView *> views = e->openedProjects();
	ProjectView *pv = nullptr;
	if (request.contains(QStringLiteral("index"))) {
		const int index = request.value(QStringLiteral("index")).toInt(-1);
		if (index < 0 || index >= views.count())
			return failure(QStringLiteral("no open project %1: %2 are open, counted from 0")
				       .arg(index).arg(views.count()));
		pv = views.at(index);
	} else {
		pv = e->currentProjectView();
		if (!pv) return failure(QStringLiteral("no project is open in QElectroTech"));
	}
	QETProject *project = pv->project();
	if (project->projectWasModified())
		return failure(QStringLiteral("\"%1\" has unsaved changes; save it first "
					      "(save_project) -- closing never discards them")
			       .arg(project->title()));
	const QString title = project->title();
	if (!e->closeProject(pv)) return failure(QStringLiteral("QElectroTech did not close \"%1\"").arg(title));
	QJsonObject answer = status();
	answer.insert(QStringLiteral("closed"), title);
	return answer;
}

/**
	@brief LiveServer::print
	Print folios of the current project with no print dialog: "folios" is
	"all" (default), "current" or a list of indexes; "printer" names one,
	else the system's default printer. Paper cannot be taken back, so the
	user is always asked first, whatever "always" says for scripts.
	"output_file" prints to a new PDF file instead (no paper, so no
	question; never over an existing file).
*/
QJsonObject LiveServer::print(const QJsonObject &request)
{
	QETDiagramEditor *e = editor();
	ProjectView *pv = e ? e->currentProjectView() : nullptr;
	if (!pv) return failure(QStringLiteral("no project is open in QElectroTech"));
	QETProject *project = pv->project();
	const QList<Diagram *> all = project->diagrams();

	QList<Diagram *> folios;
	const QJsonValue which = request.value(QStringLiteral("folios"));
	if (which.isUndefined() || which.toString() == QLatin1String("all")) {
		folios = all;
	} else if (which.toString() == QLatin1String("current")) {
		if (DiagramView *dv = e->currentDiagramView()) folios << dv->diagram();
	} else if (which.isArray()) {
		for (const QJsonValue &v : which.toArray()) {
			const int i = v.toInt(-1);
			if (i < 0 || i >= all.count())
				return failure(QStringLiteral("no folio %1: the project has %2, counted from 0")
					       .arg(v.toVariant().toString()).arg(all.count()));
			folios << all.at(i);
		}
	} else {
		return failure(QStringLiteral("folios must be \"all\", \"current\" or a list of indexes"));
	}
	if (folios.isEmpty()) return failure(QStringLiteral("no folio to print"));

	const QString file = request.value(QStringLiteral("output_file")).toString().trimmed();
	QPrinterInfo info;
	if (file.isEmpty()) {
		const QString name = request.value(QStringLiteral("printer")).toString().trimmed();
		info = name.isEmpty() ? QPrinterInfo::defaultPrinter() : QPrinterInfo::printerInfo(name);
		if (info.isNull())
			return failure(name.isEmpty()
				? QStringLiteral("this computer has no default printer")
				: QStringLiteral("no printer called \"%1\" (printers: %2)")
				  .arg(name, QPrinterInfo::availablePrinterNames().join(QStringLiteral(", "))));
		QMessageBox box(QMessageBox::Question, tr("Print"),
				tr("The assistant wants to print %n folios of “%1” on “%2”.", "", int(folios.count()))
				.arg(project->title(), info.printerName()),
				QMessageBox::NoButton, e);
		QPushButton *go = box.addButton(tr("&Print"), QMessageBox::AcceptRole);
		box.addButton(tr("&Cancel"), QMessageBox::RejectRole);
		box.setDefaultButton(go);
		box.exec();
		if (box.clickedButton() != go) return failure(QStringLiteral("refused by the user"));
	} else {
		const QFileInfo fi(file);
		if (fi.isRelative()) return failure(QStringLiteral("output_file must be absolute: %1").arg(file));
		if (fi.exists()) return failure(QStringLiteral("%1 already exists; printing never replaces a file").arg(file));
	}

		//info is still null when printing to a file, which is what QPrinter()
		//uses. No ?: here: MSVC copies its result, and QPrinter cannot be copied
	QPrinter printer(info);
		//96 dpi, as the PDF export draws: symbols are replayed at the
		//device's resolution, so at a printer's 600-1200 dpi they came out
		//many times too big next to the wires (seen on a test print)
	printer.setResolution(96);
	if (!file.isEmpty()) {
		printer.setOutputFormat(QPrinter::PdfFormat);
		printer.setOutputFileName(file);
	}
	printer.setDocName(project->title().isEmpty() ? QStringLiteral("QElectroTech") : project->title());
	const auto folioRect = [](Diagram *d) {
		QRectF r = d->border_and_titleblock.borderAndTitleBlockRect();
		r.adjust(0, 0, 1, 1);
		return r.toAlignedRect();
	};
		//One orientation for the job: some printer drivers ignore a change
		//between pages. The first folio decides.
	const QRect first = folioRect(folios.first());
	printer.setPageOrientation(first.width() > first.height() ? QPageLayout::Landscape
								  : QPageLayout::Portrait);
	QPainter painter;
	if (!painter.begin(&printer)) return failure(QStringLiteral("the printer could not be opened"));
	for (int i = 0; i < folios.count(); ++i) {
		Diagram *d = folios.at(i);
		if (i) printer.newPage();
			//As the PDF export draws a folio: no grid, guides or terminals
		const bool grid = d->displayGrid(), guides = d->displayGuides();
		const bool terms = d->drawTerminals(), names = d->drawTerminalNames();
		d->setDisplayGrid(false); d->setDisplayGuides(false);
		d->setDrawTerminals(false); d->setDrawTerminalNames(false);
		d->render(&painter, printer.pageLayout().paintRectPixels(printer.resolution()),
			  folioRect(d), Qt::KeepAspectRatio);
		d->setDisplayGrid(grid); d->setDisplayGuides(guides);
		d->setDrawTerminals(terms); d->setDrawTerminalNames(names);
	}
	painter.end();
	return {{QStringLiteral("ok"), true}, {QStringLiteral("printed"), int(folios.count())},
		{QStringLiteral("to"), file.isEmpty() ? info.printerName() : file}};
}

/**
	@brief LiveServer::snapshot
	Write a copy of the current project, as it is on screen (unsaved
	changes included), to a new file -- for a check to read -- without
	saving the project or changing its file. Never over an existing file.
*/
QJsonObject LiveServer::snapshot(const QString &raw_path)
{
	QETDiagramEditor *e = editor();
	QETProject *project = e ? e->currentProject() : nullptr;
	if (!project) return failure(QStringLiteral("no project is open in QElectroTech"));
	const QFileInfo info(raw_path.trimmed());
	if (raw_path.trimmed().isEmpty() || info.isRelative())
		return failure(QStringLiteral("path must be absolute"));
	if (info.exists()) return failure(QStringLiteral("%1 already exists").arg(raw_path));
	QDomDocument xml(project->toXml());
	QString error;
	if (QETProject::isContainerPath(info.absoluteFilePath())
			? !QETProject::writeContainer(xml, info.absoluteFilePath(), &error)
			: !QET::writeXmlFile(xml, info.absoluteFilePath(), &error))
		return failure(QStringLiteral("snapshot not written: %1").arg(error));
	return {{QStringLiteral("ok"), true}, {QStringLiteral("path"), info.absoluteFilePath()},
		{QStringLiteral("folio"), e->currentDiagramView()
			? int(project->diagrams().indexOf(e->currentDiagramView()->diagram())) : -1}};
}

/**
	@brief LiveServer::changes
	The current project's undo history: each step's name, whether the
	assistant made it, and whether it is undone. "since" (an index from an
	earlier answer) keeps only the steps after it, so an assistant can say
	what it changed since a point. Changes nothing.
*/
QJsonObject LiveServer::changes(const QJsonObject &request)
{
	QETDiagramEditor *e = editor();
	QETProject *project = e ? e->currentProject() : nullptr;
	if (!project) return failure(QStringLiteral("no project is open in QElectroTech"));
	QUndoStack *stack = project->undoStack();
	const QString prefix = tr("Assistant: %1").arg(QString());
	const int since = request.value(QStringLiteral("since")).toInt(-1);
	QJsonArray steps;
	int assistant = 0, user = 0;
	for (int i = qMax(0, since + 1); i < stack->count(); ++i) {
		const QString text = stack->text(i);
		const bool mine = text.startsWith(prefix);
		const bool undone = i >= stack->index();
		if (!undone) (mine ? assistant : user)++;
		steps.append(QJsonObject{{QStringLiteral("index"), i}, {QStringLiteral("step"), text},
					 {QStringLiteral("by"), mine ? QStringLiteral("assistant")
								     : QStringLiteral("user")},
					 {QStringLiteral("undone"), undone}});
	}
	return {{QStringLiteral("ok"), true}, {QStringLiteral("project"), project->title()},
		{QStringLiteral("steps"), steps},
		{QStringLiteral("now"), stack->index() - 1},
		{QStringLiteral("saved_at"), stack->cleanIndex() - 1},
		{QStringLiteral("done_by_assistant"), assistant}, {QStringLiteral("done_by_user"), user}};
}

/**
	@brief LiveServer::undoLast
	Undo the newest step, only if the assistant made it: what the user did
	by hand stays theirs to undo.
*/
QJsonObject LiveServer::undoLast()
{
	QETDiagramEditor *e = editor();
	QETProject *project = e ? e->currentProject() : nullptr;
	if (!project) return failure(QStringLiteral("no project is open in QElectroTech"));
	QUndoStack *stack = project->undoStack();
	const QString text = stack->text(stack->index() - 1);
	const QString prefix = tr("Assistant: %1").arg(QString());
	if (!stack->canUndo() || !text.startsWith(prefix))
		return failure(QStringLiteral("the last step is not the assistant's (\"%1\"); "
					      "only the user undoes their own").arg(text));
	stack->undo();
	return {{QStringLiteral("ok"), true}, {QStringLiteral("undone"), text}};
}

QJsonObject LiveServer::screenshot()
{
	QETDiagramEditor *e = editor();
	DiagramView *view = e ? e->currentDiagramView() : nullptr;
	if (!view) return failure(QStringLiteral("no folio is shown in QElectroTech"));
		// Only the part of the folio (frame and title block) that is on
		// screen: a tall or wide window around a zoomed-out folio would
		// otherwise give mostly empty background. The whole viewport when
		// the folio is scrolled out of sight.
	QRect area = view->viewport()->rect();
	bool cropped = false;
	if (Diagram *diagram = view->diagram()) {
		const QRect folio = view->mapFromScene(
			diagram->border_and_titleblock.borderAndTitleBlockRect()).boundingRect()
			.adjusted(-1, -1, 1, 1).intersected(area);
		if (!folio.isEmpty()) {
			cropped = folio != area;
			area = folio;
		}
	}
	const QPixmap pixmap = view->viewport()->grab(area);
	QByteArray png;
	QBuffer buffer(&png);
	buffer.open(QIODevice::WriteOnly);
	pixmap.save(&buffer, "PNG");
	return {{QStringLiteral("ok"), true},
		{QStringLiteral("width"), pixmap.width()},
		{QStringLiteral("height"), pixmap.height()},
		{QStringLiteral("cropped_to_folio"), cropped},
		{QStringLiteral("png_base64"), QString::fromLatin1(png.toBase64())}};
}

/**
	@brief LiveServer::confirm
	Show the user the script the assistant wants to run, and ask.
	@return true to run it
*/
bool LiveServer::confirm(const QString &name, const QString &source)
{
	QDialog dialog(editor());
	dialog.setWindowTitle(tr("The assistant wants to run a script"));
	auto *layout = new QVBoxLayout(&dialog);
	layout->addWidget(new QLabel(
		tr("“%1” on the open project. Once it has run, Ctrl+Z undoes it.")
		.arg(name.isEmpty() ? tr("script") : name), &dialog));
	auto *text = new QPlainTextEdit(source, &dialog);
	text->setReadOnly(true);
	text->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
	text->setMinimumSize(520, 220);
	layout->addWidget(text);
	auto *buttons = new QDialogButtonBox(&dialog);
	QPushButton *run = buttons->addButton(tr("&Run"), QDialogButtonBox::AcceptRole);
	buttons->addButton(tr("&Decline"), QDialogButtonBox::RejectRole);
	QPushButton *always = buttons->addButton(tr("&Always"),
						 QDialogButtonBox::AcceptRole);
	layout->addWidget(buttons);
	QPushButton *clicked = nullptr;
	connect(buttons, &QDialogButtonBox::clicked, &dialog, [&](QAbstractButton *b) {
		clicked = qobject_cast<QPushButton *>(b);
	});
	connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
	connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
	run->setDefault(true);
	dialog.exec();
	if (clicked == always) setAskFirst(false);
	return clicked == run || clicked == always;
}
