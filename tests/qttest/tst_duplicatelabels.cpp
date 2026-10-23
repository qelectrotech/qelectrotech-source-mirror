// SPDX-License-Identifier: GPL-2.0-or-later
#include "ElementsCollection/elementscollectionwidget.h"
#include "ElementsCollection/elementslocation.h"
#include "ElementsCollection/xmlelementcollection.h"
#include "TerminalStrip/realterminal.h"
#include "TerminalStrip/terminalstrip.h"
#include "duplicatelabels.h"
#include "elementspanel.h"
#include "elementspanelwidget.h"
#include "factory/elementfactory.h"
#include "projectview.h"
#include "qetapptest.h"
#include "qetgraphicsitem/conductor.h"
#include "qetgraphicsitem/element.h"
#include "qetgraphicsitem/terminal.h"
#include "qetgraphicsitem/terminalelement.h"
#include "qetmessagebox.h"
#include "qetproject.h"
#include "ui/duplicatelabelsdialog.h"
#include "undocommand/removediagramcommand.h"
#include "utils/qetsettings.h"

#include <QCheckBox>
#include <QFile>
#include <QLabel>
#include <QMenu>
#include <QMessageBox>
#include <QPointer>
#include <QProgressDialog>
#include <QPushButton>
#include <QScopeGuard>
#include <QTimer>
#include <QTranslator>
#include <QTreeView>
#include <QTreeWidget>

class tst_duplicatelabels : public QObject
{
	Q_OBJECT
	QTemporaryDir m_definitions;
	int			  m_position = 0;

	Element* symbol(
		Diagram*	   diagram,
		const QString& kind	 = "simple",
		const QString& label = {})
	{
		const QString path = m_definitions.filePath(kind + ".elmt");
		if (! QFile::exists(path))
		{
			QFile file(path);
			if (! file.open(QIODevice::WriteOnly)) return nullptr;
			file.write(
				QString(
					R"(<definition type="element" link_type="%1" uuid="%2" width="20" height="40" hotspot_x="10" hotspot_y="20">
<names><name lang="en">Test symbol</name></names>
<kindInformations><kindInformation name="type">%5</kindInformation></kindInformations>
<description><line x1="0" y1="-10" x2="0" y2="10"/>
<terminal x="0" y="-20" orientation="n" uuid="%3"/><terminal x="0" y="20" orientation="s" uuid="%4"/>
</description></definition>)")
					.arg(
						kind,
						QUuid::createUuid().toString(),
						QUuid::createUuid().toString(),
						QUuid::createUuid().toString(),
						kind == "slave"	   ? "simple" :
						kind == "terminal" ? "generic" :
											 "coil")
					.toUtf8());
		}
		auto* collection = diagram->project()->embeddedElementCollection();
		if (! collection->exist("import/" + kind + ".elmt"))
		{
			QFile		 file(path);
			QDomDocument definition;
			if (! file.open(QIODevice::ReadOnly)
				|| ! definition.setContent(&file))
				return nullptr;
			if (! collection->addElementDefinition(
					"import",
					kind,
					definition.documentElement()))
				return nullptr;
		}
		int		 state	 = 0;
		Element* element = ElementFactory::Instance()->createElement(
			ElementsLocation(
				"embed://import/" + kind + ".elmt",
				diagram->project()),
			nullptr,
			&state);
		if (state || ! element) return nullptr;
		diagram->addItem(element);
		const int position = m_position++;
		element->setPos(100 + 80 * (position % 8), 150 + 100 * (position / 8));
		setLabel(element, label);
		return element;
	}

	static void setLabel(Element* element, const QString& label)
	{
		auto info = element->elementInformations();
		info.addValue(QStringLiteral("label"), label);
		info.remove(QStringLiteral("formula"));
		element->setElementInformations(info);
	}

	static void exclude(Element* element, const QString& flag)
	{
		auto info = element->elementInformations();
		info.addValue(QStringLiteral("exclude_from_bom"), flag);
		element->setElementInformations(info);
	}

	static Conductor* wire(Element* a, Element* b, const QString& label)
	{
		auto* conductor =
			new Conductor(a->terminals().last(), b->terminals().first());
		a->diagram()->addItem(conductor);
		auto properties = conductor->properties();
		properties.text = label;
		conductor->setProperties(properties);
		return conductor;
	}

	Conductor* isolated(Diagram* diagram, const QString& label)
	{
		return wire(symbol(diagram), symbol(diagram), label);
	}

	static void
	setPlace(Diagram* diagram, const QString& plant, const QString& location)
	{
		auto block	  = diagram->border_and_titleblock.exportTitleBlock();
		block.plant	  = plant;
		block.locmach = location;
		diagram->border_and_titleblock.importTitleBlock(block);
	}

