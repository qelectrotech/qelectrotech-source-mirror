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

#include "ElementsCollection/elementslocation.h"
#include "ElementsCollection/xmlelementcollection.h"
#include "dataBase/projectdatabase.h"
#include "diagram.h"
#include "diagramcontext.h"
#include "factory/elementfactory.h"
#include "projectcheck.h"
#include "projectcheck/terminalbranches.h"
#include "qetgraphicsitem/conductor.h"
#include "qetgraphicsitem/element.h"
#include "qetgraphicsitem/terminal.h"
#include "qetinformation.h"
#include "qetproject.h"

#include <QDir>
#include <QFileInfo>
#include <QSqlError>
#include <QSqlQuery>

/**
	Branches between strip terminals (#1878 B4): a junction on a wire
	between two terminals of a terminal strip, with a third wire going off
	it. Read from the project database alone.
*/
class tst_terminalbranches : public QObject
{
	Q_OBJECT

	int m_connection = 0;
	static constexpr const char *folio = "{7995f515-72ba-5a71-98f2-8f7a83861c95}";

		/// A database with the tables the check reads, and one folio
	QSqlDatabase newDatabase()
	{
		QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"),
			QStringLiteral("terminalbranches%1").arg(m_connection++));
		db.setDatabaseName(QStringLiteral(":memory:"));
		if (!db.open())
			qFatal("cannot open the test database");
		for (const QString &sql : {
				QStringLiteral("CREATE TABLE diagram (uuid TEXT PRIMARY KEY, pos INTEGER)"),
				QStringLiteral("CREATE TABLE diagram_info (diagram_uuid TEXT PRIMARY KEY, title TEXT)"),
				QStringLiteral("CREATE TABLE element (uuid TEXT PRIMARY KEY, diagram_uuid TEXT, pos TEXT, type TEXT, definition TEXT)"),
				QStringLiteral("CREATE TABLE symbol_def (path TEXT PRIMARY KEY, content TEXT)"),
				QStringLiteral("CREATE TABLE element_info (element_uuid TEXT PRIMARY KEY, label TEXT)"),
				QStringLiteral("CREATE TABLE terminal (uuid TEXT, element_uuid TEXT, name TEXT)"),
				QStringLiteral("CREATE TABLE link (element_uuid TEXT, linked_uuid TEXT, group_index INTEGER, ord INTEGER DEFAULT 0)"),
				QStringLiteral("CREATE TABLE conductor (uuid TEXT PRIMARY KEY, diagram_uuid TEXT, "
					"terminal1_uuid TEXT, terminal1_element_uuid TEXT, "
					"terminal2_uuid TEXT, terminal2_element_uuid TEXT)"),
				QStringLiteral("INSERT INTO diagram VALUES ('%1', 1)").arg(QLatin1String(folio)),
				QStringLiteral("INSERT INTO diagram_info VALUES ('%1', 'Power')").arg(QLatin1String(folio))})
			exec(db, sql);
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
					   const QString &pos = QStringLiteral("A1"), const QString &definition = QString())
	{
		exec(db, QStringLiteral("INSERT INTO element VALUES ('%1', '%2', '%3', '%4', '%5')")
			 .arg(id(name), QLatin1String(folio), pos, type, definition));
		exec(db, QStringLiteral("INSERT INTO element_info VALUES ('%1', '-%2')").arg(id(name), name));
		for (int t = 1 ; t <= terminals ; ++t)
			exec(db, QStringLiteral("INSERT INTO terminal VALUES ('%1', '%2', '%3')")
				 .arg(id(name + QString::number(t)), id(name)).arg(t));
	}

		/// A wire @p name from terminal @p t1 of @p e1 to @p t2 of @p e2
	static void wire(QSqlDatabase &db, const QString &name, const QString &e1, int t1,
					 const QString &e2, int t2)
	{
		exec(db, QStringLiteral("INSERT INTO conductor VALUES ('%1', '%2', '%3', '%4', '%5', '%6')")
			 .arg(id(name), QLatin1String(folio), id(e1 + QString::number(t1)), id(e1),
				  id(e2 + QString::number(t2)), id(e2)));
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

		/// A junction definition in the project's collection, at
		/// import/@p name.elmt, whose chain is the connections @p chain of
		/// the symbol J1 (1, 2, 3: J1's terminals), the first the source
	static void junctionDefinition(QSqlDatabase &db, const QString &name, const QList<int> &chain)
	{
		QString roles;
		for (int i = 0 ; i < chain.size() ; ++i)
			roles += QStringLiteral("<kindInformation name=\"%1\">%2</kindInformation>")
					 .arg(i ? QStringLiteral("junction_target_%1").arg(i) : QStringLiteral("junction_source"),
						  id(QStringLiteral("J1") + QString::number(chain.at(i))));
		exec(db, QStringLiteral("INSERT INTO symbol_def VALUES ('import/%1.elmt', "
								"'<element name=\"%1.elmt\"><definition type=\"element\" link_type=\"junction\">"
								"<kindInformations>%2</kindInformations></definition></element>')")
			 .arg(name, roles));
	}

		/// Two strip terminals, X1 at A1 and X2 at A5, and two symbols
	void stripAndSymbols(QSqlDatabase &db)
	{
		symbol(db, QStringLiteral("X1"), QStringLiteral("terminal"), 2, QStringLiteral("A1"));
		symbol(db, QStringLiteral("X2"), QStringLiteral("terminal"), 2, QStringLiteral("A5"));
		symbol(db, QStringLiteral("K1"), QStringLiteral("simple"), 2, QStringLiteral("C3"));
		symbol(db, QStringLiteral("K2"), QStringLiteral("simple"), 2, QStringLiteral("C4"));
	}

		/// A one-connection symbol of @p link_type, its connection @p uuid
	static QString oneConnection(const QString &link_type, const QUuid &uuid)
	{
		return QStringLiteral(
			"<definition type=\"element\" link_type=\"%1\" width=\"20\" height=\"20\" "
			"hotspot_x=\"0\" hotspot_y=\"10\"><names><name lang=\"en\">%1</name></names>"
			"<description><rect x=\"0\" y=\"0\" width=\"20\" height=\"20\" "
			"style=\"line-style:normal;line-weight:normal;filling:none;color:black\"/>"
			"<terminal uuid=\"%2\" x=\"0\" y=\"10\" orientation=\"w\"/>"
			"</description></definition>").arg(link_type, uuid.toString());
	}

		/// A junction with three connections, @p chain or not
	static QString junction(bool chain, const QList<QUuid> &connections)
	{
		const QString roles = chain
			? QStringLiteral("<kindInformations>"
							 "<kindInformation name=\"junction_source\">%1</kindInformation>"
							 "<kindInformation name=\"junction_target_1\">%2</kindInformation>"
							 "<kindInformation name=\"junction_target_2\">%3</kindInformation>"
							 "</kindInformations>")
			  .arg(connections.at(0).toString(), connections.at(1).toString(),
				   connections.at(2).toString())
			: QString();
		return QStringLiteral(
			"<definition type=\"element\" link_type=\"junction\" width=\"60\" height=\"40\" "
			"hotspot_x=\"30\" hotspot_y=\"20\"><names><name lang=\"en\">junction</name></names>%1"
			"<description>"
			"<terminal name=\"1\" orientation=\"w\" uuid=\"%2\" x=\"0\" y=\"20\"/>"
			"<terminal name=\"2\" orientation=\"n\" uuid=\"%3\" x=\"30\" y=\"0\"/>"
			"<terminal name=\"3\" orientation=\"e\" uuid=\"%4\" x=\"60\" y=\"20\"/>"
			"</description></definition>")
			.arg(roles, connections.at(0).toString(), connections.at(1).toString(),
				 connections.at(2).toString());
	}

	static Element *place(QETProject &project, Diagram *diagram, const QString &name,
						  const QString &xml, const QString &label, const QPointF &pos)
	{
		auto *collection = project.embeddedElementCollection();
		QDomDocument document;
		document.setContent(xml);
		if (!collection->exist(QStringLiteral("import/%1.elmt").arg(name)))
			collection->addElementDefinition(QStringLiteral("import"), name, document.documentElement());
		int state = 0;
		Element *element = ElementFactory::Instance()->createElement(
			ElementsLocation(QStringLiteral("embed://import/%1.elmt").arg(name), &project),
			nullptr, &state);
		if (state || !element)
			return nullptr;
		diagram->addItem(element);
		element->setPos(pos);
		DiagramContext info = element->elementInformations();
		info.addValue(QETInformation::ELMT_LABEL, label);
		info.remove(QStringLiteral("formula"));
		element->setElementInformations(info);
		return element;
	}

	static Terminal *at(Element *element, int index)
	{
		return element && index < element->terminals().size() ? element->terminals().at(index) : nullptr;
	}

	static Terminal *connection(Element *element, const QUuid &uuid)
	{
		for (Terminal *terminal : element->terminals())
			if (terminal->uuid() == uuid)
				return terminal;
		return nullptr;
	}

	private slots:
		void initTestCase()
		{
			QETProject::setBackupEnabled(false);
		}

		/// The case the rule is for: X1 -- J1 -- X2, and J1 -- K1
		void branchBetweenTwoStripTerminals()
		{
			QSqlDatabase db = newDatabase();
			stripAndSymbols(db);
			symbol(db, QStringLiteral("J1"), QStringLiteral("junction"), 1, QStringLiteral("A3"));
			wire(db, QStringLiteral("W1"), QStringLiteral("X1"), 2, QStringLiteral("J1"), 1);
			wire(db, QStringLiteral("W2"), QStringLiteral("J1"), 1, QStringLiteral("X2"), 1);
			wire(db, QStringLiteral("W3"), QStringLiteral("J1"), 1, QStringLiteral("K1"), 1);

			const auto found = TerminalBranches::check(db);
			QCOMPARE(found.size(), 1);
			const auto &f = found.first();
			QCOMPARE(f.junction, uuid("J1"));
			QCOMPARE(f.label, QStringLiteral("-J1"));
			QCOMPARE(f.position, QStringLiteral("A3"));
			QCOMPARE(f.folioNumber, 1);
			QCOMPARE(f.folioTitle, QStringLiteral("Power"));
			QCOMPARE(f.terminals.size(), 2);
			QCOMPARE(f.terminals.at(0).element, uuid("X1"));
			QCOMPARE(f.terminals.at(0).terminal, QStringLiteral("2"));
			QCOMPARE(f.terminals.at(1).element, uuid("X2"));
			QCOMPARE(f.terminals.at(1).terminal, QStringLiteral("1"));
			QCOMPARE(f.others, 1);
			QCOMPARE(QSet<QUuid>(f.wires.begin(), f.wires.end()),
					 (QSet<QUuid>{uuid("W1"), uuid("W2"), uuid("W3")}));
		}

			/// What is not a branch between two strip terminals
		void notABranch_data()
		{
			QTest::addColumn<QString>("drawing");
				//A junction with a wire on each side is a point on the wire
			QTest::newRow("pass-through") << QStringLiteral("X1.2-J1.1 J1.1-X2.1");
				//A branch after one strip terminal, towards the symbols
			QTest::newRow("one strip terminal") << QStringLiteral("X1.2-J1.1 J1.1-K1.1 J1.1-K2.1");
				//Wires that meet on a terminal are clamped there
			QTest::newRow("on a terminal") << QStringLiteral("X1.2-X2.1 X1.2-K1.1");
				//The run stops at K1: X2 hangs on K1's terminal, not on the wire
			QTest::newRow("through a symbol") << QStringLiteral("X1.2-J1.1 J1.1-K1.1 J1.1-K2.1 K1.1-X2.1");
				//Both terminals of one strip terminal are still one terminal
			QTest::newRow("one strip terminal twice") << QStringLiteral("X1.1-J1.1 J1.1-X1.2 J1.1-K1.1");
				//No junction: an unlinked report passes nothing on
			QTest::newRow("unlinked report") << QStringLiteral("X1.2-R1.1 R2.1-J1.1 J1.1-X2.1 J1.1-K1.1");
		}

		void notABranch()
		{
			QFETCH(QString, drawing);
			QSqlDatabase db = newDatabase();
			stripAndSymbols(db);
			symbol(db, QStringLiteral("J1"), QStringLiteral("junction"), 1);
			symbol(db, QStringLiteral("R1"), QStringLiteral("next_report"), 1);
			symbol(db, QStringLiteral("R2"), QStringLiteral("previous_report"), 1);
			int n = 0;
			for (const QString &w : drawing.split(QLatin1Char(' '))) {
				const QStringList ends = w.split(QLatin1Char('-'));
				const QStringList a = ends.at(0).split(QLatin1Char('.'));
				const QStringList b = ends.at(1).split(QLatin1Char('.'));
				wire(db, QStringLiteral("W%1").arg(++n), a.at(0), a.at(1).toInt(), b.at(0), b.at(1).toInt());
			}
			QCOMPARE(TerminalBranches::check(db).size(), 0);
		}

			/// Linked folio reports pass the wire on to the junction
		void throughLinkedReports()
		{
			QSqlDatabase db = newDatabase();
			stripAndSymbols(db);
			symbol(db, QStringLiteral("J1"), QStringLiteral("junction"), 1, QStringLiteral("B2"));
			symbol(db, QStringLiteral("R1"), QStringLiteral("next_report"), 1);
			symbol(db, QStringLiteral("R2"), QStringLiteral("previous_report"), 1);
			wire(db, QStringLiteral("W1"), QStringLiteral("X1"), 2, QStringLiteral("R1"), 1);
			wire(db, QStringLiteral("W2"), QStringLiteral("R2"), 1, QStringLiteral("J1"), 1);
			wire(db, QStringLiteral("W3"), QStringLiteral("J1"), 1, QStringLiteral("X2"), 1);
			wire(db, QStringLiteral("W4"), QStringLiteral("J1"), 1, QStringLiteral("K1"), 1);
			link(db, QStringLiteral("R1"), QStringLiteral("R2"));
			const auto found = TerminalBranches::check(db);
			QCOMPARE(found.size(), 1);
			QCOMPARE(found.first().junction, uuid("J1"));
			QCOMPARE(found.first().terminals.size(), 2);
			QCOMPARE(found.first().wires.size(), 4);
		}

			/// Two junctions on the run: the one with the third wire is named
		void namedByTheBranchingJunction()
		{
			QSqlDatabase db = newDatabase();
			stripAndSymbols(db);
			symbol(db, QStringLiteral("J1"), QStringLiteral("junction"), 1, QStringLiteral("A2"));
			symbol(db, QStringLiteral("J2"), QStringLiteral("junction"), 1, QStringLiteral("A4"));
			wire(db, QStringLiteral("W1"), QStringLiteral("X1"), 2, QStringLiteral("J1"), 1);
			wire(db, QStringLiteral("W2"), QStringLiteral("J1"), 1, QStringLiteral("J2"), 1);
			wire(db, QStringLiteral("W3"), QStringLiteral("J2"), 1, QStringLiteral("X2"), 1);
			wire(db, QStringLiteral("W4"), QStringLiteral("J2"), 1, QStringLiteral("K1"), 1);
			const auto found = TerminalBranches::check(db);
			QCOMPARE(found.size(), 1);
			QCOMPARE(found.first().junction, uuid("J2"));
			QCOMPARE(found.first().position, QStringLiteral("A4"));
		}

			/// Two runs, two findings, in folio and cell order, the second
			/// with three strip terminals and no symbol
		void severalRuns()
		{
			QSqlDatabase db = newDatabase();
			stripAndSymbols(db);
			symbol(db, QStringLiteral("X3"), QStringLiteral("terminal"), 2, QStringLiteral("A6"));
			symbol(db, QStringLiteral("J1"), QStringLiteral("junction"), 1, QStringLiteral("B10"));
			symbol(db, QStringLiteral("J2"), QStringLiteral("junction"), 1, QStringLiteral("B9"));
			wire(db, QStringLiteral("W1"), QStringLiteral("X1"), 1, QStringLiteral("J1"), 1);
			wire(db, QStringLiteral("W2"), QStringLiteral("J1"), 1, QStringLiteral("X2"), 1);
			wire(db, QStringLiteral("W3"), QStringLiteral("J1"), 1, QStringLiteral("K1"), 1);
			wire(db, QStringLiteral("W4"), QStringLiteral("X1"), 2, QStringLiteral("J2"), 1);
			wire(db, QStringLiteral("W5"), QStringLiteral("J2"), 1, QStringLiteral("X2"), 2);
			wire(db, QStringLiteral("W6"), QStringLiteral("J2"), 1, QStringLiteral("X3"), 1);
			const auto found = TerminalBranches::check(db);
			QCOMPARE(found.size(), 2);
			QCOMPARE(found.at(0).junction, uuid("J2"));   //B9 before B10
			QCOMPARE(found.at(0).terminals.size(), 3);
			QCOMPARE(found.at(0).others, 0);
			QCOMPARE(found.at(1).junction, uuid("J1"));
			QCOMPARE(found.at(1).others, 1);
		}

		/// A junction whose definition gives its chain is wired from
		/// terminal to terminal, X1 -> K1 -> X2: no branch. It is one
		/// again as soon as the chain cannot say where a wire goes
		void chainedJunction_data()
		{
			QTest::addColumn<QString>("definition");
			QTest::addColumn<QList<int>>("chain");
			QTest::addColumn<bool>("secondWireOn3");
			QTest::addColumn<int>("found");
			QTest::newRow("chained") << QStringLiteral("chain") << QList<int>{1, 2, 3} << false << 0;
			QTest::newRow("two wires on one connection") << QStringLiteral("chain") << QList<int>{1, 2, 3} << true << 1;
			QTest::newRow("a wire off the chain") << QStringLiteral("chain") << QList<int>{1, 2} << false << 1;
			QTest::newRow("no source") << QStringLiteral("chain") << QList<int>{} << false << 1;
			QTest::newRow("no definition") << QStringLiteral("elsewhere") << QList<int>{1, 2, 3} << false << 1;
		}

		void chainedJunction()
		{
			QFETCH(QString, definition);
			QFETCH(QList<int>, chain);
			QFETCH(bool, secondWireOn3);
			QFETCH(int, found);
			QSqlDatabase db = newDatabase();
			stripAndSymbols(db);
			junctionDefinition(db, QStringLiteral("chain"), chain);
			symbol(db, QStringLiteral("J1"), QStringLiteral("junction"), 3, QStringLiteral("A3"),
				   QStringLiteral("embed://import/%1.elmt").arg(definition));
			wire(db, QStringLiteral("W1"), QStringLiteral("X1"), 2, QStringLiteral("J1"), 1);
			wire(db, QStringLiteral("W2"), QStringLiteral("J1"), 2, QStringLiteral("K1"), 1);
			wire(db, QStringLiteral("W3"), QStringLiteral("J1"), 3, QStringLiteral("X2"), 1);
			if (secondWireOn3)
				wire(db, QStringLiteral("W4"), QStringLiteral("J1"), 3, QStringLiteral("K2"), 1);
			QCOMPARE(int(TerminalBranches::check(db).size()), found);
		}

			/// A chained junction is where its wires land: a split behind it,
			/// between two symbols, is not between the strip terminals.
			/// X1 -> J1 (chained) -> X2 -> J2, and J2 splits to K1 and K2
		void splitBehindAChainedJunction()
		{
			QSqlDatabase db = newDatabase();
			stripAndSymbols(db);
			junctionDefinition(db, QStringLiteral("chain"), {1, 2, 3});
			symbol(db, QStringLiteral("J1"), QStringLiteral("junction"), 3, QStringLiteral("A3"),
				   QStringLiteral("embed://import/chain.elmt"));
			symbol(db, QStringLiteral("J2"), QStringLiteral("junction"), 1, QStringLiteral("B3"));
			wire(db, QStringLiteral("W1"), QStringLiteral("X1"), 2, QStringLiteral("J1"), 1);
			wire(db, QStringLiteral("W2"), QStringLiteral("J1"), 2, QStringLiteral("X2"), 1);
			wire(db, QStringLiteral("W3"), QStringLiteral("J1"), 3, QStringLiteral("J2"), 1);
			wire(db, QStringLiteral("W4"), QStringLiteral("J2"), 1, QStringLiteral("K1"), 1);
			wire(db, QStringLiteral("W5"), QStringLiteral("J2"), 1, QStringLiteral("K2"), 1);
			QVERIFY(TerminalBranches::check(db).isEmpty());
		}

			/// The third wire need not reach a terminal: one to a folio
			/// report not linked yet still splits the wire from X1 to X2
		void thirdWireToAnUnlinkedReport()
		{
			QSqlDatabase db = newDatabase();
			stripAndSymbols(db);
			symbol(db, QStringLiteral("J1"), QStringLiteral("junction"), 1, QStringLiteral("A3"));
			symbol(db, QStringLiteral("R1"), QStringLiteral("next_report"), 1);
			wire(db, QStringLiteral("W1"), QStringLiteral("X1"), 2, QStringLiteral("J1"), 1);
			wire(db, QStringLiteral("W2"), QStringLiteral("J1"), 1, QStringLiteral("X2"), 1);
			wire(db, QStringLiteral("W3"), QStringLiteral("J1"), 1, QStringLiteral("R1"), 1);
			const auto found = TerminalBranches::check(db);
			QCOMPARE(found.size(), 1);
			QCOMPARE(found.first().others, 0);
		}

			/// A strip terminal with no label of its own is named by its
			/// strip and its place on it, as the strip draws it
		void stripTerminalWithoutALabel()
		{
			QSqlDatabase db = newDatabase();
			stripAndSymbols(db);
			exec(db, QStringLiteral("UPDATE element_info SET label = '' WHERE element_uuid = '%1'").arg(id(QStringLiteral("X2"))));
			exec(db, QStringLiteral("CREATE TABLE qet_terminals (strip TEXT, position INTEGER, element_uuid TEXT)"));
			exec(db, QStringLiteral("INSERT INTO qet_terminals VALUES ('X5', 6, '%1')").arg(id(QStringLiteral("X2"))));
			symbol(db, QStringLiteral("J1"), QStringLiteral("junction"), 1, QStringLiteral("A3"));
			wire(db, QStringLiteral("W1"), QStringLiteral("X1"), 2, QStringLiteral("J1"), 1);
			wire(db, QStringLiteral("W2"), QStringLiteral("J1"), 1, QStringLiteral("X2"), 1);
			wire(db, QStringLiteral("W3"), QStringLiteral("J1"), 1, QStringLiteral("K1"), 1);
			const auto found = TerminalBranches::check(db);
			QCOMPARE(found.size(), 1);
			QCOMPARE(found.first().terminals.at(0).label, QStringLiteral("-X1"));
			QCOMPARE(found.first().terminals.at(1).label, QStringLiteral("X5:7"));
		}

			/// A bend on the way to a chained junction splits nothing:
			/// X1 -- J2 -- J1 (chained) -> K1 -> X2
		void bendBeforeAChainedJunction()
		{
			QSqlDatabase db = newDatabase();
			stripAndSymbols(db);
			junctionDefinition(db, QStringLiteral("chain"), {1, 2, 3});
			symbol(db, QStringLiteral("J1"), QStringLiteral("junction"), 3, QStringLiteral("A3"),
				   QStringLiteral("embed://import/chain.elmt"));
			symbol(db, QStringLiteral("J2"), QStringLiteral("junction"), 1, QStringLiteral("A2"));
			wire(db, QStringLiteral("W1"), QStringLiteral("X1"), 2, QStringLiteral("J2"), 1);
			wire(db, QStringLiteral("W2"), QStringLiteral("J2"), 1, QStringLiteral("J1"), 1);
			wire(db, QStringLiteral("W3"), QStringLiteral("J1"), 2, QStringLiteral("K1"), 1);
			wire(db, QStringLiteral("W4"), QStringLiteral("J1"), 3, QStringLiteral("X2"), 1);
			QVERIFY(TerminalBranches::check(db).isEmpty());
		}

		/// Drawn in a project, as a user draws it: the wires end on the
		/// connections the junction's definition names, so a chain is
		/// followed; without one the Check panel lists the branch, with
		/// the junction, both strip terminals and the device to show
		void drawnInAProject_data()
		{
			QTest::addColumn<bool>("chain");
			QTest::newRow("no chain") << false;
			QTest::newRow("chain") << true;
		}

		void drawnInAProject()
		{
			QFETCH(bool, chain);
			QETProject project;
			Diagram *diagram = project.addNewDiagram();
			QVERIFY(diagram);
			const QList<QUuid> connections{QUuid::createUuid(), QUuid::createUuid(), QUuid::createUuid()};
			Element *x1 = place(project, diagram, QStringLiteral("clamp"),
								oneConnection(QStringLiteral("terminal"), QUuid::createUuid()),
								QStringLiteral("X1"), QPointF(100, 200));
			Element *x2 = place(project, diagram, QStringLiteral("clamp"), QString(),
								QStringLiteral("X2"), QPointF(700, 200));
			Element *k1 = place(project, diagram, QStringLiteral("device"),
								oneConnection(QStringLiteral("simple"), QUuid::createUuid()),
								QStringLiteral("K1"), QPointF(400, 500));
			Element *j1 = place(project, diagram, QStringLiteral("junction"),
								junction(chain, connections), QStringLiteral("J1"), QPointF(400, 200));
			QVERIFY(x1 && x2 && k1 && j1);
			for (const auto &[a, b] : {std::make_pair(at(x1, 0), connection(j1, connections.at(0))),
									   std::make_pair(connection(j1, connections.at(1)), at(k1, 0)),
									   std::make_pair(connection(j1, connections.at(2)), at(x2, 0))}) {
				QVERIFY(a && b);
				diagram->addItem(new Conductor(a, b));
			}

			const auto found = project.dataBase()->terminalBranches();
			const auto result = ProjectCheck::run(&project, ProjectCheck::Kind::TerminalBranches);
			if (chain) {
				QVERIFY(found.isEmpty());
				QVERIFY(result.findings.isEmpty());
				return;
			}
			QCOMPARE(found.size(), 1);
			QCOMPARE(found.first().junction, j1->uuid());
			QCOMPARE(found.first().wires.size(), 3);
			QCOMPARE(result.findings.size(), 1);
			const auto &finding = result.findings.first();
			QCOMPARE(finding.text, QStringLiteral("J1: Branch between X1, X2"));
			QCOMPARE(finding.places.size(), 4);   //the junction, X1, X2, "1 symbol terminal"
			QCOMPARE(finding.places.at(0).element, j1->uuid());
			QCOMPARE(finding.places.at(0).wires.size(), 3);
			QCOMPARE(finding.places.at(1).element, x1->uuid());
			QCOMPARE(finding.places.at(2).element, x2->uuid());
			QVERIFY(!finding.places.at(3).canShow());
		}

		/// Every example project: the check runs on its database, builds
		/// nothing, and the Check panel lists one finding per branch
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
			const auto found = opened.dataBase()->terminalBranches();
			QCOMPARE(opened.unloadedFolioCount(), unloaded);
			fprintf(stderr, "TERMINALBRANCHES %s %d\n",
					qPrintable(QFileInfo(project).fileName()), int(found.size()));
			const auto result = ProjectCheck::run(&opened, ProjectCheck::Kind::TerminalBranches);
			QCOMPARE(result.findings.size(), found.size());
			QVERIFY(ProjectCheck::kinds().contains(ProjectCheck::Kind::TerminalBranches));
			QVERIFY(!ProjectCheck::title(ProjectCheck::Kind::TerminalBranches).isEmpty());
		}
};

QET_APP_TEST_MAIN(tst_terminalbranches)
#include "tst_terminalbranches.moc"
