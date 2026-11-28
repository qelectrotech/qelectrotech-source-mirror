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
// File > Import sPlan schematic: the target next to the schematic, the
// command line of the program, its progress lines and its result line, the
// program found in the source tree, and the menu command. The translation
// itself (tools/splan_import) has its own tests.

#include "import/splan/splanimportdialog.h"
#include "qetapptest.h"

#include <QFile>
#include <QLineEdit>
#include <QPushButton>

class TestSplanImportDialog : public QObject
{
	Q_OBJECT

	QTemporaryDir m_dir;
	QString m_schematic;	///< an empty .spl8 with a space in its name

	private slots:
		void initTestCase()
		{
			QVERIFY(m_dir.isValid());
			m_schematic = QDir(m_dir.path()).filePath(
						QStringLiteral("control cabinet.spl8"));
			QFile f(m_schematic);
			QVERIFY(f.open(QIODevice::WriteOnly));
			f.close();
		}

		void targetAndArguments()
		{
			using D = SplanImportDialog;
			QCOMPARE(D::defaultTarget(QStringLiteral("C:/x/a b.spl8")),
				 QStringLiteral("C:/x/QET/a_b.qet"));
			QVERIFY(D::defaultTarget(QString()).isEmpty());

			const QString target = D::defaultTarget(m_schematic);
			const QStringList args = D::arguments(
						QStringLiteral("C:/t/splan_to_qet.py"),
						m_schematic, target, false);
			QCOMPARE(args, (QStringList{
						QStringLiteral("-u"),
						QDir::toNativeSeparators(
							QStringLiteral("C:/t/splan_to_qet.py")),
						QDir::toNativeSeparators(m_schematic),
						QDir::toNativeSeparators(target)}));
				// no printed original next to it: no comparison
			QCOMPARE(D::arguments(QStringLiteral("s.py"), m_schematic,
					      target, true).last(),
				 QStringLiteral("--pdf"));
			QFile original(QDir(m_dir.path()).filePath(
					       QStringLiteral("control cabinet.pdf")));
			QVERIFY(original.open(QIODevice::WriteOnly));
			original.close();
			QVERIFY(D::arguments(QStringLiteral("s.py"), m_schematic,
					     target, true)
				.contains(QStringLiteral("--compare")));
		}

		void progressAndResult()
		{
			using D = SplanImportDialog;
			int percent = 0;
			QCOMPARE(D::progressText(QStringLiteral("PROGRESS 80 write"),
						 &percent),
				 QStringLiteral("Writing the QElectroTech project"));
			QCOMPARE(percent, 80);
			QVERIFY(D::progressText(QStringLiteral("PROGRESS 250 done"),
						&percent) == QStringLiteral("Done"));
			QCOMPARE(percent, 100);
			QVERIFY(D::progressText(QStringLiteral("layout: 97"),
						&percent).isEmpty());
			QCOMPARE(D::resultFromOutput(QStringLiteral(
						"ok: True\ndone: C:/a/b c.qet\r\n")),
				 QStringLiteral("C:/a/b c.qet"));
			QVERIFY(D::resultFromOutput(
					QStringLiteral("error: no file")).isEmpty());
		}

		void findsTheProgram()
		{
			qputenv("QET_SPLAN_IMPORT", QByteArray(QET_SPLAN_SCRIPT));
			const QString found = SplanImportDialog::findScript(
						QCoreApplication::applicationDirPath());
			QCOMPARE(QFileInfo(found).fileName(),
				 QStringLiteral("splan_to_qet.py"));
			QVERIFY(QFileInfo(found).isFile());
		}

		void dialogTakesTheSchematic()
		{
			qputenv("QET_SPLAN_IMPORT", QByteArray(QET_SPLAN_SCRIPT));
			SplanImportDialog dialog(QetAppTest::editor());
			QCOMPARE(dialog.windowTitle(),
				 QStringLiteral("Import an sPlan schematic"));
			const QList<QLineEdit *> edits =
					dialog.findChildren<QLineEdit *>();
			QVERIFY(edits.size() >= 2);
			dialog.setSource(m_schematic);
				// the target follows the schematic, as long as
				// nobody typed one
			QCOMPARE(QDir::fromNativeSeparators(edits.at(1)->text()),
				 SplanImportDialog::defaultTarget(
					 QDir::toNativeSeparators(m_schematic)));
			QPushButton *start = nullptr;
			for (QPushButton *b : dialog.findChildren<QPushButton *>())
				if (b->text() == QStringLiteral("Import")) start = b;
			QVERIFY(start);
			QVERIFY(start->isEnabled());
			dialog.setSource(QDir(m_dir.path()).filePath(
						 QStringLiteral("missing.spl8")));
			QVERIFY(!start->isEnabled());
		}

		void menuHasTheCommand()
		{
			QETDiagramEditor *e = QetAppTest::editor();
			QVERIFY(e);
			QAction *a = e->findChild<QAction *>(
						QStringLiteral("import_splan"));
			QVERIFY(a);
			QVERIFY(a->text().contains(QStringLiteral("sPlan")));
		}
};

int main(int argc, char **argv)
{
	return QetAppTest::run<TestSplanImportDialog>(argc, argv);
}
#include "tst_splanimportdialog.moc"
