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
#include "diagram.h"
#include "qetapptest.h"
#include "qetproject.h"
#include "qetresult.h"
#include "titleblockproperties.h"
#include "undocommand/changetitleblockcommand.h"
#include "utils/qetsettings.h"

#include <QDomDocument>
#include <QSettings>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QTemporaryDir>
#include <QtTest>

/**
	A folio's group and sub-group: named by the user, or left empty to be
	the folio's plant (=) and location (+). Title blocks show them as
	%{folio_group} and %{folio_subgroup}; a project keeps them when saved
	as .qet or .qetdb, and a project without them saves as before. The
	summary (project_summary_view) and qet_folios give the same values.
	With the setting on, a new folio takes the group of the folio before it.
*/
class tst_foliogroup : public QObject
{
	Q_OBJECT
	QTemporaryDir m_dir;

	static void setFolio(Diagram *diagram, const QString &plant,
						 const QString &location, const QString &group = {},
						 const QString &subgroup = {})
	{
		TitleBlockProperties tb = diagram->border_and_titleblock.exportTitleBlock();
		tb.plant = plant;
		tb.locmach = location;
		tb.folio_group = group;
		tb.folio_subgroup = subgroup;
		diagram->border_and_titleblock.importTitleBlock(tb);
	}

		//The values the folio's title block draws
	static QString shown(Diagram *diagram, const QString &variable)
	{
		return diagram->border_and_titleblock.titleblockInformation()
				.value(variable).toString();
	}

	static QDomElement xmlOf(const TitleBlockProperties &tb)
	{
		QDomDocument doc;
		QDomElement e = doc.createElement(QStringLiteral("diagram"));
		doc.appendChild(e);
		tb.toXml(e);
		return e;
	}

		//The group and sub-group of the first folio of @p project once
		//written to @p file and opened again
	QStringList reopened(QETProject &project, const QString &file)
	{
		project.setFilePath(m_dir.filePath(file));
		const QETResult result = project.write();
		if (!result.isOk()) return {QStringLiteral("(write failed) ") + result.errorMessage()};
		QETProject again(m_dir.filePath(file));
		if (again.diagrams().isEmpty()) return {QStringLiteral("(no folio)")};
		const TitleBlockProperties tb =
				again.diagrams().first()->border_and_titleblock.exportTitleBlock();
		return {tb.folio_group, tb.folio_subgroup};
	}

		//"group|sub-group" of every folio of @p project, in folio order,
		//as @p view gives them
	static QStringList groupsIn(QETProject &project, const QString &view)
	{
		project.dataBase()->updateDB();
		QString error;
		const QString order = view == QLatin1String("qet_folios")
				? QStringLiteral("folio_number") : QStringLiteral("pos");
		QSqlQuery query = project.dataBase()->newQuery(
			QStringLiteral("SELECT folio_group, folio_subgroup FROM %1 ORDER BY %2")
				.arg(view, order), &error);
		if (!error.isEmpty()) return {error};
		QStringList rows;
		while (query.next())
			rows << query.value(0).toString() + QLatin1Char('|') + query.value(1).toString();
		return rows;
	}

		//Two folios: FACTORY +CP1 taking its plant and location, and
		//SILO +CP2 in its own group "Lists" > "Material list"
	static void twoFolios(QETProject &project)
	{
		setFolio(project.addNewDiagram(), QStringLiteral("FACTORY"), QStringLiteral("CP1"));
		setFolio(project.addNewDiagram(), QStringLiteral("SILO"), QStringLiteral("CP2"),
				 QStringLiteral("Lists"), QStringLiteral("Material list"));
	}

private slots:
	void initTestCase()
	{
		QVERIFY(m_dir.isValid());
	}

	void emptyIsThePlantAndLocation()
	{
		TitleBlockProperties tb;
		tb.plant = QStringLiteral("FACTORY");
		tb.locmach = QStringLiteral("CP1");
		QCOMPARE(tb.resolvedFolioGroup(), QStringLiteral("FACTORY"));
		QCOMPARE(tb.resolvedFolioSubgroup(), QStringLiteral("CP1"));

		tb.folio_group = QStringLiteral("Lists");
		tb.folio_subgroup = QStringLiteral("Material list");
		QCOMPARE(tb.resolvedFolioGroup(), QStringLiteral("Lists"));
		QCOMPARE(tb.resolvedFolioSubgroup(), QStringLiteral("Material list"));

			//Each one on its own
		tb.folio_subgroup.clear();
		QCOMPARE(tb.resolvedFolioSubgroup(), QStringLiteral("CP1"));
	}

