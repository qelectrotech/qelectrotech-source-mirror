// SPDX-License-Identifier: GPL-2.0-or-later
/*
	Redoing work for nothing is the delay a big project feels: a text is
	not written again when it is already what it says, and a clean or
	dirty turn of the undo stack is not a project information change, so
	it reaches neither the conductor texts nor the project tree. Both
	used to rebuild every text of every folio built, which cost seconds
	on a 121-folio project.
*/
#include "qetapptest.h"

#include "ElementsCollection/elementslocation.h"
#include "ElementsCollection/xmlelementcollection.h"
#include "diagram.h"
#include "factory/elementfactory.h"
#include "qetgraphicsitem/conductor.h"
#include "qetgraphicsitem/conductortextitem.h"
#include "qetgraphicsitem/element.h"
#include "qetgraphicsitem/independenttextitem.h"
#include "qetgraphicsitem/terminal.h"
#include "qetproject.h"

#include <QDomDocument>
#include <QSignalSpy>
#include <QTextDocument>
#include <QUndoStack>

class tst_textrefresh : public QObject
{
	Q_OBJECT

		/// The two symbols of a wire, their definition embedded the way
		/// tst_listtables makes its symbol
	static Conductor *wire(Diagram *diagram)
	{
		auto *collection = diagram->project()->embeddedElementCollection();
		const QString kind = QStringLiteral("corner");
		if (!collection->exist(QStringLiteral("import/") + kind + QStringLiteral(".elmt"))) {
			QDomDocument definition;
			if (!definition.setContent(QString(
					R"(<definition type="element" uuid="%1" width="20" height="40" hotspot_x="10" hotspot_y="20">
<names><name lang="en">Test corner</name></names>
<kindInformations><kindInformation name="type">simple</kindInformation></kindInformations>
<description><line x1="0" y1="-10" x2="0" y2="10"/>
<terminal x="0" y="-20" orientation="n" uuid="%2" name="A1"/><terminal x="0" y="20" orientation="s" uuid="%3" name="A2"/>
</description></definition>)")
					.arg(QUuid::createUuid().toString(), QUuid::createUuid().toString(),
						 QUuid::createUuid().toString())))
				return nullptr;
			if (!collection->addElementDefinition(QStringLiteral("import"), kind,
												  definition.documentElement()))
				return nullptr;
		}
		const ElementsLocation location(
			QStringLiteral("embed://import/") + kind + QStringLiteral(".elmt"),
			diagram->project());
		int state = 0;
		Element *a = ElementFactory::Instance()->createElement(location, nullptr, &state);
		Element *b = ElementFactory::Instance()->createElement(location, nullptr, &state);
		if (!a || !b) {
			return nullptr;
		}
		diagram->addItem(a);
		diagram->addItem(b);
		a->setPos(100, 100);
		b->setPos(300, 300);
		auto *conductor = new Conductor(a->terminals().first(), b->terminals().first());
		diagram->addItem(conductor);
		return conductor;
	}

private slots:
	void initTestCase() { QETProject::setBackupEnabled(false); }

		//The same text is drawn again as it is: the document behind it is
		//not laid out again
	void sameTextIsNotWrittenAgain()
	{
		IndependentTextItem text;
		text.setPlainText(QStringLiteral("unchanged"));
		QSignalSpy changed(text.document(), &QTextDocument::contentsChanged);
		text.setPlainText(QStringLiteral("unchanged"));
		QCOMPARE(changed.count(), 0);
		text.setPlainText(QStringLiteral("different"));
		QVERIFY(changed.count() > 0);
	}

		//A clean or dirty turn of the undo stack goes out as a modified
		//state, and as nothing else: no conductor text is rewritten
	void cleanAndDirtyAreNotProjectInformation()
	{
		QETProject project;
		Diagram *folio = project.addNewDiagram();
		Conductor *conductor = wire(folio);
		QVERIFY(conductor);
		QVERIFY(conductor->isValid());

		QSignalSpy information(&project, &QETProject::projectInformationsChanged);
		QSignalSpy modified(&project, &QETProject::projectModified);
		QSignalSpy text_rewritten(conductor->textItem()->document(),
								  &QTextDocument::contentsChanged);

		project.undoStack()->setClean();
		modified.clear();
		project.undoStack()->push(new QUndoCommand);
		project.undoStack()->undo();
		QCOMPARE(information.count(), 0);
		QCOMPARE(modified.count(), 2);
		QCOMPARE(text_rewritten.count(), 0);

			//The spy sees a text that really changes
		ConductorProperties properties = conductor->properties();
		properties.text = QStringLiteral("new text");
		conductor->setProperties(properties);
		QVERIFY(text_rewritten.count() > 0);
	}
};

QET_APP_TEST_MAIN(tst_textrefresh)
#include "tst_textrefresh.moc"