	// Close @p project from the editor as the user does, discarding its
	// changes if asked, and wait until it is deleted.
	static bool closeInEditor(QETProject* project)
	{
		if (! project) return true;
		QPointer<QETProject> closing(project);
		QTimer				 discard;
		QObject::connect(
			&discard,
			&QTimer::timeout,
			[]()
			{
				auto* box = qobject_cast<QMessageBox*>(
					QApplication::activeModalWidget());
				if (auto* button =
						box ? box->button(QMessageBox::Discard) : nullptr)
					button->click();
			});
		discard.start(20);
		QetAppTest::editor()->closeProject(project);
		discard.stop();
		return QTest::qWaitFor([&closing]() { return closing.isNull(); });
	}

	// The editor's element collection, loaded on worker threads 250 ms after
	// the editor is first activated, has finished loading.
	static bool collectionLoaded()
	{
		auto* collection =
			QetAppTest::editor()->findChild<ElementsCollectionWidget*>();
		if (! collection) return false;
		for (QTreeView* view : collection->findChildren<QTreeView*>())
			if (! view->model() || ! view->isEnabled()) return false;
		return true;
	}

  private slots:
	void initTestCase()
	{
		QVERIFY(m_definitions.isValid());
		QETProject::setBackupEnabled(false);
		QET::QetMessageBox::setNonInteractive(true);
		// What the app does 250 ms after the project tree is first shown.
		// Until then the tree holds no project: the editor would select
		// items of the projects these cases open outside of it, and
		// closing a project would leave the tree pointing at freed items.
		auto* panel = QetAppTest::editor()->findChild<ElementsPanelWidget*>();
		QVERIFY(panel);
		panel->elementsPanel().reload();
		// Activated now, the editor starts loading its element collection
		// before the first case rather than during any of them.
		QetAppTest::editor()->activateWindow();
		QVERIFY(QTest::qWaitForWindowActive(QetAppTest::editor()));
		QETProject project;
		auto*	   diagram = project.addNewDiagram();
		for (const QString& kind :
			 {"simple",
			  "master",
			  "slave",
			  "terminal",
			  "next_report",
			  "previous_report"})
		{
			auto* element = symbol(diagram, kind);
			QVERIFY(element);
			QCOMPARE(element->terminals().size(), 2);
		}
	}

	void init() { m_position = 0; }

	// The editor ends without the projects these cases opened in it
	// (QetAppTest::diagram()), as the user leaves it before quitting, and
	// with its element collection loaded: worker threads still loading it
	// when the application ends would use it after it is gone.
	void cleanupTestCase()
	{
		for (ProjectView* view : QetAppTest::editor()->openedProjects())
			QVERIFY(closeInEditor(view->project()));
		QVERIFY(QetAppTest::editor()->openedProjects().isEmpty());
		QVERIFY(QTest::qWaitFor(collectionLoaded, 300000));
	}

	void isolatedSignalsAcrossSheets()
	{
		QETProject project;
		auto*	   first  = project.addNewDiagram();
		auto*	   second = project.addNewDiagram();
		auto*	   a	  = isolated(first, "W1");
		auto*	   b	  = isolated(second, "W1");
		const auto groups = DuplicateLabels::find(&project);
		QCOMPARE(groups.size(), 1);
		QCOMPARE(groups.first().kind, DuplicateLabels::Kind::Signal);
		QCOMPARE(groups.first().entity_count, 2);
		QCOMPARE(groups.first().occurrences.size(), 2);
		QCOMPARE(groups.first().occurrences.first().item.data(), a);
		QCOMPARE(groups.first().occurrences.last().item.data(), b);
	}

	void reportLinksAreOneSignal()
	{
		QETProject project;
		auto*	   first  = project.addNewDiagram();
		auto*	   second = project.addNewDiagram();
		auto*	   out	  = symbol(first, "next_report");
		auto*	   in	  = symbol(second, "previous_report");
		wire(symbol(first), out, "24V");
		wire(in, symbol(second), "24V");
		out->linkToElement(in);
		QVERIFY(! out->isFree());
		QVERIFY(DuplicateLabels::find(&project).isEmpty());
		isolated(second, "24V");
		auto groups = DuplicateLabels::find(&project);
		QCOMPARE(groups.size(), 1);
		QCOMPARE(groups.first().entity_count, 2);
		QCOMPARE(groups.first().occurrences.size(), 3);
		out->unlinkAllElements();
		groups = DuplicateLabels::find(&project);
		QCOMPARE(groups.first().entity_count, 3);
	}

