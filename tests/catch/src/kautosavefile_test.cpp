#include "../../../sources/ui/nokde/kautosavefile.h"

#include <catch2/catch.hpp>

#include <QCoreApplication>
#include <QFile>
#include <QFileInfo>
#include <QTemporaryDir>
#include <QUrl>

#include <memory>

#ifdef Q_OS_UNIX
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

	const QByteArray payload("<project><diagram /></project>\n");
	const auto child_pid = fork();
	REQUIRE(child_pid >= 0);

	if (child_pid == 0) {
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

		_exit(0);
	}

	int status = 0;
	REQUIRE(waitpid(child_pid, &status, 0) == child_pid);
	REQUIRE(WIFEXITED(status));
	REQUIRE(WEXITSTATUS(status) == 0);

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
