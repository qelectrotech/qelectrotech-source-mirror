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
#include "macrorecorder.h"

#include "assistantinfo.h"
#include "../diagram.h"
#include "../diagramview.h"
#include "../qetapp.h"
#include "../qetdiagrameditor.h"
#include "../qetgraphicsitem/element.h"
#include "../qetproject.h"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QSaveFile>
#include <QUndoStack>

namespace {
	bool writeFile(const QString &path, const QByteArray &data)
	{
		QSaveFile file(path);
		return file.open(QIODevice::WriteOnly) && file.write(data) >= 0 && file.commit();
	}
}

MacroRecorder &MacroRecorder::instance()
{
	static MacroRecorder recorder;
	return recorder;
}

QString MacroRecorder::folder()
{
	return QETApp::dataDir() + QStringLiteral("/recordings");
}

/**
	@brief MacroRecorder::recordings
	What qet-assistant.json lists: every saved recording, newest first.
*/
QJsonArray MacroRecorder::recordings()
{
	QJsonArray list;
	QDir dir(folder());
	const QStringList ids = dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot,
					      QDir::Name | QDir::Reversed);
	for (const QString &id : ids) {
		QFile file(dir.filePath(id + QStringLiteral("/recording.json")));
		if (!file.open(QIODevice::ReadOnly)) continue;
		const QJsonObject r = QJsonDocument::fromJson(file.readAll()).object();
		list.append(QJsonObject{
			{QStringLiteral("id"), id},
			{QStringLiteral("name"), r.value(QStringLiteral("name"))},
			{QStringLiteral("steps"), r.value(QStringLiteral("steps")).toArray().size()},
			{QStringLiteral("started"), r.value(QStringLiteral("started"))},
			{QStringLiteral("complete"), r.value(QStringLiteral("complete"))},
			{QStringLiteral("folder"), dir.filePath(id)}});
	}
	return list;
}

bool MacroRecorder::start(QETDiagramEditor *editor)
{
	QETProject *project = editor ? editor->currentProject() : nullptr;
	if (!project || isRecording()) return false;

	m_id = QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd-HHmmss"));
	m_dir = folder() + QLatin1Char('/') + m_id;
	if (!QDir().mkpath(m_dir + QStringLiteral("/steps"))) return false;
	if (!writeFile(m_dir + QStringLiteral("/before.qet"), project->toXml().toByteArray(4)))
		return false;

	m_project = project;
	m_editor = editor;
	m_started = QDateTime::currentDateTime().toString(Qt::ISODate);
	m_project_title = project->title();
	m_project_path = project->filePath();
	m_steps = QJsonArray();
	m_start = where();
	m_last_index = project->undoStack()->index();
	connect(project->undoStack(), &QUndoStack::indexChanged,
		this, &MacroRecorder::undoIndexChanged);
		//Closed while recording: keep what was recorded
	connect(project, &QObject::destroyed, this, [this]() { finish(true); });

	emit stateChanged(true, 0);
	AssistantInfo::write();
	return true;
}

void MacroRecorder::undoIndexChanged(int index)
{
	if (!m_project) return;
	QUndoStack *stack = m_project->undoStack();
	if (index > m_last_index) {
		for (int i = m_last_index; i < index; ++i) {
			const QUndoCommand *cmd = stack->command(i);
			if (!cmd) continue;
			QStringList parts;
			for (int c = 0; c < cmd->childCount(); ++c) parts << cmd->child(c)->text();
			addStep(QStringLiteral("do"), cmd->text(), parts);
		}
	} else if (index < m_last_index) {
		for (int i = m_last_index - 1; i >= index; --i) {
			const QUndoCommand *cmd = stack->command(i);
			addStep(QStringLiteral("undo"), cmd ? cmd->text() : QString(), {});
		}
	}
	m_last_index = index;
}

void MacroRecorder::addStep(const QString &kind, const QString &text, const QStringList &parts)
{
	const int n = int(m_steps.size()) + 1;
	QJsonObject step{
		{QStringLiteral("n"), n},
		{QStringLiteral("kind"), kind},
		{QStringLiteral("undo_text"), text},
		{QStringLiteral("parts"), QJsonArray::fromStringList(parts)}};

	const QJsonObject here = where();
	for (auto it = here.begin(); it != here.end(); ++it) step.insert(it.key(), it.value());
	DiagramView *view = m_editor ? m_editor->currentDiagramView() : nullptr;
	Diagram *diagram = view ? view->diagram() : nullptr;
	if (diagram && diagram->project() == m_project) {
		const QString file = QStringLiteral("steps/%1.xml").arg(n, 3, 10, QLatin1Char('0'));
		if (writeFile(m_dir + QLatin1Char('/') + file, diagram->toXml(true).toByteArray(4)))
			step.insert(QStringLiteral("folio_file"), file);
	}
	m_steps.append(step);
	emit stateChanged(true, n);
}

