// SPDX-License-Identifier: GPL-2.0-or-later
#include "qetapptest.h"

#include "diagramcontent.h"
#include "factory/propertieseditorfactory.h"
#include "qetgraphicsitem/conductor.h"
#include "qetgraphicsitem/dynamicelementtextitem.h"
#include "qetgraphicsitem/element.h"
#include "qetinformation.h"
#include "projectview.h"
#include "qetproject.h"
#include "ui/elementinfopartwidget.h"
#include "ui/elementinfowidget.h"
#include "ui/elementpropertieswidget.h"
#include "ui/multielementinfowidget.h"
#include "undocommand/changeelementinformationcommand.h"

#include <QCheckBox>
#include <QTabWidget>
#include <QUndoStack>

// Editing the information of several elements at once (#1439): a field left
// alone keeps each element's own value, a typed text goes to every element,
// the erase box clears the field on every element, the restore button and
// reset() bring back what the elements had, and one apply is one undo step.
class tst_multielementinfo : public QObject
{
	Q_OBJECT

	QTemporaryDir m_dir;
	Diagram *m_diagram = nullptr;
	QList<Element *> m_elements;
	Conductor *m_conductor = nullptr;

	QString info(Element *e, const QString &key) const
	{
		return e->elementInformations().value(key).toString();
	}

	QUndoStack &stack() { return m_diagram->undoStack(); }

		// The three test elements get known information, as one undo step
		// the test then leaves on the stack
	void setKnownInfo()
	{
		QMap<QPointer<Element>, QPair<DiagramContext, DiagramContext>> map;
		const QStringList suppliers {"S1", "S2", ""};
		for (int i = 0 ; i < 3 ; ++i)
		{
			Element *e = m_elements.at(i);
			DiagramContext info;
			info.addValue(QETInformation::ELMT_MANUFACTURER, "Maker");
			if (!suppliers.at(i).isEmpty())
				info.addValue(QETInformation::ELMT_SUPPLIER, suppliers.at(i));
			info.addValue(QETInformation::ELMT_DESCRIPTION, QString("desc %1").arg(i));
			info.addValue("my_custom_key", QString("custom %1").arg(i));
			if (i == 0)
				info.addValue("exclude_from_bom", "true");
			map.insert(e, qMakePair(e->elementInformations(), info));
		}
		stack().push(new ChangeElementInformationCommand(map));
	}

	private slots:
		void initTestCase()
		{
			QETDiagramEditor *editor = QetAppTest::editor();
			QVERIFY(editor);
				// The first example folio with three plain elements and a wire
			const QStringList files = QDir(QET_EXAMPLES_DIR).entryList({"*.qet"}, QDir::Files, QDir::Name);
			for (const QString &file : files)
			{
					// A copy: opening a project leaves a lock file beside it
				const QString copy = m_dir.filePath(file);
				QVERIFY(QFile::copy(QDir(QET_EXAMPLES_DIR).filePath(file), copy));
				QVERIFY(editor->openAndAddProject(copy, false));
				QETProject *project = editor->currentProjectView()->project();
				for (Diagram *d : project->diagrams())
				{
					QList<Element *> simple;
					Conductor *conductor = nullptr;
					for (QGraphicsItem *qgi : d->items())
					{
						if (qgi->type() == Element::Type
								&& static_cast<Element *>(qgi)->linkType() == Element::Simple)
							simple << static_cast<Element *>(qgi);
						else if (!conductor && qgi->type() == Conductor::Type)
							conductor = static_cast<Conductor *>(qgi);
					}
					if (simple.size() >= 3 && conductor)
					{
						m_diagram = d;
						m_elements = simple.mid(0, 3);
						m_conductor = conductor;
						break;
					}
				}
				if (m_diagram)
					break;
			}
			QVERIFY2(m_diagram, "no example folio with three plain elements and a wire");
			setKnownInfo();
		}

		void init()
		{
			stack().setClean();
		}

		void cleanup()
		{
				// Back to the known information
			while (stack().index() > stack().cleanIndex())
				stack().undo();
		}

		void showsSharedAndMixedValues()
		{
			MultiElementInfoWidget w(m_elements);
			QCOMPARE(w.fieldIsMixed(QETInformation::ELMT_MANUFACTURER), false);
			QCOMPARE(w.fieldIsMixed(QETInformation::ELMT_SUPPLIER), true);
			QCOMPARE(w.fieldIsMixed(QETInformation::ELMT_DESCRIPTION), true);
				// No element has a value: same (empty) value, not mixed
			QCOMPARE(w.fieldIsMixed(QETInformation::ELMT_QUANTITY), false);
			QVERIFY(!w.associatedUndo());
		}