	void connectedSegmentsAndSeparatedTerminals()
	{
		QETProject project;
		auto*	   diagram	= project.addNewDiagram();
		auto*	   terminal = symbol(diagram, "terminal");
		wire(symbol(diagram), terminal, "W7");
		wire(terminal, symbol(diagram), "W7");
		QVERIFY(DuplicateLabels::find(&project).isEmpty());
		auto info = terminal->elementInformations();
		info.addValue(
			QStringLiteral("potential_isolating"),
			QStringLiteral("true"));
		terminal->setElementInformations(info);
		const auto groups = DuplicateLabels::find(&project);
		QCOMPARE(groups.size(), 1);
		QCOMPARE(groups.first().entity_count, 2);
	}

	void deviceRelationsIncludingExcludedMaster()
	{
		QETProject project;
		auto*	   first  = project.addNewDiagram();
		auto*	   second = project.addNewDiagram();
		auto*	   master = symbol(first, "master", "K1");
		auto*	   a	  = symbol(first, "slave", "K1");
		auto*	   b	  = symbol(second, "slave", "K1");
		master->linkToElement(a);
		master->linkToElement(b);
		QCOMPARE(master->linkedElements().size(), 2);
		QVERIFY(DuplicateLabels::find(&project).isEmpty());
		auto* other	 = symbol(second, "master", "K1");
		auto  groups = DuplicateLabels::find(&project);
		QCOMPARE(groups.size(), 1);
		QCOMPARE(groups.first().kind, DuplicateLabels::Kind::Component);
		QCOMPARE(groups.first().entity_count, 2);
		QCOMPARE(groups.first().occurrences.size(), 4);
		exclude(master, "true");
		groups = DuplicateLabels::find(&project);
		QCOMPARE(groups.first().entity_count, 2);
		QCOMPARE(groups.first().occurrences.size(), 3);
		exclude(other, "true");
		QVERIFY(DuplicateLabels::find(&project).isEmpty());
	}

	void componentsClashOnlyInOnePlace()
	{
		QETProject project;
		auto*	   first  = project.addNewDiagram();
		auto*	   second = project.addNewDiagram();
		setPlace(first, {}, "UV1");
		setPlace(second, {}, "UV2");
		auto* a = symbol(first, "simple", "F1");
		auto* b = symbol(second, "simple", "F1");
		QCOMPARE(DuplicateLabels::place(a), QString("+UV1"));
		QVERIFY(DuplicateLabels::find(&project).isEmpty());
		// The element's own location comes before its folio's.
		auto info = b->elementInformations();
		info.addValue(QStringLiteral("location"), QStringLiteral("UV1"));
		b->setElementInformations(info);
		auto groups = DuplicateLabels::find(&project);
		QCOMPARE(groups.size(), 1);
		QCOMPARE(groups.first().place, QString("+UV1"));
		// A plant is not a location of the same name.
		setPlace(second, "UV1", {});
		info.remove(QStringLiteral("location"));
		b->setElementInformations(info);
		QCOMPARE(DuplicateLabels::place(b), QString("=UV1"));
		QVERIFY(DuplicateLabels::find(&project).isEmpty());
		setPlace(second, {}, "UV2");
		// A contact drawn on another folio is part of its master's device,
		// in the master's place.
		auto* master  = symbol(second, "master", "K1");
		auto* contact = symbol(first, "slave", "K1");
		master->linkToElement(contact);
		symbol(first, "simple", "K1");
		QVERIFY(DuplicateLabels::find(&project).isEmpty());
		symbol(second, "simple", "K1");
		groups = DuplicateLabels::find(&project);
		QCOMPARE(groups.size(), 1);
		QCOMPARE(groups.first().place, QString("+UV2"));
		QCOMPARE(groups.first().entity_count, 2);
		QCOMPARE(groups.first().occurrences.size(), 3);
		DuplicateLabelsDialog dialog(&project);
		auto* tree = dialog.findChild<QTreeWidget*>("duplicateOccurrences");
		QCOMPARE(
			tree->topLevelItem(0)->text(0),
			QCoreApplication::translate(
				"DuplicateLabelsDialog",
				"Duplicate component: %1 in %2")
				.arg("K1", "+UV2"));
	}

	void stripBridgesJoinSignals()
	{
		QETProject project;
		auto*	   diagram = project.addNewDiagram();
		auto*	   a	   = symbol(diagram, "terminal");
		auto*	   b	   = symbol(diagram, "terminal");
		wire(symbol(diagram), a, "0V");
		wire(symbol(diagram), b, "0V");
		QCOMPARE(DuplicateLabels::find(&project).size(), 1);
		auto* strip = project.newTerminalStrip({}, {}, "X1");
		QVERIFY(strip);
		QVERIFY(strip->addTerminal(a));
		QVERIFY(strip->addTerminal(b));
		QVERIFY(strip->setBridge(
			{static_cast<TerminalElement*>(a)->realTerminal(),
			 static_cast<TerminalElement*>(b)->realTerminal()}));
		QVERIFY(DuplicateLabels::find(&project).isEmpty());
		strip->unBridge({static_cast<TerminalElement*>(a)->realTerminal()});
		QCOMPARE(DuplicateLabels::find(&project).size(), 1);
	}

