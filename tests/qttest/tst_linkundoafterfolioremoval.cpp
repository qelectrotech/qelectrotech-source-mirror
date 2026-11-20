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
// Deleting a folio and then linking one of the symbols that stood on it to
// another symbol must not cost that symbol its old partner when both steps
// are undone. Reproduced with examples/2612_ats_singlephase.qet: a normally
// closed K2 contact on sheet 3 is linked to the K2 coil on sheet 1, sheet 1
// is deleted, the contact is linked to K6, and the undo stack is wound back
// twice. Nothing is unlinked by a folio going away, so undo() still has the
// original link to give back.

#include "diagram.h"
#include "qetapptest.h"
#include "qetgraphicsitem/dynamicelementtextitem.h"
#include "qetgraphicsitem/element.h"
#include "qetgraphicsitem/elementtextitemgroup.h"
#include "qetproject.h"
#include "qetresult.h"
#include "undocommand/linkelementcommand.h"
#include "undocommand/removediagramcommand.h"

#include <QFile>
#include <memory>
#include <QtTest>

class tst_linkundoafterfolioremoval : public QObject
{
	Q_OBJECT

	QTemporaryDir m_dir;

	//Let the undo stack and the views settle, as the link tests do
	static void settle()
	{
		QApplication::processEvents();
		QApplication::processEvents();
	}

	//The symbol with @p uuid, whichever folio it stands on
	static Element *symbol(QETProject &project, const QString &uuid)
	{
		const QUuid id(uuid);
		for (Diagram *folio : project.diagrams())
			for (Element *element : folio->elements())
				if (element->uuid() == id)
					return element;
		return nullptr;
	}

	//The first thing @p element draws as a text: a report names its
	//partner with it
	static QString firstText(Element *element)
	{
		const auto texts = element->dynamicTextItems();
		return texts.isEmpty() ? QString() : texts.first()->toPlainText();
	}

	//The reports of the fixture: @p outgoing is the one on the first
	//folio with a partner, @p partner the other end of it, @p spare the
	//one on the first folio nothing is linked to
	static void fixtureReports(QETProject &project, Element **outgoing,
							   Element **partner, Element **spare)
	{
		*outgoing = *partner = *spare = nullptr;
		Diagram *first = project.diagrams().first();
		for (Diagram *folio : project.diagrams())
			for (Element *element : folio->elements()) {
				if (!(element->linkType() & Element::AllReport)) continue;
				if (folio == first)
					(element->linkedElements().isEmpty() ? *spare : *outgoing) = element;
				else if (!element->linkedElements().isEmpty())
					*partner = element;
			}
	}

	//Copy the fixture into the temporary folder: opening a project
	//writes its database beside it, and the repository's fixtures are
	//not ours to write to
	static QString copiedFixture(QTemporaryDir &dir, const QString &name)
	{
		const QString source = QFINDTESTDATA("fixtures/linked_reports.qet");
		const QString path = dir.filePath(name);
		if (!QFile::exists(path))
			QFile::copy(source, path);
		return path;
	}

	//What the contact draws as its label: it has one text reading an
	//element information, and for a contact that information comes from
	//the master (the contact's own label is empty)
	static QString shownLabel(Element *element)
	{
		for (DynamicElementTextItem *text : element->dynamicTextItems())
			if (text->textFrom() == DynamicElementTextItem::ElementInfo)
				return text->toPlainText();
		return QString();
	}

	//A copy of examples/2612_ats_singlephase.qet in the temporary
	//folder, for the same reason as copiedFixture()
	QString copiedExample(const QString &name)
	{
		const QString path = m_dir.filePath(name);
		if (!QFile::exists(path))
			QFile::copy(QStringLiteral(QET_EXAMPLES_DIR
						   "/2612_ats_singlephase.qet"),
				    path);
		return path;
	}

	//What a contact draws for its master, from its own texts and from
	//its text groups: "(none)" when it draws nothing
	static QString contactReference(Element *contact)
	{
		QStringList shown;
		for (DynamicElementTextItem *text : contact->dynamicTextItems())
			if (text->slaveXrefItem())
				shown << text->slaveXrefItem()->toPlainText();
		for (ElementTextItemGroup *group : contact->textGroups())
			if (group->slaveXrefItem())
				shown << group->slaveXrefItem()->toPlainText();
		return shown.isEmpty() ? QStringLiteral("(none)")
				       : shown.join(QLatin1Char('|'));
	}

