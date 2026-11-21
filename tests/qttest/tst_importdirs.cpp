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
	"Remember the last folder a file was picked from" (a setting, off by
	default): off, every dialog starts where it always did -- the pictures
	folder for a picture, the documents folder for a PDF, and for the
	others the folder the caller passed -- and nothing of the user's disk
	is kept. On, each dialog starts in the folder its last file was picked
	from; pictures, PDFs and other files keep a folder of their own, and a
	folder that no longer exists sends the dialog back to the standard one.
*/
#include "qetapptest.h"
#include "utils/qetsettings.h"

#include <QDir>
#include <QTemporaryDir>

class tst_importdirs : public QObject
{
	Q_OBJECT

	private slots:
			// Every slot starts from the same state, whatever the slot
			// before it left behind: switching off forgets the folders,
			// so no slot depends on running first.
		void init()
		{
			QetSettings::setRememberLastDir(false);
		}

		void offByDefault()
		{
			QVERIFY(!QetSettings::rememberLastDir());
			QVERIFY(QetSettings::lastDir(QetSettings::FileDialog::Picture).isEmpty());
			QVERIFY(QetSettings::lastDir(QetSettings::FileDialog::Pdf).isEmpty());
			QVERIFY(QetSettings::lastDir(QetSettings::FileDialog::File).isEmpty());
		}

		void offStartsWhereItAlwaysDid()
		{
			QTemporaryDir folder;
			QVERIFY(folder.isValid());
			QETApp::rememberPictureDir(folder.filePath("photo.png"));
			QETApp::rememberPdfDir(folder.filePath("sheet.pdf"));
			QETApp::rememberFileDir(folder.filePath("list.csv"));

			QCOMPARE(QETApp::lastPictureDir(), QETApp::pictureDir());
			QCOMPARE(QETApp::lastPdfDir(), QETApp::documentDir());
				// The folder a dialog opened in before is passed
				// through, so that turning the setting off leaves it
				// exactly where it was
			QCOMPARE(QETApp::lastFileDir(QStringLiteral("/somewhere")), QStringLiteral("/somewhere"));
				// And nothing of the user's disk was kept
			QVERIFY(QetSettings::lastDir(QetSettings::FileDialog::Picture).isEmpty());
			QVERIFY(QetSettings::lastDir(QetSettings::FileDialog::Pdf).isEmpty());
			QVERIFY(QetSettings::lastDir(QetSettings::FileDialog::File).isEmpty());
		}

		void onStartsInTheFolderOfTheLastPicture()
		{
			QTemporaryDir folder;
			QVERIFY(folder.isValid());
			QetSettings::setRememberLastDir(true);
			QETApp::rememberPictureDir(folder.filePath("photo.png"));

			QCOMPARE(QETApp::lastPictureDir(), QDir::cleanPath(folder.path()));
				// The other two keep their own standard folder
			QCOMPARE(QETApp::lastPdfDir(), QETApp::documentDir());
			QCOMPARE(QETApp::lastFileDir(QETApp::documentDir()), QETApp::documentDir());
		}

		void onKeepsTheThreeFoldersApart()
		{
			QTemporaryDir pictures;
			QTemporaryDir pdfs;
			QTemporaryDir others;
			QVERIFY(pictures.isValid());
			QVERIFY(pdfs.isValid());
			QVERIFY(others.isValid());
			QetSettings::setRememberLastDir(true);
			QETApp::rememberPictureDir(pictures.filePath("photo.png"));
			QETApp::rememberPdfDir(pdfs.filePath("sheet.pdf"));
			QETApp::rememberFileDir(others.filePath("list.csv"));

			QCOMPARE(QETApp::lastPictureDir(), QDir::cleanPath(pictures.path()));
			QCOMPARE(QETApp::lastPdfDir(), QDir::cleanPath(pdfs.path()));
			QCOMPARE(QETApp::lastFileDir(QETApp::documentDir()), QDir::cleanPath(others.path()));
		}

		void remembersTheFolderAndNotTheFile()
		{
			QTemporaryDir folder;
			QVERIFY(folder.isValid());
			QetSettings::setRememberLastDir(true);
			QETApp::rememberPictureDir(folder.filePath("photo.png"));

			const QString stored = QetSettings::lastDir(QetSettings::FileDialog::Picture);
			QCOMPARE(stored, QDir::cleanPath(folder.path()));
			QVERIFY(!stored.contains(QLatin1String("photo.png")));
		}

			// A dialog that asks for a folder hands over a folder, which
			// is stored as it is rather than as its own parent
		void keepsAFolderPickedAsAFolder()
		{
			QTemporaryDir folder;
			QVERIFY(folder.isValid());
			QetSettings::setRememberLastDir(true);
			QETApp::rememberFileDir(folder.path());

			QCOMPARE(QetSettings::lastDir(QetSettings::FileDialog::File), QDir::cleanPath(folder.path()));
		}

		void onFallsBackWhenTheFolderIsGone()
		{
			QTemporaryDir folder;
			QVERIFY(folder.isValid());
			QetSettings::setRememberLastDir(true);
			QETApp::rememberPictureDir(folder.filePath("photo.png"));
			QETApp::rememberPdfDir(folder.filePath("sheet.pdf"));
			QETApp::rememberFileDir(folder.filePath("list.csv"));
			QVERIFY(QDir(folder.path()).removeRecursively());
			QVERIFY(!QDir(folder.path()).exists());

			QCOMPARE(QETApp::lastPictureDir(), QETApp::pictureDir());
			QCOMPARE(QETApp::lastPdfDir(), QETApp::documentDir());
			QCOMPARE(QETApp::lastFileDir(QETApp::documentDir()), QETApp::documentDir());
		}

		void switchingOffForgetsTheFolders()
		{
			QTemporaryDir folder;
			QVERIFY(folder.isValid());
			QetSettings::setRememberLastDir(true);
			QETApp::rememberPictureDir(folder.filePath("photo.png"));
			QETApp::rememberFileDir(folder.filePath("list.csv"));
			QVERIFY(!QetSettings::lastDir(QetSettings::FileDialog::Picture).isEmpty());

			QetSettings::setRememberLastDir(false);
			QVERIFY(QetSettings::lastDir(QetSettings::FileDialog::Picture).isEmpty());
			QVERIFY(QetSettings::lastDir(QetSettings::FileDialog::File).isEmpty());
		}
};

QET_APP_TEST_MAIN(tst_importdirs)
#include "tst_importdirs.moc"
