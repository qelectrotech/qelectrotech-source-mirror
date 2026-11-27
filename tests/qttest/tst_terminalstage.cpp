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

#include "TerminalStrip/physicalterminal.h"
#include "TerminalStrip/realterminal.h"
#include "TerminalStrip/terminalstrip.h"
#include "diagram.h"
#include "qetgraphicsitem/element.h"
#include "qetgraphicsitem/terminalelement.h"
#include "elementprovider.h"
#include "factory/elementfactory.h"
#include "qetproject.h"
#include "TerminalStrip/terminalstripautoimport.h"
#include "TerminalStrip/UndoCommand/changeterminallevel.h"
#include "qetresult.h"
#include "ui/terminalpropertieswidget.h"

#include <QComboBox>
#include <QDomDocument>
#include <QTemporaryDir>

/**
	The stage of a terminal within its block: 0 is the one at the foot of
	the block (the PE of a multi-level one), the ones above count up. The
	stages of a block stay in their order, foot first, and a number which
	does not continue the block before it starts a new one -- the rule
	the "Stage" combo of a terminal follows.

	The fixture has the strip X1 with two terminals, each of its own.
*/
class tst_terminalstage : public QObject
{
	Q_OBJECT

	QETProject *m_project = nullptr;
	QTemporaryDir m_dir;

	QVector<QSharedPointer<RealTerminal>> reals() const
	{
		const auto strips = m_project->terminalStrip();
		return strips.isEmpty() ? QVector<QSharedPointer<RealTerminal>>()
					: strips.first()->realTerminals();
	}

		///Set the form and the stage of @p terminal the way the
		///"Terminal" page does it, and push the change
	void setFormAndStage(Element *element, ElementData::TerminalForm form, int stage)
	{
		TerminalPropertiesWidget widget(element);
		auto *form_cb = widget.findChild<QComboBox *>(QStringLiteral("m_form_cb"));
		auto *stage_cb = widget.findChild<QComboBox *>(QStringLiteral("m_stage_cb"));
		QVERIFY(form_cb);
		QVERIFY(stage_cb);
		form_cb->setCurrentIndex(form_cb->findData(form));
		stage_cb->setCurrentIndex(stage_cb->findData(stage));
		if (QUndoCommand *undo = widget.associatedUndo()) {
			element->diagram()->undoStack().push(undo);
		}
	}

	private slots:
		void initTestCase();
		void init();
		void cleanup();
		void aTerminalWithoutAStageKeepsTheNumberOfItsPosition();
		void theStagesOfABlockLieFootFirst();
		void aStageSurvivesASaveAndAReload();
		void twoStagesWhichContinueMakeOneBlock();
	void aStageOnATerminalWithoutAStripBringsItIntoItsStrip();
		void aNumberWhichDoesNotContinueStartsANewBlock();
	void theStageOfASymbolSurvivesACopy();
	void theImportGroupsTheBlocksTheStagesName();
		void cleanupTestCase();
};

void tst_terminalstage::initTestCase()
{
}

void tst_terminalstage::init()
{
	QVERIFY(m_dir.isValid());
	const QString fixture = QFINDTESTDATA("fixtures/every_feature.qet");
	QVERIFY(!fixture.isEmpty());
	const QString copy = m_dir.filePath(QStringLiteral("every_feature.qet"));
	QFile::remove(copy);
	QVERIFY(QFile::copy(fixture, copy));

	m_project = new QETProject(copy);
	QCOMPARE(m_project->state(), QETProject::Ok);
	QVERIFY(reals().size() >= 2);
}

void tst_terminalstage::cleanup()
{
	delete m_project;
	m_project = nullptr;
}

/**
	A terminal which was given no stage keeps the number of its
	position in its block.
*/
void tst_terminalstage::aTerminalWithoutAStageKeepsTheNumberOfItsPosition()
{
	const auto terminal = reals().first();
	QCOMPARE(terminal->stage(), terminal->level());
}

