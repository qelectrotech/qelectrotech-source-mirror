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
/*
	A folio copied with a numbering which counts per folio -- the
	"Zahlenformat 1 - Seite" of the numbering rules, a unitfolio part -- is
	numbered 1, 2, 3 like the page it was made from: the copy is a folio of
	its own, so the rule starts it where a folio starts.

	What it did was number the copies from the project's counter, which
	holds wherever the last folio worked on had left it: a copy came out
	4, 5, 6 or 5, 6, 7, and the only way out was to take the numbering off,
	put the counter back and apply it again by hand, every time.

	Second, a folio's own counter is the numbers that folio has handed out,
	not the highest it ever handed out, so numbering it over from the start
	or putting its counter back by hand is what the next numbering on it
	reads -- instead of the old high value coming back the next time the
	folio is opened.
*/
#include "qetapptest.h"

#include "ElementsCollection/elementslocation.h"
#include "ElementsCollection/xmlelementcollection.h"
#include "autoNum/elementautonumschemecommand.h"
#include "autoNum/numerotationcontext.h"
#include "autoNum/renumberelementscommand.h"
#include "diagram.h"
#include "elementspanel.h"
#include "elementspanelwidget.h"
#include "factory/elementfactory.h"
#include "projectview.h"
#include "qetgraphicsitem/element.h"
#include "qetmessagebox.h"
#include "qetproject.h"
#include "titleblockproperties.h"

#include <QDomDocument>
#include <QFile>
#include <QList>
#include <QMessageBox>
#include <QPointer>
#include <QSet>
#include <QStringList>
#include <QTemporaryDir>
#include <QTimer>
#include <QTreeWidgetItem>

#include <functional>

class TstFolioAutonumCopy : public QObject
{
	Q_OBJECT

	private:
		QTemporaryDir m_definitions;

		/// The numbering rule of the report: "-<folio><prefix><number>",
		/// one number per folio, every folio starting at 1.
		static QString addPerFolioRule(QETProject *project)
		{
			NumerotationContext rule;
			rule.addValue(QStringLiteral("string"), QStringLiteral("-"), 1);
			rule.addValue(QStringLiteral("folio"), QString(), 1);
			rule.addValue(QStringLiteral("elementprefix"), QString(), 1);
			rule.addValue(QStringLiteral("unitfolio"), 1, 1, 1);
			const QString title = QStringLiteral("Sensors");
			project->addElementAutoNum(title, rule, QUuid::createUuid());
			return title;
		}

		/// What the rule's counter holds for the type @p type
		static QString counterValue(const NumerotationContext &context,
									const QString &type)
		{
			for (int i = 0 ; i < context.size() ; ++i) {
				if (context.itemAt(i).value(0) == type) {
					return context.itemAt(i).value(1);
				}
			}
			return QString();
		}

		/// The number @p folio reads in its title block: what %F works out to
		static void setFolioNumber(Diagram *folio, const QString &number)
		{
			TitleBlockProperties properties =
					folio->border_and_titleblock.exportTitleBlock();
			properties.folio = number;
			folio->border_and_titleblock.importTitleBlock(properties);
		}

		/**
			One symbol a numbering gives a label to, on @p folio: a coil, the
			kind which takes a number from a scheme (a slave or a report would
			not). The definition is written once and reused, as the elements of
			a collection are.
		*/
		Element *symbol(Diagram *folio, const QString &prefix)
		{
			const QString path = m_definitions.filePath(
					QStringLiteral("numbered.elmt"));
			if (!QFile::exists(path)) {
				QFile file(path);
				if (!file.open(QIODevice::WriteOnly)) {
					return nullptr;
				}
				file.write(QStringLiteral(
	R"(<definition type="element" link_type="simple" uuid="%1" width="20" height="40" hotspot_x="10" hotspot_y="20">
<names><name lang="en">Numbered test symbol</name></names>
<kindInformations><kindInformation name="type">coil</kindInformation></kindInformations>
<description><line x1="0" y1="-10" x2="0" y2="10"/>
<terminal x="0" y="-20" orientation="n" uuid="%2"/><terminal x="0" y="20" orientation="s" uuid="%3"/>
</description></definition>)")
					.arg(QUuid::createUuid().toString(),
						 QUuid::createUuid().toString(),
						 QUuid::createUuid().toString()).toUtf8());
			}
			QETProject *project = folio->project();
			auto *collection = project->embeddedElementCollection();
			if (!collection->exist(QStringLiteral("import/numbered.elmt"))) {
				QFile file(path);
				QDomDocument definition;
				if (!file.open(QIODevice::ReadOnly)
						|| !definition.setContent(&file)) {
					return nullptr;
				}
				if (!collection->addElementDefinition(
						QStringLiteral("import"), QStringLiteral("numbered"),
						definition.documentElement())) {
					return nullptr;
				}
			}
			int state = 0;
			Element *element = ElementFactory::Instance()->createElement(
					ElementsLocation(QStringLiteral("embed://import/numbered.elmt"),
									 project),
					nullptr, &state);
			if (state || !element) {
				return nullptr;
			}
			folio->addItem(element);
			element->setPos(QPointF(100 + 80 * folio->content().m_elements.size(),
									150));
			element->setPrefix(prefix);
			return element;
		}

