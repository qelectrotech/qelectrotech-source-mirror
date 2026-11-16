// SPDX-License-Identifier: GPL-2.0-or-later
#include "ElementsCollection/elementslocation.h"
#include "ElementsCollection/xmlelementcollection.h"
#include "devicetype/devicetype.h"
#include "duplicatelabels.h"
#include "diagram.h"
#include "diagramevent/diagrameventaddelement.h"
#include "factory/elementfactory.h"
#include "materiallist/materiallist.h"
#include "qetapptest.h"
#include "qetapp.h"
#include "qetgraphicsitem/element.h"
#include "qetproject.h"
#include "qetgraphicsitem/structureboxitem.h"
#include "qetgraphicsitem/terminal.h"
#include "titleblockproperties.h"
#include "devicetype/devicetypecheck.h"
#include "ui/devicetypechooser.h"
#include "ui/placelabelpartdialog.h"
#include "utils/qetsettings.h"

#include <QDomDocument>
#include <QGraphicsSceneMouseEvent>
#include <QApplication>
#include <QCompleter>
#include <QLineEdit>
#include <QPushButton>
#include <QTemporaryDir>
#include <QScopeGuard>
#include <QUndoCommand>
#include <QTimer>
#include <QUndoStack>
#include <QtTest>

/**
	The label and the part asked when a symbol is placed
	(PlaceLabelPartDialog): what is proposed, what is given in one undo
	step, and when the device type is still asked.
*/
class tst_placelabelpart : public QObject
{
	Q_OBJECT

	static QString define(QETProject &project, const QString &name, const QString &link = QStringLiteral("master"),
						  const QString &kind = QStringLiteral("coil"))
	{
		QDomDocument document;
		document.setContent(QStringLiteral(
			"<definition type=\"element\" link_type=\"%4\" width=\"80\" height=\"60\" hotspot_x=\"10\" hotspot_y=\"20\">"
			"<uuid uuid=\"%1\"/><names><name lang=\"en\">%2</name></names>"
			"<kindInformations><kindInformation name=\"type\">%5</kindInformation></kindInformations>"
			"<description><line x1=\"0\" y1=\"-10\" x2=\"0\" y2=\"10\"/>"
			"<terminal x=\"0\" y=\"20\" orientation=\"s\" uuid=\"%3\" name=\"A1\"/></description></definition>")
				.arg(QUuid::createUuid().toString(), name, QUuid::createUuid().toString(), link, kind));
		project.embeddedElementCollection()->addElementDefinition(QStringLiteral("import"), name, document.documentElement());
		return QStringLiteral("embed://import/") + name + QStringLiteral(".elmt");
	}

	static Element *place(QETProject &project, Diagram *folio, const QString &location, const QString &prefix,
						  const QString &label = QString())
	{
		int state = 0;
		Element *symbol = ElementFactory::Instance()->createElement(ElementsLocation(location, &project), nullptr, &state);
		if (!symbol || state) return nullptr;
		folio->addItem(symbol);
		symbol->setPrefix(prefix);
		if (!label.isEmpty()) {
			DiagramContext info = symbol->elementInformations();
			info.addValue(QStringLiteral("label"), label);
			symbol->setElementInformations(info);
		}
		return symbol;
	}

	static DeviceType headedBy(const QString &name, const QString &location)
	{
		DeviceType type;
		type.name = name;
		type.names.insert(QStringLiteral("en"), name + QStringLiteral(" relay"));
		type.parts << DeviceType::Part{QStringLiteral("master"), QStringLiteral("coil"), QString(),
									   {{location, {QStringLiteral("A1")}, {}}}};
		return type;
	}

	static QString info(Element *symbol, const QString &key)
	{
		return symbol->elementInformations().value(key).toString();
	}

	static QString defineTerminals(QETProject &project, const QString &name, int count)
	{
		QString lines;
		for (int i = 0 ; i < count ; ++i)
			lines += QStringLiteral("<terminal x=\"%1\" y=\"20\" orientation=\"s\" uuid=\"%2\" name=\"\"/>")
					.arg(i * 20).arg(QUuid::createUuid().toString());
		QDomDocument document;
		document.setContent(QStringLiteral(
			"<definition type=\"element\" link_type=\"simple\" width=\"80\" height=\"60\" hotspot_x=\"10\" hotspot_y=\"20\">"
			"<uuid uuid=\"%1\"/><names><name lang=\"en\">%2</name></names>"
			"<description><line x1=\"0\" y1=\"-10\" x2=\"0\" y2=\"10\"/>%3</description></definition>")
				.arg(QUuid::createUuid().toString(), name, lines));
		project.embeddedElementCollection()->addElementDefinition(QStringLiteral("import"), name, document.documentElement());
		return QStringLiteral("embed://import/") + name + QStringLiteral(".elmt");
	}

