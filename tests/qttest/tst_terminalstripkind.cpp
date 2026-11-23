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
#include "qetapptest.h"

#include "TerminalStrip/realterminal.h"
#include "TerminalStrip/terminalstrip.h"
#include "TerminalStrip/terminalstripdata.h"
#include "TerminalStrip/ui/terminalstripeditor.h"
#include "dataBase/projectdatabase.h"
#include "properties/elementdata.h"
#include "qetproject.h"
#include "qetresult.h"

#include <QComboBox>
#include <QDomDocument>
#include <QSqlQuery>
#include <QTemporaryDir>
#include <QUndoStack>
#include <QtTest>

/**
	A terminal strip has a kind: a terminal strip (the default), a plug or
	a socket. The kind is an information of the strip like its
	manufacturer, written only when it is not the default, so a project
	without connector is saved as before.
*/
class tst_terminalstripkind : public QObject
{
	Q_OBJECT

	QTemporaryDir m_dir;

	static QString xmlOf(const TerminalStripData &data)
	{
		QDomDocument doc;
		doc.appendChild(data.toXml(doc));
		return doc.toString(-1);
	}

	static TerminalStripData dataFromXml(const QString &xml)
	{
		QDomDocument doc;
		doc.setContent(xml);
		TerminalStripData data;
		data.fromXml(doc.documentElement());
		return data;
	}

		//A strip of one spare terminal, so it has a row in qet_terminals
	static TerminalStrip *strip(QETProject &project, const QString &name, TerminalStripData::Kind kind)
	{
		TerminalStrip *strip_ = project.newTerminalStrip(QString(), QString(), name);
		ElementData spare_data;
		spare_data.m_informations.addValue(QStringLiteral("label"), name + QStringLiteral(":1"));
		strip_->addTerminal(RealTerminal::createSpare(spare_data));
		strip_->setKind(kind);
		return strip_;
	}

	static QString stripKind(QETProject &project, const QString &name)
	{
		project.dataBase()->updateDB();
		QString error;
		QSqlQuery query = project.dataBase()->newQuery(
			QStringLiteral("SELECT strip_kind FROM qet_terminals WHERE strip = '%1'").arg(name), &error);
		if (!error.isEmpty() || !query.next()) {
			return QStringLiteral("<no row> ") + error;
		}
		return query.value(0).toString();
	}

private slots:
	void initTestCase()
	{
		QETProject::setBackupEnabled(false);
		QVERIFY(m_dir.isValid());
	}

	void defaultIsTerminalsAndNotWritten()
	{
		TerminalStripData data;
		QCOMPARE(data.kind(), TerminalStripData::Kind::Terminals);
		QVERIFY(!xmlOf(data).contains(QLatin1String("kind")));
			//Same output as before the kind existed, even with other informations
		const auto named = dataFromXml(QStringLiteral(
			"<terminal_strip_data uuid=\"{11111111-0000-4000-8000-000000000001}\"><informations>"
			"<information name=\"name\">X1</information></informations></terminal_strip_data>"));
		QVERIFY(xmlOf(named).contains(QLatin1String("X1")));
		QVERIFY(!xmlOf(named).contains(QLatin1String("kind")));
	}

	void plugAndSocketRoundTrip_data()
	{
		QTest::addColumn<int>("kind");
		QTest::addColumn<QString>("text");
		QTest::newRow("plug") << static_cast<int>(TerminalStripData::Kind::Plug) << QStringLiteral("plug");
		QTest::newRow("socket") << static_cast<int>(TerminalStripData::Kind::Socket) << QStringLiteral("socket");
	}

	void plugAndSocketRoundTrip()
	{
		QFETCH(int, kind);
		QFETCH(QString, text);
		TerminalStripData data;
		data.setKind(static_cast<TerminalStripData::Kind>(kind));
		const QString xml = xmlOf(data);
		QVERIFY(xml.contains(QStringLiteral("<information name=\"kind\">%1</information>").arg(text)));
		const auto back = dataFromXml(xml);
		QCOMPARE(static_cast<int>(back.kind()), kind);
	}

