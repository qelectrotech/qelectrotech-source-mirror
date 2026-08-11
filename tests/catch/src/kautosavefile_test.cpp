#include "../../../sources/ui/nokde/kautosavefile.h"

#include <catch2/catch.hpp>

#include <QCoreApplication>
#include <QDateTime>
#include <QFile>
#include <QFileInfo>
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
