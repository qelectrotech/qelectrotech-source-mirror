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

#include <QDomDocument>
#include <QSettings>
#include <QSqlQuery>
#include <QTemporaryDir>
#include <QtTest>

/**
	Item C8 of #1878, agreed in #1295 (rule 3): a folio's document kind,
	an IEC 61355 code such as FS, is a folio property. Title blocks show it
	as %{document_kind}, qet_folios gives it, a project keeps it in .qet,
	.qetz and .qetdb, and a project without it saves as before.
*/
class tst_documentkind : public QObject
{
	Q_OBJECT
	QTemporaryDir m_dir;

	static void setKind(Diagram *diagram, const QString &kind)
	{
		TitleBlockProperties tb = diagram->border_and_titleblock.exportTitleBlock();
		tb.document_kind = kind;
		diagram->border_and_titleblock.importTitleBlock(tb);
	}

	static QDomElement xmlOf(const TitleBlockProperties &tb)
	{
		QDomDocument doc;
		QDomElement e = doc.createElement(QStringLiteral("diagram"));
		doc.appendChild(e);
		tb.toXml(e);
		return e;
	}

private slots:
	void initTestCase()
	{
		QVERIFY(m_dir.isValid());
		QETProject::setBackupEnabled(false);
	}

	void savedOnlyWhenSet()
	{
		TitleBlockProperties tb;
		QVERIFY(!xmlOf(tb).hasAttribute(QStringLiteral("document_kind")));
		tb.document_kind = QStringLiteral("FS");
		const QDomElement e = xmlOf(tb);
		QCOMPARE(e.attribute(QStringLiteral("document_kind")), QStringLiteral("FS"));
		TitleBlockProperties read;
		read.fromXml(e);
		QCOMPARE(read.document_kind, QStringLiteral("FS"));
		QVERIFY(read == tb);
			//The folio properties dialog records a change only when the
			//properties differ
		QVERIFY(TitleBlockProperties() != tb);
	}

	void settingsKeepIt()
	{
		QSettings settings(m_dir.filePath(QStringLiteral("s.ini")), QSettings::IniFormat);
		TitleBlockProperties tb;
		tb.document_kind = QStringLiteral("FS");
		tb.toSettings(settings, QStringLiteral("default/"));
		TitleBlockProperties read;
		read.fromSettings(settings, QStringLiteral("default/"));
		QCOMPARE(read.document_kind, QStringLiteral("FS"));
	}

	void titleBlockAndFolioListShowIt()
	{
		QETProject project;
		Diagram *schematic = project.addNewDiagram();
		project.addNewDiagram();
		setKind(schematic, QStringLiteral("FS"));
		QCOMPARE(schematic->border_and_titleblock.titleblockInformation()
					 .value(QStringLiteral("document_kind")).toString(), QStringLiteral("FS"));
		project.dataBase()->updateDB();
		QString error;
		QSqlQuery query = project.dataBase()->newQuery(QStringLiteral(
			"SELECT document_kind FROM qet_folios ORDER BY folio_number"), &error);
		QVERIFY2(error.isEmpty(), qPrintable(error));
		QStringList kinds;
		while (query.next()) kinds << query.value(0).toString();
		QCOMPARE(kinds, QStringList({QStringLiteral("FS"), QString()}));
	}

	void projectKeepsIt_data()
	{
		QTest::addColumn<QString>("file");
		QTest::newRow("qet") << QStringLiteral("kind.qet");
		QTest::newRow("qetz") << QStringLiteral("kind.qetz");
		QTest::newRow("qetdb") << QStringLiteral("kind.qetdb");
	}

	void projectKeepsIt()
	{
		QFETCH(QString, file);
		for (const QString &kind : {QStringLiteral("FS"), QString()}) {
			QETProject project;
			setKind(project.addNewDiagram(), kind);
			const QString path = m_dir.filePath(kind.isEmpty() ? QStringLiteral("none_") + file : file);
			project.setFilePath(path);
			QVERIFY2(project.write().isOk(), qPrintable(path));
			QETProject again(path);
			QVERIFY(!again.diagrams().isEmpty());
			QCOMPARE(again.diagrams().first()->border_and_titleblock.documentKind(), kind);
			if (kind.isEmpty() && !file.endsWith(QLatin1String(".qetdb"))) {
				QVERIFY(!project.toXml().toString().contains(QLatin1String("document_kind")));
			}
		}
	}
};

QET_APP_TEST_MAIN(tst_documentkind)
#include "tst_documentkind.moc"