	void unknownValueReadsAsTerminals()
	{
		const QString xml = QStringLiteral(
			"<terminal_strip_data uuid=\"{11111111-0000-4000-8000-000000000001}\"><informations>"
			"<information name=\"kind\">banana</information></informations></terminal_strip_data>");
		QCOMPARE(dataFromXml(xml).kind(), TerminalStripData::Kind::Terminals);
	}

	void projectKeepsTheKind_data()
	{
		QTest::addColumn<QString>("file");
		QTest::newRow("qet") << QStringLiteral("kind.qet");
		QTest::newRow("qetz") << QStringLiteral("kind.qetz");
		QTest::newRow("qetdb") << QStringLiteral("kind.qetdb");
	}

		//Saved as .qet, and as .qetz / .qetdb where the strips go to the XML
		//fallback of the container (an information the database has no column for)
	void projectKeepsTheKind()
	{
		QFETCH(QString, file);
		const QString path = m_dir.filePath(file);
		{
			QETProject project;
			project.addNewDiagram();
			strip(project, QStringLiteral("J1"), TerminalStripData::Kind::Socket);
			strip(project, QStringLiteral("J2"), TerminalStripData::Kind::Plug);
			strip(project, QStringLiteral("X1"), TerminalStripData::Kind::Terminals);
			project.setFilePath(path);
			QVERIFY(project.write().isOk());
		}
		QETProject again(path);
		QCOMPARE(again.state(), QETProject::Ok);
		QCOMPARE(again.terminalStrip().size(), 3);
		for (TerminalStrip *strip_ : again.terminalStrip()) {
			if (strip_->name() == QLatin1String("J1")) {
				QCOMPARE(strip_->kind(), TerminalStripData::Kind::Socket);
				QVERIFY(strip_->isConnector());
			} else if (strip_->name() == QLatin1String("J2")) {
				QCOMPARE(strip_->kind(), TerminalStripData::Kind::Plug);
				QVERIFY(strip_->isConnector());
			} else {
				QCOMPARE(strip_->kind(), TerminalStripData::Kind::Terminals);
				QVERIFY(!strip_->isConnector());
			}
		}
	}

	void viewTellsTheKind()
	{
		QETProject project;
		project.addNewDiagram();
		strip(project, QStringLiteral("J1"), TerminalStripData::Kind::Socket);
		strip(project, QStringLiteral("J2"), TerminalStripData::Kind::Plug);
		strip(project, QStringLiteral("X1"), TerminalStripData::Kind::Terminals);
		QCOMPARE(stripKind(project, QStringLiteral("J1")), QStringLiteral("socket"));
		QCOMPARE(stripKind(project, QStringLiteral("J2")), QStringLiteral("plug"));
		QCOMPARE(stripKind(project, QStringLiteral("X1")), QStringLiteral("terminals"));
	}

	void editorChangesTheKindInOneUndoStep()
	{
		QETProject project;
		project.addNewDiagram();
		TerminalStrip *strip_ = strip(project, QStringLiteral("X1"), TerminalStripData::Kind::Terminals);

		TerminalStripEditor editor(&project);
		editor.setCurrentStrip(strip_);
		auto *combo = editor.findChild<QComboBox *>(QStringLiteral("m_kind_cb"));
		QVERIFY(combo);
		QCOMPARE(combo->count(), 3);
		QCOMPARE(combo->currentIndex(), 0);

		const int steps_before = project.undoStack()->count();
		combo->setCurrentIndex(combo->findText(QStringLiteral("Socket")));
		editor.apply();
		QCOMPARE(strip_->kind(), TerminalStripData::Kind::Socket);
		QCOMPARE(combo->currentIndex(), 2);
		QCOMPARE(project.undoStack()->count(), steps_before + 1);

		project.undoStack()->undo();
		QCOMPARE(strip_->kind(), TerminalStripData::Kind::Terminals);
	}
};

QET_APP_TEST_MAIN(tst_terminalstripkind)
#include "tst_terminalstripkind.moc"