		void setGivesTextToEveryElementAsOneUndo()
		{
			MultiElementInfoWidget w(m_elements);
			w.setFieldText(QETInformation::ELMT_SUPPLIER, "ACME");
			QCOMPARE(w.fieldState(QETInformation::ELMT_SUPPLIER), MultiElementInfoWidget::Set);
			const int before = stack().index();
			w.apply();
			QCOMPARE(stack().index(), before + 1);
			for (Element *e : m_elements)
			{
				QCOMPARE(info(e, QETInformation::ELMT_SUPPLIER), QString("ACME"));
					// Untouched fields keep each element's own value
				QCOMPARE(info(e, QETInformation::ELMT_MANUFACTURER), QString("Maker"));
				QCOMPARE(info(e, "my_custom_key"),
						 QString("custom %1").arg(m_elements.indexOf(e)));
			}
			QCOMPARE(info(m_elements[0], QETInformation::ELMT_DESCRIPTION), QString("desc 0"));

			stack().undo();
			QCOMPARE(info(m_elements[0], QETInformation::ELMT_SUPPLIER), QString("S1"));
			QCOMPARE(info(m_elements[1], QETInformation::ELMT_SUPPLIER), QString("S2"));
			QVERIFY(!m_elements[2]->elementInformations().contains(QETInformation::ELMT_SUPPLIER));
		}

		void eraseClearsEveryElement()
		{
			MultiElementInfoWidget w(m_elements);
			w.setFieldCleared(QETInformation::ELMT_DESCRIPTION, true);
			QCOMPARE(w.fieldState(QETInformation::ELMT_DESCRIPTION), MultiElementInfoWidget::Clear);
			w.apply();
			for (Element *e : m_elements) {
				QVERIFY(!e->elementInformations().contains(QETInformation::ELMT_DESCRIPTION));
				QCOMPARE(info(e, QETInformation::ELMT_MANUFACTURER), QString("Maker"));
			}
		}

			// Emptying a field every element shares clears it, as in the
			// one-element editor
		void emptyingASharedValueClearsIt()
		{
			MultiElementInfoWidget w(m_elements);
			w.setFieldText(QETInformation::ELMT_MANUFACTURER, QString());
			QCOMPARE(w.fieldState(QETInformation::ELMT_MANUFACTURER), MultiElementInfoWidget::Set);
			w.apply();
			for (Element *e : m_elements)
				QVERIFY(!e->elementInformations().contains(QETInformation::ELMT_MANUFACTURER));
		}

			// A mixed field emptied again, or a shared one typed back to its
			// value, changes nothing
		void typingBackToTheValueKeeps()
		{
			MultiElementInfoWidget w(m_elements);
			w.setFieldText(QETInformation::ELMT_SUPPLIER, "x");
			w.setFieldText(QETInformation::ELMT_SUPPLIER, QString());
			QCOMPARE(w.fieldState(QETInformation::ELMT_SUPPLIER), MultiElementInfoWidget::Keep);
			w.setFieldText(QETInformation::ELMT_MANUFACTURER, "Make");
			w.setFieldText(QETInformation::ELMT_MANUFACTURER, "Maker");
			QCOMPARE(w.fieldState(QETInformation::ELMT_MANUFACTURER), MultiElementInfoWidget::Keep);
			QVERIFY(!w.associatedUndo());
		}

		void restoreAndResetBringBackTheValues()
		{
			MultiElementInfoWidget w(m_elements);
			w.setFieldText(QETInformation::ELMT_SUPPLIER, "ACME");
			w.setFieldCleared(QETInformation::ELMT_DESCRIPTION, true);
			w.restoreField(QETInformation::ELMT_SUPPLIER);
			QCOMPARE(w.fieldState(QETInformation::ELMT_SUPPLIER), MultiElementInfoWidget::Keep);
			QCOMPARE(w.fieldState(QETInformation::ELMT_DESCRIPTION), MultiElementInfoWidget::Clear);
			w.reset();
			QCOMPARE(w.fieldState(QETInformation::ELMT_DESCRIPTION), MultiElementInfoWidget::Keep);
			QVERIFY(!w.associatedUndo());
		}

