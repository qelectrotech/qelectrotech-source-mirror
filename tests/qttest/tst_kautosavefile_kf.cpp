// SPDX-License-Identifier: GPL-2.0-or-later
#include "../../thirdparty/kcoreaddons/include/kautosavefile.h"

#include <QtTest>

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QLatin1Char>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QTemporaryFile>
#include <QUrl>

#include <memory>

#ifdef Q_OS_UNIX
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#endif

namespace {

/**
 * Autosave file name in the exact KAutoSaveFilePrivate::tempFileName()
 * format: encoded file name + 3 junk chars + scheme + '_' + encoded
 * directory + 8 junk chars. The junk must not collide with the other
 * components: the upstream parser finds the file-name/path separator
 * by searching for its last three characters.
 */
QString fabricatedStaleFileName(const QUrl &managed_file)
{
	const QString protocol(managed_file.scheme());
	const QByteArray encoded_directory = QUrl::toPercentEncoding(
		managed_file.adjusted(QUrl::RemoveFilename | QUrl::StripTrailingSlash).path());
	const QString directory = QString::fromLatin1(encoded_directory);
	const QByteArray encoded_file_name = QUrl::toPercentEncoding(
		managed_file.fileName());
	const QString junk = QStringLiteral("k7m9x2b4");

	QString file_name = QString::fromLatin1(encoded_file_name);
	file_name += junk.right(3) + protocol + QLatin1Char('_')
		+ directory + junk;
	return file_name;
}

// <GenericDataLocation>/stalefiles/<application name>, as upstream stores them.
QString staleFilesDir(const QString &app_name)
{
	return QStandardPaths::writableLocation(
		QStandardPaths::GenericDataLocation)
		+ QStringLiteral("/stalefiles/") + app_name;
}

/**
 * Writes one fabricated backup, lock file included, and returns its path.
 * Returns {} on failure: QVERIFY() cannot be used in here, its failing
 * branch expands to a plain "return;" which is invalid for a QString.
 * Opening a recovered backup consumes it (destruction removes the
 * autosave file), so call this again before every lookup.
 */
QString fabricateStaleBackup(const QString &stale_files_dir,
							 const QUrl &managed_file,
							 const QByteArray &payload)
{
	if (!QDir().mkpath(stale_files_dir)) {
		return {};
	}
	const auto file_path = stale_files_dir + QLatin1Char('/')
		+ fabricatedStaleFileName(managed_file);

	QFile file(file_path);
	if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
		return {};
	}
	if (file.write(payload) != payload.size()) {
		return {};
	}
	file.close();

	// The crash-leftover lock must look exactly like a real stale one, but
	// QLockFile's on-disk format differs per platform (Unix: "pid;host;app",
	// Windows: newline-separated with a machine UUID). Lock a scratch file
	// for real and reuse its content with a PID no living process has:
	// that is what makes the backup "stale" to QLockFile once a recovered
	// KAutoSaveFile is opened.
	QTemporaryFile scratch;
	if (!scratch.open()) {
		return {};
	}
	const QString scratch_lock = scratch.fileName() + QStringLiteral(".lock");
	QLockFile real(scratch_lock);
	if (!real.tryLock()) {
		return {};
	}
	QFile read_lock(scratch_lock);
	if (!read_lock.open(QIODevice::ReadOnly)) {
		return {};
	}
	QByteArray lock_data = read_lock.readAll();
	read_lock.close();
	qint64 own_pid = 0;
	 QString hostname, appname;
	if (!real.getLockInfo(&own_pid, &hostname, &appname) || own_pid <= 0) {
		return {};
	}
	const QByteArray pid_str = QByteArray::number(own_pid);
	if (!lock_data.startsWith(pid_str)) {
		return {};
	}
	lock_data.replace(0, pid_str.size(), QByteArrayLiteral("4000000"));
	real.unlock();

	QFile lock(file_path + QStringLiteral(".lock"));
	if (!lock.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
		return {};
	}
	if (lock.write(lock_data) != lock_data.size()) {
		return {};
	}
	lock.close();

