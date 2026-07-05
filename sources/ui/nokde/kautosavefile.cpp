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
#include "kautosavefile.h"

#include <QCryptographicHash>
#include <QDir>
#include <QFileInfo>
#include <QLockFile>
#include <QRandomGenerator>
#include <QStandardPaths>
#include <QTextStream>

namespace {

const auto autosaveSuffix = QStringLiteral(".qetautosave");

QString staleFilesDir()
{
	auto data_dir = QStandardPaths::writableLocation(
		QStandardPaths::AppDataLocation);
	while (data_dir.endsWith(QLatin1Char('/'))) {
		data_dir.chop(1);
	}
	if (data_dir.isEmpty()) {
		return {};
	}

	return data_dir + QDir::separator() + QStringLiteral("autosave");
}

QString metadataFileName(const QString &autosave_file_name)
{
	return autosave_file_name + QStringLiteral(".path");
}

QString lockFileName(const QString &autosave_file_name)
{
	return autosave_file_name + QStringLiteral(".lock");
}

QUrl normalizeManagedFile(const QUrl &url)
{
	if (url.isEmpty()) {
		return {};
	}

	if (url.isLocalFile() || url.scheme().isEmpty()) {
		const auto path = url.isLocalFile() ? url.toLocalFile() : url.path();
		QUrl normalized;
		normalized.setPath(QDir::cleanPath(QFileInfo(path).absoluteFilePath()));
		return normalized;
	}

	return url;
}

QString storedManagedFile(const QUrl &url)
{
	if (url.isLocalFile() || url.scheme().isEmpty()) {
		return url.path();
	}

	return url.toString(QUrl::FullyEncoded);
}

QUrl managedFileFromStorage(const QString &stored_path)
{
	if (!stored_path.startsWith(QLatin1Char('/'))) {
		return QUrl(stored_path);
	}

	QUrl url;
	url.setPath(QDir::cleanPath(stored_path));
	return url;
}

bool writeManagedFileMetadata(const QString &autosave_file_name, const QUrl &managed_file)
{
	QFile metadata_file(metadataFileName(autosave_file_name));
	if (!metadata_file.open(QIODevice::WriteOnly
					  | QIODevice::Truncate
					  | QIODevice::Text)) {
		return false;
	}

	QTextStream stream(&metadata_file);
	stream << storedManagedFile(managed_file) << '\n';
	stream.flush();
	return metadata_file.error() == QFile::NoError;
}

QUrl readManagedFileMetadata(const QString &autosave_file_name)
{
	QFile metadata_file(metadataFileName(autosave_file_name));
	if (!metadata_file.open(QIODevice::ReadOnly | QIODevice::Text)) {
		return {};
	}

	QTextStream stream(&metadata_file);
	const auto stored_path = stream.readLine().trimmed();
	if (stored_path.isEmpty()) {
		return {};
	}

	return normalizeManagedFile(managedFileFromStorage(stored_path));
}

QString autosaveFileName(const QUrl &managed_file)
{
	const auto stored_path = storedManagedFile(managed_file);
	const auto digest = QCryptographicHash::hash(
		stored_path.toUtf8(),
		QCryptographicHash::Sha256).toHex().left(16);

	auto basename = QFileInfo(managed_file.path()).fileName();
	if (basename.isEmpty()) {
		basename = QStringLiteral("autosave");
	}

	auto encoded_basename = QString::fromLatin1(
		QUrl::toPercentEncoding(basename));
	if (encoded_basename.size() > 80) {
		encoded_basename.truncate(80);
	}

	const auto random = QString::number(
		QRandomGenerator::global()->generate64(), 16);

	return QStringLiteral("%1_%2_%3%4").arg(
		encoded_basename,
		QString::fromLatin1(digest),
		random,
		autosaveSuffix);
}

QStringList findAllStaleFiles(const QString &application_name)
{
	Q_UNUSED(application_name)

	const auto dir_path = staleFilesDir();
	if (dir_path.isEmpty()) {
		return {};
	}

	QDir dir(dir_path);
	const auto entries = dir.entryList(
		{QStringLiteral("*") + autosaveSuffix},
		QDir::Files);

	QStringList files;
	for (const auto &entry : entries) {
		files << dir.absoluteFilePath(entry);
	}

	return files;
}

bool autosaveFileIsRecoverable(const QString &autosave_file_name)
{
	QLockFile lock(lockFileName(autosave_file_name));
	lock.setStaleLockTime(60 * 1000);
	if (!lock.tryLock()) {
		return false;
	}

	lock.unlock();
	return true;
}

} // namespace

KAutoSaveFile::KAutoSaveFile(const QUrl &filename, QObject *parent) :
	QFile{parent}
{
	setManagedFile(filename);
}

KAutoSaveFile::KAutoSaveFile(QObject *parent) :
	QFile{parent}
{
}

KAutoSaveFile::~KAutoSaveFile()
{
	releaseLock();
}

QUrl KAutoSaveFile::managedFile() const
{
	return m_managed_file;
}

void KAutoSaveFile::setManagedFile(const QUrl &filename)
{
	releaseLock();

	m_managed_file = normalizeManagedFile(filename);
	m_managed_file_name_changed = true;
	setFileName({});
}

void KAutoSaveFile::releaseLock()
{
	if (m_lock && m_lock->isLocked()) {
		const auto autosave_file_name = fileName();
		m_lock.reset();

		if (!autosave_file_name.isEmpty()) {
			QFile::remove(metadataFileName(autosave_file_name));
			remove();
		}
	} else {
		m_lock.reset();
	}
}

bool KAutoSaveFile::open(OpenMode openmode)
{
	if (m_managed_file.isEmpty()) {
		return false;
	}

	if (m_managed_file_name_changed) {
		const auto stale_dir = staleFilesDir();
		if (stale_dir.isEmpty() || !QDir().mkpath(stale_dir)) {
			return false;
		}

		setFileName(QDir(stale_dir).absoluteFilePath(
			autosaveFileName(m_managed_file)));
		if (!writeManagedFileMetadata(fileName(), m_managed_file)) {
			setFileName({});
			return false;
		}

		m_managed_file_name_changed = false;
	}

	if (!QFile::open(openmode)) {
		return false;
	}

	if (!m_lock) {
		m_lock = std::make_unique<QLockFile>(lockFileName(fileName()));
		m_lock->setStaleLockTime(60 * 1000);
	}

	if (m_lock->isLocked() || m_lock->tryLock()) {
		return true;
	}

	close();
	return false;
}

QList<KAutoSaveFile *> KAutoSaveFile::staleFiles(
	const QUrl &url,
	const QString &applicationName)
{
	const auto managed_file_filter = normalizeManagedFile(url);
	QList<KAutoSaveFile *> stale_files;

	for (const auto &file : findAllStaleFiles(applicationName)) {
		const auto managed_file = readManagedFileMetadata(file);
		if (managed_file.isEmpty()) {
			continue;
		}
		if (!managed_file_filter.isEmpty()
			&& managed_file != managed_file_filter) {
			continue;
		}
		if (!autosaveFileIsRecoverable(file)) {
			continue;
		}

		auto *stale_file = new KAutoSaveFile(managed_file);
		stale_file->setFileName(file);
		stale_file->m_managed_file_name_changed = false;
		stale_files << stale_file;
	}

	return stale_files;
}

QList<KAutoSaveFile *> KAutoSaveFile::allStaleFiles(const QString &applicationName)
{
	return staleFiles({}, applicationName);
}