			// The selection panel applies each keystroke; they make one undo
			// step, and restoring the field in the panel undoes it
		void liveEditMergesAndRestores()
		{
			MultiElementInfoWidget w(m_elements);
			w.setLiveEdit(true);
			const int before = stack().index();
			w.setFieldText(QETInformation::ELMT_SUPPLIER, "A");
			w.setFieldText(QETInformation::ELMT_SUPPLIER, "AC");
			w.setFieldText(QETInformation::ELMT_SUPPLIER, "ACME");
			QCOMPARE(stack().index(), before + 1);
			for (Element *e : m_elements)
				QCOMPARE(info(e, QETInformation::ELMT_SUPPLIER), QString("ACME"));

			w.restoreField(QETInformation::ELMT_SUPPLIER);
			QCOMPARE(info(m_elements[0], QETInformation::ELMT_SUPPLIER), QString("S1"));
			QCOMPARE(info(m_elements[1], QETInformation::ELMT_SUPPLIER), QString("S2"));
			QVERIFY(!m_elements[2]->elementInformations().contains(QETInformation::ELMT_SUPPLIER));
		}

			// The first live change never joins an earlier change to the
			// same elements (here the paste-like command of initTestCase)
		void liveEditDoesNotJoinEarlierChange()
		{
			QMap<QPointer<Element>, QPair<DiagramContext, DiagramContext>> map;
			for (Element *e : m_elements) {
				DiagramContext info = e->elementInformations();
				info.addValue(QETInformation::ELMT_PLANT, "P1");
				map.insert(e, qMakePair(e->elementInformations(), info));
			}
			stack().push(new ChangeElementInformationCommand(map));
			const int before = stack().index();

			MultiElementInfoWidget w(m_elements);
			w.setLiveEdit(true);
			w.setFieldText(QETInformation::ELMT_SUPPLIER, "ACME");
			QCOMPARE(stack().index(), before + 1);
			stack().undo();
			for (Element *e : m_elements)
				QCOMPARE(info(e, QETInformation::ELMT_PLANT), QString("P1"));
		}

			// An undo from outside the editor shows again what the elements have
		void undoRefreshesTheEditor()
		{
			MultiElementInfoWidget w(m_elements);
			w.setLiveEdit(true);
			w.setFieldText(QETInformation::ELMT_SUPPLIER, "ACME");
			QCOMPARE(w.fieldState(QETInformation::ELMT_SUPPLIER), MultiElementInfoWidget::Set);
			stack().undo();
			QCoreApplication::processEvents();
			QCOMPARE(w.fieldIsMixed(QETInformation::ELMT_SUPPLIER), true);
			QCOMPARE(w.fieldState(QETInformation::ELMT_SUPPLIER), MultiElementInfoWidget::Keep);
		}

			// "Exclude from the bill of materials": partly checked when the
			// elements differ, a click sets it on every element
		void flagMixedThenSet()
		{
			MultiElementInfoWidget w(m_elements);
			QCheckBox *bom = nullptr;
			for (QCheckBox *cb : w.findChildren<QCheckBox *>())
				if (cb->text() == QObject::tr("Exclude from the bill of materials"))
					bom = cb;
			QVERIFY(bom);
			QCOMPARE(bom->checkState(), Qt::PartiallyChecked);
			bom->click();
			w.apply();
			for (Element *e : m_elements)
				QVERIFY(QET::infoFlagIsTrue(info(e, "exclude_from_bom")));
		}

		void formulaIdKeptOnlyWithItsFormula()
		{
			QMap<QPointer<Element>, QPair<DiagramContext, DiagramContext>> map;
			for (Element *e : m_elements)
			{
				DiagramContext info = e->elementInformations();
				info.addValue(QETInformation::ELMT_FORMULA, "K%{id}");
				info.addValue(QETInformation::ELMT_FORMULA_ID, "{00000000-0000-0000-0000-000000000001}", false);
				map.insert(e, qMakePair(e->elementInformations(), info));
			}
				// Element drops an id naming no numbering of the project;
				// what is checked is that the editor itself keeps or drops it
			stack().push(new ChangeElementInformationCommand(map));
			const bool stored = m_elements[0]->elementInformations().contains(QETInformation::ELMT_FORMULA_ID);

			MultiElementInfoWidget w(m_elements);
			w.setFieldText(QETInformation::ELMT_SUPPLIER, "ACME");
			w.apply();
			QCOMPARE(m_elements[0]->elementInformations().contains(QETInformation::ELMT_FORMULA_ID), stored);

			MultiElementInfoWidget w2(m_elements);
			w2.setFieldText(QETInformation::ELMT_FORMULA, "Q%{id}");
			w2.apply();
			for (Element *e : m_elements) {
				QVERIFY(!e->elementInformations().contains(QETInformation::ELMT_FORMULA_ID));
				QCOMPARE(info(e, QETInformation::ELMT_FORMULA), QString("Q%{id}"));
			}
		}

