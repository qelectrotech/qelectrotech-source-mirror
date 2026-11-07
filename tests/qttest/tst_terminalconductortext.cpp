// SPDX-License-Identifier: GPL-2.0-or-later
#include "qetapptest.h"
#include "TerminalStrip/realterminal.h"
#include "TerminalStrip/terminalstrip.h"
#include "diagram.h"
#include "properties/elementdata.h"
#include "qetgraphicsitem/conductor.h"
#include "qetgraphicsitem/dynamicelementtextitem.h"
#include "qetgraphicsitem/element.h"
#include "qetgraphicsitem/terminal.h"
#include "qetinformation.h"
#include "qetproject.h"
#include "autoNum/assignvariables.h"
#include "ui/compositetexteditdialog.h"

#include <QComboBox>

/**
	A terminal symbol shows the number of the wire on it with
	%{conductor_text} (#1585): in a composite text and as the
	information of a text. Display only, and it follows the wire.
	Also its colour and section, and each of them for one connection
	point only (%{conductor_text_1} ...).
*/
class tst_terminalconductortext : public QObject
{
	Q_OBJECT

		//Two terminal symbols on one folio, with two connection points each
	static QVector<Element *> twoTerminals(QETProject &project)
	{
		QVector<Element *> terminals;
		for (const QSharedPointer<RealTerminal> &real_t
			 : project.terminalStrip().value(0)->realTerminals())
		{
			Element *e = real_t ? real_t->element() : nullptr;
			if (e && e->terminals().size() >= 2
				&& (terminals.isEmpty() || e->diagram() == terminals.first()->diagram())
				&& !terminals.contains(e))
				terminals << e;
			if (terminals.size() == 2) break;
		}
		return terminals;
	}

	static DynamicElementTextItem *addComposite(Element *e, const QString &text)
	{
		auto *deti = new DynamicElementTextItem(e);
		e->addDynamicTextItem(deti);
		deti->setCompositeText(text);
		deti->setTextFrom(DynamicElementTextItem::CompositeText);
		return deti;
	}

	static DynamicElementTextItem *addInfo(Element *e, const QString &info)
	{
		auto *deti = new DynamicElementTextItem(e);
		e->addDynamicTextItem(deti);
		deti->setInfoName(info);
		deti->setTextFrom(DynamicElementTextItem::ElementInfo);
		return deti;
	}

private slots:
	void textFollowsTheWire()
	{
		QETProject project(QFINDTESTDATA("fixtures/every_feature.qet"));
		QCOMPARE(project.state(), QETProject::Ok);

		QVector<Element *> terminals;
		for (const QSharedPointer<RealTerminal> &real_t
			 : project.terminalStrip().value(0)->realTerminals())
		{
			Element *e = real_t ? real_t->element() : nullptr;
			if (e && !e->terminals().isEmpty() && e->diagram() == (terminals.isEmpty() ? e->diagram() : terminals.first()->diagram()))
				terminals << e;
			if (terminals.size() == 2) break;
		}
		QVERIFY2(terminals.size() == 2, "two terminal symbols on one folio");
		Element *a = terminals.at(0), *b = terminals.at(1);

		auto *composite = new DynamicElementTextItem(a);
		a->addDynamicTextItem(composite);
		composite->setCompositeText(QStringLiteral("w=%{conductor_text}"));
		composite->setTextFrom(DynamicElementTextItem::CompositeText);
		auto *info = new DynamicElementTextItem(a);
		a->addDynamicTextItem(info);
		info->setInfoName(QETInformation::ELMT_CONDUCTOR_TEXT);
		info->setTextFrom(DynamicElementTextItem::ElementInfo);

		const QString before = a->conductorTextForDisplay();
		QCOMPARE(composite->toPlainText(), "w=" + before);

		auto *wire = new Conductor(a->terminals().first(), b->terminals().first());
		a->diagram()->addItem(wire);
		ConductorProperties p = wire->properties();
		p.text = QStringLiteral("W101");
		wire->setProperties(p);
		QVERIFY(a->conductorTextForDisplay().contains(QStringLiteral("W101")));
		QCOMPARE(composite->toPlainText(), "w=" + a->conductorTextForDisplay());
		QCOMPARE(info->toPlainText(), a->conductorTextForDisplay());

		p.text = QStringLiteral("W102");
		wire->setProperties(p);
		QVERIFY(a->conductorTextForDisplay().contains(QStringLiteral("W102")));
		QVERIFY(!a->conductorTextForDisplay().contains(QStringLiteral("W101")));
		QCOMPARE(composite->toPlainText(), "w=" + a->conductorTextForDisplay());
		QCOMPARE(info->toPlainText(), a->conductorTextForDisplay());

			//Display only: the element stores nothing for it
		QVERIFY(!a->elementInformations().keys().contains(QETInformation::ELMT_CONDUCTOR_TEXT));

		delete wire;
		QCOMPARE(a->conductorTextForDisplay(), before);
		QCOMPARE(composite->toPlainText(), "w=" + before);
		QCOMPARE(info->toPlainText(), before);
	}

