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
#include <QtTest>

#include <QDateTime>
#include <QFile>
#include <QTemporaryDir>

#include "utils/previousversions.h"

// The versions of a project kept before it is saved over
// (sources/utils/previousversions.*)
class tst_previousversions : public QObject
{
	Q_OBJECT

	QTemporaryDir m_dir;

		// @p path holding @p text, saved at @p when
	static void write(const QString &path, const QByteArray &text, const QDateTime &when)
	{
		QFile file(path);
		QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Truncate));
		file.write(text);
		file.flush();	//else closing writes, and sets the time to now
		QVERIFY(file.setFileTime(when, QFileDevice::FileModificationTime));
	}

	static QByteArray read(const QString &path)
	{
		QFile file(path);
		return file.open(QIODevice::ReadOnly) ? file.readAll() : QByteArray();
	}

private slots:
	void initTestCase() { QVERIFY(m_dir.isValid()); }

	// Each save keeps the file as it was, newest first, named by the time
	// that version was saved, in "<name>-backups" beside it; only the
	// newest are kept
	void keepsTheNewest()
	{
		const QString project = m_dir.filePath(QStringLiteral("plant.qetz"));
		const QDateTime start(QDate(2026, 10, 16), QTime(8, 0, 0));
		for (int i = 0 ; i < 7 ; ++i) {
			write(project, "version " + QByteArray::number(i), start.addSecs(60 * i));
			QVERIFY(PreviousVersions::keep(project, 3));
		}
		const QStringList kept = PreviousVersions::kept(project);
		QCOMPARE(kept.size(), 3);
		QCOMPARE(read(kept.at(0)), QByteArray("version 6"));
		QCOMPARE(read(kept.at(2)), QByteArray("version 4"));
		QCOMPARE(QFileInfo(kept.at(0)).fileName(), QStringLiteral("plant 2026-10-16 08-06-00.qetz"));
		QCOMPARE(QFileInfo(kept.at(0)).absolutePath(), m_dir.filePath(QStringLiteral("plant-backups")));
			// The project itself is untouched
		QCOMPARE(read(project), QByteArray("version 6"));
	}

	// Saving the same bytes again keeps no second copy
	void sameBytesNotKeptTwice()
	{
		const QString project = m_dir.filePath(QStringLiteral("same.qet"));
		const QDateTime when(QDate(2026, 10, 16), QTime(9, 0, 0));
		write(project, "unchanged", when);
		QVERIFY(PreviousVersions::keep(project, 5));
		write(project, "unchanged", when.addSecs(30));
		QVERIFY(PreviousVersions::keep(project, 5));
		QCOMPARE(PreviousVersions::kept(project).size(), 1);
	}

	// Two versions saved the same second both kept
	void sameSecond()
	{
		const QString project = m_dir.filePath(QStringLiteral("fast.qetz"));
		const QDateTime when(QDate(2026, 10, 16), QTime(10, 0, 0));
		write(project, "first", when);
		QVERIFY(PreviousVersions::keep(project, 5));
		write(project, "second", when);
		QVERIFY(PreviousVersions::keep(project, 5));
		QCOMPARE(PreviousVersions::kept(project).size(), 2);
	}

	// Nothing kept for a new file, or when the count is 0; another
	// project's versions in the same folder are not counted
	void nothingToKeep()
	{
		const QString project = m_dir.filePath(QStringLiteral("new.qetz"));
		QVERIFY(PreviousVersions::keep(project, 5));
		QVERIFY(!QFileInfo::exists(PreviousVersions::folder(project)));
		write(project, "x", QDateTime(QDate(2026, 10, 16), QTime(11, 0, 0)));
		QVERIFY(PreviousVersions::keep(project, 0));
		QVERIFY(!QFileInfo::exists(PreviousVersions::folder(project)));
		QVERIFY(PreviousVersions::kept(m_dir.filePath(QStringLiteral("other.qetz"))).isEmpty());
	}

	// A copy that cannot be made says so, and leaves the project as it is
	void copyFails()
	{
		const QString project = m_dir.filePath(QStringLiteral("blocked.qetz"));
		write(project, "data", QDateTime(QDate(2026, 10, 16), QTime(12, 0, 0)));
			// A file where the folder should be
		write(PreviousVersions::folder(project), "", QDateTime::currentDateTime());
		QString error;
		QVERIFY(!PreviousVersions::keep(project, 5, &error));
		QVERIFY(!error.isEmpty());
		QCOMPARE(read(project), QByteArray("data"));
	}
};

QTEST_GUILESS_MAIN(tst_previousversions)

#include "tst_previousversions.moc"
