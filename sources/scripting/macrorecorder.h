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
#ifndef MACRORECORDER_H
#define MACRORECORDER_H

#include <QJsonArray>
#include <QJsonObject>
#include <QObject>
#include <QPointer>

class QETDiagramEditor;
class QETProject;

/**
	@brief The MacroRecorder class
	Records what a person does by hand on one project, so an AI assistant
	can turn it into a script (see MACRO-RECORDER-SCOPE.md):

	@code
	<data folder>/recordings/<id>/
	    recording.json   name, project, times, the steps
	    before.qet       the whole project when recording started
	    after.qet        the whole project when it stopped
	    steps/003.xml    the folio a step happened on, after the step
	@endcode

	Steps come from the project's undo history -- each new entry is one
	step, with its name and the names of the commands inside it -- so no
	editing command has to be taught to the recorder. Before and after are
	exact whatever was done; the steps give the order. Nothing new walks
	the scene: the files are QElectroTech's own serialisation.

	QElectroTech cannot send anything to an assistant; the assistant fetches
	recordings through the qet MCP server, and qet-assistant.json lists them.
*/
class MacroRecorder : public QObject
{
	Q_OBJECT

	public:
		static MacroRecorder &instance();
		static QString folder();
		static QJsonArray recordings();

		bool start(QETDiagramEditor *editor);
		void stop();
		bool isRecording() const { return !m_project.isNull(); }
		int stepCount() const { return int(m_steps.size()); }
		QString lastId() const { return m_last_id; }
		static QString assistantRequest(const QJsonObject &recording);

	signals:
		void stateChanged(bool recording, int steps);
			/// A recording was saved; @a recording is its recording.json
		void finished(const QJsonObject &recording, QETDiagramEditor *editor);

	private:
		MacroRecorder() = default;
		void undoIndexChanged(int index);
		void addStep(const QString &kind, const QString &text, const QStringList &parts);
		void finish(bool project_closed);
		QJsonObject where() const;

		QPointer<QETProject> m_project;
		QPointer<QETDiagramEditor> m_editor;
		QString m_id;
		QString m_dir;
		QString m_started;
		QString m_project_title;
		QString m_project_path;
		QJsonArray m_steps;
		QJsonObject m_start;	///< folio and selection when recording started
		QString m_last_id;
		int m_last_index = 0;
};

#endif // MACRORECORDER_H