	void titleBlockShowsTheGroup()
	{
		QETProject project;
		Diagram *diagram = project.addNewDiagram();
		setFolio(diagram, QStringLiteral("FACTORY"), QStringLiteral("CP1"));
		QCOMPARE(shown(diagram, QStringLiteral("folio_group")), QStringLiteral("FACTORY"));
		QCOMPARE(shown(diagram, QStringLiteral("folio_subgroup")), QStringLiteral("CP1"));

		setFolio(diagram, QStringLiteral("FACTORY"), QStringLiteral("CP1"),
				 QStringLiteral("Lists"), QStringLiteral("Material list"));
		QCOMPARE(shown(diagram, QStringLiteral("folio_group")), QStringLiteral("Lists"));
		QCOMPARE(shown(diagram, QStringLiteral("folio_subgroup")), QStringLiteral("Material list"));
			//plant and location are still the folio's own
		QCOMPARE(shown(diagram, QStringLiteral("plant")), QStringLiteral("FACTORY"));
		QCOMPARE(shown(diagram, QStringLiteral("locmach")), QStringLiteral("CP1"));
	}

	void savedOnlyWhenSet()
	{
		TitleBlockProperties tb;
		tb.plant = QStringLiteral("FACTORY");
		QDomElement e = xmlOf(tb);
		QVERIFY(!e.hasAttribute(QStringLiteral("folio_group")));
		QVERIFY(!e.hasAttribute(QStringLiteral("folio_subgroup")));

		tb.folio_group = QStringLiteral("Lists");
		tb.folio_subgroup = QStringLiteral("Material list");
		e = xmlOf(tb);
		QCOMPARE(e.attribute(QStringLiteral("folio_group")), QStringLiteral("Lists"));
		QCOMPARE(e.attribute(QStringLiteral("folio_subgroup")), QStringLiteral("Material list"));

		TitleBlockProperties read;
		read.fromXml(e);
		QCOMPARE(read.folio_group, QStringLiteral("Lists"));
		QCOMPARE(read.folio_subgroup, QStringLiteral("Material list"));
		QVERIFY(read == tb);
	}

	void oldFolioHasNoGroup()
	{
			//A folio written before groups existed
		QDomDocument doc;
		QVERIFY(doc.setContent(QStringLiteral(
			"<diagram title=\"Old\" plant=\"SILO\" locmach=\"CP2\"/>")));
		TitleBlockProperties tb;
		tb.fromXml(doc.documentElement());
		QVERIFY(tb.folio_group.isEmpty());
		QVERIFY(tb.folio_subgroup.isEmpty());
		QCOMPARE(tb.resolvedFolioGroup(), QStringLiteral("SILO"));
	}

	void groupCountsInComparison()
	{
			//The folio properties dialog only records a change when the
			//properties differ
		TitleBlockProperties a, b;
		QVERIFY(a == b);
		b.folio_group = QStringLiteral("Lists");
		QVERIFY(a != b);
		b = a;
		b.folio_subgroup = QStringLiteral("Material list");
		QVERIFY(a != b);
	}

	void settingsKeepTheGroup()
	{
		QSettings settings(m_dir.filePath(QStringLiteral("s.ini")), QSettings::IniFormat);
		TitleBlockProperties tb;
		tb.folio_group = QStringLiteral("Lists");
		tb.folio_subgroup = QStringLiteral("Material list");
		tb.toSettings(settings, QStringLiteral("default/"));
		TitleBlockProperties read;
		read.fromSettings(settings, QStringLiteral("default/"));
		QCOMPARE(read.folio_group, QStringLiteral("Lists"));
		QCOMPARE(read.folio_subgroup, QStringLiteral("Material list"));
	}

	void projectKeepsTheGroup_data()
	{
		QTest::addColumn<QString>("file");
		QTest::newRow("qet") << QStringLiteral("groups.qet");
		QTest::newRow("qetdb") << QStringLiteral("groups.qetdb");
	}

	void projectKeepsTheGroup()
	{
		QFETCH(QString, file);
		QETProject project;
		Diagram *diagram = project.addNewDiagram();
		setFolio(diagram, QStringLiteral("FACTORY"), QStringLiteral("CP1"),
				 QStringLiteral("Lists"), QStringLiteral("Material list"));
		QCOMPARE(reopened(project, file),
				 QStringList({QStringLiteral("Lists"), QStringLiteral("Material list")}));
	}

	void projectWithoutGroupStaysWithout_data()
	{
		projectKeepsTheGroup_data();
	}

	void projectWithoutGroupStaysWithout()
	{
		QFETCH(QString, file);
		QETProject project;
		Diagram *diagram = project.addNewDiagram();
		setFolio(diagram, QStringLiteral("FACTORY"), QStringLiteral("CP1"));
		QCOMPARE(reopened(project, QStringLiteral("none_") + file),
				 QStringList({QString(), QString()}));
	}

