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
#ifndef QET_SCRIPT_API_H
#define QET_SCRIPT_API_H

#include <QObject>
#include <QString>
#include <QStringList>

class QETProject;

/**
	@brief The QetScriptApi class
	The object a script sees as `qet` (bugtracker #162): a small,
	deliberately read-mostly surface over an open project, for scripted
	batch/CI work and human-written macros.

	@b Scope, on purpose: this reads the model (folio count, titles, element
	and conductor counts -- the same data the `--info` CLI export already
	reports) and can trigger the same export/save operations the `--export-*`
	CLI flags already do. It does not create or edit diagram geometry, does
	not touch the undo stack, and does not drive the GUI. Those are all
	explicitly out of scope for this first version; see the discussion on
	bugtracker #162.

	Export/save methods are thin wrappers around CLIExport::run() -- the
	exact same, already-tested code path the `--export-*` flags use -- built
	from the project's own file path, not the live in-memory instance. That
	keeps this file free of any dependency on cli_export.cpp's internals,
	at the cost of re-opening the project from disk for each call: fine for
	the batch/CI use case this targets, and for the headless `--run` entry
	point it's exactly what a second `--export-*` invocation would have
	done anyway. A macro acting on unsaved GUI edits should call save()
	first.
*/
class QetScriptApi : public QObject
{
	Q_OBJECT

	public:
		explicit QetScriptApi(QETProject *project, QObject *parent = nullptr);

		// -- read the model --
		Q_INVOKABLE QString projectTitle() const;
		Q_INVOKABLE QString filePath() const;
		Q_INVOKABLE int folioCount() const;
		Q_INVOKABLE QString folioTitle(int index) const;
		Q_INVOKABLE int elementCount(int folioIndex) const;
		Q_INVOKABLE int conductorCount(int folioIndex) const;

		// -- export / save: thin wrappers around the --export-* CLI paths --
		Q_INVOKABLE bool exportPdf(const QString &output, bool showTerminals = false);
		Q_INVOKABLE bool exportPng(const QString &outDir, bool showTerminals = false);
		Q_INVOKABLE bool exportSvg(const QString &outDir, bool showTerminals = false);
		Q_INVOKABLE bool exportCables(const QString &output);
		Q_INVOKABLE bool exportWires(const QString &output);
		Q_INVOKABLE bool exportBom(const QString &output);
		Q_INVOKABLE bool exportWiring(const QString &output);
		Q_INVOKABLE bool exportNets(const QString &output);
		Q_INVOKABLE bool exportLinks(const QString &output);
		Q_INVOKABLE bool exportInfo(const QString &output);
		Q_INVOKABLE bool setTitleBlock(const QString &output, const QStringList &assignments);
		Q_INVOKABLE bool save(const QString &output);

		// -- logging: a script has no console of its own --
		Q_INVOKABLE void log(const QString &message);

	private:
		bool runFlag(const QString &flag, const QStringList &args);

		QETProject *m_project;
};

#endif // QET_SCRIPT_API_H