	void colorAndSectionFollowTheWire()
	{
		QETProject project(QFINDTESTDATA("fixtures/every_feature.qet"));
		QCOMPARE(project.state(), QETProject::Ok);
		const QVector<Element *> terminals = twoTerminals(project);
		QVERIFY2(terminals.size() == 2, "two terminal symbols on one folio");
		Element *a = terminals.at(0), *b = terminals.at(1);

		DynamicElementTextItem *color = addInfo(a, QETInformation::COND_COLOR);
		DynamicElementTextItem *section = addComposite(a, QStringLiteral("s=%{conductor_section}"));

		auto *wire = new Conductor(a->terminals().first(), b->terminals().first());
		a->diagram()->addItem(wire);
		ConductorProperties p = wire->properties();
		p.m_wire_color = QStringLiteral("GNYE-T");
		p.m_wire_section = QStringLiteral("2.5-T");
		wire->setProperties(p);
		QVERIFY(color->toPlainText().contains(QStringLiteral("GNYE-T")));
		QVERIFY(section->toPlainText().contains(QStringLiteral("2.5-T")));

		p.m_wire_color = QStringLiteral("BU-T");
		wire->setProperties(p);
		QVERIFY(color->toPlainText().contains(QStringLiteral("BU-T")));
		QVERIFY(!color->toPlainText().contains(QStringLiteral("GNYE-T")));

			//Display only
		QVERIFY(!a->elementInformations().keys().contains(QETInformation::COND_COLOR));
		QVERIFY(!a->elementInformations().keys().contains(QETInformation::COND_SECTION));

		delete wire;
		QVERIFY(!color->toPlainText().contains(QStringLiteral("BU-T")));
		QVERIFY(!section->toPlainText().contains(QStringLiteral("2.5-T")));
	}

	void oneConnectionPoint()
	{
		QETProject project(QFINDTESTDATA("fixtures/every_feature.qet"));
		QCOMPARE(project.state(), QETProject::Ok);
		const QVector<Element *> terminals = twoTerminals(project);
		QVERIFY2(terminals.size() == 2, "two terminal symbols on one folio");
		Element *a = terminals.at(0), *b = terminals.at(1);

		DynamicElementTextItem *both = addComposite(a, QStringLiteral("%{conductor_text}"));
		DynamicElementTextItem *first = addComposite(a, QStringLiteral("1=%{conductor_text_1}"));
		DynamicElementTextItem *second = addInfo(a, QStringLiteral("conductor_text_2"));
		DynamicElementTextItem *none = addComposite(a, QStringLiteral("9=%{conductor_text_9}"));

		auto *top = new Conductor(a->terminals().at(0), b->terminals().at(0));
		auto *bottom = new Conductor(a->terminals().at(1), b->terminals().at(1));
		a->diagram()->addItem(top);
		a->diagram()->addItem(bottom);
		ConductorProperties p = top->properties();
		p.text = QStringLiteral("W201");
		top->setProperties(p);
		p = bottom->properties();
		p.text = QStringLiteral("W202");
		bottom->setProperties(p);

		QVERIFY(both->toPlainText().contains(QStringLiteral("W201")));
		QVERIFY(both->toPlainText().contains(QStringLiteral("W202")));
		QVERIFY(first->toPlainText().contains(QStringLiteral("W201")));
		QVERIFY(!first->toPlainText().contains(QStringLiteral("W202")));
		QVERIFY(second->toPlainText().contains(QStringLiteral("W202")));
		QVERIFY(!second->toPlainText().contains(QStringLiteral("W201")));
			//A connection point the symbol does not have shows nothing
		QCOMPARE(none->toPlainText(), QStringLiteral("9="));

		p.text = QStringLiteral("W203");
		bottom->setProperties(p);
		QVERIFY(second->toPlainText().contains(QStringLiteral("W203")));
		QVERIFY(!first->toPlainText().contains(QStringLiteral("W203")));

		delete top;
		QVERIFY(!first->toPlainText().contains(QStringLiteral("W201")));
		QVERIFY(second->toPlainText().contains(QStringLiteral("W203")));
		delete bottom;
	}