	return file_path;
}

} // namespace

/**
 * Recovery tests for the bundled minimal KF KAutoSaveFile
 * (thirdparty/kcoreaddons), used by builds with BUILD_KF_MINIMAL=ON.
 * The crash-style test is Unix-only; the fabricated stale-file lookups
 * also exercise the Windows-style drive-letter and UNC managed URLs on
 * every platform.
 */
class tst_kautosavefile_kf : public QObject
{
	Q_OBJECT

private slots:
	void crashRecovery();
	void driveLetterPathRoundTrip();
	void uncPathLosesHost();
};

void tst_kautosavefile_kf::crashRecovery()
{
#ifndef Q_OS_UNIX
	QSKIP("crash-style stale lock test is Unix-only");
#else
	QTemporaryDir data_home;
	QVERIFY(data_home.isValid());

	qputenv("XDG_DATA_HOME", QFile::encodeName(data_home.path()));
	QCoreApplication::setOrganizationName(QStringLiteral("QElectroTech"));
	QCoreApplication::setApplicationName(QStringLiteral("KAutoSaveFileKFTest"));

	const auto managed_path = data_home.filePath(QStringLiteral("project.qet"));
	QFile managed_file(managed_path);
	QVERIFY(managed_file.open(QIODevice::WriteOnly | QIODevice::Text));
	QVERIFY(managed_file.write("<project/>\n") > 0);
	managed_file.close();

	int ready_pipe[2] = {-1, -1};
	QVERIFY(pipe(ready_pipe) == 0);

	// Simulate a crash: the child writes the backup, then hangs until
	// SIGKILL, so no cleanup runs.
	const QByteArray payload("<project><diagram /></project>\n");
	const auto child_pid = fork();
	QVERIFY(child_pid >= 0);

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
	QVERIFY(read(ready_pipe[0], &ready, 1) == 1);
	close(ready_pipe[0]);
	QVERIFY(ready == '1');

	// While the child lives the backup is locked: staleFiles() lists it
	// either way (it does not check locks), but opening must fail.
	auto live_files = KAutoSaveFile::allStaleFiles();
	QCOMPARE(live_files.size(), 1);
	{
		std::unique_ptr<KAutoSaveFile> live_file(live_files.takeFirst());
		QVERIFY(!live_file->open(QIODevice::ReadOnly | QIODevice::Text));
	}

	QVERIFY(kill(child_pid, SIGKILL) == 0);
	int status = 0;
	QVERIFY(waitpid(child_pid, &status, 0) == child_pid);
	QVERIFY(WIFSIGNALED(status));
	QVERIFY(WTERMSIG(status) == SIGKILL);

	// With the holding process gone the lock is stale and the backup
	// is readable again.
	auto stale_files = KAutoSaveFile::allStaleFiles();
	QCOMPARE(stale_files.size(), 1);

	std::unique_ptr<KAutoSaveFile> stale_file(stale_files.takeFirst());
	QCOMPARE(stale_file->managedFile().path(),
			 QFileInfo(managed_path).absoluteFilePath());
	QVERIFY(stale_file->open(QIODevice::ReadOnly | QIODevice::Text));
	QCOMPARE(stale_file->readAll(), payload);

	const auto autosave_file_name = stale_file->fileName();
	const auto lock_file_name = autosave_file_name + QStringLiteral(".lock");
	stale_file.reset();

	QVERIFY(!QFile::exists(autosave_file_name));
	QVERIFY(!QFile::exists(lock_file_name));
#endif
}