		//"group|sub-group" of each folio of @p project, as stored on it
	static QStringList ownGroups(QETProject &project)
	{
		QStringList rows;
		for (Diagram *diagram : project.diagrams())
			rows << diagram->border_and_titleblock.folioGroup() + QLatin1Char('|')
					+ diagram->border_and_titleblock.folioSubgroup();
		return rows;
	}

	void newFolioHasNoGroupByDefault()
	{
		QVERIFY(!QetSettings::newFolioTakesPreviousGroup());
		QETProject project;
		twoFolios(project);
		project.addNewDiagram();
		QCOMPARE(ownGroups(project).last(), QStringLiteral("|"));
	}

	void newFolioTakesTheGroupBeforeIt()
	{
		QetSettings::setNewFolioTakesPreviousGroup(true);
		QETProject project;
		twoFolios(project);

			//At the end: after the "Lists" folio
		project.addNewDiagram();
			//Inserted second: after the FACTORY folio, which has no group
			//of its own, only its plant
		project.addNewDiagram(1);
			//First: no folio before it
		project.addNewDiagram(0);
		QetSettings::setNewFolioTakesPreviousGroup(false);

		QCOMPARE(ownGroups(project), QStringList({
			QStringLiteral("|"),
			QStringLiteral("|"),
			QStringLiteral("|"),
			QStringLiteral("Lists|Material list"),
			QStringLiteral("Lists|Material list")}));
	}

	void takingTheGroupIsUndoneWithTheFolio()
	{
		QetSettings::setNewFolioTakesPreviousGroup(true);
		QETProject project;
		twoFolios(project);
		Diagram *added = project.addNewDiagram();
		QetSettings::setNewFolioTakesPreviousGroup(false);
		QVERIFY(added);
		QCOMPARE(project.diagrams().count(), 3);
		project.undoStack()->undo();
		QCOMPARE(project.diagrams().count(), 2);
		project.undoStack()->redo();
		QCOMPARE(ownGroups(project).last(), QStringLiteral("Lists|Material list"));
	}

	void viewsGiveTheGroup_data()
	{
		QTest::addColumn<QString>("view");
		QTest::newRow("summary") << QStringLiteral("project_summary_view");
		QTest::newRow("qet_folios") << QStringLiteral("qet_folios");
	}

	void viewsGiveTheGroup()
	{
		QFETCH(QString, view);
		QETProject project;
		twoFolios(project);
		QCOMPARE(groupsIn(project, view), QStringList({
			QStringLiteral("FACTORY|CP1"),
			QStringLiteral("Lists|Material list")}));
	}

	void viewsSayWhatTheTitleBlockShows()
	{
		QETProject project;
		twoFolios(project);
		QStringList shown_rows;
		for (Diagram *diagram : project.diagrams())
			shown_rows << shown(diagram, QStringLiteral("folio_group")) + QLatin1Char('|')
					+ shown(diagram, QStringLiteral("folio_subgroup"));
		QCOMPARE(groupsIn(project, QStringLiteral("qet_folios")), shown_rows);
	}

	void viewsFollowAChangeAndItsUndo()
	{
		QETProject project;
		twoFolios(project);
		Diagram *first = project.diagrams().first();
		const TitleBlockProperties before = first->border_and_titleblock.exportTitleBlock();
		TitleBlockProperties after = before;
		after.folio_group = QStringLiteral("Power");
		first->undoStack().push(new ChangeTitleBlockCommand(first, before, after));
		QCOMPARE(groupsIn(project, QStringLiteral("project_summary_view")).first(),
				 QStringLiteral("Power|CP1"));

		first->undoStack().undo();
		QCOMPARE(groupsIn(project, QStringLiteral("project_summary_view")).first(),
				 QStringLiteral("FACTORY|CP1"));
	}

	void qetdbFileGivesTheGroup()
	{
			//Another tool reading the saved file, without QElectroTech
		QETProject project;
		twoFolios(project);
		const QString file = m_dir.filePath(QStringLiteral("views.qetdb"));
		project.setFilePath(file);
		QVERIFY(project.write().isOk());

		QStringList rows;
		const QString name = QStringLiteral("foliogroup_file");
		{
			QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), name);
			db.setDatabaseName(file);
			db.setConnectOptions(QStringLiteral("QSQLITE_OPEN_READONLY"));
			QVERIFY(db.open());
			QSqlQuery query(db);
			QVERIFY(query.exec(QStringLiteral(
				"SELECT folio_group, folio_subgroup FROM qet_folios ORDER BY folio_number")));
			while (query.next())
				rows << query.value(0).toString() + QLatin1Char('|') + query.value(1).toString();
			db.close();
		}
		QSqlDatabase::removeDatabase(name);
		QCOMPARE(rows, QStringList({
			QStringLiteral("FACTORY|CP1"),
			QStringLiteral("Lists|Material list")}));
	}
};

QET_APP_TEST_MAIN(tst_foliogroup)
#include "tst_foliogroup.moc"