/**
	The stages of a block lie in their order, foot first, whatever the
	order they were given in.
*/
void tst_terminalstage::theStagesOfABlockLieFootFirst()
{
	const auto terminals = reals();
	auto strip = m_project->terminalStrip().first();
	QVERIFY(strip->groupTerminals(terminals.at(0)->physicalTerminal(),
						  {terminals.at(1)}));

	QVERIFY(strip->setLevel(terminals.at(0), 2));
	QVERIFY(strip->setLevel(terminals.at(1), 1));
	QCOMPARE(terminals.at(0)->stage(), 2);
	QCOMPARE(terminals.at(1)->stage(), 1);

		//Foot first: the stage 1 comes before the stage 2
	const auto block = terminals.at(0)->physicalTerminal()->realTerminals();
	QCOMPARE(block.size(), 2);
	QCOMPARE(block.at(0)->stage(), 1);
	QCOMPARE(block.at(1)->stage(), 2);
}

/**
	The stage of a terminal is saved with the strip and read back.
*/
void tst_terminalstage::aStageSurvivesASaveAndAReload()
{
	const auto terminals = reals();
	auto strip = m_project->terminalStrip().first();
	QVERIFY(strip->groupTerminals(terminals.at(0)->physicalTerminal(),
						  {terminals.at(1)}));
	QVERIFY(strip->setLevel(terminals.at(1), 1));

	QVERIFY(m_project->write().isOk());
	QETProject reloaded(m_dir.filePath(QStringLiteral("every_feature.qet")));
	QCOMPARE(reloaded.state(), QETProject::Ok);

	const auto strips = reloaded.terminalStrip();
	QCOMPARE(strips.size(), 1);
	const auto reloaded_reals = strips.first()->realTerminals();
	QCOMPARE(reloaded_reals.size(), 2);
	QCOMPARE(reloaded_reals.at(1)->stage(), 1);
	QCOMPARE(reloaded_reals.at(0)->stage(), reloaded_reals.at(0)->level());
}

/**
	Two terminals whose stages continue one another (2 then 1) are one
	block, the way the "Stage" combo of the terminal page works it out.
*/
void tst_terminalstage::twoStagesWhichContinueMakeOneBlock()
{
	const auto terminals = reals();
	setFormAndStage(terminals.at(0)->element(), ElementData::TFormMultiLevel, 2);
	setFormAndStage(terminals.at(1)->element(), ElementData::TFormMultiLevel, 1);

	QCOMPARE(terminals.at(0)->stage(), 2);
	QCOMPARE(terminals.at(1)->stage(), 1);
	QCOMPARE(terminals.at(0)->physicalTerminal(), terminals.at(1)->physicalTerminal());

		//Foot first: the stage 1 comes before the stage 2
	const auto block = terminals.at(0)->physicalTerminal()->realTerminals();
	QCOMPARE(block.size(), 2);
	QCOMPARE(block.at(0)->stage(), 1);
	QCOMPARE(block.at(1)->stage(), 2);
}

/**
	A number which does not continue the block before it (two times the
	same stage) starts a new one.
*/
void tst_terminalstage::aNumberWhichDoesNotContinueStartsANewBlock()
{
	const auto terminals = reals();
	setFormAndStage(terminals.at(0)->element(), ElementData::TFormMultiLevel, 2);
	setFormAndStage(terminals.at(1)->element(), ElementData::TFormMultiLevel, 2);

	QVERIFY(terminals.at(0)->physicalTerminal() != terminals.at(1)->physicalTerminal());
	QCOMPARE(terminals.at(0)->stage(), 2);
	QCOMPARE(terminals.at(1)->stage(), 2);
}

