/*
	Copyright 2006-2026 The QElectroTech Team
	This file is part of QElectroTech.

	QElectroTech is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	QElectroTech is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with QElectroTech.  If not, see <http://www.gnu.org/licenses/>.
*/
/*
	"Remember the last folder a picture or a PDF was picked from" (a
	setting, off by default): off, the dialog of "Add a picture" and
	"Add a PDF" starts in the pictures or the documents folder, as it
	always did; on, in the folder the last file was picked from, as long
	as that folder still exists. The two folders are kept apart, so the
	PDF dialog never starts in the pictures folder and the picture
	dialog never in the documents folder.
*/
#include "qetapptest.h"
#include "utils/qetsettings.h"

#include <QDir>

class tst_importdirs : public QObject
{
	Q_OBJECT

	private slots:
		void offByDefault()
		{
			QVERIFY(!QetSettings::rememberLastImportDir());
			QVERIFY(QetSettings::lastImportPictureDir().isEmpty());
			QVERIFY(QetSettings::lastImportPdfDir().isEmpty());
		}

		void offStartsInTheStandardFolder()
		{
			QTemporaryDir folder;
			QVERIFY(folder.isValid());
			QETApp::rememberPictureDir(folder.filePath("photo.png"));
			QETApp::rememberPdfDir(folder.filePath("sheet.pdf"));
			QetSettings::setRememberLastImportDir(false);

			QCOMPARE(QETApp::lastPictureDir(), QETApp::pictureDir());
			QCOMPARE(QETApp::lastPdfDir(), QETApp::documentDir());
				// The folder is recorded all the same, so that turning
				// the setting on later already has one to start from
			QCOMPARE(QetSettings::lastImportPictureDir(), QDir::cleanPath(folder.path()));
		}

		void remembersTheFolderAndNotTheFile()
		{
			QTemporaryDir folder;
			QVERIFY(folder.isValid());
			QETApp::rememberPictureDir(folder.filePath("photo.png"));

			const QString stored = QetSettings::lastImportPictureDir();
			QCOMPARE(stored, QDir::cleanPath(folder.path()));
			QVERIFY(!stored.contains(QLatin1String("photo.png")));
		}

		void onStartsInTheFolderOfTheLastPicture()
		{
			QTemporaryDir folder;
			QVERIFY(folder.isValid());
			QetSettings::setRememberLastImportDir(true);
			QETApp::rememberPictureDir(folder.filePath("photo.png"));

			QCOMPARE(QETApp::lastPictureDir(), QDir::cleanPath(folder.path()));
		}

		void onKeepsTheTwoFoldersApart()
		{
			QTemporaryDir pictures;
			QTemporaryDir pdfs;
			QVERIFY(pictures.isValid());
			QVERIFY(pdfs.isValid());
			QetSettings::setRememberLastImportDir(true);
			QETApp::rememberPictureDir(pictures.filePath("photo.png"));
			QETApp::rememberPdfDir(pdfs.filePath("sheet.pdf"));

			QCOMPARE(QETApp::lastPictureDir(), QDir::cleanPath(pictures.path()));
			QCOMPARE(QETApp::lastPdfDir(), QDir::cleanPath(pdfs.path()));
		}

		void onFallsBackWhenTheFolderIsGone()
		{
			QTemporaryDir folder;
			QVERIFY(folder.isValid());
			QetSettings::setRememberLastImportDir(true);
			QETApp::rememberPictureDir(folder.filePath("photo.png"));
			QETApp::rememberPdfDir(folder.filePath("sheet.pdf"));
			QVERIFY(QDir(folder.path()).removeRecursively());
			QVERIFY(!QDir(folder.path()).exists());

			QCOMPARE(QETApp::lastPictureDir(), QETApp::pictureDir());
			QCOMPARE(QETApp::lastPdfDir(), QETApp::documentDir());
		}
};

QET_APP_TEST_MAIN(tst_importdirs)
#include "tst_importdirs.moc"