			// The selection panel: one element, several, and a mix
		void factoryPicksTheEditor()
		{
			QList<QGraphicsItem *> one {m_elements[0]};
			std::unique_ptr<PropertiesEditorWidget> e1(PropertiesEditorFactory::propertiesEditor(one, nullptr));
			QVERIFY(qobject_cast<ElementPropertiesWidget *>(e1.get()));

			QList<QGraphicsItem *> two {m_elements[0], m_elements[1]};
			std::unique_ptr<PropertiesEditorWidget> e2(PropertiesEditorFactory::propertiesEditor(two, nullptr));
			auto multi = qobject_cast<MultiElementInfoWidget *>(e2.get());
			QVERIFY(multi);
			QCOMPARE(multi->elements().size(), 2);

				// Same kind of editor: reused for the new selection
			QList<QGraphicsItem *> mix {m_elements[0], m_elements[1], m_elements[2], m_conductor};
			PropertiesEditorWidget *e3 = PropertiesEditorFactory::propertiesEditor(mix, e2.get());
			QCOMPARE(e3, e2.get());
			QCOMPARE(multi->elements().size(), 3);

				// Select all takes the elements' own texts: not other items
			QList<QGraphicsItem *> with_texts {m_elements[0], m_elements[1]};
			for (QGraphicsItem *child : m_elements[0]->childItems())
				if (child->type() == DynamicElementTextItem::Type)
					with_texts << child;
			QVERIFY(with_texts.size() > 2);
			QCOMPARE(MultiElementInfoWidget::withoutOwnTexts(with_texts).size(), 2);
				// A text alone, or with another element, stays
			QList<QGraphicsItem *> other_text {m_elements[1], with_texts.last()};
			QCOMPARE(MultiElementInfoWidget::withoutOwnTexts(other_text).size(), 2);

			QList<QGraphicsItem *> wire {m_conductor};
			std::unique_ptr<PropertiesEditorWidget> e4(PropertiesEditorFactory::propertiesEditor(wire, nullptr));
			QVERIFY(!qobject_cast<MultiElementInfoWidget *>(e4.get()));
		}

			// Edit (Ctrl+E) is offered for several elements, not for a mix
		void editActionForSeveralElements()
		{
			QETDiagramEditor *editor = QetAppTest::editor();
			QAction *edit = QetAppTest::action("edit_selected_element");
			QVERIFY(edit);

			m_diagram->clearSelection();
			m_elements[0]->setSelected(true);
			m_elements[1]->setSelected(true);
			editor->slot_updateComplexActions();
			QVERIFY(edit->isEnabled());

				// with their own texts, as select all gives them
			for (QGraphicsItem *child : m_elements[0]->childItems())
				child->setSelected(true);
			editor->slot_updateComplexActions();
			QVERIFY(edit->isEnabled());

			m_conductor->setSelected(true);
			editor->slot_updateComplexActions();
			QVERIFY(!edit->isEnabled());
			m_diagram->clearSelection();
		}

			// The selection panel builds a new editor when the selection goes
			// from one element to several and back: it opens on the tab
			// picked last there, not on the first one
		void panelKeepsTheTabPicked()
		{
			auto tabsOf = [](QWidget *w) { return w->findChild<QTabWidget *>(); };
			int general = -1;
			{
				ElementPropertiesWidget first(m_elements[0]);
				first.setLiveEdit(true);
				QTabWidget *tabs = tabsOf(&first);
				QVERIFY(tabs);
				for (int i = 0 ; i < tabs->count() ; ++i)
					if (tabs->tabText(i) == QObject::tr("General"))
						general = i;
				QVERIFY(general > 0);
				QVERIFY(tabs->currentIndex() != general);
				emit tabs->tabBarClicked(general);
				tabs->setCurrentIndex(general);
			}
			ElementPropertiesWidget second(m_elements[1]);
			second.setLiveEdit(true);
			QCOMPARE(tabsOf(&second)->currentIndex(), general);

				// A dialog (not live) still opens on the first tab
			ElementPropertiesWidget dialog(m_elements[1]);
			QCOMPARE(tabsOf(&dialog)->currentIndex(), 0);
		}

			// The Reset button of the one-element dialog brings back the
			// Information tab's values
		void oneElementResetRestores()
		{
			ElementInfoWidget w(m_elements[0]);
			ElementInfoPartWidget *part = nullptr;
			for (auto *p : w.findChildren<ElementInfoPartWidget *>())
				if (p->key() == QETInformation::ELMT_SUPPLIER)
					part = p;
			QVERIFY(part);
			part->setText("changed");
			QCOMPARE(w.currentInfo().value(QETInformation::ELMT_SUPPLIER).toString(), QString("changed"));
			w.reset();
			QCOMPARE(w.currentInfo().value(QETInformation::ELMT_SUPPLIER).toString(), QString("S1"));
		}
};

QET_APP_TEST_MAIN(tst_multielementinfo)

#include "tst_multielementinfo.moc"