/**
	@brief MacroRecorder::where
	The folio on screen and the elements selected on it: where the person
	was working, which a script that acts "on the selection" needs to be
	checked against.
*/
QJsonObject MacroRecorder::where() const
{
	DiagramView *view = m_editor ? m_editor->currentDiagramView() : nullptr;
	Diagram *diagram = view ? view->diagram() : nullptr;
	if (!diagram || diagram->project() != m_project) return {};
	QJsonArray selected;
	for (QGraphicsItem *item : diagram->selectedItems()) {
		if (item->type() != Element::Type) continue;
		auto *element = static_cast<Element *>(item);
		selected.append(QJsonObject{
			{QStringLiteral("uuid"), element->uuid().toString()},
			{QStringLiteral("label"), element->actualLabel()},
			{QStringLiteral("x"), element->pos().x()},
			{QStringLiteral("y"), element->pos().y()}});
	}
	return {{QStringLiteral("folio"), int(m_project->diagrams().indexOf(diagram))},
		{QStringLiteral("folio_title"), diagram->title()},
		{QStringLiteral("selected_elements"), selected}};
}

void MacroRecorder::stop()
{
	if (isRecording()) finish(false);
}

void MacroRecorder::finish(bool project_closed)
{
	if (m_dir.isEmpty()) return;
	QETDiagramEditor *editor = m_editor;
	if (m_project) {
		disconnect(m_project->undoStack(), nullptr, this, nullptr);
		disconnect(m_project, nullptr, this, nullptr);
	}
	bool complete = false;
	if (!project_closed && m_project)
		complete = writeFile(m_dir + QStringLiteral("/after.qet"),
				     m_project->toXml().toByteArray(4));

	const QJsonObject recording{
		{QStringLiteral("format"), 1},
		{QStringLiteral("id"), m_id},
		{QStringLiteral("name"), tr("Macro du %1").arg(
			QDateTime::fromString(m_started, Qt::ISODate).toString(QStringLiteral("dd/MM/yyyy HH:mm")))},
		{QStringLiteral("project_title"), m_project_title},
		{QStringLiteral("project_path"), m_project_path},
		{QStringLiteral("started"), m_started},
		{QStringLiteral("stopped"), QDateTime::currentDateTime().toString(Qt::ISODate)},
		{QStringLiteral("complete"), complete},
		{QStringLiteral("note"), complete ? QString()
						  : tr("projet fermé pendant l'enregistrement : pas de after.qet")},
		{QStringLiteral("before"), QStringLiteral("before.qet")},
		{QStringLiteral("after"), complete ? QJsonValue(QStringLiteral("after.qet")) : QJsonValue()},
		{QStringLiteral("folder"), m_dir},
		{QStringLiteral("start"), m_start},
		{QStringLiteral("steps"), m_steps}};
	writeFile(m_dir + QStringLiteral("/recording.json"),
		  QJsonDocument(recording).toJson(QJsonDocument::Indented));

	m_last_id = m_id;
	m_project = nullptr;
	m_editor = nullptr;
	m_dir.clear();
	emit stateChanged(false, int(m_steps.size()));
	AssistantInfo::write();
	emit finished(recording, editor);
}

/**
	@brief MacroRecorder::assistantRequest
	What "Copier la demande pour l'assistant" puts on the clipboard: the
	recording named, and the round the assistant should follow.
*/
QString MacroRecorder::assistantRequest(const QJsonObject &recording)
{
	const int steps = int(recording.value(QStringLiteral("steps")).toArray().size());
	return tr("J'ai enregistré une macro dans QElectroTech : « %1 », %n étape(s) "
		  "(identifiant %2, dossier %3).\n"
		  "Avec le serveur MCP qet : lis-la avec qet_recording_read, écris un "
		  "script qui fait la même chose de façon générale (par exemple sur les "
		  "éléments sélectionnés plutôt que sur ceux-là précisément), vérifie-le "
		  "avec qet_recording_check jusqu'à ce qu'il corresponde, puis propose-le "
		  "comme bouton avec qet_script_install.", nullptr, steps)
		.arg(recording.value(QStringLiteral("name")).toString())
		.arg(recording.value(QStringLiteral("id")).toString(),
		     QDir::toNativeSeparators(recording.value(QStringLiteral("folder")).toString()));
}
