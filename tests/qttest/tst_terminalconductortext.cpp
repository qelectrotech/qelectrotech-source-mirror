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

/**
	A terminal symbol shows the number of the wire on it with
	%{conductor_text} (#1585): in a composite text and as the
	information of a text. Display only, and it follows the wire.
*/
class tst_terminalconductortext : public QObject
{
	Q_OBJECT

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

	void otherSymbolsShowNothing()
	{
		QETProject project(QFINDTESTDATA("fixtures/every_feature.qet"));
		for (Diagram *d : project.diagrams())
			for (Element *e : d->elements())
				if (e->elementData().m_type != ElementData::Terminal)
					QVERIFY(e->conductorTextForDisplay().isEmpty());
	}
};

QET_APP_TEST_MAIN(tst_terminalconductortext)
#include "tst_terminalconductortext.moc"