	void keys()
	{
		const QStringList keys = QETInformation::terminalConductorInfoKeys(2);
		QCOMPARE(keys.size(), 9);
		QVERIFY(keys.contains(QStringLiteral("conductor_section_2")));
		for (const QString &key : keys) {
			QVERIFY2(QETInformation::isTerminalConductorInfo(key), qPrintable(key));
			QVERIFY2(!QETInformation::translatedInfoKey(key).isEmpty(), qPrintable(key));
				//Never a stored information: no database column, no BOM
			QVERIFY(!QETInformation::elementInfoKeys().contains(key));
		}
		QString property;
		int connection = -1;
		QVERIFY(QETInformation::isTerminalConductorInfo(QStringLiteral("conductor_color_12"), &property, &connection));
		QCOMPARE(property, QETInformation::COND_COLOR);
		QCOMPARE(connection, 12);
		QVERIFY(!QETInformation::isTerminalConductorInfo(QStringLiteral("conductor_text_0")));
		QVERIFY(!QETInformation::isTerminalConductorInfo(QStringLiteral("conductor_type")));
		QVERIFY(QETInformation::usesTerminalConductorInfo(QStringLiteral("x %{conductor_color_1}")));
		QVERIFY(!QETInformation::usesTerminalConductorInfo(QStringLiteral("%{label}")));
	}

	void wireTextIsNotReadAgain()
	{
			//A wire whose own text looks like a variable is shown as it is
		DiagramContext dc;
		dc.addValue(QStringLiteral("conductor_text_1"), QStringLiteral("%{conductor_text_1}"));
		dc.addValue(QStringLiteral("conductor_text_2"), QStringLiteral("B"));
		QCOMPARE(autonum::AssignVariables::replaceVariable(
					 QStringLiteral("%{conductor_text_1}/%{conductor_text_2}"), dc),
				 QStringLiteral("%{conductor_text_1}/B"));
	}

	void compositeDialogOffersThem()
	{
		QETProject project(QFINDTESTDATA("fixtures/every_feature.qet"));
		QCOMPARE(project.state(), QETProject::Ok);
		const QVector<Element *> terminals = twoTerminals(project);
		QVERIFY2(!terminals.isEmpty(), "a terminal symbol");
		Element *a = terminals.first();
		DynamicElementTextItem *deti = addComposite(a, QString());

		CompositeTextEditDialog dialog(deti);
		auto *combo = dialog.findChild<QComboBox *>(QStringLiteral("m_info_cb"));
		QVERIFY(combo);
		QStringList vars;
		for (int i = 0 ; i < combo->count() ; ++i)
			vars << combo->itemData(i).toString();
		for (const QString &key
			 : QETInformation::terminalConductorInfoKeys(a->terminals().size()))
			QVERIFY2(vars.contains("%{" + key + "}"), qPrintable(key));
		QVERIFY(vars.contains(QStringLiteral("%{conductor_text_2}")));
	}

	void otherSymbolsShowNothing()
	{
		QETProject project(QFINDTESTDATA("fixtures/every_feature.qet"));
		for (Diagram *d : project.diagrams())
			for (Element *e : d->elements())
				if (e->elementData().m_type != ElementData::Terminal) {
					QVERIFY(e->conductorTextForDisplay().isEmpty());
					QVERIFY(e->conductorInfoForDisplay(QStringLiteral("conductor_text_1")).isEmpty());
				}
	}
};

QET_APP_TEST_MAIN(tst_terminalconductortext)
#include "tst_terminalconductortext.moc"