	static QStringList pinsOf(Element *element)
	{
		QStringList names;
		for (Terminal *terminal : element->terminals()) names << terminal->baseName();
		return names;
	}

	static Element *theOne(Diagram *folio, const QStringList &pins)
	{
		for (Element *element : folio->elements())
			if (pinsOf(element) == pins) return element;
		return nullptr;
	}

	static DeviceType socket(const QString &four, const QString &one)
	{
		DeviceType type;
		type.name = QStringLiteral("SOCKET");
		type.names.insert(QStringLiteral("en"), QStringLiteral("Socket"));
		const auto part = [](const QString &location, const QStringList &pins) {
			DeviceType::Part p;
			p.role = QStringLiteral("simple");
			p.kind = QStringLiteral("simple");
			DeviceType::Symbol symbol;
			symbol.location = location;
			symbol.pins = pins;
			p.symbols << symbol;
			return p;
		};
		type.parts << part(four, {QStringLiteral("2"), QStringLiteral("3"), QStringLiteral("PE"), QStringLiteral("1")})
				   << part(one, {QStringLiteral("C1")}) << part(one, {QStringLiteral("C2")});
		return type;
	}

	QTemporaryDir m_dir;

  private slots:
	void initTestCase()
	{
		QETProject::setBackupEnabled(false);
		QVERIFY(m_dir.isValid());
		const QString path = m_dir.filePath(QStringLiteral("materials.csv"));
		MaterialList::setConfiguredPath(path);
		QVERIFY(MaterialList::createFile(path));
		MaterialRecord relay;
		relay.setValue(QStringLiteral("description"), QStringLiteral("Power relay 24 V"));
		relay.setValue(QStringLiteral("manufacturer"), QStringLiteral("Acme"));
		relay.setValue(QStringLiteral("manufacturer_reference"), QStringLiteral("AC-REL-24"));
		relay.setValue(QStringLiteral("device_type"), QStringLiteral("PLACE-RELAY"));
		QVERIFY(MaterialList::appendRecord(path, relay));
	}

	void cleanup() {QetSettings::setAskLabelAndPartPlacingSymbol(false);}

	void settingOffAsksNothing()
	{
		QETProject project;
		Diagram *folio = project.addNewDiagram();
		const QString coil = define(project, QStringLiteral("coil"));
		const QString lamp = define(project, QStringLiteral("lamp"), QStringLiteral("simple"), QStringLiteral("simple"));
		const QString terminal = define(project, QStringLiteral("term"), QStringLiteral("terminal"), QStringLiteral("terminal"));
		const QString contact = define(project, QStringLiteral("contact"), QStringLiteral("slave"), QStringLiteral("simple"));
		Element *master = place(project, folio, coil, QStringLiteral("K"));
		Element *simple = place(project, folio, lamp, QStringLiteral("H"));
		Element *strip = place(project, folio, terminal, QString());
		Element *slave = place(project, folio, contact, QString());
		QVERIFY(master && simple && strip && slave);

		QVERIFY(!QetSettings::askLabelAndPartPlacingSymbol());
		QVERIFY(!PlaceLabelPartDialog::asksFor(master));
		QVERIFY(!PlaceLabelPartDialog::asksFor(simple));

			//Placing with the setting off: nothing is asked (a dialog would
			//block here), the symbol is as placed
		const int before = int(folio->elements().size());
		QETApp::registerProject(&project);
		const auto registered = qScopeGuard([&project]() {QETApp::unregisterProject(&project);});
		ElementsLocation location(coil, &project);
		folio->setEventInterface(new DiagramEventAddElement(location, folio, QPointF(100, 100)));
		auto *placing = qobject_cast<DiagramEventAddElement *>(folio->eventInterface());
		QVERIFY(placing && placing->isRunning());
		QGraphicsSceneMouseEvent release(QEvent::GraphicsSceneMouseRelease);
		release.setButton(Qt::LeftButton);
		release.setScenePos(QPointF(100, 100));
		placing->mouseReleaseEvent(&release);
		QCoreApplication::processEvents();
		QCOMPARE(int(folio->elements().size()), before + 2);

		QetSettings::setAskLabelAndPartPlacingSymbol(true);
		QVERIFY(PlaceLabelPartDialog::asksFor(master));
		QVERIFY(PlaceLabelPartDialog::asksFor(simple));
		QVERIFY(PlaceLabelPartDialog::asksFor(strip));
		QVERIFY(!PlaceLabelPartDialog::asksFor(slave));
	}

