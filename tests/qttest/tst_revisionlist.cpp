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
#include "dataBase/projectdatabase.h"
#include "properties/revisionlist.h"
#include "qetapptest.h"
#include "qetproject.h"
#include "qetresult.h"
#include "ui/configpage/revisionsconfigpage.h"
#include "factory/ui/revisionquerywidget.h"
#include "qetgraphicsitem/ViewItem/projectdbmodel.h"

#include <QDomDocument>
#include <QApplication>
#include <QVBoxLayout>
#include <QDialog>
#include <QPushButton>
#include <QSqlQuery>
#include <QTableWidget>
#include <QTemporaryDir>
#include <QtTest>

/**
	Item C8 of #1878, agreed in #1295 (rule 2): the project's revision
	history is one list in the project properties -- revision, date,
	description, author, approver. Saved as <revisions> only when it has
	one; a .qet, .qetz and .qetdb keep it; qet_revisions gives it.
*/
class tst_revisionlist : public QObject
{
	Q_OBJECT
	QTemporaryDir m_dir;

	static RevisionList twoRevisions()
	{
		RevisionList list;
		list.setRevisions({{QStringLiteral("A"), QStringLiteral("2026-11-01"),
							QStringLiteral("First issue"), QStringLiteral("SR"), QString()},
						   {QStringLiteral("B"), QStringLiteral("2026-11-25"),
							QStringLiteral("Pump P2 added; \"quoted\" & <marked>"),
							QStringLiteral("SR"), QStringLiteral("JD")}});
		return list;
	}

private slots:
	void initTestCase()
	{
		QVERIFY(m_dir.isValid());
		QETProject::setBackupEnabled(false);
	}

	void xmlRoundTrip()
	{
		QDomDocument document;
		const RevisionList list = twoRevisions();
		const QDomElement e = list.toXml(document);
		QCOMPARE(e.tagName(), QStringLiteral("revisions"));
			//An empty field is not written
		QVERIFY(!e.firstChildElement().hasAttribute(QStringLiteral("approver")));
		QCOMPARE(RevisionList::fromXml(e), list);
		QCOMPARE(list.latest().revision, QStringLiteral("B"));
		QVERIFY(RevisionList::fromXml(QDomElement()).isEmpty());
	}

	void savedOnlyWhenSet()
	{
		QETProject project;
		project.addNewDiagram();
		QVERIFY(!project.toXml().toString().contains(QLatin1String("<revisions")));
		project.setRevisions(twoRevisions());
		QVERIFY(project.toXml().toString().contains(QLatin1String("<revisions")));
		project.setRevisions(RevisionList());
		QVERIFY(!project.toXml().toString().contains(QLatin1String("<revisions")));
	}

	void projectKeepsIt_data()
	{
		QTest::addColumn<QString>("file");
		QTest::newRow("qet") << QStringLiteral("revisions.qet");
		QTest::newRow("qetz") << QStringLiteral("revisions.qetz");
		QTest::newRow("qetdb") << QStringLiteral("revisions.qetdb");
	}

	void projectKeepsIt()
	{
		QFETCH(QString, file);
		QETProject project;
		project.addNewDiagram();
		project.setRevisions(twoRevisions());
		project.setFilePath(m_dir.filePath(file));
		const QETResult written = project.write();
		QVERIFY2(written.isOk(), qPrintable(written.errorMessage()));
		QETProject again(m_dir.filePath(file));
		QCOMPARE(again.revisions(), twoRevisions());
	}

	void theViewGivesIt()
	{
		QETProject project;
		project.addNewDiagram();
		project.setRevisions(twoRevisions());
		project.dataBase()->updateDB();
		QString error;
		QSqlQuery query = project.dataBase()->newQuery(QStringLiteral(
			"SELECT position, revision, date, description, author, approver "
			"FROM qet_revisions"), &error);
		QVERIFY2(error.isEmpty(), qPrintable(error));
		QStringList rows;
		while (query.next()) {
			QStringList row;
			for (int i = 0 ; i < 6 ; ++i) row << query.value(i).toString();
			rows << row.join(QLatin1Char('|'));
		}
		QCOMPARE(rows, QStringList({
			QStringLiteral("1|A|2026-11-01|First issue|SR|"),
			QStringLiteral("2|B|2026-11-25|Pump P2 added; \"quoted\" & <marked>|SR|JD")}));
	}

