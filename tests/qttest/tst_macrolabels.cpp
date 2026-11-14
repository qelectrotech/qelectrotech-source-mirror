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
#include "ElementsCollection/elementslocation.h"
#include "diagram.h"
#include "diagramevent/diagrameventaddmacro.h"
#include "qetapp.h"
#include "qetapptest.h"
#include "qetgraphicsitem/element.h"
#include "qetproject.h"
#include "ui/macrolabelsdialog.h"
#include "utils/qetsettings.h"

#include <QDir>
#include <QGraphicsSceneMouseEvent>
#include <QPushButton>
#include <QScopeGuard>
#include <QTableWidget>
#include <QtTest>

/**
	Placing a macro: which labels and texts it asks for, the next free
	label proposed, and what is placed.
*/
class tst_macrolabels : public QObject
{
	Q_OBJECT

		//A coil K1 with a linked contact K1, a report and texts
	static QString macroText(const QString &label = QStringLiteral("K1"),
							 const QString &function = QStringLiteral("RUN"),
							 const QString &note = QStringLiteral("NOTE"))
	{
		return QStringLiteral(R"(<qet_macro><collection>
<element path="import/coil.elmt"><definition type="element" link_type="simple" uuid="{1d1e0000-0000-4000-8000-000000000001}" width="20" height="40" hotspot_x="10" hotspot_y="20"><names><name lang="en">Coil</name></names><description><line x1="0" y1="-10" x2="0" y2="10"/></description></definition></element>
<element path="import/arrow.elmt"><definition type="element" link_type="next_report" uuid="{1d1e0000-0000-4000-8000-000000000002}" width="20" height="40" hotspot_x="10" hotspot_y="20"><names><name lang="en">Arrow</name></names><description><line x1="0" y1="-10" x2="0" y2="10"/></description></definition></element>
</collection><diagram_content><diagram><elements>
<element type="macro://import/coil.elmt" uuid="{aaaaaaaa-0000-4000-8000-000000000001}" x="100" y="100" orientation="0" z="10" prefix=""><terminals/><inputs/><elementInformations><elementInformation name="label" show="1">%1</elementInformation><elementInformation name="formula" show="1">%1</elementInformation><elementInformation name="function" show="1">%2</elementInformation></elementInformations><dynamic_texts><dynamic_elmt_text text_from="UserText" x="0" y="0" uuid="{bbbbbbbb-0000-4000-8000-000000000001}"><text>%3</text></dynamic_elmt_text></dynamic_texts><texts_groups/></element>
<element type="macro://import/coil.elmt" uuid="{aaaaaaaa-0000-4000-8000-000000000002}" x="200" y="100" orientation="0" z="10" prefix=""><terminals/><inputs/><elementInformations><elementInformation name="label" show="1">%1</elementInformation></elementInformations><dynamic_texts/><texts_groups/></element>
<element type="macro://import/arrow.elmt" uuid="{aaaaaaaa-0000-4000-8000-000000000003}" x="300" y="100" orientation="0" z="10" prefix=""><terminals/><inputs/><elementInformations><elementInformation name="label" show="1">SIG1</elementInformation></elementInformations><dynamic_texts/><texts_groups/></element>
</elements><inputs><input x="10" y="10" rotation="0" text="&lt;p&gt;%3 here&lt;/p&gt;"/></inputs></diagram></diagram_content></qet_macro>)")
				.arg(label, function, note);
	}

	static QDomElement root(const QString &text, QDomDocument &doc)
	{
		[&]() {QVERIFY(doc.setContent(text));}();
		return doc.documentElement();
	}

	static QDomElement diagramOf(const QDomElement &r)
	{
		return r.firstChildElement(QStringLiteral("diagram_content")).firstChildElement(QStringLiteral("diagram"));
	}

	static QString saveMacro(const QString &name, const QString &text)
	{
		const QString path = QDir(QETApp::customElementsDir()).filePath(name + QStringLiteral(".qetmak"));
		QDir().mkpath(QETApp::customElementsDir());
		QFile file(path);
		if (!file.open(QIODevice::WriteOnly)) return QString();
		file.write(text.toUtf8());
		return QStringLiteral("custom://") + name + QStringLiteral(".qetmak");
	}

	static void click(DiagramEventAddMacro &placing, const QPointF &at)
	{
		QGraphicsSceneMouseEvent event(QEvent::GraphicsSceneMouseRelease);
		event.setButton(Qt::LeftButton);
		event.setScenePos(at);
		placing.mouseReleaseEvent(&event);
	}

