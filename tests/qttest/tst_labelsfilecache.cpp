// SPDX-License-Identifier: GPL-2.0-or-later
#include "ElementsCollection/elementslocation.h"
#include "ElementsCollection/qetlabelsfile.h"
#include "qetapptest.h"

#include <QFile>
#include <QTemporaryDir>
#include <QThread>

/*
	The prefix lookup is asked once for every symbol of a project that is
	loaded, so the labels file is read once and again only when it changed.
*/
class tst_labelsfilecache : public QObject
{
	Q_OBJECT

	static void write(const QString &path, const QString &prefix)
	{
		QFile file(path);
		QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Truncate));
		file.write(QString("<labels><category name=\"10_electric\">"
						   "<category name=\"relays\"><prefix>%1</prefix>"
						   "</category></category></labels>").arg(prefix).toUtf8());
	}

	private slots:
	void sameAnswerAsReadingTheFile()
	{
		QTemporaryDir dir;
		const QString file = dir.filePath("qet_labels.xml");
		write(file, "K");
		const QStringList path = {"coil.elmt", "relays", "10_electric"};
		QCOMPARE(QetLabelsFile::prefixForPath(file, path, 2), QString("K"));
		QCOMPARE(QetLabelsFile::prefixForPath(file, path, 2), QString("K"));
			//a folder the file does not describe: null, not empty
		QVERIFY(QetLabelsFile::prefixForPath(file, {"x.elmt", "other", "10_electric"}, 2).isNull());
	}

	void anEditedFileIsReadAgain()
	{
		QTemporaryDir dir;
		const QString file = dir.filePath("qet_labels.xml");
		const QStringList path = {"coil.elmt", "relays", "10_electric"};
		write(file, "K");
		QCOMPARE(QetLabelsFile::prefixForPath(file, path, 2), QString("K"));
			//same size, so only the time of last change tells
		QThread::msleep(20);
		write(file, "Q");
		QCOMPARE(QetLabelsFile::prefixForPath(file, path, 2), QString("Q"));
			//another size
		write(file, "KQ");
		QCOMPARE(QetLabelsFile::prefixForPath(file, path, 2), QString("KQ"));
	}

	void aFileThatGoesAndComesBack()
	{
		QTemporaryDir dir;
		const QString file = dir.filePath("qet_labels.xml");
		const QStringList path = {"coil.elmt", "relays", "10_electric"};
		write(file, "K");
		QCOMPARE(QetLabelsFile::prefixForPath(file, path, 2), QString("K"));
		QVERIFY(QFile::remove(file));
		QVERIFY(QetLabelsFile::prefixForPath(file, path, 2).isNull());
		write(file, "M");
		QCOMPARE(QetLabelsFile::prefixForPath(file, path, 2), QString("M"));
			//not XML: null, and a good file after it is read again
		QFile bad(file);
		QVERIFY(bad.open(QIODevice::WriteOnly | QIODevice::Truncate));
		bad.write("<labels><oops");
		bad.close();
		QVERIFY(QetLabelsFile::prefixForPath(file, path, 2).isNull());
		write(file, "N");
		QCOMPARE(QetLabelsFile::prefixForPath(file, path, 2), QString("N"));
	}

	void parentFolders()
	{
		const ElementsLocation element("common://10_electric/relays/coil.elmt");
		QCOMPARE(element.parent().collectionPath(), QString("common://10_electric/relays"));
		QCOMPARE(element.parent().parent().collectionPath(), QString("common://10_electric"));
			//asked twice in a row: the pattern is shared, not consumed
		QCOMPARE(element.parent().collectionPath(), QString("common://10_electric/relays"));
	}
};

QET_APP_TEST_MAIN(tst_labelsfilecache)
#include "tst_labelsfilecache.moc"
