// SPDX-License-Identifier: GPL-2.0-or-later
// A contact draws the BMK of its master, a PLC contact as well. The
// master's label comes from its numbering formula, and that formula holds
// the page number, so moving the sheet to another place changes the label:
// the contact must show the new one at once, whether it is a coil's
// contact or a PLC one.
//
// A PLC contact stores a copy of the master's label for its %{plc_*}
// information. Nothing writes that copy when only the folio numbering
// changes, so the label it draws has to come from the master itself.
#include "ElementsCollection/elementslocation.h"
#include "ElementsCollection/xmlelementcollection.h"
#include "autoNum/elementautonumschemecommand.h"
#include "autoNum/numerotationcontext.h"
#include "factory/elementfactory.h"
#include "properties/elementdata.h"
#include "qetapptest.h"
#include "qetgraphicsitem/dynamicelementtextitem.h"
#include "qetgraphicsitem/element.h"
#include "qetinformation.h"
#include "qetmessagebox.h"
#include "qetproject.h"
#include "undocommand/linkelementcommand.h"

#include <QFile>
#include <QTemporaryDir>

class tst_plcslavelabel : public QObject
{
	Q_OBJECT
	QTemporaryDir m_definitions;
	int			  m_position = 0;

	// The symbol of @p kind, written once and put in the project's own
	// embedded collection, the way the editor's symbols are built.
	Element* symbol(Diagram* diagram, const QString& kind)
	{
		const QString path = m_definitions.filePath(kind + ".elmt");
		if (! QFile::exists(path))
		{
			QFile file(path);
			if (! file.open(QIODevice::WriteOnly)) return nullptr;
			file.write(definitionXml(kind).toUtf8());
		}
		auto* collection = diagram->project()->embeddedElementCollection();
		if (! collection->exist("import/" + kind + ".elmt"))
		{
			QFile			file(path);
			QDomDocument	definition;
			if (! file.open(QIODevice::ReadOnly)
				|| ! definition.setContent(&file)) return nullptr;
			if (! collection->addElementDefinition(
					"import", kind, definition.documentElement()))
				return nullptr;
		}
		int		 state = 0;
		Element* element = ElementFactory::Instance()->createElement(
			ElementsLocation(
				"embed://import/" + kind + ".elmt", diagram->project()),
			nullptr,
			&state);
		if (state || ! element) return nullptr;
		diagram->addItem(element);
		element->setPos(120 + 160 * m_position++, 240);
		return element;
	}

	// plc_master and plc_slave are a PLC master with one I/O and its
	// contact; coil_master and coil_slave are a coil and its contact.
	static QString definitionXml(const QString& kind)
	{
		const bool master = kind.endsWith(QLatin1String("master"));
		const bool plc	   = kind.startsWith(QLatin1String("plc"));
		return QStringLiteral(
				   R"(<definition type="element" link_type="%1" uuid="%2" width="40" height="60"
hotspot_x="20" hotspot_y="30">
<names><name lang="en">%3</name></names>
<kindInformations>
<kindInformation name="type">%4</kindInformation>
%5
</kindInformations>
<description>
<line x1="-20" y1="-30" x2="20" y2="-30"/>
<dynamic_text x="0" y="-46" z="7" text_width="-1" Halignment="AlignLeft"
Valignment="AlignTop" frame="false" rotation="0" keep_visual_rotation="false"
text_from="ElementInfo" uuid="%6" font="Liberation Sans,9,-1,5,50,0,0,0,0,0,Regular">
<info_name>label</info_name>
</dynamic_text>
<dynamic_text x="0" y="-64" z="8" text_width="-1" Halignment="AlignLeft"
Valignment="AlignTop" frame="false" rotation="0" keep_visual_rotation="false"
text_from="CompositeText" uuid="%8" font="Liberation Sans,9,-1,5,50,0,0,0,0,0,Regular">
<composite_text>%{label}</composite_text>
</dynamic_text>
<terminal x="0" y="30" orientation="s" uuid="%7"/>
</description>
</definition>)")
			.arg(
				master ? QStringLiteral("master") : QStringLiteral("slave"),
				QUuid::createUuid().toString(),
				kind,
				master ? (plc ? QStringLiteral("plc") : QStringLiteral("coil"))
					   : (plc ? QStringLiteral("plc") : QStringLiteral("simple")),
				master && plc
					? QStringLiteral(R"(<plcMasterData>
<plcIOs>
<plcIO address="1.0" functionText="Funktion 1" terminalCount="1"/>
</plcIOs>
</plcMasterData>)")
					: QString(),
				QUuid::createUuid().toString(),
				QUuid::createUuid().toString(),
				QUuid::createUuid().toString());
	}

