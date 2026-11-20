// SPDX-License-Identifier: GPL-2.0-or-later
#include "ElementsCollection/qetlabelsfile.h"
#include "qetapptest.h"
#include "ui/configpage/generalconfigurationpage.h"
#include "ui/prefixconfigurationdialog.h"

#include <QApplication>
#include <QComboBox>
#include <QDialog>
#include <QDir>
#include <QFileDevice>
#include <QFileInfo>
#include <QPushButton>
#include <QScopeGuard>
#include <QTimer>
#include <QFile>
#include <QLabel>
#include <QLineEdit>
#include <QTemporaryDir>
#include <QTest>
#include <QTreeWidget>

#ifdef Q_OS_UNIX
#include <unistd.h>
#endif

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

		///Click @a button and return the title of the modal window it
		///opens (closed right away), or an empty string when none opens
	static QString titleOfModalOpenedBy(QPushButton *button)
	{
		QString title;
		QTimer timer;
		timer.setInterval(20);
		QObject::connect(&timer, &QTimer::timeout, [&title, &timer]() {
			if (QWidget *modal = QApplication::activeModalWidget()) {
				title = modal->windowTitle();
				timer.stop();
				if (auto *dialog = qobject_cast<QDialog *>(modal)) {
					dialog->reject();
				} else {
					modal->close();
				}
			}
		});
		timer.start();
		QTimer::singleShot(5000, &timer, &QTimer::stop);
		button->click();
		timer.stop();
		return title;
	}

		///Point the company path of @a page at @a path, the way picking
		///it with "Browse..." does, without the folder picker
	static bool setCompanyPath(GeneralConfigurationPage &page, const QString &path)
	{
		auto *combo = page.findChild<QComboBox *>("m_company_elmt_path_cb");
		if (combo == nullptr) {
			return false;
		}
		combo->blockSignals(true);
		combo->setCurrentIndex(1);
		combo->setItemData(1, path, Qt::DisplayRole);
		combo->blockSignals(false);
		return true;
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
		PrefixConfigurationDialog dialog(labels, QetLabelsFile::scanFolders(company_dir.path()), nullptr,
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
			hint_found |= label->text().contains("The last column shows");
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
		PrefixConfigurationDialog company(labels, QetLabelsFile::scanFolders(company_dir.path()), nullptr,
										  PrefixConfigurationDialog::CompanyCollection,
										  QDir(company_dir.path()).filePath("missing/qet_labels.xml"));
		QCOMPARE(company.findChild<QTreeWidget *>()->columnCount(), 2);

			//both settings on the same folder: its own prefixes are not
			//shown as winning over themselves
		writeFile(QetLabelsFile::labelsFilePath(company_dir.path()),
				  "<labels><category name=\"abb\"><prefix>Q</prefix></category></labels>");
		QetLabelsFile same;
		QVERIFY(same.load(company_dir.path()));
		PrefixConfigurationDialog same_dialog(same, QetLabelsFile::scanFolders(company_dir.path()), nullptr,
											  PrefixConfigurationDialog::CompanyCollection,
											  QetLabelsFile::labelsFilePath(company_dir.path()));
		QCOMPARE(same_dialog.findChild<QTreeWidget *>()->columnCount(), 2);

			//the user dialog keeps its title and its two columns
		PrefixConfigurationDialog user(labels, QetLabelsFile::scanFolders(company_dir.path()));
		QCOMPARE(user.windowTitle(), QString("User collection prefixes"));
		QCOMPARE(user.findChild<QTreeWidget *>()->columnCount(), 2);
	}

	void pageButtonOpensCompanyDialog()
	{
		QTemporaryDir company_dir;
		QVERIFY(QDir(company_dir.path()).mkpath("abb"));

		GeneralConfigurationPage page;
		QVERIFY(setCompanyPath(page, company_dir.path()));
		auto *button = page.findChild<QPushButton *>("m_company_prefix_pb");
		QVERIFY(button);
		QCOMPARE(titleOfModalOpenedBy(button), QString("Company collection prefixes"));
	}

	void missingCompanyFolderIsNotCreated()
	{
		QTemporaryDir parent;
		const QString missing = QDir(parent.path()).filePath("unmounted-share");

		GeneralConfigurationPage page;
		QVERIFY(setCompanyPath(page, missing));
		auto *button = page.findChild<QPushButton *>("m_company_prefix_pb");
		QVERIFY(button);
		QCOMPARE(titleOfModalOpenedBy(button), QString("Folder not found"));
		QVERIFY(!QDir(missing).exists());
	}

	void readOnlyCompanyFolderIsRefusedFirst()
	{
#ifdef Q_OS_UNIX
		if (::geteuid() == 0) {
			QSKIP("root can write a read-only folder");
		}
#endif
		QTemporaryDir company_dir;
		QVERIFY(QDir(company_dir.path()).mkpath("abb"));
		const QFile::Permissions read_only = QFileDevice::ReadOwner | QFileDevice::ExeOwner;
		QVERIFY(QFile::setPermissions(company_dir.path(), read_only));
			//put back whatever happens, or the folder cannot be removed
		const auto restore = qScopeGuard([&company_dir, read_only]() {
			QFile::setPermissions(company_dir.path(), read_only | QFileDevice::WriteOwner);
		});
		if (QFileInfo(company_dir.path()).isWritable()) {
			QSKIP("this file system ignores the permission");
		}

		GeneralConfigurationPage page;
		QVERIFY(setCompanyPath(page, company_dir.path()));
		auto *button = page.findChild<QPushButton *>("m_company_prefix_pb");
		QVERIFY(button);
		QCOMPARE(titleOfModalOpenedBy(button), QString("Prefix file cannot be written"));
		QVERIFY(!QFile::exists(QetLabelsFile::labelsFilePath(company_dir.path())));
	}
};

QET_APP_TEST_MAIN(tst_companyprefixes)
#include "tst_companyprefixes.moc"