	void thePageEditsIt()
	{
		QETProject project;
		project.addNewDiagram();
		project.setRevisions(twoRevisions());
		RevisionsConfigPage page(&project);
		auto *table = page.findChild<QTableWidget *>(QStringLiteral("revisionTable"));
		QVERIFY(table);
		QCOMPARE(table->rowCount(), 2);
			//The buttons come before the table, so a tall page keeps them
			//in view
		QPushButton *add = page.findChild<QPushButton *>(QStringLiteral("addRevision"));
		page.resize(600, 3000);
		page.show();
		QVERIFY(add->mapTo(&page, QPoint(0, 0)).y() < table->mapTo(&page, QPoint(0, 0)).y());
		add->click();
		QCOMPARE(table->rowCount(), 3);
			//Dated today, the rest to type
		QCOMPARE(table->item(2, 1)->text(), QDate::currentDate().toString(Qt::ISODate));
		table->item(2, 0)->setText(QStringLiteral(" C "));
		table->item(2, 2)->setText(QStringLiteral("Cable list"));
		table->setCurrentCell(0, 0);
		page.findChild<QPushButton *>(QStringLiteral("removeRevision"))->click();
		page.applyProjectConf();
		const QList<Revision> revisions = project.revisions().revisions();
		QCOMPARE(revisions.size(), 2);
		QCOMPARE(revisions.at(0).revision, QStringLiteral("B"));
		QCOMPARE(revisions.at(1).revision, QStringLiteral("C"));
		QCOMPARE(revisions.at(1).description, QStringLiteral("Cable list"));
		QVERIFY(project.projectOptionsWereModified() || project.projectWasModified());
	}


		//Return typed in a cell ends its edit and keeps the text, but does
		//not close the dialog the page is in (its default button)
	void returnInACellDoesNotCloseTheDialog()
	{
		QETProject project;
		project.addNewDiagram();
		QDialog dialog;
		auto *layout = new QVBoxLayout(&dialog);
		auto *page = new RevisionsConfigPage(&project, &dialog);
		auto *ok = new QPushButton(QStringLiteral("OK"), &dialog);
		ok->setDefault(true);
		connect(ok, &QPushButton::clicked, &dialog, &QDialog::accept);
		layout->addWidget(page);
		layout->addWidget(ok);
		dialog.show();
		QVERIFY(QTest::qWaitForWindowExposed(&dialog));
		page->findChild<QPushButton *>(QStringLiteral("addRevision"))->click();
		auto *table = page->findChild<QTableWidget *>(QStringLiteral("revisionTable"));
			//Add a revision opens the new row's first cell for typing
		QWidget *editor = nullptr;
		QTRY_VERIFY((editor = QApplication::focusWidget()) && editor != table
					&& table->isAncestorOf(editor));
		QTest::keyClicks(editor, QStringLiteral("A"));
		QTest::keyClick(editor, Qt::Key_Return);
			//The editor commits on Return through the event loop
		QTRY_COMPARE(table->item(0, 0)->text(), QStringLiteral("A"));
		QVERIFY(dialog.isVisible());
		QCOMPARE(dialog.result(), 0);
			//And Return on the table itself, no cell being edited
		QTRY_VERIFY(QApplication::focusWidget() == table || table->isAncestorOf(QApplication::focusWidget()));
		QTest::keyClick(QApplication::focusWidget(), Qt::Key_Return);
		QApplication::processEvents();
		QVERIFY(dialog.isVisible());
		QCOMPARE(dialog.result(), 0);
	}

		//The table a folio prints it with: a database table over
		//qet_revisions, its columns named as on the Revisions page, and
		//following the list when it changes
	void aTablePrintsIt()
	{
		QETProject project;
		project.addNewDiagram();
		project.setRevisions(twoRevisions());
		project.dataBase()->updateDB();
		ProjectDBModel model(&project);
		model.setIdentifier(RevisionQueryWidget::modelIdentifier());
		model.setQuery(RevisionQueryWidget::queryStr());
		QStringList headers;
		for (int c = 0 ; c < model.columnCount() ; ++c)
			headers << model.headerData(c, Qt::Horizontal).toString();
		QCOMPARE(headers, QStringList({QStringLiteral("Revision"), QStringLiteral("Date"),
									   QStringLiteral("Description"), QStringLiteral("Author"),
									   QStringLiteral("Approved by")}));
		QCOMPARE(model.rowCount(), 2);
		QCOMPARE(model.data(model.index(1, 0)).toString(), QStringLiteral("B"));
		QCOMPARE(model.data(model.index(1, 4)).toString(), QStringLiteral("JD"));

		RevisionList three = twoRevisions();
		QList<Revision> revisions = three.revisions();
		revisions << Revision{QStringLiteral("C"), QStringLiteral("2026-12-01"),
							  QStringLiteral("Cable list"), QStringLiteral("SR"), QString()};
		three.setRevisions(revisions);
			//As Project properties does it: no database update asked for
		project.setRevisions(three);
		QCOMPARE(model.rowCount(), 3);
		QCOMPARE(model.data(model.index(2, 2)).toString(), QStringLiteral("Cable list"));
	}
};

QET_APP_TEST_MAIN(tst_revisionlist)
#include "tst_revisionlist.moc"