	void terminalSymbolsOnOnePotentialAreOneComponent()
	{
		QETProject project;
		auto*	   first  = project.addNewDiagram();
		auto*	   second = project.addNewDiagram();
		auto*	   splice = symbol(first, "terminal", "X0:6L");
		auto*	   out	  = symbol(first, "next_report");
		auto*	   in	  = symbol(second, "previous_report");
		auto*	   again  = symbol(second, "terminal", "X0:6L");
		wire(symbol(first), splice, "6L");
		wire(splice, out, "6L");
		wire(in, again, "6L");
		out->linkToElement(in);
		QVERIFY(DuplicateLabels::find(&project).isEmpty());
		// Another terminal block with that number, on another potential.
		auto* other = symbol(second, "terminal", "X0:6L");
		wire(other, symbol(second), "7L");
		auto groups = DuplicateLabels::find(&project);
		QCOMPARE(groups.size(), 1);
		QCOMPARE(groups.first().kind, DuplicateLabels::Kind::Component);
		QCOMPARE(groups.first().entity_count, 2);
		QCOMPARE(groups.first().occurrences.size(), 3);
		// Unwired terminal blocks are separate components.
		symbol(first, "terminal", "X9");
		symbol(second, "terminal", "X9");
		groups = DuplicateLabels::find(&project);
		QCOMPARE(groups.size(), 2);
		QCOMPARE(groups.at(1).label, QString("X9"));
		QCOMPARE(groups.at(1).entity_count, 2);
	}

	void caseSensitivityIsConfigurable()
	{
		QETProject project;
		auto*	   first  = project.addNewDiagram();
		auto*	   second = project.addNewDiagram();
		isolated(first, "w1");
		isolated(second, "W1");
		symbol(first, "simple", "k1");
		symbol(second, "simple", "K1");
		QVERIFY(DuplicateLabels::find(&project, Qt::CaseSensitive).isEmpty());
		const auto groups =
			DuplicateLabels::find(&project, Qt::CaseInsensitive);
		QCOMPARE(groups.size(), 2);
		QCOMPARE(groups.at(0).kind, DuplicateLabels::Kind::Signal);
		QCOMPARE(groups.at(0).label, QString("W1, w1"));
		QCOMPARE(groups.at(0).occurrences.at(0).label, QString("w1"));
		QCOMPARE(groups.at(0).occurrences.at(1).label, QString("W1"));
		QCOMPARE(groups.at(1).kind, DuplicateLabels::Kind::Component);
		QCOMPARE(groups.at(1).label, QString("K1, k1"));
	}

	void folioVariablesCompareAsDisplayed()
	{
		QETProject project;
		auto*	   first  = project.addNewDiagram();
		auto*	   second = project.addNewDiagram();
		first->border_and_titleblock.setFolio("1");
		second->border_and_titleblock.setFolio("2");
		auto* formula_wire	 = isolated(first, {});
		auto  properties	 = formula_wire->properties();
		properties.m_formula = "%F21";
		formula_wire->setProperties(properties);
		auto* coil = symbol(first);
		auto  info = coil->elementInformations();
		info.addValue(QStringLiteral("formula"), QStringLiteral("%F-K1"));
		coil->setElementInformations(info);
		QCOMPARE(DuplicateLabels::displayedLabel(formula_wire), QString("121"));
		QCOMPARE(DuplicateLabels::displayedLabel(coil), QString("1-K1"));
		auto* manual_wire = isolated(second, "121");
		auto* manual_coil = symbol(second, "simple", "1-K1");
		auto  groups	  = DuplicateLabels::find(&project);
		QCOMPARE(groups.size(), 2);
		QCOMPARE(groups.at(0).label, QString("121"));
		QCOMPARE(groups.at(1).label, QString("1-K1"));
		// Renaming the folio leaves stored texts behind; the sheet and the
		// check follow the folio.
		first->border_and_titleblock.setFolio("3");
		QVERIFY(DuplicateLabels::find(&project).isEmpty());
		properties		= manual_wire->properties();
		properties.text = "321";
		manual_wire->setProperties(properties);
		setLabel(manual_coil, "3-K1");
		groups = DuplicateLabels::find(&project);
		QCOMPARE(groups.size(), 2);
		QCOMPARE(groups.at(0).occurrences.at(0).item.data(), formula_wire);
		QCOMPARE(groups.at(0).occurrences.at(0).label, QString("321"));
		QCOMPARE(groups.at(1).occurrences.at(0).item.data(), coil);
		QCOMPARE(groups.at(1).occurrences.at(0).label, QString("3-K1"));
		// A frozen label keeps the text it was given on folio 1.
		coil->freezeLabel(true);
		QCOMPARE(DuplicateLabels::displayedLabel(coil), QString("1-K1"));
		formula_wire->setFreezeLabel(true);
		QCOMPARE(
			DuplicateLabels::displayedLabel(formula_wire),
			formula_wire->properties().text);
	}