	void proposesTheNextFreeLabel()
	{
		QETProject project;
		Diagram *folio = project.addNewDiagram();
		const QString coil = define(project, QStringLiteral("coil"));
		QVERIFY(place(project, folio, coil, QStringLiteral("K"), QStringLiteral("-K1")));
		Element *fresh = place(project, folio, coil, QStringLiteral("K"));
		QVERIFY(fresh);
		QCOMPARE(PlaceLabelPartDialog::proposedLabel(fresh), QStringLiteral("-K2"));

			//No class letter: nothing to propose; a label it has is kept
		Element *plain = place(project, folio, coil, QString());
		QVERIFY(plain);
		QVERIFY(PlaceLabelPartDialog::proposedLabel(plain).isEmpty());
		Element *numbered = place(project, folio, coil, QStringLiteral("K"), QStringLiteral("-K7"));
		QCOMPARE(PlaceLabelPartDialog::proposedLabel(numbered), QStringLiteral("-K7"));
	}

	void proposesTheNextFreeTerminal()
	{
		QETProject project;
		Diagram *folio = project.addNewDiagram();
		const QString terminal = define(project, QStringLiteral("term"), QStringLiteral("terminal"), QStringLiteral("terminal"));
		Element *first = place(project, folio, terminal, QString());
		QVERIFY(first);
		QCOMPARE(PlaceLabelPartDialog::proposedLabel(first), QStringLiteral("X1:1"));

		QUndoCommand *undo = PlaceLabelPartDialog::apply(first, QStringLiteral("X1:3"), MaterialRecord());
		QVERIFY(undo);
		folio->undoStack().push(undo);
		QCOMPARE(info(first, QStringLiteral("label")), QStringLiteral("X1:3"));

		Element *next = place(project, folio, terminal, QString());
		QVERIFY(next);
		QCOMPARE(PlaceLabelPartDialog::proposedLabel(next), QStringLiteral("X1:4"));
		QVERIFY(PlaceLabelPartDialog::terminalExists(next, QStringLiteral("X1:3")));
		QVERIFY(!PlaceLabelPartDialog::terminalExists(next, QStringLiteral("X1:4")));
			//A strip alone: its next free terminal
		QCOMPARE(PlaceLabelPartDialog::resolvedLabel(next, QStringLiteral("X2")), QStringLiteral("X2:1"));
		QCOMPARE(PlaceLabelPartDialog::resolvedLabel(next, QStringLiteral("X1:9")), QStringLiteral("X1:9"));
	}

	void aFullPartNumberThenReturnAccepts()
	{
		QETProject project;
		Diagram *folio = project.addNewDiagram();
		const QString lamp = define(project, QStringLiteral("lamp-ret"), QStringLiteral("simple"), QStringLiteral("simple"));
		Element *symbol = place(project, folio, lamp, QStringLiteral("H"));
		QVERIFY(symbol);
		PlaceLabelPartDialog dialog(symbol);
		dialog.show();
		QVERIFY(QTest::qWaitForWindowExposed(&dialog));
		QLineEdit *part = dialog.findChild<QLineEdit *>(QStringLiteral("placePart"));
		QVERIFY(part);
		part->setFocus();
		QTest::keyClicks(part, QStringLiteral("AC-REL-24"));
		QCOMPARE(dialog.record().value(QStringLiteral("manufacturer")), QStringLiteral("Acme"));
		QTest::keyClick(part, Qt::Key_Return);
		QCOMPARE(dialog.result(), int(QDialog::Accepted));
		QCOMPARE(dialog.part(), QStringLiteral("AC-REL-24"));
	}

