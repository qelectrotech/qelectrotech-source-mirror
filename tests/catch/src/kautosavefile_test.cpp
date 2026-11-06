#include "../../../sources/ui/nokde/kautosavefile.h"

#include <catch2/catch.hpp>

#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QProcess>
#include <QScopeGuard>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QUrl>

#include <algorithm>
#include <memory>
#include <vector>

#ifdef Q_OS_UNIX
#include <ctime>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#endif

TEST_CASE("Qt-only KAutoSaveFile recovers stale files", "[nokde][autosave]")
{
#ifndef Q_OS_UNIX
	SUCCEED("crash-style stale lock test is Unix-only");
#else
	QTemporaryDir data_home;
	REQUIRE(data_home.isValid());

	qputenv("XDG_DATA_HOME", QFile::encodeName(data_home.path()));
	QCoreApplication::setOrganizationName(QStringLiteral("QElectroTech"));
	QCoreApplication::setApplicationName(QStringLiteral("KAutoSaveFileTest"));

	const auto managed_path = data_home.filePath(QStringLiteral("project.qet"));
	QFile managed_file(managed_path);
	REQUIRE(managed_file.open(QIODevice::WriteOnly | QIODevice::Text));
	REQUIRE(managed_file.write("<project/>\n") > 0);
	managed_file.close();

	int ready_pipe[2] = {-1, -1};
	REQUIRE(pipe(ready_pipe) == 0);

	const QByteArray payload("<project><diagram /></project>\n");
	const auto child_pid = fork();
	REQUIRE(child_pid >= 0);

	if (child_pid == 0) {
		close(ready_pipe[0]);

		KAutoSaveFile backup(QUrl::fromLocalFile(managed_path));
		if (!backup.open(QIODevice::WriteOnly
					  | QIODevice::Truncate
					  | QIODevice::Text)) {
			_exit(2);
		}
		if (backup.write(payload) != payload.size()) {
			_exit(3);
		}
		if (!backup.flush()) {
			_exit(4);
		}

		const char ready = '1';
		if (write(ready_pipe[1], &ready, 1) != 1) {
			_exit(5);
		}
		close(ready_pipe[1]);

		for (;;) {
			pause();
		}
	}

	close(ready_pipe[1]);
	char ready = 0;
	REQUIRE(read(ready_pipe[0], &ready, 1) == 1);
	close(ready_pipe[0]);
	REQUIRE(ready == '1');

	auto active_files = KAutoSaveFile::allStaleFiles();
	CHECK(active_files.isEmpty());
	for (auto *file : active_files) {
		delete file;
	}

	REQUIRE(kill(child_pid, SIGKILL) == 0);
	int status = 0;
	REQUIRE(waitpid(child_pid, &status, 0) == child_pid);
	REQUIRE(WIFSIGNALED(status));
	REQUIRE(WTERMSIG(status) == SIGKILL);

	auto stale_files = KAutoSaveFile::allStaleFiles();
	REQUIRE(stale_files.size() == 1);

	std::unique_ptr<KAutoSaveFile> stale_file(stale_files.takeFirst());
	CHECK(stale_file->managedFile().path()
		  == QFileInfo(managed_path).absoluteFilePath());
	REQUIRE(stale_file->open(QIODevice::ReadOnly | QIODevice::Text));
	CHECK(stale_file->readAll() == payload);

	const auto autosave_file_name = stale_file->fileName();
	const auto metadata_file_name = autosave_file_name + QStringLiteral(".path");
	const auto lock_file_name = autosave_file_name + QStringLiteral(".lock");
	stale_file.reset();

	CHECK_FALSE(QFile::exists(autosave_file_name));
	CHECK_FALSE(QFile::exists(metadata_file_name));
	CHECK_FALSE(QFile::exists(lock_file_name));
#endif
}

