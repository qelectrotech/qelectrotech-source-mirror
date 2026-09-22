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
#include "qetscripting.h"

#include "qetscriptapi.h"
#include "../qetmessagebox.h"
#include "../qetproject.h"

#include <QFile>
#include <QFileInfo>
#include <QObject>
#include <QTextStream>

#ifdef QET_HAS_SCRIPTING
#include <QJSEngine>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <thread>
#endif

namespace {
	QTextStream out(stdout);
	QTextStream err(stderr);
}

namespace QetScripting {

bool isRunRequest(const QStringList &args)
{
	return args.contains(QStringLiteral("--run"));
}

#ifdef QET_HAS_SCRIPTING

int run(const QStringList &args)
{
	const int idx = args.indexOf(QStringLiteral("--run"));
	const QString script_path = args.value(idx + 1);
	const QString project_path = args.value(idx + 2);
	if (script_path.isEmpty() || project_path.isEmpty()) {
		err << "Usage: qelectrotech --run <script.js> <project.qet>\n";
		return 2;
	}
	if (!QFileInfo::exists(script_path)) {
		err << "Script not found: " << script_path << "\n";
		return 2;
	}
	if (!QFileInfo::exists(project_path)) {
		err << "Project not found: " << project_path << "\n";
		return 2;
	}

	QETProject project(project_path);
	if (project.state() != QETProject::Ok) {
		err << "Failed to open project: " << project_path
			<< " (state " << project.state() << ")\n";
		return 1;
	}

	return runOnProject(script_path, &project, nullptr) ? 0 : 1;
}

namespace {
	// A runaway script (an infinite loop, or just a very slow one) would
	// otherwise freeze the GUI forever, or hang a CI job running --run with
	// no way out. QJSEngine::setInterrupted() is documented callable from
	// another thread; the engine polls it during evaluation and returns an
	// error QJSValue, which the normal error-reporting path below already
	// handles.
	constexpr int kScriptTimeoutMs = 30000;
}

bool runOnProject(const QString &scriptPath, QETProject *project, DiagramView *view)
{
	QFile file(scriptPath);
	if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
		err << "Cannot open script: " << scriptPath << "\n";
		return false;
	}
	const QString source = QString::fromUtf8(file.readAll());
	file.close();

	QJSEngine engine;
	auto *api = new QetScriptApi(project, view, &engine);
	QJSValue qet_value = engine.newQObject(api);
	// newQObject() takes ownership by default (QJSEngine::JavaScriptOwnership),
	// which would delete api as soon as the engine's GC decides to -- api's
	// real owner is the engine itself via the parent-child relationship set
	// above, so keep the engine, not the GC, in charge of its lifetime.
	engine.setObjectOwnership(api, QJSEngine::CppOwnership);
	engine.globalObject().setProperty(QStringLiteral("qet"), qet_value);

	// wait_for(), not sleep_for(): a script that finishes well inside the
	// timeout must let the watchdog thread wake immediately, not force
	// every run -- including a fast, successful one -- to block on join()
	// for the full budget. Caught by testing this against a one-line
	// script: it took the full 30 seconds to exit before this fix.
	bool finished = false;
	std::mutex mtx;
	std::condition_variable cv;
	std::thread watchdog([&]() {
		std::unique_lock<std::mutex> lock(mtx);
		cv.wait_for(lock, std::chrono::milliseconds(kScriptTimeoutMs), [&finished]{ return finished; });
		if (!finished) {
			engine.setInterrupted(true);
		}
	});

	QJSValue result = engine.evaluate(source, scriptPath);
	{
		std::lock_guard<std::mutex> lock(mtx);
		finished = true;
	}
	cv.notify_one();
	watchdog.join();

	if (result.isError()) {
		const QString message = QStringLiteral("Script error: %1:%2: %3")
				.arg(scriptPath)
				.arg(result.property(QStringLiteral("lineNumber")).toInt())
				.arg(result.toString());
		err << message << "\n";
		// Interactive "Run Script...": stderr is invisible to a user who
		// launched the GUI normally (nowhere on Windows, easy to miss
		// everywhere else). Headless --run has no GUI to show this in, and
		// no session for it to block.
		if (view) {
			QET::QetMessageBox::critical(nullptr, QObject::tr("Script"), message);
		}
		return false;
	}
	return true;
}

#else // !QET_HAS_SCRIPTING

// Qt::Qml was not found at configure time (see the QET_HAS_SCRIPTING probe
// in the top-level CMakeLists.txt). Compile to a clear, non-silent failure
// rather than omitting these symbols -- the same "always compiled, the
// disabled path says why" shape as the QtPdf feature guard.

int run(const QStringList &)
{
	err << "This build of QElectroTech was compiled without the Qt Qml "
		   "module, so JavaScript scripting (--run) is not available.\n";
	return 1;
}

bool runOnProject(const QString &, QETProject *, DiagramView *)
{
	err << "This build of QElectroTech was compiled without the Qt Qml "
		   "module, so JavaScript scripting is not available.\n";
	return false;
}

#endif // QET_HAS_SCRIPTING

}