	void removedSheetsDoNotJoinSignals()
	{
		QETProject project;
		auto*	   first	  = project.addNewDiagram();
		auto*	   bridge	  = project.addNewDiagram();
		auto*	   last		  = project.addNewDiagram();
		auto*	   out		  = symbol(first, "next_report");
		auto*	   in		  = symbol(bridge, "previous_report");
		auto*	   bridge_out = symbol(bridge, "next_report");
		auto*	   last_in	  = symbol(last, "previous_report");
		wire(symbol(first), out, "W1");
		wire(in, bridge_out, "W1");
		wire(last_in, symbol(last), "W1");
		out->linkToElement(in);
		bridge_out->linkToElement(last_in);
		QVERIFY(DuplicateLabels::find(&project).isEmpty());
		project.undoStack()->push(new RemoveDiagramCommand(&project, bridge));
		const auto groups = DuplicateLabels::find(&project);
		QCOMPARE(groups.size(), 1);
		QCOMPARE(groups.first().entity_count, 2);
		QCOMPARE(groups.first().occurrences.size(), 2);
		project.undoStack()->undo();
		QVERIFY(DuplicateLabels::find(&project).isEmpty());
	}

	void removedSheetsDoNotJoinDevices()
	{
		QETProject project;
		auto*	   first  = project.addNewDiagram();
		auto*	   second = project.addNewDiagram();
		auto*	   master = symbol(second, "master", "K1");
		auto*	   a	  = symbol(first, "slave", "K1");
		auto*	   b	  = symbol(first, "slave", "K1");
		master->linkToElement(a);
		master->linkToElement(b);
		QVERIFY(DuplicateLabels::find(&project).isEmpty());
		project.undoStack()->push(new RemoveDiagramCommand(&project, second));
		const auto groups = DuplicateLabels::find(&project);
		QCOMPARE(groups.size(), 1);
		QCOMPARE(groups.first().entity_count, 2);
		QCOMPARE(groups.first().occurrences.size(), 2);
		project.undoStack()->undo();
		QVERIFY(DuplicateLabels::find(&project).isEmpty());
	}

	void bomFlags_data()
	{
		QTest::addColumn<QString>("flag");
		for (const QString& flag : {"true", "1", "yes", "on", " TRUE "})
			QTest::newRow(qPrintable(flag)) << flag;
	}

	void bomFlags()
	{
		QFETCH(QString, flag);
		QETProject project;
		auto*	   diagram = project.addNewDiagram();
		symbol(diagram, "simple", "Q1");
		auto* second = symbol(diagram, "slave", "Q1");
		QCOMPARE(DuplicateLabels::find(&project).size(), 1);
		exclude(second, flag);
		QVERIFY(DuplicateLabels::find(&project).isEmpty());
		exclude(second, "false");
		QCOMPARE(DuplicateLabels::find(&project).size(), 1);
	}

	void placeholdersAndNamespaces()
	{
		QETProject project;
		auto*	   diagram = project.addNewDiagram();
		isolated(diagram, "_");
		isolated(diagram, "_");
		isolated(diagram, " ");
		isolated(diagram, "W1");
		isolated(diagram, "w1");
		symbol(diagram, "simple", "W1"); // components and signals are separate
		symbol(diagram, "simple", "_");
		symbol(diagram, "simple", "_");
		QVERIFY(DuplicateLabels::find(&project).isEmpty());
		isolated(diagram, " W1 ");
		QCOMPARE(DuplicateLabels::find(&project).size(), 1);
	}

