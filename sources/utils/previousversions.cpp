/*
	Copyright 2006-2026 The QElectroTech Team
	This file is part of QElectroTech.

	QElectroTech is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 2 of the License, or
	(at your option) any later version.

	QElectroTech is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with QElectroTech. If not, see <http://www.gnu.org/licenses/>.
*/
#include "previousversions.h"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSettings>

#ifdef Q_OS_WIN
#	include <windows.h>
#else
#	include <unistd.h>
#endif

namespace {
const QString CountKey = QStringLiteral("diagrameditor/previous-versions");
constexpr int DefaultCount = 5;

	//What a version of @p info is named, without the extension: its base
	//name and a time
QString versionPrefix(const QFileInfo &info)
{
	return info.completeBaseName() + QLatin1Char(' ');
}

	//True if the files @p a and @p b hold the same bytes, read until the
	//first that differs: a saved project differs from its previous
	//version within its first pages, where hashing both read them whole
bool sameContent(const QString &a, const QString &b)
{
	QFile first(a), second(b);
	if (!first.open(QIODevice::ReadOnly) || !second.open(QIODevice::ReadOnly)
			|| first.size() != second.size())
		return false;
	constexpr qint64 chunk = 1 << 16;
	while (!first.atEnd()) {
		const QByteArray x = first.read(chunk), y = second.read(chunk);
		if (x.isEmpty() || x != y) return false;
	}
	return second.atEnd();
}

	//@p target as a second name of the file @p path, without copying it.
	//Only for a .qetdb: its save puts a new file in place by rename
	//(QetDbFile::save()), never writing the old one, which then has no
	//other name than @p target. False where the folder's file system
	//has no hard links, and the version is copied
bool linkVersion(const QString &path, const QString &target)
{
	if (!path.endsWith(QLatin1String(".qetdb"), Qt::CaseInsensitive)) return false;
#ifdef Q_OS_WIN
	return CreateHardLinkW(reinterpret_cast<const wchar_t *>(QDir::toNativeSeparators(target).utf16()),
						   reinterpret_cast<const wchar_t *>(QDir::toNativeSeparators(path).utf16()),
						   nullptr);
#else
	return ::link(QFile::encodeName(path).constData(), QFile::encodeName(target).constData()) == 0;
#endif
}
}

int PreviousVersions::count()
{
	return qMax(0, QSettings().value(CountKey, DefaultCount).toInt());
}

void PreviousVersions::setCount(int count)
{
	QSettings().setValue(CountKey, qMax(0, count));
}

QString PreviousVersions::folder(const QString &file_path)
{
	const QFileInfo info(file_path);
	return info.absoluteDir().filePath(info.completeBaseName() + QStringLiteral("-backups"));
}

QStringList PreviousVersions::kept(const QString &file_path)
{
	const QFileInfo info(file_path);
	const QDir dir(folder(file_path));
		//Each copy has the time its version was saved (keep()); a .qet and
		//a .qetz or a .qetdb of the same project are versions of it alike
		//"????-??-?? ??-??-??" written with "\?", so that the compiler doesn't
		//see the trigraph "??-" in it (-Wtrigraphs)
	const QString stamp_ = QStringLiteral("?\?\?\?-?\?-?\? ?\?-?\?-?\?");
	QStringList names = dir.entryList({versionPrefix(info) + stamp_ + QStringLiteral("*.qet"),
									   versionPrefix(info) + stamp_ + QStringLiteral("*.qetz"),
									   versionPrefix(info) + stamp_ + QStringLiteral("*.qetdb")},
									  QDir::Files, QDir::Time);
	QStringList paths;
	for (const QString &name : std::as_const(names))
		paths << dir.filePath(name);
	return paths;
}

bool PreviousVersions::keep(const QString &file_path, int count, QString *error)
{
	const QFileInfo info(file_path);
	if (count <= 0 || !info.isFile()) return true;

	const QStringList versions = kept(file_path);
	if (!versions.isEmpty() && sameContent(versions.first(), file_path))
		return true;

	QDir dir(folder(file_path));
	if (!dir.exists() && !QDir().mkpath(dir.path())) {
		if (error) *error = QStringLiteral("cannot create the folder %1").arg(dir.path());
		return false;
	}
		//Named by the time this version was saved, made unique if another
		//was saved the same second
	const QDateTime saved = info.lastModified();
	const QString stem = versionPrefix(info) + saved.toString(QStringLiteral("yyyy-MM-dd HH-mm-ss"));
	QString target = dir.filePath(stem + QLatin1Char('.') + info.suffix());
	for (int n = 2 ; QFileInfo::exists(target) ; ++n)
		target = dir.filePath(stem + QStringLiteral(" (%1).").arg(n) + info.suffix());
	if (!linkVersion(file_path, target)) {
		if (!QFile::copy(file_path, target)) {
			if (error) *error = QStringLiteral("cannot copy %1 to %2").arg(file_path, target);
			return false;
		}
		QFile copy(target);
		if (copy.open(QIODevice::ReadWrite))
			copy.setFileTime(saved, QFileDevice::FileModificationTime);
	}

	const QStringList all = kept(file_path);
	for (int i = count ; i < all.size() ; ++i)
		QFile::remove(all.at(i));
	return true;
}