	// The label field of @p element, the one the sheet draws.
	static DynamicElementTextItem* labelText(Element* element)
	{
		for (DynamicElementTextItem* text : element->dynamicTextItems())
			if (text->textFrom() == DynamicElementTextItem::ElementInfo
				&& text->infoName() == QLatin1String("label"))
				return text;
		return nullptr;
	}

	// What the label field of @p element draws on the sheet.
	static QString shownLabel(Element* element)
	{
		DynamicElementTextItem* text = labelText(element);
		return text ? text->toPlainText() : QString();
	}

	// What the composite text of @p element draws: the one using %{label},
	// which reads its own element's information -- and so, for a contact,
	// has to reach the master for the label (@sa labelSource()).
	static QString shownCompositeLabel(Element* element)
	{
		for (DynamicElementTextItem* text : element->dynamicTextItems())
			if (text->textFrom() == DynamicElementTextItem::CompositeText
				&& text->compositeText().contains(QLatin1String("%{label}")))
				return text->toPlainText();
		return QString();
	}

	static void settle()
	{
		QApplication::processEvents();
		QApplication::processEvents();
	}

	// Links @p contact to @p master, gives @p master a numbering whose label
	// carries the page number and checks both show it; then the page
	// numbering moves on -- @p insert_first puts a sheet before the others,
	// otherwise a sheet is moved to another place -- and both must show the
	// new label.
	void pageChangeKeepsContactOnMaster(
		QETProject& project,
		Diagram*	folio,
		Element*	master,
		Element*	contact,
		const QString& title,
		bool			insert_first)
	{
		auto* link = new LinkElementCommand(contact);
		link->setLink(master);
		link->setGroupIndex(0);
		project.undoStack()->push(link);
		settle();
		QVERIFY(contact->linkedElements().contains(master));
		QCOMPARE(master->groupIndexForElement(contact), 0);

		NumerotationContext nc;
		nc.addValue(QStringLiteral("string"), QStringLiteral("-K"));
		nc.addValue(QStringLiteral("idfolio"));
		auto* create = ElementAutoNumSchemeCommand::create(
			&project, title, nc);
		QVERIFY(create);
		project.undoStack()->push(create);
		auto* assign = ElementAutoNumSchemeCommand::assign(
			&project, title, {master}, false);
		QVERIFY(assign);
		project.undoStack()->push(assign);
		settle();

		QCOMPARE(master->actualLabel(), QStringLiteral("-K1"));
		QCOMPARE(shownLabel(master), QStringLiteral("-K1"));
		QCOMPARE(shownLabel(contact), QStringLiteral("-K1"));
		QCOMPARE(shownCompositeLabel(contact), QStringLiteral("-K1"));

		if (insert_first)
		{
			Diagram* added = project.addNewDiagram(0);
			QVERIFY(added);
			settle();
			QCOMPARE(project.diagrams().size(), 2);
			QCOMPARE(project.folioIndex(folio), 1);
		}
		else
		{
			Diagram* added = project.addNewDiagram();
			QVERIFY(added);
			QCOMPARE(project.diagrams().size(), 2);
			QCOMPARE(project.folioIndex(folio), 0);
			project.diagramOrderChanged(1, 0);
			settle();
			QCOMPARE(project.folioIndex(folio), 1);
		}

		QCOMPARE(master->actualLabel(), QStringLiteral("-K2"));
		QCOMPARE(shownLabel(master), QStringLiteral("-K2"));
		QVERIFY2(
			shownLabel(contact) == QStringLiteral("-K2"),
			"the contact must show its master's new label");
		QVERIFY2(
			shownCompositeLabel(contact) == QStringLiteral("-K2"),
			"the composite %{label} of the contact must show its master's new label");
	}

  private slots:
	void initTestCase()
	{
		QVERIFY(m_definitions.isValid());
		QETProject::setBackupEnabled(false);
		QET::QetMessageBox::setNonInteractive(true);
	}

	void init() { m_position = 0; }

