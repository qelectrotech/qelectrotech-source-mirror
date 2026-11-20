// SPDX-License-Identifier: GPL-2.0-or-later
#include "ElementsCollection/qetlabelsfile.h"
#include "qetapptest.h"
#include "ui/prefixconfigurationdialog.h"

#include <QDir>
#include <QFile>
#include <QLabel>
#include <QLineEdit>
#include <QTemporaryDir>
#include <QTest>
#include <QTreeWidget>

/*
	The prefix dialog also edits the qet_labels.xml of the company
	collection (#1918). The prefix lookup reads the user file before the
	company one, so the company dialog shows, beside each folder, the
	prefix the user file gives it when it gives one.
*/
class tst_companyprefixes : public QObject
{
	Q_OBJECT

	static void writeFile(const QString &path, const QByteArray &content)
	{
		QFile file(path);
		QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Truncate));
		file.write(content);
	}

	static QTreeWidgetItem *itemWithTooltip(QTreeWidget *tree, const QString &tooltip)
	{
		for (QTreeWidgetItemIterator it(tree) ; *it ; ++it) {
			if ((*it)->toolTip(0) == tooltip) {
				return *it;
			}
		}
		return nullptr;
	}

	private slots:
	void prefixForFolderFollowsTheLookup()
	{
		QTemporaryDir dir;
		const QString file = QDir(dir.path()).filePath("qet_labels.xml");
		writeFile(file,
				  "<labels>"
				  "<category name=\"siemens\"><category name=\"relays\"><prefix>K</prefix></category>"
				  "<category name=\"plain\"/></category>"
				  "<category name=\"fuses\"><prefix>F</prefix></category>"
				  "<category name=\"blank\"><prefix/></category>"
				  "</labels>");

			//the whole path from the top-level folder
		QCOMPARE(QetLabelsFile::prefixForFolder(file, {"siemens", "relays"}), QString("K"));
			//the path without its top-level folder, the second layout
		QCOMPARE(QetLabelsFile::prefixForFolder(file, {"abb", "fuses"}), QString("F"));
			//described, with an explicit empty prefix: it still wins
		const QString blank = QetLabelsFile::prefixForFolder(file, {"blank"});
		QVERIFY(!blank.isNull());
		QVERIFY(blank.isEmpty());
			//described, but no prefix anywhere along it: the lookup goes
			//on to the next file, as for a folder it does not describe
		QVERIFY(QetLabelsFile::prefixForFolder(file, {"siemens", "plain"}).isNull());
		QVERIFY(QetLabelsFile::prefixForFolder(file, {"siemens"}).isNull());
		QVERIFY(QetLabelsFile::prefixForFolder(file, {"abb"}).isNull());
			//a top-level folder is never tried without itself
		QVERIFY(QetLabelsFile::prefixForFolder(file, {"fuses2"}).isNull());
		QVERIFY(QetLabelsFile::prefixForFolder(QDir(dir.path()).filePath("none.xml"), {"fuses"}).isNull());
	}

	void companyDialogShowsUserPrefixes()
	{
		QTemporaryDir user_dir;
		QTemporaryDir company_dir;
		const QDir company(company_dir.path());
		QVERIFY(company.mkpath("siemens/relays"));
		QVERIFY(company.mkpath("abb"));

		const QString user_file = QDir(user_dir.path()).filePath("qet_labels.xml");
		const QByteArray user_content =
				"<labels><category name=\"siemens\"><category name=\"relays\">"
				"<prefix>KU</prefix></category></category></labels>";
		writeFile(user_file, user_content);

		QetLabelsFile labels;
		QVERIFY(labels.load(company_dir.path()));
		PrefixConfigurationDialog dialog(labels, QetLabelsFile::scanFolders(company_dir.path()),
										 PrefixConfigurationDialog::CompanyCollection,
										 user_file);
		QCOMPARE(dialog.windowTitle(), QString("Company collection prefixes"));

		auto *tree = dialog.findChild<QTreeWidget *>();
		QVERIFY(tree);
		QCOMPARE(tree->columnCount(), 3);
		QTreeWidgetItem *relays = itemWithTooltip(tree, "relays");
		QTreeWidgetItem *abb = itemWithTooltip(tree, "abb");
		QVERIFY(relays && abb);
		QCOMPARE(relays->text(2), QString("KU"));
		QVERIFY(abb->text(2).isEmpty());

		bool hint_found = false;
		for (QLabel *label : dialog.findChildren<QLabel *>()) {
			hint_found |= label->text().contains("1 folder");
		}
		QVERIFY(hint_found);

			//what is typed goes to the company file, the user file is left alone
		auto *edit = qobject_cast<QLineEdit *>(tree->itemWidget(abb, 1));
		QVERIFY(edit);
		QTest::keyClicks(edit, "Q");
		dialog.accept();

		QetLabelsFile reread;
		QVERIFY(reread.load(company_dir.path()));
		QCOMPARE(reread.prefix({"abb"}), QString("Q"));
		QFile user(user_file);
		QVERIFY(user.open(QIODevice::ReadOnly));
		QCOMPARE(user.readAll(), user_content);
	}

	void noUserColumnWithoutOverlap()
	{
		QTemporaryDir company_dir;
		QVERIFY(QDir(company_dir.path()).mkpath("abb"));

		QetLabelsFile labels;
		QVERIFY(labels.load(company_dir.path()));
			//no user file at all
		PrefixConfigurationDialog company(labels, QetLabelsFile::scanFolders(company_dir.path()),
										  PrefixConfigurationDialog::CompanyCollection,
										  QDir(company_dir.path()).filePath("missing/qet_labels.xml"));
		QCOMPARE(company.findChild<QTreeWidget *>()->columnCount(), 2);

			//the user dialog keeps its title and its two columns
		PrefixConfigurationDialog user(labels, QetLabelsFile::scanFolders(company_dir.path()));
		QCOMPARE(user.windowTitle(), QString("User collection prefixes"));
		QCOMPARE(user.findChild<QTreeWidget *>()->columnCount(), 2);
	}
};

QET_APP_TEST_MAIN(tst_companyprefixes)
#include "tst_companyprefixes.moc"
