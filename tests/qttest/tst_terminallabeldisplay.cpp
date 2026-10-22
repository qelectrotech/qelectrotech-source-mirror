// SPDX-License-Identifier: GPL-2.0-or-later
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
#include "diagram.h"
#include "properties/elementdata.h"
#include "qetgraphicsitem/dynamicelementtextitem.h"
#include "qetgraphicsitem/element.h"
#include "qetgraphicsitem/terminal.h"
#include "qetinformation.h"
#include "qetproject.h"
#include "utils/qetsettings.h"

/**
	How a terminal's own label is drawn, the setting of the moment being
	read (@sa QetSettings::terminalLabelSplit).

	The label stays whole wherever it is used as data: what the element
	stores, the numbering, the sorting in the terminal strip editor and
	the exports are never touched by the setting. Only the drawing reacts:
	with the labelling split, the label ("X1") and the terminal number
	("1") are two parts -- in their own fields when the symbol has a
	field for the number, one under the other when it has not.

	fixtures/every_feature.qet has the strip X1 whose terminals are
	plain terminal symbols: a field tagged "label", no field for the
	number, and -- as every element of the fixture -- a label the test
	gives them before it looks at the drawing.
*/
class tst_terminallabeldisplay : public QObject
{
	Q_OBJECT

	//A terminal element of the fixture's strip X1, given @p label
	static Element *terminalWithLabel(QETProject &project, const QString &label)
	{
		const QVector<TerminalStrip *> strips = project.terminalStrip();
		if (strips.isEmpty()) return nullptr;
		for (const QSharedPointer<RealTerminal> &real_t
			 : strips.first()->realTerminals())
		{
			Element *element = real_t ? real_t->element() : nullptr;
			if (!element || element->terminals().isEmpty()) continue;

			DiagramContext dc = element->elementInformations();
			dc.addValue(QStringLiteral("label"), label);
			element->setElementInformations(dc);
			if (element->actualLabel() == label) return element;
		}
		return nullptr;
	}

	//The text field of @p element bound to @p info_name, the field the
	//symbol carries for it
	static DynamicElementTextItem *fieldBoundTo(Element *element, const QString &info_name)
	{
		for (DynamicElementTextItem *text : element->dynamicTextItems())
			if (text->infoName() == info_name) return text;
		return nullptr;
	}

	//A text field as a symbol made for the split would carry one: bound
	//to the terminal number, and nothing else
	static DynamicElementTextItem *addNumberField(Element *element)
	{
		DynamicElementTextItem *number = new DynamicElementTextItem(element);
		element->addDynamicTextItem(number);
		number->setInfoName(QETInformation::ELMT_TERMINAL_NUMBER);
		number->setTextFrom(DynamicElementTextItem::ElementInfo);
		return number;
	}

	//The "Show name" box of the selection properties, unticked: the
	//element then stores the information "label_shown" = "false", the
	//only thing it stores for it
	static void hideName(Element *element)
	{
		DiagramContext dc = element->elementInformations();
		dc.addValue(QStringLiteral("label_shown"), QStringLiteral("false"));
		element->setElementInformations(dc);
	}

	//Any element of the fixture which is not a terminal and carries a
	//name to draw
	static Element *aNonTerminal(QETProject &project)
	{
		for (Diagram *diagram : project.diagrams())
			for (Element *element : diagram->elements())
				if (element->elementData().m_type != ElementData::Terminal
					&& !element->actualLabel().isEmpty())
					return element;
		return nullptr;
	}

private slots:
	void cleanup()
	{
			//Whatever a test left behind, the next one (and the rest of
			//the suite) starts from the shipped default
		QetSettings::setTerminalLabelSplit(false);
	}

	//The shipped default: one field with the whole label, the way it has
	//always been drawn
	void oneFieldKeepsTheWholeLabel()
	{
		QetSettings::setTerminalLabelSplit(false);

		const QString fixture = QFINDTESTDATA("fixtures/every_feature.qet");
		QETProject project(fixture);
		QCOMPARE(project.state(), QETProject::Ok);

		Element *element = terminalWithLabel(project, QStringLiteral("X1:1"));
		QVERIFY2(element, "the fixture has a terminal of the strip X1");

		QCOMPARE(element->actualLabel(), QStringLiteral("X1:1"));
		QCOMPARE(element->labelForDisplay(), QStringLiteral("X1:1"));
		QVERIFY(element->terminalNumberForDisplay().isEmpty());
		QVERIFY(element->infoForDisplay(QETInformation::ELMT_TERMINAL_NUMBER).isEmpty());
		QVERIFY(!element->hasTerminalNumberText());

			//The field the symbol carries shows the whole label
		DynamicElementTextItem *label = fieldBoundTo(element, QETInformation::ELMT_LABEL);
		QVERIFY2(label, "the terminal symbol has a field for its label");
		QCOMPARE(label->toPlainText(), QStringLiteral("X1:1"));
	}

