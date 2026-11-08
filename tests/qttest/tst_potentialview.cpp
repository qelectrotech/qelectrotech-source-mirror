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
#include "conductorproperties.h"
#include "dataBase/projectdatabase.h"
#include "diagram.h"
#include "qetapptest.h"
#include "qetgraphicsitem/conductor.h"
#include "qetgraphicsitem/element.h"
#include "qetgraphicsitem/terminal.h"
#include "qetproject.h"

#include <QSqlQuery>
#include <QtTest>

/**
	potential_view: one row per potential, made with wire_potential_view
	when a query names it. Each row says what its wires say: how many,
	the terminals they end on, the folios, the wire numbers, and whether
	the potential carries more than one number.
*/
class tst_potentialview : public QObject
{
	Q_OBJECT

	struct Row {
		QString key;
		int wires = 0;
		int terminals = 0;
		int folios = 0;
		QString numbers;
		bool several = false;
		QString names;
	};

	static QHash<int, Row> rows(QETProject &project, QString *error)
	{
		project.dataBase()->updateDB();
		QSqlQuery query = project.dataBase()->newQuery(QStringLiteral(
			"SELECT potential_id, potential_key, wires, terminals, folios, "
			"wire_numbers, several_numbers, names FROM potential_view"), error);
		QHash<int, Row> result;
		while (query.next()) {
			Row row;
			row.key = query.value(1).toString();
			row.wires = query.value(2).toInt();
			row.terminals = query.value(3).toInt();
			row.folios = query.value(4).toInt();
			row.numbers = query.value(5).toString();
			row.several = query.value(6).toBool();
			row.names = query.value(7).toString();
			result.insert(query.value(0).toInt(), row);
		}
		return result;
	}

		//The wires of the project by uuid
	static QHash<QUuid, Conductor *> wiresOf(QETProject &project)
	{
		QHash<QUuid, Conductor *> wires;
		for (Diagram *diagram : project.diagrams())
			for (Conductor *c : diagram->conductors())
				wires.insert(c->uuid(), c);
		return wires;
	}

		//What potential_view should say of each potential, from the wires
		//QElectroTech groups into it
	static QHash<int, Row> expected(QETProject &project)
	{
		const projectDataBase::PotentialGroups groups = project.dataBase()->potentialGroups();
		const QHash<QUuid, Conductor *> wires = wiresOf(project);
		QHash<int, QList<Conductor *>> members;
		for (auto it = groups.group_of.constBegin() ; it != groups.group_of.constEnd() ; ++it)
			if (Conductor *c = wires.value(it.key()))
				members[it.value()] << c;
		QHash<int, Row> result;
		for (auto it = members.constBegin() ; it != members.constEnd() ; ++it) {
			Row row;
			QSet<QPair<QUuid, QUuid>> ends;
			QSet<Diagram *> folios;
			QStringList numbers, keys, names;
			for (Conductor *c : it.value()) {
				keys << c->uuid().toString();
				for (Terminal *t : {c->terminal1, c->terminal2}) {
					ends.insert({t->parentElement()->uuid(), t->uuid()});
					Element *e = t->parentElement();
					const QString name = e->elementInformations()
							.value(QStringLiteral("label")).toString();
					if ((e->linkType() & Element::AllReport) && !name.isEmpty()
							&& !names.contains(name))
						names << name;
				}
				folios.insert(c->diagram());
				const QString text = c->properties().text;
				if (!text.isEmpty() && !numbers.contains(text))
					numbers << text;
			}
			keys.sort();
			numbers.sort();
			names.sort();
			row.names = names.join(QStringLiteral(", "));
			row.key = keys.first();
			row.wires = it.value().size();
			row.terminals = ends.size();
			row.folios = folios.size();
			row.numbers = numbers.join(QStringLiteral(", "));
			row.several = numbers.size() > 1;
			result.insert(it.key(), row);
		}
		return result;
	}

	static QString describe(const Row &r)
	{
		return QStringLiteral("%1 wires=%2 terminals=%3 folios=%4 numbers=[%5] several=%6 names=[%7]")
				.arg(r.key).arg(r.wires).arg(r.terminals).arg(r.folios)
				.arg(r.numbers).arg(r.several).arg(r.names);
	}