	void plcContactFollowsItsMasterOverAFolioChange()
	{
		QETProject project;
		Diagram*	folio = project.addNewDiagram();
		QVERIFY(folio);

		Element* master  = symbol(folio, "plc_master");
		Element* contact = symbol(folio, "plc_slave");
		QVERIFY(master);
		QVERIFY(contact);
		QCOMPARE(master->linkType(), Element::Master);
		QCOMPARE(contact->linkType(), Element::Slave);
		QCOMPARE(master->elementData().m_master_type, ElementData::PLC);
		QCOMPARE(contact->elementData().m_slave_type, ElementData::PLCSlave);
		QCOMPARE(master->elementData().plcMasterData().ios.size(), 1);

		pageChangeKeepsContactOnMaster(
			project, folio, master, contact, "PLC page numbering", false);

		// The copy the contact stores is not what it draws; it is written
		// when the master's label is written, not by a folio renumbering
		qInfo("the contact's stored label is %s",
			  qPrintable(contact->elementInformations()
							 .value(QETInformation::ELMT_LABEL)
							 .toString()));
	}

	// A sheet inserted before the others moves every sheet after it one
	// number up: the labels carrying a page number must follow, the drawn
	// ones as much as the calculated value behind them.
	void sheetInsertedKeepsContactOnMaster()
	{
		QETProject project;
		Diagram*	folio = project.addNewDiagram();
		QVERIFY(folio);

		Element* master  = symbol(folio, "plc_master");
		Element* contact = symbol(folio, "plc_slave");
		QVERIFY(master);
		QVERIFY(contact);

		pageChangeKeepsContactOnMaster(
			project, folio, master, contact, "PLC page numbering", true);
	}

	void coilContactFollowsItsMasterOverAFolioChange()
	{
		QETProject project;
		Diagram*	folio = project.addNewDiagram();
		QVERIFY(folio);

		Element* master  = symbol(folio, "coil_master");
		Element* contact = symbol(folio, "coil_slave");
		QVERIFY(master);
		QVERIFY(contact);
		QCOMPARE(master->elementData().m_master_type, ElementData::Coil);
		QCOMPARE(contact->elementData().m_slave_type, ElementData::SSimple);

		pageChangeKeepsContactOnMaster(
			project, folio, master, contact, "Coil page numbering", false);
	}

	// A text which becomes an element info field afterwards must watch the
	// master it draws the label from: the master's label moves on with the
	// numbering given again, and nothing writes the copy the contact keeps
	// then, so only that watch redraws the contact's text.
	void contactTextSwitchedToElementInfoFollowsItsMaster()
	{
		QETProject project;
		Diagram*	folio = project.addNewDiagram();
		QVERIFY(folio);

		Element* master  = symbol(folio, "plc_master");
		Element* contact = symbol(folio, "plc_slave");
		QVERIFY(master);
		QVERIFY(contact);
		DynamicElementTextItem* text = labelText(contact);
		QVERIFY(text);

			//The text is a user text while the link and the numbering
			//happen, as a text the user switches to element info later is
		text->setTextFrom(DynamicElementTextItem::UserText);
		settle();

		auto* link = new LinkElementCommand(contact);
		link->setLink(master);
		link->setGroupIndex(0);
		project.undoStack()->push(link);
		settle();
		QVERIFY(contact->linkedElements().contains(master));

			//A numbering with a counter, so that numbering the master again
			//gives it another label while the page stays the one
		NumerotationContext nc;
		nc.addValue(QStringLiteral("string"), QStringLiteral("-K"));
		nc.addValue(QStringLiteral("idfolio"));
		nc.addValue(QStringLiteral("string"), QStringLiteral("-"));
		nc.addValue(QStringLiteral("unit"));
		auto* create = ElementAutoNumSchemeCommand::create(
			&project, "Counter numbering", nc);
		QVERIFY(create);
		project.undoStack()->push(create);
		auto* assign = ElementAutoNumSchemeCommand::assign(
			&project, "Counter numbering", {master}, false);
		QVERIFY(assign);
		project.undoStack()->push(assign);
		settle();
		QCOMPARE(master->actualLabel(), QStringLiteral("-K1-1"));

		text->setTextFrom(DynamicElementTextItem::ElementInfo);
		settle();
		QCOMPARE(shownLabel(contact), QStringLiteral("-K1-1"));

			//The master is numbered again: its label moves on, and the copy
			//the contact stores is left where it was
		master->setUpFormula(true);
		settle();
		QCOMPARE(master->actualLabel(), QStringLiteral("-K1-2"));
		QVERIFY2(
			shownLabel(contact) == QStringLiteral("-K1-2"),
			"the contact's text must follow its master directly");
	}
};

QET_APP_TEST_MAIN(tst_plcslavelabel)
#include "tst_plcslavelabel.moc"
