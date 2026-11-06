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
#include "qet.h"

#include <QUrl>
#include <QtTest>

/*
	QET::autosaveManagedFilePath() gives the project file of a
	crash-recovery file back, from the URL either KAutoSaveFile gives:
	a file URL, as QElectroTech's own gives built without KDE
	Frameworks, or a URL without a scheme, as KDE's gives.
*/
class tst_AutosaveManagedFilePath : public QObject
{
	Q_OBJECT

	private slots:
		void fileUrl_data()
		{
			QTest::addColumn<QString>("path");
#ifdef Q_OS_WIN
			QTest::newRow("drive letter")
				<< QStringLiteral(u"C:/Users/\u00C5sa/project.qet");
			QTest::newRow("network share")
				<< QStringLiteral("//server/share/folder/project.qet");
			QTest::newRow("WSL folder")
				<< QStringLiteral("//wsl.localhost/Ubuntu/home/project.qet");
			QTest::newRow("characters of URLs")
				<< QStringLiteral("C:/Projekt #1/100% klar & ny.qet");
#else
			QTest::newRow("home folder")
				<< QStringLiteral(u"/home/\u00E5sa/project.qet");
			QTest::newRow("characters of URLs")
				<< QStringLiteral("/home/Projekt #1/100% klar & ny.qet");
			QTest::newRow("a folder named like a drive")
				<< QStringLiteral("/C:/Users/project.qet");
#endif
		}

		void fileUrl()
		{
			QFETCH(QString, path);
			QCOMPARE(QET::autosaveManagedFilePath(QUrl::fromLocalFile(path)),
					 path);
		}

		void urlWithoutScheme_data()
		{
			QTest::addColumn<QString>("url_path");
			QTest::addColumn<QString>("path");
#ifdef Q_OS_WIN
			QTest::newRow("drive letter")
				<< QStringLiteral(u"/C:/Users/\u00C5sa/project.qet")
				<< QStringLiteral(u"C:/Users/\u00C5sa/project.qet");
			QTest::newRow("lower case drive letter")
				<< QStringLiteral("/c:/project.qet")
				<< QStringLiteral("c:/project.qet");
#else
			QTest::newRow("home folder")
				<< QStringLiteral(u"/home/\u00E5sa/project.qet")
				<< QStringLiteral(u"/home/\u00E5sa/project.qet");
			QTest::newRow("a folder named like a drive")
				<< QStringLiteral("/C:/Users/project.qet")
				<< QStringLiteral("/C:/Users/project.qet");
#endif
		}

		void urlWithoutScheme()
		{
			QFETCH(QString, url_path);
			QFETCH(QString, path);
			QUrl url;
			url.setPath(url_path);
			QCOMPARE(QET::autosaveManagedFilePath(url), path);
		}
};

QTEST_GUILESS_MAIN(tst_AutosaveManagedFilePath)

#include "tst_autosavemanagedfilepath.moc"