		/// The labels of every symbol of @p folio, sorted to compare sets
		static QStringList labelsOf(Diagram *folio)
		{
			QStringList labels;
			const QList<QGraphicsItem *> items = folio->items();
			for (QGraphicsItem *item : items) {
				if (auto *element = qgraphicsitem_cast<Element *>(item)) {
					labels << element->actualLabel();
				}
			}
			labels.sort();
			return labels;
		}

		/// The folio of @p project which was not among @p before: the one a
		/// duplication has just made
		static Diagram *folioNotIn(QETProject *project,
								  const QList<Diagram *> &before)
		{
			Diagram *made = nullptr;
			const QList<Diagram *> now = project->folios();
			for (Diagram *folio : now) {
				if (!before.contains(folio)) {
					made = folio;
				}
			}
			return made;
		}

		/// Give @p elements the numbering @p project has under @p title
		static bool number(QETProject *project, const QString &title,
						   const QVector<Element *> &elements, Diagram *folio)
		{
			int skipped = 0;
			RenumberElementsCommand *command =
					ElementAutoNumSchemeCommand::assign(project, title, elements,
														true, &skipped);
			if (!command) {
				return false;
			}
			folio->undoStack().push(command);
			return true;
		}

		/// Select the tree item of @p folio under @p project_item, as a click
		/// does, so that the panel reports it as selected.
		static bool selectFolio(ElementsPanel &panel,
							   QTreeWidgetItem *project_item, Diagram *folio)
		{
			const std::function<bool(QTreeWidgetItem *)> walk =
					[&](QTreeWidgetItem *item) {
				panel.clearSelection();
				item->setSelected(true);
				panel.setCurrentItem(item);
				if (panel.selectedDiagrams().contains(folio)) {
					return true;
				}
				for (int i = 0 ; i < item->childCount() ; ++i) {
					if (walk(item->child(i))) {
						return true;
					}
				}
				return false;
			};
			return walk(project_item);
		}

		/// Close @p project from the editor as the user does, discarding its
		/// changes if asked, and wait until it is deleted.
		static bool closeInEditor(QETProject *project)
		{
			if (!project) return true;
			QPointer<QETProject> closing(project);
			QTimer discard;
			QObject::connect(
				&discard, &QTimer::timeout, []()
				{
					auto *box = qobject_cast<QMessageBox *>(
						QApplication::activeModalWidget());
					if (auto *button =
							box ? box->button(QMessageBox::Discard) : nullptr)
						button->click();
				});
			discard.start(20);
			QetAppTest::editor()->closeProject(project);
			discard.stop();
			return QTest::qWaitFor([&closing]() { return closing.isNull(); });
		}

		QPointer<QETProject> m_open_project;

	private slots:
		void initTestCase()
		{
			QVERIFY(m_definitions.isValid());
			QETProject::setBackupEnabled(false);
			QET::QetMessageBox::setNonInteractive(true);
			// What the app does 250 ms after the project tree is first shown.
			auto *panel = QetAppTest::editor()->findChild<ElementsPanelWidget *>();
			QVERIFY(panel);
			panel->elementsPanel().reload();
		}

		void cleanup()
		{
			if (m_open_project) {
				QVERIFY(closeInEditor(m_open_project));
			}
		}

		void cleanupTestCase()
		{
			for (ProjectView *view : QetAppTest::editor()->openedProjects()) {
				QVERIFY(closeInEditor(view->project()));
			}
		}