void tst_kautosavefile_kf::driveLetterPathRoundTrip()
{
	QTemporaryDir data_home;
	QVERIFY(data_home.isValid());

	qputenv("XDG_DATA_HOME", QFile::encodeName(data_home.path()));
	QCoreApplication::setOrganizationName(QStringLiteral("QElectroTech"));
	QCoreApplication::setApplicationName(
		QStringLiteral("KAutoSaveFileKFDriveLetterTest"));

	const QString app_name =
		QCoreApplication::instance()->applicationName();
	const auto dir = staleFilesDir(app_name);

	// A Windows URL's path is /C:/CAD/project.qet; the lookup is
	// percent-encoded string matching, so this runs on every platform.
	const QUrl managed_file = QUrl(QStringLiteral("file:///C:/CAD/project.qet"));
	const QByteArray payload("<project><windows /></project>\n");

	// Lookup by the managed file's URL.
	const auto fabricated_file_name = fabricateStaleBackup(dir, managed_file, payload);
	QVERIFY(!fabricated_file_name.isEmpty());
	auto stale_files = KAutoSaveFile::staleFiles(managed_file);
	QCOMPARE(stale_files.size(), 1);
	{
		std::unique_ptr<KAutoSaveFile> by_url(stale_files.takeFirst());
		QVERIFY(by_url->open(QIODevice::ReadOnly));
		QCOMPARE(by_url->readAll(), payload);
	}

	// allStaleFiles() rebuilds a scheme-less URL, keeping the drive letter.
	QVERIFY(!fabricateStaleBackup(dir, managed_file, payload).isEmpty());
	stale_files = KAutoSaveFile::allStaleFiles();
	QCOMPARE(stale_files.size(), 1);
	{
		std::unique_ptr<KAutoSaveFile> by_scan(stale_files.takeFirst());
		QCOMPARE(by_scan->managedFile().path(),
				 QStringLiteral("/C:/CAD/project.qet"));
		QVERIFY(by_scan->open(QIODevice::ReadOnly));
		QCOMPARE(by_scan->readAll(), payload);
	}

	QVERIFY(!QFile::exists(fabricated_file_name));
	QVERIFY(!QFile::exists(fabricated_file_name + QStringLiteral(".lock")));
	QDir(dir).removeRecursively();
}

void tst_kautosavefile_kf::uncPathLosesHost()
{
	QTemporaryDir data_home;
	QVERIFY(data_home.isValid());

	qputenv("XDG_DATA_HOME", QFile::encodeName(data_home.path()));
	QCoreApplication::setOrganizationName(QStringLiteral("QElectroTech"));
	QCoreApplication::setApplicationName(QStringLiteral("KAutoSaveFileKFUNCPathTest"));

	const QString app_name =
		QCoreApplication::instance()->applicationName();
	const auto dir = staleFilesDir(app_name);

	// The URL carries the host; the autosave file name never encodes it.
	const QUrl managed_file = QUrl(
		QStringLiteral("file://hefs01/Filservern/CAD/project.qet"));
	const QByteArray payload("<project><unc /></project>\n");

	// Still matches: the comparison uses path and file name only.
	QVERIFY(!fabricateStaleBackup(dir, managed_file, payload).isEmpty());
	auto stale_files = KAutoSaveFile::staleFiles(managed_file);
	QCOMPARE(stale_files.size(), 1);
	{
		std::unique_ptr<KAutoSaveFile> by_url(stale_files.takeFirst());
		QVERIFY(by_url->open(QIODevice::ReadOnly));
		QCOMPARE(by_url->readAll(), payload);
	}

	// Upstream limitation kept visible here: recovery loses the host,
	// returning /Filservern/CAD/project.qet rather than the UNC path.
	QVERIFY(!fabricateStaleBackup(dir, managed_file, payload).isEmpty());
	stale_files = KAutoSaveFile::allStaleFiles();
	QCOMPARE(stale_files.size(), 1);
	{
		std::unique_ptr<KAutoSaveFile> by_scan(stale_files.takeFirst());
		QVERIFY(by_scan->managedFile().host().isEmpty());
		QCOMPARE(by_scan->managedFile().path(),
				 QStringLiteral("/Filservern/CAD/project.qet"));
		QVERIFY(by_scan->open(QIODevice::ReadOnly));
		QCOMPARE(by_scan->readAll(), payload);
	}
	QDir(dir).removeRecursively();
}

QTEST_APPLESS_MAIN(tst_kautosavefile_kf)
#include "tst_kautosavefile_kf.moc"
