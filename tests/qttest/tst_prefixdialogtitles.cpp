// SPDX-License-Identifier: GPL-2.0-or-later
#include "ElementsCollection/qetlabelsfile.h"
#include "qetapptest.h"
#include "ui/prefixconfigurationdialog.h"

#include <QDir>
#include <QFile>
#include <QLineEdit>
#include <QTemporaryDir>
#include <QTest>
#include <QTreeWidget>

/*
	The prefix dialog lists the folders of the user collection by the
	title the collection panel shows (from each folder's qet_directory),
	not by the folder name on disk, which stays as the tooltip (#1802).
	qet_labels.xml is still keyed by the folder name.
*/
class tst_prefixdialogtitles : public QObject
{
	Q_OBJECT

	static void makeFolder(const QDir &root, const QString &name, const QString &names_xml)
	{
		QVERIFY(root.mkpath(name));
		if (names_xml.isEmpty()) {
			return;
		}
		QFile file(root.filePath(name + "/qet_directory"));
		QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Truncate));
		file.write(QString("<qet-directory><names>%1</names></qet-directory>")
				   .arg(names_xml).toUtf8());
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
	void foldersShowTheirTitle()
	{
		QTemporaryDir dir;
		const QDir root(dir.path());
		makeFolder(root, "abb", "<name lang=\"en\">ABB</name>");
			//only a Dutch name: the panel shows it whatever the language
		makeFolder(root, "schema", "<name lang=\"nl\">Schema symbolen</name>");
		makeFolder(root, "schema/drukknoppen", "<name lang=\"en\">Push buttons</name>");
			//no qet_directory: the folder name, as in the panel
		makeFolder(root, "plain", QString());

		QetLabelsFile labels;
		QVERIFY(labels.load(dir.path()));
		PrefixConfigurationDialog dialog(labels, QetLabelsFile::scanFolders(dir.path()));
		auto *tree = dialog.findChild<QTreeWidget *>();
		QVERIFY(tree);

		QTreeWidgetItem *abb = itemWithTooltip(tree, "abb");
		QTreeWidgetItem *schema = itemWithTooltip(tree, "schema");
		QTreeWidgetItem *buttons = itemWithTooltip(tree, "drukknoppen");
		QTreeWidgetItem *plain = itemWithTooltip(tree, "plain");
		QVERIFY(abb && schema && buttons && plain);
		QCOMPARE(abb->text(0), QString("ABB"));
		QCOMPARE(schema->text(0), QString("Schema symbolen"));
		QCOMPARE(buttons->text(0), QString("Push buttons"));
		QCOMPARE(buttons->parent(), schema);
		QCOMPARE(plain->text(0), QString("plain"));

			//a prefix typed against the title is stored under the folder name
		auto *edit = qobject_cast<QLineEdit *>(tree->itemWidget(buttons, 1));
		QVERIFY(edit);
		QTest::keyClicks(edit, "S");
		dialog.accept();

		QetLabelsFile reread;
		QVERIFY(reread.load(dir.path()));
		QCOMPARE(reread.prefix({"schema", "drukknoppen"}), QString("S"));
	}
};

QET_APP_TEST_MAIN(tst_prefixdialogtitles)
#include "tst_prefixdialogtitles.moc"