	void dialogNavigatesAndHandlesDeletedItems()
	{
		QETProject project;
		auto*	   first  = project.addNewDiagram();
		auto*	   second = project.addNewDiagram();
		// A group of signals, then one of components
		const QList<QGraphicsObject*> items {
			isolated(first, "W1"),
			isolated(second, "W1"),
			symbol(first, "simple", "K1"),
			symbol(second, "simple", "K1")};
		ProjectView view(&project, QetAppTest::editor());
		view.resize(1000, 700);
		view.show();
		DuplicateLabelsDialog dialog(&project);
		dialog.show();
		QCoreApplication::processEvents();
		auto* tree = dialog.findChild<QTreeWidget*>("duplicateOccurrences");
		QVERIFY(tree);
		QCOMPARE(tree->topLevelItemCount(), 2);
		for (int group = 0; group < 2; ++group)
		{
			auto* root = tree->topLevelItem(group);
			QCOMPARE(root->childCount(), 2);
			for (int i = 0; i < 2; ++i)
			{
				QTest::mouseClick(
					tree->viewport(),
					Qt::LeftButton,
					Qt::NoModifier,
					tree->visualItemRect(root->child(i)).center());
				QCoreApplication::processEvents();
				QGraphicsObject* expected = items.at(2 * group + i);
				auto*			 diagram = DuplicateLabels::diagramOf(expected);
				QCOMPARE(view.currentDiagram()->diagram(), diagram);
				QVERIFY(expected->isSelected());
				QCOMPARE(diagram->selectedItems().size(), 1);
				QVERIFY(
					view.currentDiagram()
						->mapToScene(view.currentDiagram()->viewport()->rect())
						.boundingRect()
						.intersects(expected->sceneBoundingRect()));
			}
		}
		delete items.at(3);
		dialog.showOccurrence(tree->topLevelItem(1)->child(1));
		QCOMPARE(
			dialog.findChild<QLabel*>("duplicateSummary")->text(),
			QCoreApplication::translate(
				"DuplicateLabelsDialog",
				"This occurrence no longer exists. Refresh the results."));
		dialog.findChild<QPushButton*>("refreshDuplicates")->click();
		QCOMPARE(tree->topLevelItemCount(), 1);
	}

	void dialogGroupsAreCollapsibleRows()
	{
		QETProject			  project;
		auto*				  first	   = project.addNewDiagram();
		auto*				  second   = project.addNewDiagram();
		auto*				  terminal = symbol(first, "terminal");
		auto*				  a		   = wire(symbol(first), terminal, "W1");
		auto*				  b		   = wire(terminal, symbol(first), "W1");
		auto*				  alone	   = isolated(second, "W1");
		DuplicateLabelsDialog dialog(&project);
		auto* tree = dialog.findChild<QTreeWidget*>("duplicateOccurrences");
		QCOMPARE(tree->columnCount(), 3);
		QCOMPARE(tree->topLevelItemCount(), 1);
		auto* root = tree->topLevelItem(0);
		QVERIFY(root->isExpanded());
		QCOMPARE(root->childCount(), 2);
		const auto occurrences = [](int count)
		{
			return QCoreApplication::translate(
				"DuplicateLabelsDialog",
				"%n occurrences",
				nullptr,
				count);
		};
		auto* joined = root->child(0);
		QCOMPARE(
			joined->text(0),
			QCoreApplication::translate("DuplicateLabelsDialog", "Group %1")
				.arg(1));
		QCOMPARE(joined->text(1), QString("1"));
		QCOMPARE(joined->text(2), occurrences(2));
		QVERIFY(! joined->isExpanded());
		QCOMPARE(joined->childCount(), 2);
		auto* single = root->child(1);
		QCOMPARE(single->text(1), QString("2"));
		QCOMPARE(single->text(2), occurrences(1));
		QCOMPARE(single->childCount(), 1);
		QCOMPARE(single->child(0)->text(0), QString("W1"));
		// A group row shows its first occurrence.
		dialog.showOccurrence(single);
		QVERIFY(alone->isSelected());
		dialog.showOccurrence(joined->child(1));
		QVERIFY(a->isSelected() != b->isSelected());
	}

	void dialogCaseOption()
	{
		const bool previous = QetSettings::duplicateLabelsCaseSensitive();
		const auto restore	= qScopeGuard(
			 [previous]()
			 { QetSettings::setDuplicateLabelsCaseSensitive(previous); });
		QetSettings::setDuplicateLabelsCaseSensitive(false);
		QETProject project;
		auto*	   diagram = project.addNewDiagram();
		isolated(diagram, "w1");
		isolated(diagram, "W1");
		DuplicateLabelsDialog dialog(&project);
		auto* tree	 = dialog.findChild<QTreeWidget*>("duplicateOccurrences");
		auto* option = dialog.findChild<QCheckBox*>("duplicateCaseSensitive");
		QVERIFY(option);
		QVERIFY(! option->isChecked());
		QCOMPARE(tree->topLevelItemCount(), 1);
		auto* root = tree->topLevelItem(0);
		QCOMPARE(root->child(0)->child(0)->text(0), QString("w1"));
		QCOMPARE(root->child(1)->child(0)->text(0), QString("W1"));
		option->setChecked(true);
		QVERIFY(QetSettings::duplicateLabelsCaseSensitive());
		QCOMPARE(tree->topLevelItemCount(), 0);
		DuplicateLabelsDialog reopened(&project);
		QVERIFY(reopened.findChild<QCheckBox*>("duplicateCaseSensitive")
					->isChecked());
	}