	void returnAcceptsAfterAPartIsChosen()
	{
		QETProject project;
		Diagram *folio = project.addNewDiagram();
		const QString lamp = define(project, QStringLiteral("lamp-choose"), QStringLiteral("simple"), QStringLiteral("simple"));
		Element *symbol = place(project, folio, lamp, QStringLiteral("H"));
		QVERIFY(symbol);
		PlaceLabelPartDialog dialog(symbol);
		dialog.show();
		QVERIFY(QTest::qWaitForWindowExposed(&dialog));
		QPushButton *choose = dialog.findChild<QPushButton *>(QStringLiteral("placePartChoose"));
		QVERIFY(choose);
			//The Find part window opens modal: pick its first part
		QTimer::singleShot(200, [] {
			if (QWidget *window = QApplication::activeModalWidget())
				if (auto *find = qobject_cast<QDialog *>(window)) find->accept();
		});
		choose->setFocus();
		choose->click();
		QVERIFY(!dialog.record().values.isEmpty());
		QCOMPARE(dialog.focusWidget(), dialog.findChild<QLineEdit *>(QStringLiteral("placePart")));
		QTest::keyClick(dialog.focusWidget(), Qt::Key_Return);
		QCOMPARE(dialog.result(), int(QDialog::Accepted));
	}

	void wordsTypedListTheirPartsInThePopup()
	{
		QETProject project;
		Diagram *folio = project.addNewDiagram();
		const QString lamp = define(project, QStringLiteral("lamp-pop"), QStringLiteral("simple"), QStringLiteral("simple"));
		Element *symbol = place(project, folio, lamp, QStringLiteral("H"));
		QVERIFY(symbol);
		PlaceLabelPartDialog dialog(symbol);
		dialog.show();
		QVERIFY(QTest::qWaitForWindowExposed(&dialog));
		QLineEdit *part = dialog.findChild<QLineEdit *>(QStringLiteral("placePart"));
		part->setFocus();
			//"relai" is roughly "relay": the part is offered, not chosen
		QTest::keyClicks(part, QStringLiteral("relai acme"));
		QVERIFY(dialog.record().values.isEmpty());
		QCompleter *completer = dialog.findChild<QCompleter *>();
		QVERIFY(completer);
		QCOMPARE(completer->completionModel()->rowCount(), 1);
		const QModelIndex row = completer->completionModel()->index(0, 0);
		QVERIFY(row.data().toString().startsWith(QStringLiteral("AC-REL-24")));
		QVERIFY(row.data().toString().contains(QStringLiteral("Power relay 24 V")));
		QVERIFY(row.data().toString().endsWith(QStringLiteral("Acme")));
		emit completer->activated(row);
		QCOMPARE(dialog.part(), QStringLiteral("AC-REL-24"));
		QCOMPARE(dialog.record().value(QStringLiteral("manufacturer")), QStringLiteral("Acme"));
	}

	void applyingAPartIsOneUndoStep()
	{
		QETProject project;
		Diagram *folio = project.addNewDiagram();
		const QString coil = define(project, QStringLiteral("coil"));
		const DeviceType library_type = headedBy(QStringLiteral("PLACE-RELAY"), coil);
		QVERIFY(DeviceTypeLibrary::save(library_type));
		Element *symbol = place(project, folio, coil, QStringLiteral("K"));
		QVERIFY(symbol);

		PlaceLabelPartDialog dialog(symbol);
		QCOMPARE(dialog.label(), QStringLiteral("K1"));
		QLineEdit *part = dialog.findChild<QLineEdit *>(QStringLiteral("placePart"));
		QVERIFY(part && dialog.findChild<QLineEdit *>(QStringLiteral("placeLabel"))
				&& dialog.findChild<QPushButton *>(QStringLiteral("placePartChoose")));
		dialog.setPart(QStringLiteral("ac-rel-24"));
		QCOMPARE(dialog.record().value(QStringLiteral("manufacturer")), QStringLiteral("Acme"));

		const int steps = folio->undoStack().count();
		QUndoCommand *undo = PlaceLabelPartDialog::apply(symbol, QStringLiteral("K5"), dialog.record());
		QVERIFY(undo);
		folio->undoStack().push(undo);
		QCOMPARE(folio->undoStack().count(), steps + 1);
		QCOMPARE(info(symbol, QStringLiteral("label")), QStringLiteral("K5"));
		QCOMPARE(info(symbol, QStringLiteral("manufacturer")), QStringLiteral("Acme"));
		QCOMPARE(info(symbol, QStringLiteral("manufacturer_reference")), QStringLiteral("AC-REL-24"));
		QCOMPARE(info(symbol, QStringLiteral("description")), QStringLiteral("Power relay 24 V"));
		QCOMPARE(info(symbol, QStringLiteral("device_type")), QStringLiteral("PLACE-RELAY"));
		QCOMPARE(project.deviceType(QStringLiteral("PLACE-RELAY")), library_type);

		folio->undoStack().undo();
		QVERIFY(info(symbol, QStringLiteral("label")) != QStringLiteral("K5"));
		QVERIFY(info(symbol, QStringLiteral("manufacturer")).isEmpty());
		QVERIFY(info(symbol, QStringLiteral("device_type")).isEmpty());
		QVERIFY(project.deviceType(QStringLiteral("PLACE-RELAY")).isNull());
		folio->undoStack().redo();
		QCOMPARE(info(symbol, QStringLiteral("device_type")), QStringLiteral("PLACE-RELAY"));
		QVERIFY(DeviceTypeLibrary::remove(library_type.name));
	}