	static QStringList labels(Diagram *folio)
	{
		QStringList list;
		for (Element *e : folio->elements())
			list << e->elementInformations().value(QStringLiteral("label")).toString();
		list.sort();
		return list;
	}

	QScopedPointer<QETProject> m_project;

  private slots:
	void initTestCase() {QETProject::setBackupEnabled(false);}
	void init() {QetSettings::setAskLabelsPlacingMacro(false);}

	void whatIsAsked()
	{
		QDomDocument doc;
		const auto fields = MacroLabels::fields(root(macroText(QStringLiteral("K1"), QStringLiteral("PUMP ?"), QStringLiteral("LINE ?")), doc));
			//One label for the two symbols sharing it; a report's is a signal
		QCOMPARE(fields.size(), 4);
		QCOMPARE(fields.at(0).kind, MacroLabels::Field::Label);
		QCOMPARE(fields.at(0).saved, QStringLiteral("K1"));
		QCOMPARE(fields.at(1).kind, MacroLabels::Field::Text);
		QCOMPARE(fields.at(1).saved, QStringLiteral("PUMP ?"));
		QCOMPARE(fields.at(2).saved, QStringLiteral("LINE ?"));
		QCOMPARE(fields.at(3).saved, QStringLiteral("LINE ? here"));   //a free text, read without its HTML
	}

	void nextFreeLabel_data()
	{
		QTest::addColumn<QString>("label");
		QTest::addColumn<QString>("expected");
		QTest::newRow("free stays") << QStringLiteral("K9") << QStringLiteral("K9");
		QTest::newRow("counted up") << QStringLiteral("K1") << QStringLiteral("K3");
		QTest::newRow("keeps its tail") << QStringLiteral("K1A") << QStringLiteral("K2A");
		QTest::newRow("no number") << QStringLiteral("M") << QStringLiteral("M1");
		QTest::newRow("question mark") << QStringLiteral("K?") << QStringLiteral("K3");
		QTest::newRow("strip terminal") << QStringLiteral("X1:2") << QStringLiteral("X1:3");
	}

	void nextFreeLabel()
	{
		QFETCH(QString, label);
		QFETCH(QString, expected);
		const QSet<QString> taken{QStringLiteral("=P-K1"), QStringLiteral("=P-K2"), QStringLiteral("=P-K1A"),
								  QStringLiteral("=P-M"), QStringLiteral("=P-X1:2")};
		QCOMPARE(MacroLabels::nextFree(label, QStringLiteral("=P"), taken), expected);
			//Another place has its own K1
		QCOMPARE(MacroLabels::nextFree(QStringLiteral("K1"), QStringLiteral("=Q"), taken), QStringLiteral("K1"));
	}

	void answersAreWritten()
	{
		QDomDocument doc;
		QDomElement r = root(macroText(QStringLiteral("K1"), QStringLiteral("PUMP ?"), QStringLiteral("LINE ?")), doc);
		auto fields = MacroLabels::fields(r);
		fields[0].value = QStringLiteral("K7");
		fields[1].value = QStringLiteral("PUMP 2");
		fields[2].value = QStringLiteral("LINE <A>");
		fields[3].value = QStringLiteral("LINE <B>");
		QDomElement diagram = diagramOf(r);
		MacroLabels::apply(diagram, fields);
		const QString saved = doc.toString();
		QVERIFY(saved.contains(QLatin1String(">K7<")));
		QVERIFY(!saved.contains(QLatin1String(">K1<")));
		QVERIFY(saved.contains(QLatin1String(">PUMP 2<")));
		QVERIFY(saved.contains(QLatin1String("LINE &lt;A>")));     //the element's user text
			//The free text holds HTML: the answer is escaped for it
		QVERIFY(saved.contains(QLatin1String("LINE &amp;lt;B&amp;gt;")));
			//The relabelled symbol no longer follows its formula
		QVERIFY(!saved.contains(QLatin1String("name=\"formula\"")));
	}

