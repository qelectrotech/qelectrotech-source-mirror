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
#include "dataBase/ui/elementquerywidget.h"
#include "diagram.h"
#include "ui/configpage/configpages.h"
#include "ui/elementinfopartwidget.h"
#include "ui/elementinfowidget.h"
#include "ui/multielementinfowidget.h"
#include <QGroupBox>
#include <QLabel>
#include <QListWidget>
#include <QRadioButton>
#include <QSettings>
#include "properties/elementdata.h"
#include "qetgraphicsitem/dynamicelementtextitem.h"
#include "qetgraphicsitem/element.h"
#include "qetgraphicsitem/terminal.h"
#include "qetinformation.h"
#include "qetproject.h"

/**
	How a terminal's own label is drawn, the terminal labelling of the
	project being read (@sa QETProject::terminalLabelSplit).

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

	//A non-terminal element of the fixture whose label field draws the
	//label it is given, colon and all: a terminal's own kind of drawing
	//is what the split changes, so the test needs an element which is
	//none of the things drawing a label of their own -- a report (its
	//label is where its partner is), a slave (its label follows its
	//master) or a numbered symbol (a formula composes its label)
	static Element *nonTerminalWithLabel(QETProject &project, const QString &label)
	{
		for (Diagram *diagram : project.diagrams())
			for (Element *element : diagram->elements())
			{
				if (element->elementData().m_type == ElementData::Terminal) {
					continue;
				}
				if (element->linkType() & (Element::Slave | Element::AllReport)) {
					continue;
				}
				DiagramContext dc = element->elementInformations();
				if (!dc.value(QStringLiteral("formula")).toString().isEmpty()) {
					continue;
				}
				dc.addValue(QStringLiteral("label"), label);
				element->setElementInformations(dc);
				if (element->actualLabel() != label) {
					continue;
				}
					//Only an element whose own label field draws the
					//label it stores: everything else draws a text of
					//its own, which this test would not understand
				DynamicElementTextItem *field = fieldBoundTo(element,
															 QETInformation::ELMT_LABEL);
				if (!field || field->toPlainText() != label) {
					continue;
				}
				return element;
			}
		return nullptr;
	}

private slots:
	//The shipped default: one field with the whole label, the way it has
	//always been drawn
	void oneFieldKeepsTheWholeLabel()
	{
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
		const QString fixture = QFINDTESTDATA("fixtures/every_feature.qet");
		QETProject project(fixture);
		QCOMPARE(project.state(), QETProject::Ok);
		project.setTerminalLabelSplit(true);

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
		const QString fixture = QFINDTESTDATA("fixtures/every_feature.qet");
		QETProject project(fixture);
		QCOMPARE(project.state(), QETProject::Ok);
		project.setTerminalLabelSplit(true);

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
		const QString fixture = QFINDTESTDATA("fixtures/every_feature.qet");
		QETProject project(fixture);
		QCOMPARE(project.state(), QETProject::Ok);
		project.setTerminalLabelSplit(true);

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

		project.setTerminalLabelSplit(true);
		Element *element = terminalWithLabel(project, QStringLiteral("X1:1"));
		QVERIFY(element);
		DynamicElementTextItem *label = fieldBoundTo(element, QETInformation::ELMT_LABEL);
		QVERIFY(label);
		QCOMPARE(label->toPlainText(), QStringLiteral("X1\n1"));

			//The setter itself tells the labels of the drawings already
			//open to draw again: nothing else to ask for
		project.setTerminalLabelSplit(false);
		QCOMPARE(label->toPlainText(), QStringLiteral("X1:1"));
		QCOMPARE(element->actualLabel(), QStringLiteral("X1:1"));
	}

	//A terminal whose name must not be drawn draws nothing of it, but
	//keeps everything: the stored label is still there, and so is every
	//use of it as data -- only the drawing changes
	void hiddenNameKeepsEverythingStored()
	{
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
		const QString fixture = QFINDTESTDATA("fixtures/every_feature.qet");
		QETProject project(fixture);
		QCOMPARE(project.state(), QETProject::Ok);
		project.setTerminalLabelSplit(true);

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
		const QString fixture = QFINDTESTDATA("fixtures/every_feature.qet");
		QETProject project(fixture);
		QCOMPARE(project.state(), QETProject::Ok);
		project.setTerminalLabelSplit(true);

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

	//The labelling is drawn split for an element of the terminal type
	//only: the colon in the label of any other element ("-K1:13") is
	//part of the label, not a split waiting to happen
	void splitLeavesNonTerminalsWithColonAlone()
	{
		const QString fixture = QFINDTESTDATA("fixtures/every_feature.qet");
		QETProject project(fixture);
		QCOMPARE(project.state(), QETProject::Ok);
		project.setTerminalLabelSplit(true);

		Element *element = nonTerminalWithLabel(project, QStringLiteral("-K1:13"));
		QVERIFY2(element,
				 "the fixture has an element which is no terminal and draws the label it is given");

		QCOMPARE(element->actualLabel(), QStringLiteral("-K1:13"));
		QCOMPARE(element->labelForDisplay(), QStringLiteral("-K1:13"));
		QVERIFY(element->terminalNumberForDisplay().isEmpty());

		DynamicElementTextItem *label = fieldBoundTo(element, QETInformation::ELMT_LABEL);
		QVERIFY2(label, "the symbol has a field for its label");
		QCOMPARE(label->toPlainText(), QStringLiteral("-K1:13"));
	}

	//The field of a hidden name does not stay on the sheet as an empty
	//frame to select: it leaves the sheet with the box and comes back
	//with it, both ways through the same signal
	void hiddenNameHidesTheLabelField()
	{
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

	//The nomenclature/CSV export builds its columns from the element info
	//keys, but its query has no terminal_number column: offering the key
	//there made the export fail. The key itself stays an info key, so the
	//Bauteileditor keeps offering %{terminal_number} as a text variable
	void csvExportDoesNotOfferTerminalNumber()
	{
		ElementQueryWidget widget;
		QListWidget *offer = widget.findChild<QListWidget *>(QStringLiteral("m_var_list"));
		QVERIFY2(offer, "the widget which lists the exportable columns");

		QStringList offered;
		for (int row = 0; row < offer->count(); ++row)
			offered.append(offer->item(row)->text());

		const QString terminal_number = QETInformation::translatedInfoKey(
					QETInformation::ELMT_TERMINAL_NUMBER);
		QVERIFY2(!offered.contains(terminal_number),
				 qPrintable(QStringLiteral("The export must not offer \"%1\"")
							.arg(terminal_number)));

			//The other columns are still offered...
		QVERIFY(offered.contains(
					QETInformation::translatedInfoKey(QETInformation::ELMT_LABEL)));

			//...and the variable is still a text variable
		QVERIFY(QETInformation::elementInfoKeys().contains(
					QETInformation::ELMT_TERMINAL_NUMBER));
		QCOMPARE(QETInformation::elementInfoToVar(QETInformation::ELMT_TERMINAL_NUMBER),
				 QStringLiteral("%{terminal_number}"));
	}

	//The properties of a terminal have no row for its number: the number
	//is derived from the label, so a row there would offer an edit which
	//stores a value nothing reads -- the export has no column for it, and
	//the drawing takes the number from the label
	void propertiesHaveNoRowForTheTerminalNumber()
	{
		const QString fixture = QFINDTESTDATA("fixtures/every_feature.qet");
		QETProject project(fixture);
		QCOMPARE(project.state(), QETProject::Ok);

		Element *element = terminalWithLabel(project, QStringLiteral("X1:1"));
		QVERIFY(element);

			//The properties of one element...
		ElementInfoWidget one(element);
		QStringList one_rows;
		const QList<ElementInfoPartWidget *> parts = one.findChildren<ElementInfoPartWidget *>();
		for (const ElementInfoPartWidget *part : parts)
			one_rows << part->key();
		QVERIFY2(one_rows.contains(QETInformation::ELMT_LABEL),
				 "The other properties are still offered");
		QVERIFY2(!one_rows.contains(QETInformation::ELMT_TERMINAL_NUMBER),
				 "The terminal number is no property of the element");

			//...and the properties of several elements at once
		MultiElementInfoWidget many({element});
		QStringList rows;
		const QList<QLabel *> labels = many.findChildren<QLabel *>();
		for (const QLabel *label : labels)
			rows << label->text();
		QVERIFY2(rows.contains(QETInformation::translatedInfoKey(QETInformation::ELMT_LABEL)),
				 "The other properties are still offered");
		QVERIFY2(!rows.contains(
					 QETInformation::translatedInfoKey(QETInformation::ELMT_TERMINAL_NUMBER)),
				 "The terminal number is no property of the element");
	}

	//A new project starts with the labelling the settings chose for new
	//projects (Configure QElectroTech > New project), and draws the whole
	//label in one field when the settings say nothing
	void newProjectStartsWithTheDefaultLabelling()
	{
		const QString key = QStringLiteral("diagrameditor/defaultterminallabelling");

		QSettings().setValue(key, true);
		{
			QETProject fresh;
			QVERIFY2(fresh.terminalLabelSplit(),
					 "A new project starts with the labelling the settings chose");
		}

		QSettings().setValue(key, false);
		{
			QETProject fresh;
			QVERIFY(!fresh.terminalLabelSplit());
		}

		QSettings().remove(key);
		{
			QETProject fresh;
			QVERIFY2(!fresh.terminalLabelSplit(),
					 "Without a setting in the settings, a new project draws as it always did");
		}
	}

	//The default for new projects is offered in Configure QElectroTech >
	//New project, as the same group box the project properties use, and
	//saving the page keeps the choice for the next new project
	void settingsOfferTheDefaultLabelling()
	{
		const QString key = QStringLiteral("diagrameditor/defaultterminallabelling");
		QSettings().remove(key);

		NewDiagramPage settings_page;
		QGroupBox *group = nullptr;
		const QList<QGroupBox *> groups = settings_page.findChildren<QGroupBox *>();
		for (QGroupBox *gb : groups) {
			if (gb->title() == QStringLiteral("Display of the terminal labelling")) group = gb;
		}
		QVERIFY2(group, "The settings for new projects offer the labelling");

		const QList<QRadioButton *> radios = group->findChildren<QRadioButton *>();
		QCOMPARE(radios.size(), 2);
		QRadioButton *split = nullptr;
		for (QRadioButton *rb : radios) {
			if (rb->text() == QStringLiteral("Split the label and the terminal number")) split = rb;
		}
		QVERIFY2(split, "The same choice as in the project properties");

		split->setChecked(true);
		settings_page.applyConf();
		QCOMPARE(QSettings().value(key).toBool(), true);

			//And the next new project draws the way it was saved
		QETProject fresh;
		QVERIFY(fresh.terminalLabelSplit());

		QSettings().remove(key);
	}

	//The project keeps the one place to set it: its own properties (General
	//page) are the only ones which change the drawing of a saved project,
	//so the page of the new sheets does not offer the setting again
	void projectSheetPageHasNoDefaultLabelling()
	{
		const QString fixture = QFINDTESTDATA("fixtures/every_feature.qet");
		QETProject project(fixture);
		QCOMPARE(project.state(), QETProject::Ok);

		NewDiagramPage project_page(&project);
		const QList<QGroupBox *> groups = project_page.findChildren<QGroupBox *>();
		for (const QGroupBox *gb : groups) {
			QVERIFY2(gb->title() != QStringLiteral("Display of the terminal labelling"),
					 "The labelling of a saved project is set in its own properties, "
					 "not in the defaults of its sheets");
		}
	}
};

QET_APP_TEST_MAIN(tst_terminallabeldisplay)
#include "tst_terminallabeldisplay.moc"