		/*
			Copying the folio numbers the copies from their own folio: 1, 2, 3,
			the numbers the page they were made from carries, rather than the
			project's counter wherever it had got to.
		*/
		void copyStartsTheNumberingOfItsOwnFolio()
		{
			QETDiagramEditor *editor = QetAppTest::editor();
			QVERIFY(editor);
			auto *panel_widget = editor->findChild<ElementsPanelWidget *>();
			QVERIFY(panel_widget);
			ElementsPanel &panel = panel_widget->elementsPanel();

			QVERIFY(editor->newProject());
			m_open_project = editor->currentProjectView()->project();
			QETProject *project = m_open_project;
			QVERIFY(project);
			const QString title = addPerFolioRule(project);

			QVERIFY(!project->folios().isEmpty());
			Diagram *source = project->folios().first();
			setFolioNumber(source, QStringLiteral("42"));

			QVector<Element *> elements;
			for (int i = 0 ; i < 3 ; ++i) {
				Element *element = symbol(source, QStringLiteral("B"));
				QVERIFY2(element, "a symbol is placed");
				elements << element;
			}
			QVERIFY2(number(project, title, elements, source),
					 "the three symbols are given the numbering");
			QCOMPARE(labelsOf(source),
					 (QStringList{QStringLiteral("-42B1"),
								  QStringLiteral("-42B2"),
								  QStringLiteral("-42B3")}));

			// Copy the folio the way the project tree's "Copy and paste" does.
			QTreeWidgetItem *project_item = panel.itemForProject(project);
			QVERIFY(project_item);
			QVERIFY2(selectFolio(panel, project_item, source),
					 "the source folio is selected in the project tree");
			const QList<Diagram *> before = project->folios();
			panel_widget->duplicateDiagram();
			Diagram *copy = folioNotIn(project, before);
			QVERIFY2(copy, "the duplication made a folio of its own");

			// The copy is numbered from where a folio starts: 1, 2, 3. Its
			// labels read 42 because the copy still carries the folio number
			// of the page it was made from -- the numbers are its own, and
			// they follow the number the copy is given later on.
			QCOMPARE(labelsOf(copy),
					 (QStringList{QStringLiteral("-42B1"),
								  QStringLiteral("-42B2"),
								  QStringLiteral("-42B3")}));

			// A copy of the copy numbers 1, 2, 3 as well. It reads 42 like
			// the copy it was made from and like the page that one was made
			// from, and neither of them holds the numbers back: they are the
			// same page as far as %F sees it, until one of them gets a
			// folio number of its own.
			project_item = panel.itemForProject(project);
			QVERIFY(project_item);
			QVERIFY2(selectFolio(panel, project_item, copy),
					 "the first copy is selected in the project tree");
			const QList<Diagram *> before_second = project->folios();
			panel_widget->duplicateDiagram();
			Diagram *second_copy = folioNotIn(project, before_second);
			QVERIFY2(second_copy, "the second duplication made a folio too");
			QCOMPARE(labelsOf(second_copy),
					 (QStringList{QStringLiteral("-42B1"),
								  QStringLiteral("-42B2"),
								  QStringLiteral("-42B3")}));

			// The copy's own counter goes on from there: the next symbol on
			// it is 4, while the source folio still numbers from 1.
			Element *fourth = symbol(copy, QStringLiteral("B"));
			QVERIFY(fourth);
			QVERIFY2(number(project, title, {fourth}, copy),
					 "a fourth symbol on the copy is given a number");
			QCOMPARE(labelsOf(copy),
					 (QStringList{QStringLiteral("-42B1"),
								  QStringLiteral("-42B2"),
								  QStringLiteral("-42B3"),
								  QStringLiteral("-42B4")}));

			// The labels are worked out again for every display, so giving
			// the copy a folio number of its own moves them with it: the
			// numbers stay, and the two pages no longer read the same.
			setFolioNumber(copy, QStringLiteral("43"));
			QCOMPARE(labelsOf(copy),
					 (QStringList{QStringLiteral("-43B1"),
								  QStringLiteral("-43B2"),
								  QStringLiteral("-43B3"),
								  QStringLiteral("-43B4")}));
			QCOMPARE(labelsOf(source),
					 (QStringList{QStringLiteral("-42B1"),
								  QStringLiteral("-42B2"),
								  QStringLiteral("-42B3")}));
			QCOMPARE(labelsOf(second_copy),
					 (QStringList{QStringLiteral("-42B1"),
								  QStringLiteral("-42B2"),
								  QStringLiteral("-42B3")}));
		}

