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
#include "diagram.h"
#include "qetapptest.h"
#include "qetproject.h"
#include "qetresult.h"
#include "titleblockproperties.h"

#include <QDomDocument>
#include <QSettings>
#include <QTemporaryDir>
#include <QtTest>

/**
	A folio's group and sub-group: named by the user, or left empty to be
	the folio's plant (=) and location (+). Title blocks show them as
	%{folio_group} and %{folio_subgroup}; a project keeps them when saved
	as .qet or .qetdb, and a project without them saves as before.
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
};

QET_APP_TEST_MAIN(tst_foliogroup)
#include "tst_foliogroup.moc"