TEST_CASE("Multiple KAutoSaveFile generations for one managed file are all "
		  "found stale after a crash", "[nokde][autosave]")
{
#ifndef Q_OS_UNIX
	SUCCEED("crash-style stale lock test is Unix-only");
#else
		//Simulates QETProject's rotating crash-recovery generations
		//(BackupGenerations snapshots written round-robin): several
		//KAutoSaveFile instances sharing one managed file, alive at once.
	QTemporaryDir data_home;
	REQUIRE(data_home.isValid());

	qputenv("XDG_DATA_HOME", QFile::encodeName(data_home.path()));
	QCoreApplication::setOrganizationName(QStringLiteral("QElectroTech"));
	QCoreApplication::setApplicationName(
		QStringLiteral("KAutoSaveFileGenerationsTest"));

	const auto managed_path = data_home.filePath(QStringLiteral("project.qet"));
	QFile managed_file(managed_path);
	REQUIRE(managed_file.open(QIODevice::WriteOnly | QIODevice::Text));
	REQUIRE(managed_file.write("<project/>\n") > 0);
	managed_file.close();

	constexpr int generations = 3;

	int ready_pipe[2] = {-1, -1};
	REQUIRE(pipe(ready_pipe) == 0);

	const auto child_pid = fork();
	REQUIRE(child_pid >= 0);

	if (child_pid == 0) {
		close(ready_pipe[0]);

		std::vector<std::unique_ptr<KAutoSaveFile>> backups;
		for (int i = 0; i < generations; ++i) {
			auto backup = std::make_unique<KAutoSaveFile>(
				QUrl::fromLocalFile(managed_path));
			if (!backup->open(QIODevice::WriteOnly
						  | QIODevice::Truncate
						  | QIODevice::Text)) {
				_exit(2);
			}
			const QByteArray payload =
				"<project><generation n=\"" + QByteArray::number(i)
				+ "\" /></project>\n";
			if (backup->write(payload) != payload.size()) {
				_exit(3);
			}
			if (!backup->flush()) {
				_exit(4);
			}
				//Give each generation a distinct, increasing mtime.
			struct timespec pause{0, 20 * 1000 * 1000};
			nanosleep(&pause, nullptr);
			backups.push_back(std::move(backup));
		}

		const char ready = '1';
		if (write(ready_pipe[1], &ready, 1) != 1) {
			_exit(5);
		}
		close(ready_pipe[1]);

		for (;;) {
			pause();
		}
	}

	close(ready_pipe[1]);
	char ready = 0;
	REQUIRE(read(ready_pipe[0], &ready, 1) == 1);
	close(ready_pipe[0]);
	REQUIRE(ready == '1');

	REQUIRE(kill(child_pid, SIGKILL) == 0);
	int status = 0;
	REQUIRE(waitpid(child_pid, &status, 0) == child_pid);
	REQUIRE(WIFSIGNALED(status));
	REQUIRE(WTERMSIG(status) == SIGKILL);

	auto stale_files = KAutoSaveFile::allStaleFiles();
	REQUIRE(stale_files.size() == generations);

	std::sort(stale_files.begin(), stale_files.end(),
		[](KAutoSaveFile *a, KAutoSaveFile *b) {
			return QFileInfo(*a).lastModified() < QFileInfo(*b).lastModified();
		});

	for (int i = 0; i < generations; ++i) {
		std::unique_ptr<KAutoSaveFile> stale_file(stale_files.at(i));
		CHECK(stale_file->managedFile().path()
			  == QFileInfo(managed_path).absoluteFilePath());
		REQUIRE(stale_file->open(QIODevice::ReadOnly | QIODevice::Text));
		const QByteArray content = stale_file->readAll();
		CHECK(content.contains(
			"generation n=\"" + QByteArray::number(i) + "\""));
	}
#endif
}