/**
	A terminal which is on no strip yet -- a symbol dragged onto the
	schematic and adjusted, with the manager never opened: the stage has
	to stick on such a terminal either way. The reported bug was that it
	came back as 0 as soon as the terminal was picked again.
*/
void tst_terminalstage::aStageOnATerminalWithoutAStripBringsItIntoItsStrip()
{
		//A fixture of terminals on folios, with no strip at all
	const QString fixture = QFINDTESTDATA("fixtures/strip_labels.qet");
	QVERIFY(!fixture.isEmpty());
	QETProject project(fixture);
	QCOMPARE(project.state(), QETProject::Ok);

	const ElementProvider provider(&project);
	const auto free_terminals = provider.freeTerminal();
	QVERIFY2(!free_terminals.isEmpty(), "the fixture has no terminal");
	auto *free_terminal = free_terminals.first();
	QVERIFY(free_terminal->realTerminal());
	QVERIFY(!free_terminal->parentTerminalStrip());

	setFormAndStage(free_terminal, ElementData::TFormMultiLevel, 2);

		//The stage is kept on the terminal, with or without a strip
	QCOMPARE(free_terminal->realTerminal()->stage(), 2);
		//With a label which names a strip, the terminal lands there
	const auto name = TerminalStripAutoImport::stripNameForLabel(
		free_terminal->realTerminal()->label());
	qInfo() << "label:" << free_terminal->realTerminal()->label() << "strip:" << name
			<< "in a strip:" << bool(free_terminal->parentTerminalStrip());
	if (!name.isEmpty()) {
		QVERIFY(free_terminal->parentTerminalStrip());
	}
}

/**
	The stage of a terminal is kept with its symbol: a copy of a symbol
	is on no strip, and has to come back with the stage it was given --
	with every other choice of the Terminal page in the same XML.
*/
void tst_terminalstage::theStageOfASymbolSurvivesACopy()
{
	const auto terminals = reals();
	auto strip = m_project->terminalStrip().first();
	QVERIFY(strip->setLevel(terminals.first(), 2));

	auto *element = terminals.first()->element();
	auto data = element->elementData();
	data.setTerminalForm(ElementData::TFormMultiLevel);
	element->setElementData(data);

		//The XML of the symbol, the way a copy puts it on the clipboard
	QDomDocument document;
	QHash<Terminal *, int> table_id;
	QDomElement xml = element->toXml(document, table_id);

		//A paste reads it into a new symbol of the same definition
	int state = 0;
	Element *copy = ElementFactory::Instance()->createElement(
		element->location(), nullptr, &state);
	QVERIFY2(copy, "a copy of the same symbol could not be made");
	QHash<int, Terminal *> table_adr;
	QVERIFY(copy->fromXml(xml, table_adr));

	auto *copy_terminal = qobject_cast<TerminalElement *>(copy);
	QVERIFY(copy_terminal);
	QCOMPARE(copy_terminal->realTerminal()->stage(), 2);
	QCOMPARE(copy->elementData().terminalForm(), ElementData::TFormMultiLevel);
	QCOMPARE(copy->elementData().terminalType(), element->elementData().terminalType());

	delete copy;
}

/**
	The action behind the "Re-group all terminals" button joins the
	terminals whose stages continue one another into one block -- the
	automatic recognition for terminals which were given their stages
	before they were in a strip together.
*/
void tst_terminalstage::theImportGroupsTheBlocksTheStagesName()
{
	const auto terminals = reals();
	QVERIFY(terminals.size() >= 2);
	auto strip = m_project->terminalStrip().first();

		//Both terminals are multi-level, each of its own block, with
		//stages which continue one another (2 then 1)
	for (const auto &real_ : terminals) {
		auto data = real_->element()->elementData();
		data.setTerminalForm(ElementData::TFormMultiLevel);
		real_->element()->setElementData(data);
	}
	QVERIFY(strip->setLevel(terminals.at(0), 2));
	QVERIFY(strip->setLevel(terminals.at(1), 1));
	QVERIFY(terminals.at(0)->physicalTerminal() != terminals.at(1)->physicalTerminal());

	TerminalStripAutoImport::importFreeTerminals(m_project, false);

		//One block now, foot first
	QCOMPARE(terminals.at(0)->physicalTerminal(), terminals.at(1)->physicalTerminal());
	const auto block = terminals.at(0)->physicalTerminal()->realTerminals();
	QCOMPARE(block.size(), 2);
	QCOMPARE(block.at(0)->stage(), 1);
	QCOMPARE(block.at(1)->stage(), 2);

		//Running it again changes nothing
	TerminalStripAutoImport::importFreeTerminals(m_project, false);
	QCOMPARE(terminals.at(0)->physicalTerminal()->realTerminals().size(), 2);
}

void tst_terminalstage::cleanupTestCase()
{
}

QET_APP_TEST_MAIN(tst_terminalstage)
#include "tst_terminalstage.moc"