	//How many functions run when @p project sends @p signal.
	//QObject::receivers() is protected: a class that makes it public
	//gives a pointer to it that any QObject can be called with.
	struct ReceiversAccess : QObject
	{
		using QObject::receivers;
	};
	static int registered(QETProject &project, const char *signal)
	{
		int (QObject::*count)(const char *) const =
				&ReceiversAccess::receivers;
		return (project.*count)(signal);
	}

private slots:
	void initTestCase()
	{
		QETProject::setBackupEnabled(false);
	}

	void undoTwiceKeepsTheContactOnItsCoil()
	{
		//The example is opened from a copy: opening a project writes
		//its database beside it, and the repository's examples are
		//not ours to write to
		const QString source =
				QStringLiteral(QET_EXAMPLES_DIR "/2612_ats_singlephase.qet");
		QVERIFY2(QFile::exists(source), "the example project is missing");
		const QString path =
				m_dir.filePath(QStringLiteral("2612_ats_singlephase.qet"));
		QVERIFY(QFile::copy(source, path));

		QETProject project(path);
		QCOMPARE(project.state(), QETProject::Ok);

		//A normally closed K2 contact on sheet 3, the K2 coil on
		//sheet 1 and the K6 coil on sheet 3, as the example holds
		//them
		Element *contact = symbol(
				project,
				QStringLiteral("{249c6e27-152a-44b6-9ef4-2291a7d78d41}"));
		Element *k2 = symbol(
				project,
				QStringLiteral("{23c999bc-c56d-4c0e-8135-98fae2085421}"));
		Element *k6 = symbol(
				project,
				QStringLiteral("{52f5a878-7eed-4bbf-8a30-9ba3b85e8eb9}"));
		QVERIFY(contact);
		QVERIFY(k2);
		QVERIFY(k6);
		QCOMPARE(contact->linkType(), Element::Slave);
		QVERIFY2(contact->linkedElements().contains(k2),
				 "the example has to start with the contact on K2");

		Diagram *sheet1 = k2->diagram();
		QVERIFY(sheet1);

		//Sheet 1 goes, the way the Delete sheet command takes it out
		project.undoStack()->push(new RemoveDiagramCommand(&project, sheet1));
		settle();
		QVERIFY2(!project.diagrams().contains(sheet1), "sheet 1 has to be gone");

		//The contact takes another coil while its own partner's
		//folio is away
		auto *link = new LinkElementCommand(contact);
		link->setLink(k6);
		project.undoStack()->push(link);
		settle();

		//Both steps back: the new link, then the deletion
		project.undoStack()->undo();
		project.undoStack()->undo();
		settle();

		QVERIFY2(project.diagrams().contains(sheet1),
				 "undo has to bring sheet 1 back");
		QVERIFY2(contact->linkedElements().contains(k2),
				 "undo left the contact without the coil it was linked to");
	}

	//The same two steps, watched on the sheet: the contact draws its
	//label from the master, and after undo() it has to draw it again.
	//The link was back already; what stayed away was the text.
	void labelComesBackWithTheContact()
	{
		const QString source =
				QStringLiteral(QET_EXAMPLES_DIR "/2612_ats_singlephase.qet");
		QVERIFY2(QFile::exists(source), "the example project is missing");
		const QString path =
				m_dir.filePath(QStringLiteral("label_comes_back.qet"));
		QVERIFY(QFile::copy(source, path));

		QETProject project(path);
		QCOMPARE(project.state(), QETProject::Ok);

		Element *contact = symbol(
				project,
				QStringLiteral("{249c6e27-152a-44b6-9ef4-2291a7d78d41}"));
		Element *k2 = symbol(
				project,
				QStringLiteral("{23c999bc-c56d-4c0e-8135-98fae2085421}"));
		Element *k6 = symbol(
				project,
				QStringLiteral("{52f5a878-7eed-4bbf-8a30-9ba3b85e8eb9}"));
		QVERIFY(contact);
		QVERIFY(k2);
		QVERIFY(k6);
		QCOMPARE(shownLabel(contact), QStringLiteral("K2"));

		Diagram *sheet1 = k2->diagram();
		QVERIFY(sheet1);
		project.undoStack()->push(new RemoveDiagramCommand(&project, sheet1));
		settle();

		auto *link = new LinkElementCommand(contact);
		link->setLink(k6);
		project.undoStack()->push(link);
		settle();
		QCOMPARE(shownLabel(contact), QStringLiteral("K6"));

		project.undoStack()->undo();
		project.undoStack()->undo();
		settle();

		QVERIFY2(contact->linkedElements().contains(k2),
				 "undo left the contact without the coil it was linked to");
		QVERIFY2(shownLabel(contact) == QStringLiteral("K2"),
				 qPrintable(QStringLiteral(
					 "the contact draws \"%1\" after its folio came back")
							.arg(shownLabel(contact))));
	}