namespace {

struct ManagedPath
{
	const char *kind;
		//As a file dialog, the command line or Explorer gives it
	QString path;
		//The project file opened again from the recovery file
	QString expected;
};

QList<ManagedPath> managedPaths()
{
		//A folder name of 30 characters, for a path longer than 260
	const QString long_folders
		= QStringLiteral("a folder name of thirty chars/").repeated(10);
#ifdef Q_OS_WIN
	return {
		{"drive letter",
		 QStringLiteral(u"C:/Users/\u00C5sa/Documents/project.qet"),
		 QStringLiteral(u"C:/Users/\u00C5sa/Documents/project.qet")},
		{"backslashes",
		 QStringLiteral(u"C:\\Users\\\u00C5sa\\Documents\\project.qet"),
		 QStringLiteral(u"C:/Users/\u00C5sa/Documents/project.qet")},
		{"lower case drive letter",
		 QStringLiteral(u"c:/users/\u00E5sa/project.qet"),
		 QStringLiteral(u"c:/users/\u00E5sa/project.qet")},
		{"root of a drive",
		 QStringLiteral("C:/project.qet"),
		 QStringLiteral("C:/project.qet")},
		{"mapped network drive",
		 QStringLiteral("Z:/Projekt/plan.qet"),
		 QStringLiteral("Z:/Projekt/plan.qet")},
		{"MSYS2 home folder",
		 QStringLiteral(u"C:/msys64/home/\u00E5sa/project.qet"),
		 QStringLiteral(u"C:/msys64/home/\u00E5sa/project.qet")},
		{"network share",
		 QStringLiteral("\\\\server\\share\\folder\\project.qet"),
		 QStringLiteral("//server/share/folder/project.qet")},
		{"network share with slashes",
		 QStringLiteral("//server/share/folder/project.qet"),
		 QStringLiteral("//server/share/folder/project.qet")},
		{"WSL folder",
		 QStringLiteral(u"\\\\wsl.localhost\\Ubuntu\\home\\\u00E5sa\\project.qet"),
		 QStringLiteral(u"//wsl.localhost/Ubuntu/home/\u00E5sa/project.qet")},
		{"WSL folder, older name",
		 QStringLiteral(u"\\\\wsl$\\Ubuntu\\home\\\u00E5sa\\project.qet"),
		 QStringLiteral(u"//wsl$/Ubuntu/home/\u00E5sa/project.qet")},
		{"characters of URLs",
		 QStringLiteral(u"C:/Users/\u00C5sa/OneDrive - Firma AB/"
						u"Projekt #1/100% klar & ny;'v2'.qet"),
		 QStringLiteral(u"C:/Users/\u00C5sa/OneDrive - Firma AB/"
						u"Projekt #1/100% klar & ny;'v2'.qet")},
		{"other scripts",
		 QStringLiteral(u"C:/Users/\u0416/\u65E5\u672C/project.qet"),
		 QStringLiteral(u"C:/Users/\u0416/\u65E5\u672C/project.qet")},
		{"dot folders",
		 QStringLiteral(u"C:/Users/\u00C5sa/a/../b/./project.qet"),
		 QStringLiteral(u"C:/Users/\u00C5sa/b/project.qet")},
		{"path longer than 260",
		 QStringLiteral("C:/") + long_folders + QStringLiteral("project.qet"),
		 QStringLiteral("C:/") + long_folders + QStringLiteral("project.qet")},
		{"long path prefix",
		 QStringLiteral(u"\\\\?\\C:\\Users\\\u00C5sa\\project.qet"),
		 QString()},
	};
#else
	return {
		{"home folder",
		 QStringLiteral(u"/home/\u00E5sa/Documents/project.qet"),
		 QStringLiteral(u"/home/\u00E5sa/Documents/project.qet")},
		{"Windows drive in WSL",
		 QStringLiteral(u"/mnt/c/Users/\u00C5sa/OneDrive - Firma AB/project.qet"),
		 QStringLiteral(u"/mnt/c/Users/\u00C5sa/OneDrive - Firma AB/project.qet")},
		{"characters of URLs",
		 QStringLiteral(u"/home/\u00E5sa/Projekt #1/100% klar & ny;'v2'.qet"),
		 QStringLiteral(u"/home/\u00E5sa/Projekt #1/100% klar & ny;'v2'.qet")},
		{"colon and backslash in names",
		 QStringLiteral(u"/home/\u00E5sa/C:\\x/a:b.qet"),
		 QStringLiteral(u"/home/\u00E5sa/C:\\x/a:b.qet")},
		{"a folder named like a drive",
		 QStringLiteral("/C:/Users/project.qet"),
		 QStringLiteral("/C:/Users/project.qet")},
		{"other scripts",
		 QStringLiteral(u"/home/\u0416/\u65E5\u672C/project.qet"),
		 QStringLiteral(u"/home/\u0416/\u65E5\u672C/project.qet")},
		{"dot folders",
		 QStringLiteral(u"/home/\u00E5sa/a/../b/./project.qet"),
		 QStringLiteral(u"/home/\u00E5sa/b/project.qet")},
		{"path longer than 260",
		 QStringLiteral("/home/") + long_folders + QStringLiteral("project.qet"),
		 QStringLiteral("/home/") + long_folders + QStringLiteral("project.qet")},
	};
#endif
}

bool samePath(const QString &first, const QString &second)
{
#ifdef Q_OS_WIN
	return first.compare(second, Qt::CaseInsensitive) == 0;
#else
	return first == second;
#endif
}

} // namespace