	void aPartTakesItsPinsFromTheDeviceType()
	{
		QETProject project;
		Diagram *folio = project.addNewDiagram();
		const QString four = defineTerminals(project, QStringLiteral("socket4"), 4);
		const QString one = defineTerminals(project, QStringLiteral("conn1"), 1);
		project.setDeviceType(socket(four, one));
		const QStringList head_pins{QStringLiteral("2"), QStringLiteral("3"), QStringLiteral("PE"), QStringLiteral("1")};
		const QStringList unnamed4{QString(), QString(), QString(), QString()};
		const QStringList unnamed1{QString()};

		Element *head = place(project, folio, four, QString());
		QVERIFY(head);
		MaterialRecord part;
		part.setValue(QStringLiteral("device_type"), QStringLiteral("SOCKET"));
		const int steps = folio->undoStack().count();
		bool none = true;
		Element *named = PlaceLabelPartDialog::applyAndPush(head, QStringLiteral("-XS1"), part, &none);
		QVERIFY(named && named != head && !none);
		QCOMPARE(folio->undoStack().count(), steps + 1);
		QCOMPARE(pinsOf(named), head_pins);
		QCOMPARE(info(named, QStringLiteral("label")), QStringLiteral("-XS1"));
		QCOMPARE(info(named, QStringLiteral("device_type")), QStringLiteral("SOCKET"));

			//Without a part, the label of that device gives the type and the next free part
		QList<Element *> children, originals;
		const QStringList expected{QStringLiteral("C1"), QStringLiteral("C2")};
		for (const QString &pin : expected) {
			Element *child = place(project, folio, one, QString());
			QVERIFY(child);
			originals << child;
			Element *took = PlaceLabelPartDialog::applyAndPush(child, QStringLiteral("-XS1"), MaterialRecord(), &none);
			QVERIFY(took && !none);
			QCOMPARE(pinsOf(took), QStringList({pin}));
			QCOMPARE(info(took, QStringLiteral("device_type")), QStringLiteral("SOCKET"));
			children << took;
		}
			//A third: no part is free
		Element *third = place(project, folio, one, QString());
		QVERIFY(third);
		Element *left = PlaceLabelPartDialog::applyAndPush(third, QStringLiteral("-XS1"), MaterialRecord(), &none);
		QVERIFY(none);
		QCOMPARE(pinsOf(left), unnamed1);
		QCOMPARE(info(left, QStringLiteral("label")), QStringLiteral("-XS1"));

			//Each step undone gives the unnamed symbol back
		folio->undoStack().undo();
		QVERIFY(folio->elements().contains(third));
		QVERIFY(info(third, QStringLiteral("label")).isEmpty());
		QVERIFY(info(third, QStringLiteral("device_type")).isEmpty());
		for (int i = 1 ; i >= 0 ; --i) {
			folio->undoStack().undo();
			QVERIFY(folio->elements().contains(originals.at(i)));
			QVERIFY(!folio->elements().contains(children.at(i)));
			QCOMPARE(pinsOf(originals.at(i)), unnamed1);
			QVERIFY(info(originals.at(i), QStringLiteral("label")).isEmpty());
		}
		folio->undoStack().undo();
		QVERIFY(folio->elements().contains(head));
		QCOMPARE(pinsOf(head), unnamed4);
		QVERIFY(info(head, QStringLiteral("device_type")).isEmpty());
		QVERIFY(info(head, QStringLiteral("label")).isEmpty());
		folio->undoStack().redo();
		QVERIFY(theOne(folio, head_pins));
	}