	void dialogRejectsRemovedSheets()
	{
		QETProject project;
		auto*	   first  = project.addNewDiagram();
		auto*	   second = project.addNewDiagram();
		isolated(first, "W1");
		auto*				  removed = isolated(second, "W1");
		DuplicateLabelsDialog dialog(&project);
		auto* tree = dialog.findChild<QTreeWidget*>("duplicateOccurrences");
		auto* row  = tree->topLevelItem(0)->child(1)->child(0);
		project.undoStack()->push(new RemoveDiagramCommand(&project, second));
		dialog.showOccurrence(row);
		QVERIFY(! removed->isSelected());
		QCOMPARE(
			dialog.findChild<QLabel*>("duplicateSummary")->text(),
			QCoreApplication::translate(
				"DuplicateLabelsDialog",
				"This occurrence no longer exists. Refresh the results."));
		project.undoStack()->undo();
		dialog.showOccurrence(row);
		QVERIFY(removed->isSelected());
	}

	void dialogActivatesItsProject()
	{
		auto* editor = QetAppTest::editor();
		editor->newProject();
		auto* project = editor->currentProjectView()->project();
		QList<QPointer<QETProject>> opened {project};

		// Closed once the dialog is gone, as the user closes them, so that
		// the next cases find the editor without these projects.
		const auto close = qScopeGuard(
			[&opened]()
			{
				for (const auto& p : std::as_const(opened)) closeInEditor(p);
			});
		auto* diagram =
			editor->currentProjectView()->currentDiagram()->diagram();
		isolated(diagram, "W1");
		isolated(diagram, "W1");
		DuplicateLabelsDialog dialog(project, editor);
		editor->newProject();
		auto* other = editor->currentProjectView()->project();
		opened << other;
		editor->activateProject(other);
		QCOMPARE(editor->currentProjectView()->project(), other);
		auto* tree = dialog.findChild<QTreeWidget*>("duplicateOccurrences");
		dialog.showOccurrence(tree->topLevelItem(0)->child(0)->child(0));
		QCOMPARE(editor->currentProjectView()->project(), project);
		QCOMPARE(
			editor->currentProjectView()->currentDiagram()->diagram(),
			diagram);
	}

	void translatedDialog_data()
	{
		QTest::addColumn<QString>("language");
		QTest::addColumn<int>("count");
		QTest::addColumn<QString>("summary");
		QTest::newRow("English singular")
			<< "en" << 1 << "1 duplicate label found.";
		QTest::newRow("English plural")
			<< "en" << 2 << "2 duplicate labels found.";
		QTest::newRow("Spanish singular")
			<< "es" << 1 << "Se ha encontrado 1 identificador duplicado.";
		QTest::newRow("Spanish plural")
			<< "es" << 2 << "Se han encontrado 2 identificadores duplicados.";
	}

	void translatedDialog()
	{
		QFETCH(QString, language);
		QFETCH(int, count);
		QFETCH(QString, summary);
		QTranslator translator;
		QVERIFY(translator.load(
			QStringLiteral(QET_TEST_TRANSLATIONS_DIR "/qet_") + language
			+ ".qm"));
		QVERIFY(QCoreApplication::installTranslator(&translator));
		const auto remove_translator =
			qScopeGuard([&translator]()
						{ QCoreApplication::removeTranslator(&translator); });
		QETProject project;
		auto*	   first  = project.addNewDiagram();
		auto*	   second = project.addNewDiagram();
		for (int i = 0; i < count; ++i)
		{
			const QString label = QStringLiteral("K%1").arg(i + 1);
			symbol(first, "simple", label);
			symbol(second, "simple", label);
		}
		DuplicateLabelsDialog dialog(&project);
		QCOMPARE(
			dialog.findChild<QLabel*>("duplicateSummary")->text(),
			summary);
		QCOMPARE(
			dialog.windowTitle(),
			language == "es" ?
				QString::fromUtf8("Señales y componentes duplicados") :
				QStringLiteral("Duplicate signals and components"));
		auto* tree = dialog.findChild<QTreeWidget*>("duplicateOccurrences");
		QCOMPARE(
			tree->topLevelItem(0)->text(0),
			language == "es" ? QStringLiteral("Componente duplicado: K1") :
							   QStringLiteral("Duplicate component: K1"));
	}