	//A folio report whose partner's folio is deleted: taking the link
	//apart and undoing it has to put the two reports back together, the
	//way it does for a contact and its master.
	void reportLinkComesBackAfterUnlinkUndo()
	{
		QETProject project(copiedFixture(m_dir, QStringLiteral("unlink.qet")));
		QCOMPARE(project.state(), QETProject::Ok);

		Element *outgoing = nullptr;
		Element *partner = nullptr;
		Element *spare = nullptr;
		fixtureReports(project, &outgoing, &partner, &spare);
		QVERIFY(outgoing);
		QVERIFY(partner);
		QVERIFY2(outgoing->linkedElements().contains(partner),
				 "the fixture has to start with the two linked");

		auto *unlink = new LinkElementCommand(outgoing);
		unlink->unlinkAll();
		project.undoStack()->push(unlink);
		settle();
		QVERIFY2(outgoing->linkedElements().isEmpty(),
				 "Unlink did not take the reports apart");

		project.undoStack()->undo();
		settle();
		QVERIFY2(outgoing->linkedElements().contains(partner),
				 "undo did not put the report back on its partner");
	}

	//The same reports watched on the sheet: the partner's folio goes
	//away and comes back, and the text naming it goes and comes with it.
	void reportTextComesBackWithItsFolio()
	{
		QETProject project(copiedFixture(m_dir, QStringLiteral("text.qet")));
		QCOMPARE(project.state(), QETProject::Ok);

		Element *outgoing = nullptr;
		Element *partner = nullptr;
		Element *spare = nullptr;
		fixtureReports(project, &outgoing, &partner, &spare);
		QVERIFY(outgoing);
		QVERIFY(partner);
		const QString before = firstText(outgoing);
		QVERIFY2(!before.isEmpty(), "the report draws nothing to begin with");

		Diagram *other = partner->diagram();
		QVERIFY(other);
		project.undoStack()->push(new RemoveDiagramCommand(&project, other));
		settle();
		QVERIFY2(!outgoing->linkedElements().contains(partner),
				 "the report still lists a partner whose folio is gone");

		project.undoStack()->undo();
		settle();
		QVERIFY2(outgoing->linkedElements().contains(partner),
				 "undo did not bring the partner back to the report");
		QVERIFY2(firstText(outgoing) == before,
				 qPrintable(QStringLiteral(
					 "the report draws \"%1\" now, it drew \"%2\" before")
							.arg(firstText(outgoing), before)));
	}
	//A contact draws where its master stands. Deleting the master's
	//sheet takes that reference away and undo() brings it back. A
	//contact whose master stays is renumbered both ways.
	void contactReferencesComeBackWithTheSheet()
	{
		QETProject project(copiedExample(QStringLiteral("references.qet")));
		QCOMPARE(project.state(), QETProject::Ok);

		//The normally closed K2 contact on sheet 3 and its coil on
		//sheet 1, and a contact on sheet 3 whose master is on sheet 3
		Element *away = symbol(
				project,
				QStringLiteral("{249c6e27-152a-44b6-9ef4-2291a7d78d41}"));
		Element *k2 = symbol(
				project,
				QStringLiteral("{23c999bc-c56d-4c0e-8135-98fae2085421}"));
		Element *master = symbol(
				project,
				QStringLiteral("{205bc729-7e88-4c83-9867-23b8fd4c42c9}"));
		QVERIFY(away);
		QVERIFY(k2);
		QVERIFY(master);
		Element *staying = nullptr;
		for (Element *element : away->diagram()->elements())
			if (element->linkType() == Element::Slave
					&& element->linkedElements().contains(master))
				staying = element;
		QVERIFY(staying);
		QCOMPARE(contactReference(away), QStringLiteral("(1-I13)"));
		QCOMPARE(contactReference(staying), QStringLiteral("(3-H21)"));

		project.undoStack()->push(
				new RemoveDiagramCommand(&project, k2->diagram()));
		settle();
		QCOMPARE(contactReference(away), QStringLiteral("(none)"));
		QCOMPARE(contactReference(staying), QStringLiteral("(2-H21)"));

		project.undoStack()->undo();
		settle();
		QCOMPARE(contactReference(away), QStringLiteral("(1-I13)"));
		QCOMPARE(contactReference(staying), QStringLiteral("(3-H21)"));
	}