	void aChildTakesTheWholeIdOfItsDevice()
	{
		QETProject project;
		Diagram *a = project.addNewDiagram();
		Diagram *b = project.addNewDiagram();
		for (Diagram *folio : {a, b}) {
			TitleBlockProperties tb = folio->border_and_titleblock.exportTitleBlock();
			tb.plant = QStringLiteral("PS1");
			tb.locmach = QStringLiteral("CP1");
			folio->border_and_titleblock.importTitleBlock(tb);
		}
		auto *box = new StructureBoxItem(QPointF(0, 0), QPointF(400, 400));
		box->setLocation(QStringLiteral("F1"));
		a->addItem(box);
		const QString four = defineTerminals(project, QStringLiteral("socket4"), 4);
		const QString one = defineTerminals(project, QStringLiteral("conn1"), 1);
		project.setDeviceType(socket(four, one));

		Element *head = place(project, a, four, QString());
		QVERIFY(head);
		head->setPos(100, 100);
		MaterialRecord part;
		part.setValue(QStringLiteral("device_type"), QStringLiteral("SOCKET"));
		head = PlaceLabelPartDialog::applyAndPush(head, QStringLiteral("-XS1"), part);

		for (const QString &pin : {QStringLiteral("C1"), QStringLiteral("C2")}) {
			Element *child = place(project, b, one, QString());
			QVERIFY(child);
			QCOMPARE(PlaceLabelPartDialog::placeToJoin(child, QStringLiteral("-XS1")), QStringLiteral("=PS1+F1"));
			child = PlaceLabelPartDialog::applyAndPush(child, QStringLiteral("-XS1"), MaterialRecord());
			QCOMPARE(pinsOf(child), QStringList({pin}));
			QCOMPARE(DuplicateLabels::place(child), DuplicateLabels::place(head));
			QVERIFY(PlaceLabelPartDialog::placeToJoin(child, QStringLiteral("-XS1")).isEmpty());
		}
		int found = 0;
		for (const DeviceTypeCheck::Device &device : DeviceTypeCheck::check(&project)) {
			++found;
			QVERIFY(device.complete());
		}
		QCOMPARE(found, 1);
	}

	void cancelLeavesTheSymbolAsPlaced()
	{
		QETProject project;
		Diagram *folio = project.addNewDiagram();
		const QString coil = define(project, QStringLiteral("coil"));
		Element *symbol = place(project, folio, coil, QStringLiteral("K"));
		QVERIFY(symbol);
		const DiagramContext before = symbol->elementInformations();
		const int steps = folio->undoStack().count();
			//Nothing typed, no part: no step at all
		QVERIFY(!PlaceLabelPartDialog::apply(symbol, QString(), MaterialRecord()));
		QCOMPARE(folio->undoStack().count(), steps);
		QCOMPARE(symbol->elementInformations(), before);
	}

	void theDeviceTypeQuestionIsNotAskedWhenThePartBroughtOne()
	{
		QETProject project;
		project.setFollowDeviceTypes(true);
		Diagram *folio = project.addNewDiagram();
		const QString coil = define(project, QStringLiteral("coil"));
		const DeviceType type = headedBy(QStringLiteral("PLACE-OWN"), coil);
		project.setDeviceType(type);
		Element *symbol = place(project, folio, coil, QStringLiteral("K"));
		QVERIFY(symbol);
			//No part, or a part with no type: still asked
		QVERIFY(PlaceLabelPartDialog::asksDeviceType(symbol));
		MaterialRecord plain;
		plain.setValue(QStringLiteral("manufacturer"), QStringLiteral("Acme"));
		folio->undoStack().push(PlaceLabelPartDialog::apply(symbol, QStringLiteral("K1"), plain));
		QVERIFY(PlaceLabelPartDialog::asksDeviceType(symbol));
			//A part naming the type of the project: not asked
		MaterialRecord typed;
		typed.setValue(QStringLiteral("device_type"), type.name);
		folio->undoStack().push(PlaceLabelPartDialog::apply(symbol, QString(), typed));
		QVERIFY(!PlaceLabelPartDialog::asksDeviceType(symbol));
	}
};

QET_APP_TEST_MAIN(tst_placelabelpart)
#include "tst_placelabelpart.moc"