		/*
			A rule which counts over the whole project and whose label holds
			no folio number keeps every claim on its numbers: the copies of a
			page go on where the project's counter is and do not take the
			numbers of the page they were made from again. Nothing would tell
			-K1 on the copy from -K1 on that page -- the label never changes
			with the folio number.
		*/
		void schemeWithoutFolioNumberKeepsItsLabelsUnique()
		{
			QETDiagramEditor *editor = QetAppTest::editor();
			QVERIFY(editor);
			auto *panel_widget = editor->findChild<ElementsPanelWidget *>();
			QVERIFY(panel_widget);
			ElementsPanel &panel = panel_widget->elementsPanel();

			QVERIFY(editor->newProject());
			m_open_project = editor->currentProjectView()->project();
			QETProject *project = m_open_project;
			QVERIFY(project);

			// -K1: one counter over the whole project, no folio number in it
			NumerotationContext rule;
			rule.addValue(QStringLiteral("string"), QStringLiteral("-K"), 1);
			rule.addValue(QStringLiteral("unit"), 1, 1);
			const QString title = QStringLiteral("Contacts");
			project->addElementAutoNum(title, rule, QUuid::createUuid());

			QVERIFY(!project->folios().isEmpty());
			Diagram *source = project->folios().first();

			QVector<Element *> elements;
			for (int i = 0 ; i < 3 ; ++i) {
				Element *element = symbol(source, QStringLiteral("K"));
				QVERIFY2(element, "a symbol is placed");
				elements << element;
			}
			QVERIFY2(number(project, title, elements, source),
					 "the three symbols are given the numbering");
			QCOMPARE(labelsOf(source),
					 (QStringList{QStringLiteral("-K1"),
								  QStringLiteral("-K2"),
								  QStringLiteral("-K3")}));

			// The counter is put back to 1 the way a user puts it back: the
			// numbers the page holds stay taken by it.
			project->addElementAutoNum(title, rule);

			const QList<Diagram *> before = project->folios();
			QTreeWidgetItem *project_item = panel.itemForProject(project);
			QVERIFY(project_item);
			QVERIFY2(selectFolio(panel, project_item, source),
					 "the source folio is selected in the project tree");
			panel_widget->duplicateDiagram();
			Diagram *copy = folioNotIn(project, before);
			QVERIFY2(copy, "the duplication made a folio of its own");

			// The copies go on at 4: nothing in -K1 says which page it is
			// on, so -K1 twice would be two parts with the same name.
			QCOMPARE(labelsOf(copy),
					 (QStringList{QStringLiteral("-K4"),
								  QStringLiteral("-K5"),
								  QStringLiteral("-K6")}));
		}

		/*
			A folio numbered from the start again reads 1, 2, 3 afterwards, and
			its own counter reads 3: not 5, the highest it ever handed out,
			which would make the next numbering on it go on from 6.
		*/
		void renumberingPutsTheFoliosCounterBack()
		{
			QETProject project;
			const QString title = addPerFolioRule(&project);
			Diagram *folio = project.addNewDiagram();
			QVERIFY(folio);
			setFolioNumber(folio, QStringLiteral("7"));

			QVector<Element *> elements;
			for (int i = 0 ; i < 5 ; ++i) {
				Element *element = symbol(folio, QStringLiteral("B"));
				QVERIFY2(element, "a symbol is placed");
				elements << element;
			}
			QVERIFY2(number(&project, title, elements, folio),
					 "the five symbols are given the numbering");
			QCOMPARE(labelsOf(folio),
					 (QStringList{QStringLiteral("-7B1"),
								  QStringLiteral("-7B2"),
								  QStringLiteral("-7B3"),
								  QStringLiteral("-7B4"),
								  QStringLiteral("-7B5")}));

			// Two of them go, and the folio is numbered over from the start.
			for (int i = 3 ; i < 5 ; ++i) {
				folio->removeItem(elements.at(i));
				delete elements.at(i);
			}
			QVector<Element *> frozen;
			RenumberElementsCommand *command = ElementAutoNumSchemeCommand::renumber(
					&project, title, &frozen, QStringLiteral("Renumber"));
			QVERIFY(command);
			folio->undoStack().push(command);
			QCOMPARE(labelsOf(folio),
					 (QStringList{QStringLiteral("-7B1"),
								  QStringLiteral("-7B2"),
								  QStringLiteral("-7B3")}));

			// Opening the folio again reads its own counter: the next number
			// on it is 4, which is what the numbers it holds say.
			project.setCurrrentElementAutonum(title);
			folio->loadElmtFolioSeq();
			QCOMPARE(counterValue(project.elementAutoNum(title),
								  QStringLiteral("unitfolio")),
					 QStringLiteral("4"));
		}
};

QET_APP_TEST_MAIN(TstFolioAutonumCopy)

#include "tst_folioautonumcopy.moc"