	//Split, and the symbol has no field for the number: the number stays
	//with the label, on its own line, so that switching the setting on
	//can never make a number vanish
	void splitWithoutNumberFieldShowsTwoLines()
	{
		QetSettings::setTerminalLabelSplit(true);

		const QString fixture = QFINDTESTDATA("fixtures/every_feature.qet");
		QETProject project(fixture);
		QCOMPARE(project.state(), QETProject::Ok);

		Element *element = terminalWithLabel(project, QStringLiteral("X1:1"));
		QVERIFY(element);

		QCOMPARE(element->labelForDisplay(), QStringLiteral("X1\n1"));
		QCOMPARE(element->terminalNumberForDisplay(), QStringLiteral("1"));
		QCOMPARE(element->infoForDisplay(QETInformation::ELMT_TERMINAL_NUMBER),
				 QStringLiteral("1"));

		DynamicElementTextItem *label = fieldBoundTo(element, QETInformation::ELMT_LABEL);
		QVERIFY(label);
		QCOMPARE(label->toPlainText(), QStringLiteral("X1\n1"));

			//A text which uses the label as a variable follows the same
			//rule as the field bound to the label itself
		DynamicElementTextItem *composite = new DynamicElementTextItem(element);
		composite->setCompositeText(QStringLiteral("%{label}"));
		composite->setTextFrom(DynamicElementTextItem::CompositeText);
		QCOMPARE(composite->toPlainText(), QStringLiteral("X1\n1"));
	}

	//Split, and the symbol has a field of its own for the number: each
	//part in its field, the label no longer carrying the number
	void splitWithNumberFieldShowsEachInItsOwnField()
	{
		QetSettings::setTerminalLabelSplit(true);

		const QString fixture = QFINDTESTDATA("fixtures/every_feature.qet");
		QETProject project(fixture);
		QCOMPARE(project.state(), QETProject::Ok);

		Element *element = terminalWithLabel(project, QStringLiteral("X1:1"));
		QVERIFY(element);

		DynamicElementTextItem *number = addNumberField(element);
		QVERIFY(element->hasTerminalNumberText());
		QCOMPARE(number->toPlainText(), QStringLiteral("1"));

		QCOMPARE(element->labelForDisplay(), QStringLiteral("X1"));
		DynamicElementTextItem *label = fieldBoundTo(element, QETInformation::ELMT_LABEL);
		QVERIFY(label);
		QCOMPARE(label->toPlainText(), QStringLiteral("X1"));
	}

	//A label with no colon -- every label which is not a terminal's, and
	//a terminal named by hand -- is drawn as it is stored, whatever the
	//setting says
	void splitLeavesLabelsWithoutColonAlone()
	{
		QetSettings::setTerminalLabelSplit(true);

		const QString fixture = QFINDTESTDATA("fixtures/every_feature.qet");
		QETProject project(fixture);
		QCOMPARE(project.state(), QETProject::Ok);

		Element *element = terminalWithLabel(project, QStringLiteral("S7"));
		QVERIFY(element);

		QCOMPARE(element->labelForDisplay(), QStringLiteral("S7"));
		QVERIFY(element->terminalNumberForDisplay().isEmpty());

		DynamicElementTextItem *label = fieldBoundTo(element, QETInformation::ELMT_LABEL);
		QVERIFY(label);
		QCOMPARE(label->toPlainText(), QStringLiteral("S7"));
	}

	//The setting is read where it is drawn: the settings page tells the
	//drawings already open to draw it again, and the whole label comes
	//back without anything to rewrite first
	void switchingBackGivesTheWholeLabel()
	{
		const QString fixture = QFINDTESTDATA("fixtures/every_feature.qet");
		QETProject project(fixture);
		QCOMPARE(project.state(), QETProject::Ok);

		QetSettings::setTerminalLabelSplit(true);
		Element *element = terminalWithLabel(project, QStringLiteral("X1:1"));
		QVERIFY(element);
		DynamicElementTextItem *label = fieldBoundTo(element, QETInformation::ELMT_LABEL);
		QVERIFY(label);
		QCOMPARE(label->toPlainText(), QStringLiteral("X1\n1"));

		QetSettings::setTerminalLabelSplit(false);
		element->refreshLabelFields();
		QCOMPARE(label->toPlainText(), QStringLiteral("X1:1"));
		QCOMPARE(element->actualLabel(), QStringLiteral("X1:1"));
	}