	void groupedContactReferenceComesBackWithTheSheet_data()
	{
		QTest::addColumn<bool>("from_file");
		QTest::newRow("group made now") << false;
		QTest::newRow("group read from the file") << true;
	}

	//The same contact with its label in a text group: the group draws
	//the reference then, and it has to come back too, also for a group
	//read from a saved project
	void groupedContactReferenceComesBackWithTheSheet()
	{
		QFETCH(bool, from_file);
		const QString contact_uuid =
				QStringLiteral("{249c6e27-152a-44b6-9ef4-2291a7d78d41}");
		const QString path = copiedExample(from_file
				? QStringLiteral("group_from_file.qet")
				: QStringLiteral("group_made_now.qet"));

		auto project = std::make_unique<QETProject>(path);
		QCOMPARE(project->state(), QETProject::Ok);
		Element *contact = symbol(*project, contact_uuid);
		QVERIFY(contact);
		DynamicElementTextItem *label = nullptr;
		for (DynamicElementTextItem *text : contact->dynamicTextItems())
			if (text->textFrom() == DynamicElementTextItem::ElementInfo
					&& text->infoName() == QLatin1String("label"))
				label = text;
		QVERIFY2(label, "the contact has no label text");
		ElementTextItemGroup *group =
				contact->addTextGroup(QStringLiteral("label"));
		QVERIFY(group);
		QVERIFY(contact->addTextToGroup(label, group));
		settle();
		QCOMPARE(contactReference(contact), QStringLiteral("(1-I13)"));

		if (from_file) {
			const QETResult written = project->write();
			QVERIFY2(written.isOk(), qPrintable(written.errorMessage()));
			project = std::make_unique<QETProject>(path);
			QCOMPARE(project->state(), QETProject::Ok);
			contact = symbol(*project, contact_uuid);
			QVERIFY(contact);
			QVERIFY2(contact->textGroup(QStringLiteral("label")),
				 "the group was not read back");
			settle();
			QCOMPARE(contactReference(contact), QStringLiteral("(1-I13)"));
		}

		Element *k2 = symbol(
				*project,
				QStringLiteral("{23c999bc-c56d-4c0e-8135-98fae2085421}"));
		QVERIFY(k2);
		project->undoStack()->push(
				new RemoveDiagramCommand(project.get(), k2->diagram()));
		settle();
		QCOMPARE(contactReference(contact), QStringLiteral("(none)"));

		project->undoStack()->undo();
		settle();
		QCOMPARE(contactReference(contact), QStringLiteral("(1-I13)"));
	}

	//Every sheet added or deleted runs reportChanged() on each report
	//text, which registers updateReportText() for the formula. It has
	//to remove what it registered before: else, after ten sheet
	//changes, the next one runs updateReportText() ten times per text.
	void sheetChangesLeaveTheRegistrationsAsTheyWere()
	{
		QETProject project(copiedFixture(m_dir, QStringLiteral("cycles.qet")));
		QCOMPARE(project.state(), QETProject::Ok);
		Element *outgoing = nullptr;
		Element *partner = nullptr;
		Element *spare = nullptr;
		fixtureReports(project, &outgoing, &partner, &spare);
		QVERIFY(outgoing);
		QVERIFY(partner);

		Diagram *empty = project.addNewDiagram();
		settle();
		const char *added = SIGNAL(diagramAdded(QETProject*,Diagram*));
		const char *removed = SIGNAL(diagramRemoved(QETProject*,Diagram*));
		const int added_before = registered(project, added);
		const int removed_before = registered(project, removed);

		project.undoStack()->push(new RemoveDiagramCommand(&project, empty));
		settle();
		for (int i = 0 ; i < 10 ; ++i) {
			project.undoStack()->undo();
			settle();
			project.undoStack()->redo();
			settle();
		}
		project.undoStack()->undo();
		settle();
		QVERIFY(project.diagrams().contains(empty));

		QCOMPARE(registered(project, added), added_before);
		QCOMPARE(registered(project, removed), removed_before);
	}
};

QET_APP_TEST_MAIN(tst_linkundoafterfolioremoval)
#include "tst_linkundoafterfolioremoval.moc"
