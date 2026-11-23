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
// SPDX-License-Identifier: GPL-2.0-or-later
#include "qetapptest.h"

#include "dataBase/projectdatabase.h"
#include "projectcheck/openwireends.h"
#include "diagramcontent.h"
#include "qetgraphicsitem/conductor.h"
#include "qetgraphicsitem/element.h"
#include "qetgraphicsitem/terminal.h"
#include "qetproject.h"
#include "undocommand/deleteqgraphicsitemcommand.h"
#include "ui/openwireendsdialog.h"

#include <QLocale>
#include <QScopeGuard>
#include <QSqlError>
#include <QSqlQuery>
#include <QTreeWidget>

/**
	Check open wire ends (#1878 B2): a wire that runs to a folio report
	linked to nothing, into a junction on one side only, or carries a
	potential that reaches one symbol terminal or none. Read from the
	project database alone.
*/
class tst_openwireends : public QObject
{
	Q_OBJECT

	int m_connection = 0;

		/// A database with the tables the check reads, and one folio
	QSqlDatabase newDatabase()
	{
		QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"),
			QStringLiteral("openwireends%1").arg(m_connection++));
		db.setDatabaseName(QStringLiteral(":memory:"));
		if (!db.open())
			qFatal("cannot open the test database");
		QSqlQuery q(db);
		for (const char *sql : {
				"CREATE TABLE diagram (uuid TEXT PRIMARY KEY, pos INTEGER)",
				"CREATE TABLE diagram_info (diagram_uuid TEXT PRIMARY KEY, title TEXT)",
				"CREATE TABLE element (uuid TEXT PRIMARY KEY, diagram_uuid TEXT, pos TEXT, type TEXT)",
				"CREATE TABLE element_info (element_uuid TEXT PRIMARY KEY, label TEXT)",
				"CREATE TABLE element_information (element_uuid TEXT, name TEXT, value TEXT)",
				"CREATE TABLE terminal (uuid TEXT, element_uuid TEXT, name TEXT)",
				"CREATE TABLE link (element_uuid TEXT, linked_uuid TEXT, group_index INTEGER, ord INTEGER DEFAULT 0)",
				"CREATE TABLE conductor (uuid TEXT PRIMARY KEY, diagram_uuid TEXT, "
					"terminal1_uuid TEXT, terminal1_element_uuid TEXT, "
					"terminal2_uuid TEXT, terminal2_element_uuid TEXT)",
				"INSERT INTO diagram VALUES ('{7995f515-72ba-5a71-98f2-8f7a83861c95}', 1)",
				"INSERT INTO diagram_info VALUES ('{7995f515-72ba-5a71-98f2-8f7a83861c95}', 'Power')"})
			if (!q.exec(QString::fromLatin1(sql)))
				qFatal("%s", qPrintable(q.lastError().text()));
		return db;
	}

	static void exec(QSqlDatabase &db, const QString &sql)
	{
			//A fixture that cannot be built stops the run: a slot going on
			//with rows missing could pass for the wrong reason
		QSqlQuery q(db);
		if (!q.exec(sql))
			qFatal("%s", qPrintable(q.lastError().text() + QStringLiteral(": ") + sql));
	}

		/// A symbol @p name of @p type with terminals named 1, 2...
	static void symbol(QSqlDatabase &db, const QString &name, const QString &type, int terminals,
					   const QString &pos = QStringLiteral("A1"))
	{
		exec(db, QStringLiteral("INSERT INTO element VALUES ('%1', '{7995f515-72ba-5a71-98f2-8f7a83861c95}', '%2', '%3')").arg(id(name), pos, type));
		exec(db, QStringLiteral("INSERT INTO element_info VALUES ('%1', '-%2')").arg(id(name), name));
		for (int t = 1 ; t <= terminals ; ++t)
			exec(db, QStringLiteral("INSERT INTO terminal VALUES ('%1', '%2', '%3')")
				 .arg(id(name + QString::number(t)), id(name)).arg(t));
	}

		/// A wire @p name from terminal @p t1 of @p e1 to @p t2 of @p e2
	static void wire(QSqlDatabase &db, const QString &name, const QString &e1, int t1,
					 const QString &e2, int t2)
	{
		exec(db, QStringLiteral("INSERT INTO conductor VALUES ('%1', '{7995f515-72ba-5a71-98f2-8f7a83861c95}', '%2', '%3', '%4', '%5')")
			 .arg(id(name), id(e1 + QString::number(t1)), id(e1), id(e2 + QString::number(t2)), id(e2)));
	}

	static void link(QSqlDatabase &db, const QString &a, const QString &b)
	{
		exec(db, QStringLiteral("INSERT INTO link VALUES ('%1', '%2', NULL, 0)").arg(id(a), id(b)));
		exec(db, QStringLiteral("INSERT INTO link VALUES ('%2', '%1', NULL, 0)").arg(id(a), id(b)));
	}

		/// The uuid the test gives the symbol or wire @p name
	static QString id(const QString &name)
	{
		return QUuid::createUuidV5(QUuid(), name).toString();
	}

	static QUuid uuid(const char *name) { return QUuid(id(QString::fromLatin1(name))); }

	static int count(const QList<OpenWireEnds::Finding> &found, OpenWireEnds::Kind kind)
	{
		return int(std::count_if(found.begin(), found.end(),
			[kind](const OpenWireEnds::Finding &f) { return f.kind == kind; }));
	}

	private slots:
		/// Two symbols joined directly, or through linked folio reports or
		/// a junction with a wire on each side: nothing is found
		void wiresThatLeadSomewhere()
		{
			QSqlDatabase db = newDatabase();
			symbol(db, QStringLiteral("K1"), QStringLiteral("simple"), 2);
			symbol(db, QStringLiteral("K2"), QStringLiteral("simple"), 2);
			symbol(db, QStringLiteral("R1"), QStringLiteral("next_report"), 1);
			symbol(db, QStringLiteral("R2"), QStringLiteral("previous_report"), 1);
			symbol(db, QStringLiteral("J1"), QStringLiteral("junction"), 1);
			wire(db, QStringLiteral("W1"), QStringLiteral("K1"), 1, QStringLiteral("K2"), 1);
			wire(db, QStringLiteral("W2"), QStringLiteral("K1"), 2, QStringLiteral("R1"), 1);
			wire(db, QStringLiteral("W3"), QStringLiteral("R2"), 1, QStringLiteral("J1"), 1);
			wire(db, QStringLiteral("W4"), QStringLiteral("J1"), 1, QStringLiteral("K2"), 2);
			link(db, QStringLiteral("R1"), QStringLiteral("R2"));
			const auto found = OpenWireEnds::check(db);
			QVERIFY2(found.isEmpty(), qPrintable(QString::number(found.size())));
		}

		/// A report linked to nothing is found once, as a report, not again
		/// as the potential it is on
		void unlinkedReport()
		{
			QSqlDatabase db = newDatabase();
			symbol(db, QStringLiteral("K1"), QStringLiteral("simple"), 1, QStringLiteral("B4"));
			symbol(db, QStringLiteral("R1"), QStringLiteral("next_report"), 1, QStringLiteral("C9"));
			wire(db, QStringLiteral("W1"), QStringLiteral("K1"), 1, QStringLiteral("R1"), 1);
			const auto found = OpenWireEnds::check(db);
			QCOMPARE(found.size(), 1);
			QCOMPARE(found.first().kind, OpenWireEnds::Kind::UnlinkedReport);
			QCOMPARE(found.first().element, uuid("R1"));
			QCOMPARE(found.first().position, QStringLiteral("C9"));
			QCOMPARE(found.first().folioNumber, 1);
			QCOMPARE(found.first().folioTitle, QStringLiteral("Power"));
			QCOMPARE(found.first().wires, QList<QUuid>{uuid("W1")});

				//Linked: gone
			symbol(db, QStringLiteral("R2"), QStringLiteral("previous_report"), 1);
			symbol(db, QStringLiteral("K2"), QStringLiteral("simple"), 1);
			wire(db, QStringLiteral("W2"), QStringLiteral("R2"), 1, QStringLiteral("K2"), 1);
			link(db, QStringLiteral("R1"), QStringLiteral("R2"));
			QVERIFY(OpenWireEnds::check(db).isEmpty());
		}

		/// A report without a wire is not a wire end
		void reportWithoutWire()
		{
			QSqlDatabase db = newDatabase();
			symbol(db, QStringLiteral("R1"), QStringLiteral("next_report"), 1);
			QVERIFY(OpenWireEnds::check(db).isEmpty());
		}

		/// A junction with one wire is found; with two it is not
		void loneJunction()
		{
			QSqlDatabase db = newDatabase();
			symbol(db, QStringLiteral("K1"), QStringLiteral("simple"), 1);
			symbol(db, QStringLiteral("K2"), QStringLiteral("simple"), 1);
			symbol(db, QStringLiteral("J1"), QStringLiteral("junction"), 1);
			wire(db, QStringLiteral("W1"), QStringLiteral("K1"), 1, QStringLiteral("K2"), 1);
			wire(db, QStringLiteral("W2"), QStringLiteral("K1"), 1, QStringLiteral("J1"), 1);
			auto found = OpenWireEnds::check(db);
			QCOMPARE(found.size(), 1);
			QCOMPARE(found.first().kind, OpenWireEnds::Kind::LoneJunction);
			QCOMPARE(found.first().wires.size(), 2);   // the whole potential

			symbol(db, QStringLiteral("K3"), QStringLiteral("simple"), 1);
			wire(db, QStringLiteral("W3"), QStringLiteral("J1"), 1, QStringLiteral("K3"), 1);
			QVERIFY(OpenWireEnds::check(db).isEmpty());
		}

		/// Linked reports whose far side has no wire: the potential reaches
		/// one symbol terminal, which is named
		void potentialWithOneEnd()
		{
			QSqlDatabase db = newDatabase();
			symbol(db, QStringLiteral("K1"), QStringLiteral("master"), 2, QStringLiteral("D2"));
			symbol(db, QStringLiteral("R1"), QStringLiteral("next_report"), 1);
			symbol(db, QStringLiteral("R2"), QStringLiteral("previous_report"), 1);
			wire(db, QStringLiteral("W1"), QStringLiteral("K1"), 2, QStringLiteral("R1"), 1);
			link(db, QStringLiteral("R1"), QStringLiteral("R2"));
			const auto found = OpenWireEnds::check(db);
			QCOMPARE(found.size(), 1);
			QCOMPARE(found.first().kind, OpenWireEnds::Kind::OneEnd);
			QCOMPARE(found.first().element, uuid("K1"));
			QCOMPARE(found.first().label, QStringLiteral("-K1"));
			QCOMPARE(found.first().terminal, QStringLiteral("2"));
			QCOMPARE(found.first().position, QStringLiteral("D2"));
		}

		/// A wire between two linked reports reaches no symbol at all
		void potentialWithNoEnd()
		{
			QSqlDatabase db = newDatabase();
			symbol(db, QStringLiteral("J1"), QStringLiteral("junction"), 1);
			symbol(db, QStringLiteral("J2"), QStringLiteral("junction"), 1);
			symbol(db, QStringLiteral("J3"), QStringLiteral("junction"), 1);
			wire(db, QStringLiteral("W1"), QStringLiteral("J1"), 1, QStringLiteral("J2"), 1);
			wire(db, QStringLiteral("W2"), QStringLiteral("J2"), 1, QStringLiteral("J3"), 1);
			wire(db, QStringLiteral("W3"), QStringLiteral("J3"), 1, QStringLiteral("J1"), 1);
			const auto found = OpenWireEnds::check(db);
			QCOMPARE(found.size(), 1);
			QCOMPARE(found.first().kind, OpenWireEnds::Kind::OneEnd);
			QVERIFY(found.first().element.isNull());
			QCOMPARE(found.first().wires.size(), 3);
			QCOMPARE(found.first().folioNumber, 1);
		}

		/// Two terminals of one symbol are two ends: a wire looping back
		/// to its own symbol is not reported
		void twoTerminalsOfOneSymbol()
		{
			QSqlDatabase db = newDatabase();
			symbol(db, QStringLiteral("K1"), QStringLiteral("simple"), 2);
			wire(db, QStringLiteral("W1"), QStringLiteral("K1"), 1, QStringLiteral("K1"), 2);
			QVERIFY(OpenWireEnds::check(db).isEmpty());
		}

		/// Cells sort as row then column: A9 before A10, A10 before B1,
		/// B1 before AA1. In the C locale too, where CI runs: QCollator's
		/// numeric mode did nothing there and A10 came first
		void cellsInRowColumnOrder()
		{
			const QLocale before;
			QLocale::setDefault(QLocale::c());
			const auto restore = qScopeGuard([&before] { QLocale::setDefault(before); });
			QSqlDatabase db = newDatabase();
			symbol(db, QStringLiteral("K1"), QStringLiteral("simple"), 1);
			for (const char *cell : {"AA1", "B1", "A10", "A9"}) {
				const QString name = QStringLiteral("R") + QString::fromLatin1(cell);
				symbol(db, name, QStringLiteral("next_report"), 1, QString::fromLatin1(cell));
				wire(db, QStringLiteral("W") + name, QStringLiteral("K1"), 1, name, 1);
			}
			const auto found = OpenWireEnds::check(db);
			QStringList cells;
			for (const auto &f : found)
				cells << f.position;
			QCOMPARE(cells, (QStringList{"A9", "A10", "B1", "AA1"}));
		}

		/// A real project, through the undo command a user's delete goes
		/// through: the wires on the far side of a linked folio report are
		/// deleted, and the near side is found as a potential with one end;
		/// undo, and it is gone again
		void plantedInARealProject()
		{
			QETProject project(QStringLiteral(QET_EXAMPLES_DIR "/industrial.qet"));
			QVERIFY(project.state() == QETProject::Ok);
			project.loadFolios();
			const auto before = project.dataBase()->openWireEnds();
			QCOMPARE(count(before, OpenWireEnds::Kind::OneEnd), 0);

				//A pair of linked reports on two folios, each with one wire
				//to a symbol terminal that has no other wire: deleting the
				//far wire leaves the near one a potential with one end, and
				//no other potential changes. Any linked report will not do:
				//a near wire that runs on to another report carries the
				//potential further, and nothing new is found. Picked in
				//uuid order, as the scene's item order differs between runs
			const auto lone_wire_to_symbol = [](Element *report) -> bool {
				if (report->conductors().size() != 1)
					return false;
				Conductor *wire = report->conductors().first();
				Terminal *other = wire->terminal1->parentElement() == report ? wire->terminal2
																			 : wire->terminal1;
				const ElementData::Type kind = other->parentElement()->elementData().m_type;
				return !(kind & (ElementData::AllReport | ElementData::Junction | ElementData::Terminal))
						&& other->conductors().size() == 1;
			};
			QList<Element *> candidates;
			for (Diagram *diagram : project.diagrams())
				for (Element *element : diagram->elements())
					if ((element->elementData().m_type & ElementData::AllReport)
							&& element->linkedElements().size() == 1
							&& element->linkedElements().first()->linkedElements().size() == 1
							&& element->linkedElements().first()->diagram() != diagram
							&& lone_wire_to_symbol(element)
							&& lone_wire_to_symbol(element->linkedElements().first()))
						candidates << element;
			QVERIFY(!candidates.isEmpty());
			Element *far = *std::min_element(candidates.begin(), candidates.end(),
				[](Element *a, Element *b) { return a->uuid() < b->uuid(); });
			Diagram *folio = far->diagram();
			folio->clearSelection();
			for (Conductor *conductor : far->conductors())
				conductor->setSelected(true);
			const int potentials = count(before, OpenWireEnds::Kind::OneEnd);
			folio->undoStack().push(new DeleteQGraphicsItemCommand(folio, DiagramContent(folio, true)));
			QVERIFY(far->conductors().isEmpty());

			const auto after = project.dataBase()->openWireEnds();
			QCOMPARE(count(after, OpenWireEnds::Kind::OneEnd), potentials + 1);
			const auto planted = std::find_if(after.begin(), after.end(),
				[](const OpenWireEnds::Finding &f) { return f.kind == OpenWireEnds::Kind::OneEnd; });
			QVERIFY(!planted->element.isNull());
			QVERIFY(!planted->wires.isEmpty());
			QVERIFY(planted->folio != folio->uuid());

				//Picking the row selects its wires and symbol, and only what
				//the row names: the selection made for the delete is gone
			OpenWireEndsDialog dialog(&project);
			auto *rows = dialog.findChild<QTreeWidget *>(QStringLiteral("openWireEndsRows"));
			QVERIFY(rows);
			QTreeWidgetItem *planted_row = nullptr;
			for (int i = 0 ; i < rows->topLevelItemCount() ; ++i)
				if (rows->topLevelItem(i)->text(0) == OpenWireEndsDialog::problemText(*planted))
					planted_row = rows->topLevelItem(i);
			QVERIFY(planted_row);
				//A stray selection on a folio the row does not name
			Element *stray = nullptr;
			for (Diagram *diagram : project.diagrams())
				if (diagram != folio && diagram->uuid() != planted->folio && !diagram->elements().isEmpty()) {
					stray = diagram->elements().first();
					break;
				}
			QVERIFY(stray);
			stray->setSelected(true);
			rows->setCurrentItem(planted_row);
			QSet<QUuid> selected;
			for (Diagram *diagram : project.diagrams())
				for (QGraphicsItem *item : diagram->selectedItems()) {
					if (auto *c = dynamic_cast<Conductor *>(item))
						selected << c->uuid();
					else if (auto *e = dynamic_cast<Element *>(item))
						selected << e->uuid();
				}
			QSet<QUuid> expected(planted->wires.begin(), planted->wires.end());
			expected << planted->element;
			QCOMPARE(selected, expected);

			folio->undoStack().undo();
			QCOMPARE(count(project.dataBase()->openWireEnds(), OpenWireEnds::Kind::OneEnd),
					 potentials);
		}

		/// Every example project: the check runs on its database, the
		/// dialog lists one row per finding, and nothing is built
		void exampleProjects_data()
		{
			QTest::addColumn<QString>("project");
			const QDir examples(QStringLiteral(QET_EXAMPLES_DIR));
			for (const QString &f : examples.entryList({QStringLiteral("*.qet")}, QDir::Files, QDir::Name))
				QTest::newRow(f.toUtf8().constData()) << examples.filePath(f);
		}

		void exampleProjects()
		{
			QFETCH(QString, project);
			QETProject opened(project);
			QVERIFY(opened.state() == QETProject::Ok);
			const int unloaded = opened.unloadedFolioCount();
			const auto found = opened.dataBase()->openWireEnds();
			QCOMPARE(opened.unloadedFolioCount(), unloaded);
			fprintf(stderr, "OPENWIREENDS %s reports=%d junctions=%d potentials=%d\n",
					qPrintable(QFileInfo(project).fileName()),
					count(found, OpenWireEnds::Kind::UnlinkedReport),
					count(found, OpenWireEnds::Kind::LoneJunction),
					count(found, OpenWireEnds::Kind::OneEnd));
			OpenWireEndsDialog dialog(&opened);
			auto *rows = dialog.findChild<QTreeWidget *>(QStringLiteral("openWireEndsRows"));
			QVERIFY(rows);
			QCOMPARE(rows->topLevelItemCount(), found.size());
		}
};

QET_APP_TEST_MAIN(tst_openwireends)
#include "tst_openwireends.moc"