	static void compare(const QHash<int, Row> &actual, const QHash<int, Row> &want)
	{
		QCOMPARE(actual.size(), want.size());
		for (auto it = want.constBegin() ; it != want.constEnd() ; ++it) {
			QVERIFY2(actual.contains(it.key()), qPrintable(QStringLiteral("no row for potential %1").arg(it.key())));
			QCOMPARE(describe(actual.value(it.key())), describe(it.value()));
		}
	}

private slots:
	void oneRowPerPotential()
	{
		QETProject project(QFINDTESTDATA("fixtures/every_feature.qet"));
		QVERIFY(!project.diagrams().isEmpty());
		QString error;
		const QHash<int, Row> actual = rows(project, &error);
		QVERIFY2(error.isEmpty(), qPrintable(error));
		QVERIFY(!actual.isEmpty());
		compare(actual, expected(project));

			//Every wire is in one of them
		int wires = 0;
		for (const Row &row : actual) wires += row.wires;
		QCOMPARE(wires, wiresOf(project).size());
	}

	void secondNumberIsMarked()
	{
		QETProject project(QFINDTESTDATA("fixtures/every_feature.qet"));
			//A potential of at least two wires, numbered once
		const projectDataBase::PotentialGroups groups = project.dataBase()->potentialGroups();
		QHash<int, QList<QUuid>> members;
		for (auto it = groups.group_of.constBegin() ; it != groups.group_of.constEnd() ; ++it)
			members[it.value()] << it.key();
		int potential = -1;
		for (auto it = members.constBegin() ; it != members.constEnd() ; ++it)
			if (it.value().size() >= 2) { potential = it.key(); break; }
		QVERIFY2(potential >= 0, "the fixture has no potential of two wires");

		const QHash<QUuid, Conductor *> wires = wiresOf(project);
		Conductor *first = wires.value(members.value(potential).at(0));
		Conductor *second = wires.value(members.value(potential).at(1));
		QVERIFY(first && second);
			//A number of its own: the fixture's wires are numbered by a
			//formula, which would give the text back
		const auto number = [](Conductor *c, const QString &text) {
			ConductorProperties p = c->properties();
			p.m_formula.clear();
			p.text = text;
			c->setProperties(p);
		};
		number(first, QStringLiteral("101"));
		number(second, QStringLiteral("101"));
		QString error;
		QHash<int, Row> actual = rows(project, &error);
		QVERIFY2(error.isEmpty(), qPrintable(error));
		QVERIFY(actual.value(potential).numbers.split(QStringLiteral(", ")).contains(QStringLiteral("101")));

			//RSWire's busbar: one stub with a number of its own
		number(second, QStringLiteral("102"));
		actual = rows(project, &error);
		QVERIFY(actual.value(potential).several);
		QVERIFY(actual.value(potential).numbers.contains(QStringLiteral("101, 102")));
		compare(actual, expected(project));
	}

	void namesAreTheReportLabels()
	{
			//RSWire's signal names (R_1, LOADMIXER) arrive as the labels of
			//the folio reports a potential crosses the folios with
		QETProject project(QFINDTESTDATA("fixtures/wiring_list_arrows.qet"));
		QList<Element *> reports;
		for (Diagram *diagram : project.diagrams())
			for (Element *e : diagram->elements())
				if (e->linkType() & Element::AllReport)
					reports << e;
		QVERIFY2(reports.size() >= 2, "the fixture has no folio reports");
		for (Element *e : reports) {
			DiagramContext info = e->elementInformations();
			info.addValue(QStringLiteral("label"), QStringLiteral("LOADMIXER"));
			e->setElementInformations(info);
		}
		QString error;
		const QHash<int, Row> actual = rows(project, &error);
		QVERIFY2(error.isEmpty(), qPrintable(error));
		int named = 0;
		for (const Row &row : actual)
			if (row.names == QLatin1String("LOADMIXER")) ++named;
		QVERIFY(named >= 1);
		compare(actual, expected(project));
	}

	void keyStaysWithItsWires()
	{
			//potential_id is the refresh's own number; potential_key is the
			//same from one refresh to the next
		QETProject project(QFINDTESTDATA("fixtures/every_feature.qet"));
		QString error;
		QSet<QString> keys;
		for (const Row &row : rows(project, &error)) keys.insert(row.key);
		QSet<QString> again;
		for (const Row &row : rows(project, &error)) again.insert(row.key);
		QVERIFY2(error.isEmpty(), qPrintable(error));
		QCOMPARE(again, keys);
	}
};

QET_APP_TEST_MAIN(tst_potentialview)
#include "tst_potentialview.moc"