	void editorActionWorksForReadOnlyProjects()
	{
		auto* diagram = QetAppTest::diagram();
		QVERIFY(diagram);
		auto* editor = QetAppTest::editor();
		auto* action = editor->findChild<QAction*>("findDuplicateLabels");
		QVERIFY(action);
		// In the Project menu, right below Clean project
		QAction* above = nullptr;
		for (QMenu* menu : editor->findChildren<QMenu*>())
		{
			const auto actions = menu->actions();
			const auto at	   = actions.indexOf(action);
			if (at > 0) above = actions.at(at - 1);
		}
		QVERIFY(above);
		QCOMPARE(
			above->text(),
			QCoreApplication::translate("QETDiagramEditor", "Clean project"));
		diagram->project()->setReadOnly(true);
		QCoreApplication::processEvents();
		QVERIFY(action->isEnabled());
		action->trigger();
		auto* dialog = editor->findChild<DuplicateLabelsDialog*>();
		QVERIFY(dialog);
		QVERIFY(dialog->isVisible());
		dialog->close();
		diagram->project()->setReadOnly(false);
	}

	// A project opened with folios not built yet: the dialog builds them
	// first, showing its progress, and leaves the project as it found it.
	void dialogShowsFolioBuildProgress()
	{
		QETProject project;
		for (int i = 0; i < 3; ++i)
			isolated(project.addNewDiagram(), QStringLiteral("W1"));
		const QString path = m_definitions.filePath("progress.qet");
		{
			QFile file(path);
			QVERIFY(file.open(QIODevice::WriteOnly));
			file.write(project.toXml().toByteArray());
		}
		QETProject reopened(path);
		QCOMPARE(reopened.state(), QETProject::Ok);
		QVERIFY2(
			reopened.unloadedFolioCount() > 0,
			"the fixture must exercise lazy folios");
		QStringList shown;
		QTimer		watch;
		connect(
			&watch,
			&QTimer::timeout,
			this,
			[&shown]()
			{
				for (QWidget* widget : QApplication::topLevelWidgets())
					if (auto* progress = qobject_cast<QProgressDialog*>(widget);
						progress && progress->isVisible())
						shown << progress->labelText();
			});
		watch.start(0);
		DuplicateLabelsDialog dialog(&reopened);
		watch.stop();
		QCOMPARE(reopened.unloadedFolioCount(), 0);
		QVERIFY2(
			! shown.isEmpty()
				&& shown.last()
					   == QCoreApplication::translate(
							  "DuplicateLabelsDialog",
							  "Reading folio %1 of %2...")
							  .arg(3)
							  .arg(3),
			qPrintable(shown.join(QStringLiteral(" | "))));
		QVERIFY(! QApplication::overrideCursor());
		QCOMPARE(
			dialog.findChild<QTreeWidget*>("duplicateOccurrences")
				->topLevelItemCount(),
			1);
		// Built now: refreshing builds nothing and shows nothing.
		shown.clear();
		watch.start(0);
		dialog.refresh();
		watch.stop();
		QVERIFY(shown.isEmpty());
	}

	// A .qetz container; dialogShowsFolioBuildProgress() reads a .qet file
	// with folios not built yet.
	void lazySaveReload()
	{
		QETProject project;
		auto*	   first  = project.addNewDiagram();
		auto*	   second = project.addNewDiagram();
		auto*	   out	  = symbol(first, "next_report");
		auto*	   in	  = symbol(second, "previous_report");
		wire(symbol(first), out, "LINKED");
		wire(in, symbol(second), "LINKED");
		out->linkToElement(in);
		isolated(first, "ISOLATED");
		isolated(second, "ISOLATED");
		symbol(first, "simple", "DUPLICATE");
		symbol(second, "simple", "DUPLICATE");
		const auto before = DuplicateLabels::find(&project);
		QCOMPARE(before.size(), 2);
		const QString path = m_definitions.filePath("roundtrip.qetz");
		QString		  error;
		QVERIFY2(
			QETProject::writeContainer(project.toXml(), path, &error),
			qPrintable(error));
		QETProject reopened(path);
		QCOMPARE(reopened.state(), QETProject::Ok);
		QVERIFY2(
			reopened.unloadedFolioCount() > 0,
			"the fixture must exercise lazy folios");
		const int  undo_count = reopened.undoStack()->count();
		const auto after	  = DuplicateLabels::find(&reopened);
		QCOMPARE(after.size(), before.size());
		for (int i = 0; i < before.size(); ++i)
		{
			QCOMPARE(after.at(i).kind, before.at(i).kind);
			QCOMPARE(after.at(i).label, before.at(i).label);
			QCOMPARE(after.at(i).entity_count, before.at(i).entity_count);
			QCOMPARE(
				after.at(i).occurrences.size(),
				before.at(i).occurrences.size());
		}
		QCOMPARE(reopened.undoStack()->count(), undo_count);
		QCOMPARE(reopened.unloadedFolioCount(), 0);
	}
};

QET_APP_TEST_MAIN(tst_duplicatelabels)
#include "tst_duplicatelabels.moc"