	void offByDefaultAMacroKeepsItsLabels()
	{
		QETProject project;
		QETApp::registerProject(&project);
		const auto registered = qScopeGuard([&project]() {QETApp::unregisterProject(&project);});
		Diagram *folio = project.addNewDiagram();
		const QString location = saveMacro(QStringLiteral("plain"), macroText());
		QVERIFY(!location.isEmpty());
		bool asked = false;
		DiagramEventAddMacro placing{ElementsLocation(location), folio, QPointF(50, 50)};
		QVERIFY(placing.isRunning());
		placing.setAsker([&asked](QList<MacroLabels::Field> &) {asked = true; return true;});
		click(placing, QPointF(100, 100));
		click(placing, QPointF(100, 300));
		QVERIFY(!asked);
		QCOMPARE(labels(folio), QStringList({QStringLiteral("K1"), QStringLiteral("K1"), QStringLiteral("K1"),
											 QStringLiteral("K1"), QStringLiteral("SIG1"), QStringLiteral("SIG1")}));
	}

	void theSettingNumbersEachPlacing()
	{
		QetSettings::setAskLabelsPlacingMacro(true);
		QETProject project;
		QETApp::registerProject(&project);
		const auto registered = qScopeGuard([&project]() {QETApp::unregisterProject(&project);});
		Diagram *folio = project.addNewDiagram();
		const QString location = saveMacro(QStringLiteral("numbered"), macroText());
		DiagramEventAddMacro placing{ElementsLocation(location), folio, QPointF(50, 50)};
		placing.setAsker([](QList<MacroLabels::Field> &) {return true;});
		click(placing, QPointF(100, 100));
		click(placing, QPointF(100, 300));
		QCOMPARE(labels(folio), QStringList({QStringLiteral("K1"), QStringLiteral("K1"), QStringLiteral("K2"),
											 QStringLiteral("K2"), QStringLiteral("SIG1"), QStringLiteral("SIG1")}));
			//One undo takes a placing away
		folio->undoStack().undo();
		QCOMPARE(labels(folio).count(QStringLiteral("K2")), 0);
	}

	void aQuestionMarkWaitsForTheSetting()
	{
		QETProject project;
		QETApp::registerProject(&project);
		const auto registered = qScopeGuard([&project]() {QETApp::unregisterProject(&project);});
		Diagram *folio = project.addNewDiagram();
		const QString location = saveMacro(QStringLiteral("question"), macroText(QStringLiteral("K?")));
		DiagramEventAddMacro placing{ElementsLocation(location), folio, QPointF(50, 50)};
		int asked = 0;
		placing.setAsker([&asked](QList<MacroLabels::Field> &fields) {
			++asked;
			return !fields.isEmpty() && fields.first().value == QLatin1String("K1");
		});
		click(placing, QPointF(100, 100));
		QCOMPARE(asked, 0);
		QVERIFY(labels(folio).contains(QStringLiteral("K?")));
	}

	void cancelPlacesNothing()
	{
		QetSettings::setAskLabelsPlacingMacro(true);
		QETProject project;
		QETApp::registerProject(&project);
		const auto registered = qScopeGuard([&project]() {QETApp::unregisterProject(&project);});
		Diagram *folio = project.addNewDiagram();
		const QString location = saveMacro(QStringLiteral("cancelled"), macroText());
		DiagramEventAddMacro placing{ElementsLocation(location), folio, QPointF(50, 50)};
		placing.setAsker([](QList<MacroLabels::Field> &) {return false;});
		click(placing, QPointF(100, 100));
		QVERIFY(labels(folio).isEmpty());
		QVERIFY(placing.isRunning());   //still following the mouse
	}

	void dialogEditsAndAcceptsAll()
	{
		MacroLabels::Field field;
		field.saved = QStringLiteral("K1");
		field.value = QStringLiteral("K2");
		MacroLabelsDialog dialog({field});
		auto *table = dialog.findChild<QTableWidget *>(QStringLiteral("macroLabels"));
		QVERIFY(table);
		QCOMPARE(table->item(0, 2)->text(), QStringLiteral("K2"));
		table->item(0, 2)->setText(QStringLiteral("K5"));
		QCOMPARE(dialog.fields().first().value, QStringLiteral("K5"));
			//Accept All goes back to the labels proposed
		dialog.findChild<QPushButton *>(QStringLiteral("macroAcceptAll"))->click();
		QCOMPARE(dialog.result(), int(QDialog::Accepted));
		QCOMPARE(dialog.fields().first().value, QStringLiteral("K2"));
	}
};

QET_APP_TEST_MAIN(tst_macrolabels)
#include "tst_macrolabels.moc"