	//A terminal whose name must not be drawn draws nothing of it, but
	//keeps everything: the stored label is still there, and so is every
	//use of it as data -- only the drawing changes
	void hiddenNameKeepsEverythingStored()
	{
		QetSettings::setTerminalLabelSplit(false);

		const QString fixture = QFINDTESTDATA("fixtures/every_feature.qet");
		QETProject project(fixture);
		QCOMPARE(project.state(), QETProject::Ok);

		Element *element = terminalWithLabel(project, QStringLiteral("X1:1"));
		QVERIFY(element);
		hideName(element);

		QVERIFY(!element->labelShown());
		QCOMPARE(element->actualLabel(), QStringLiteral("X1:1"));
		QCOMPARE(element->labelForDisplay(), QString());

		DynamicElementTextItem *label = fieldBoundTo(element, QETInformation::ELMT_LABEL);
		QVERIFY(label);
		QCOMPARE(label->toPlainText(), QString());
	}

	//Hidden, and the labelling is split: the terminal keeps its number,
	//drawn on its own line while the symbol has no field for it, so
	//hiding the name can never hide the number
	void hiddenNameKeepsTheNumberWithSplit()
	{
		QetSettings::setTerminalLabelSplit(true);

		const QString fixture = QFINDTESTDATA("fixtures/every_feature.qet");
		QETProject project(fixture);
		QCOMPARE(project.state(), QETProject::Ok);

		Element *element = terminalWithLabel(project, QStringLiteral("X1:1"));
		QVERIFY(element);
		hideName(element);

		QCOMPARE(element->labelForDisplay(), QStringLiteral("1"));
		QCOMPARE(element->terminalNumberForDisplay(), QStringLiteral("1"));
		QCOMPARE(element->actualLabel(), QStringLiteral("X1:1"));

		DynamicElementTextItem *label = fieldBoundTo(element, QETInformation::ELMT_LABEL);
		QVERIFY(label);
		QCOMPARE(label->toPlainText(), QStringLiteral("1"));
	}

	//Hidden, split, and the symbol has a field of its own for the
	//number: the number is drawn in its own field, the label field
	//draws nothing -- the number is never drawn twice
	void hiddenNameLeavesTheNumberToItsOwnField()
	{
		QetSettings::setTerminalLabelSplit(true);

		const QString fixture = QFINDTESTDATA("fixtures/every_feature.qet");
		QETProject project(fixture);
		QCOMPARE(project.state(), QETProject::Ok);

		Element *element = terminalWithLabel(project, QStringLiteral("X1:1"));
		QVERIFY(element);
		DynamicElementTextItem *number = addNumberField(element);
		hideName(element);

		QCOMPARE(element->labelForDisplay(), QString());
		DynamicElementTextItem *label = fieldBoundTo(element, QETInformation::ELMT_LABEL);
		QVERIFY(label);
		QCOMPARE(label->toPlainText(), QString());
		QCOMPARE(number->toPlainText(), QStringLiteral("1"));
	}

	//Only a terminal may have its name undrawn: another element which
	//carries the same information still draws its name, whatever the
	//file says
	void nonTerminalAlwaysDrawsItsName()
	{
		QetSettings::setTerminalLabelSplit(false);

		const QString fixture = QFINDTESTDATA("fixtures/every_feature.qet");
		QETProject project(fixture);
		QCOMPARE(project.state(), QETProject::Ok);

		Element *element = aNonTerminal(project);
		QVERIFY2(element, "the fixture has an element which is no terminal");
		DiagramContext dc = element->elementInformations();
		dc.addValue(QStringLiteral("label_shown"), QStringLiteral("false"));
		element->setElementInformations(dc);

			//Whatever the file says, a non-terminal draws its name:
			//the information is not obeyed, and the drawn name is the
			//stored one, composed as it always is
		QVERIFY(element->labelShown());
		QVERIFY(!element->actualLabel().isEmpty());
		QCOMPARE(element->labelForDisplay(), element->actualLabel());
	}

	//The field of a hidden name does not stay on the sheet as an empty
	//frame to select: it leaves the sheet with the box and comes back
	//with it, both ways through the same signal
	void hiddenNameHidesTheLabelField()
	{
		QetSettings::setTerminalLabelSplit(false);

		const QString fixture = QFINDTESTDATA("fixtures/every_feature.qet");
		QETProject project(fixture);
		QCOMPARE(project.state(), QETProject::Ok);

		Element *element = terminalWithLabel(project, QStringLiteral("X1:1"));
		QVERIFY(element);
		DynamicElementTextItem *label = fieldBoundTo(element, QETInformation::ELMT_LABEL);
		QVERIFY(label);
		QVERIFY(label->isVisible());

		hideName(element);
		QVERIFY(!label->isVisible());

			//The box ticked again: the element drops the information,
			//and the field is drawn as it was before
		DiagramContext dc = element->elementInformations();
		dc.remove(QStringLiteral("label_shown"));
		element->setElementInformations(dc);

		QVERIFY(label->isVisible());
		QCOMPARE(label->toPlainText(), QStringLiteral("X1:1"));
	}
};

QET_APP_TEST_MAIN(tst_terminallabeldisplay)
#include "tst_terminallabeldisplay.moc"