TEST_CASE("Qt-only KAutoSaveFile recovers the project of every kind of path "
		  "after a crash", "[nokde][autosave]")
{
		//The test and kautosavefile_writer share their recovery folder
	const auto application = QStringLiteral("KAutoSaveFilePathTest%1")
		.arg(QCoreApplication::applicationPid());
	QStandardPaths::setTestModeEnabled(true);
	QCoreApplication::setOrganizationName(QStringLiteral("QElectroTech"));
	QCoreApplication::setApplicationName(application);
	const auto data_dir = QStandardPaths::writableLocation(
		QStandardPaths::AppDataLocation);
	REQUIRE_FALSE(data_dir.isEmpty());
	const auto cleanup = qScopeGuard([&] {
		QDir(data_dir).removeRecursively();
		QStandardPaths::setTestModeEnabled(false);
	});

	for (const auto &managed : managedPaths()) {
		INFO(managed.kind << ": " << managed.path.toStdString());
			//As QElectroTech names the project it opens
		const auto project_path
			= QDir::cleanPath(QFileInfo(managed.path).absoluteFilePath());

		QProcess writer;
		writer.start(QStringLiteral(QET_AUTOSAVE_WRITER),
					 {application, managed.path});
		REQUIRE(writer.waitForStarted());
		REQUIRE(writer.waitForReadyRead(30000));
		REQUIRE(writer.readAllStandardOutput().startsWith("ready"));

			//QElectroTech still runs: its recovery file is locked
		auto active_files = KAutoSaveFile::allStaleFiles();
		CHECK(active_files.isEmpty());
		qDeleteAll(active_files);

		writer.kill();
		REQUIRE(writer.waitForFinished());

		auto stale_files = KAutoSaveFile::allStaleFiles();
		REQUIRE(stale_files.size() == 1);
		std::unique_ptr<KAutoSaveFile> stale_file(stale_files.takeFirst());

		const auto recovered = stale_file->managedFile();
		CHECK(recovered.isLocalFile());
		CHECK(samePath(recovered.toLocalFile(), project_path));
		if (!managed.expected.isNull()) {
			CHECK(samePath(recovered.toLocalFile(), managed.expected));
		}

		auto matching = KAutoSaveFile::staleFiles(
			QUrl::fromLocalFile(managed.path));
		CHECK(matching.size() == 1);
		qDeleteAll(matching);

		REQUIRE(stale_file->open(QIODevice::ReadOnly));
		CHECK(stale_file->readAll() == managed.path.toUtf8());

			//Opened, the recovery file is deleted with its object
		stale_file.reset();
		auto left_files = KAutoSaveFile::allStaleFiles();
		CHECK(left_files.isEmpty());
		qDeleteAll(left_files);
	}
}

TEST_CASE("Qt-only KAutoSaveFile reads the recovery files of earlier versions",
		  "[nokde][autosave]")
{
		//Before file URLs, the project's path was stored as it is
	const auto application = QStringLiteral("KAutoSaveFileEarlierTest%1")
		.arg(QCoreApplication::applicationPid());
	QStandardPaths::setTestModeEnabled(true);
	QCoreApplication::setOrganizationName(QStringLiteral("QElectroTech"));
	QCoreApplication::setApplicationName(application);
	const auto data_dir = QStandardPaths::writableLocation(
		QStandardPaths::AppDataLocation);
	REQUIRE_FALSE(data_dir.isEmpty());
	const auto cleanup = qScopeGuard([&] {
		QDir(data_dir).removeRecursively();
		QStandardPaths::setTestModeEnabled(false);
	});

#ifdef Q_OS_WIN
	const auto project_path
		= QStringLiteral(u"C:/Users/\u00C5sa/Projekt #1/project.qet");
#else
	const auto project_path
		= QStringLiteral(u"/home/\u00E5sa/Projekt #1/project.qet");
#endif
	QDir autosave_dir(data_dir + QStringLiteral("/autosave"));
	REQUIRE(autosave_dir.mkpath(QStringLiteral(".")));
	const auto recovery = autosave_dir.filePath(
		QStringLiteral("project.qet_0123456789abcdef_1.qetautosave"));
	QFile recovery_file(recovery);
	REQUIRE(recovery_file.open(QIODevice::WriteOnly));
	REQUIRE(recovery_file.write("<project/>") > 0);
	recovery_file.close();
	QFile path_file(recovery + QStringLiteral(".path"));
	REQUIRE(path_file.open(QIODevice::WriteOnly | QIODevice::Text));
	REQUIRE(path_file.write(project_path.toUtf8() + '\n') > 0);
	path_file.close();

	auto stale_files = KAutoSaveFile::allStaleFiles();
	REQUIRE(stale_files.size() == 1);
	std::unique_ptr<KAutoSaveFile> stale_file(stale_files.takeFirst());
	CHECK(stale_file->managedFile().toLocalFile() == project_path);

	auto matching = KAutoSaveFile::staleFiles(
		QUrl::fromLocalFile(project_path));
	CHECK(matching.size() == 1);
	qDeleteAll(matching);
}
