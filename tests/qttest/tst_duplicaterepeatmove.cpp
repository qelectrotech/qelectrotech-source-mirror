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
	Duplicate (Ctrl+D) repeating the move of the copy, a setting off by
	default: once the copy just made is moved, the next Duplicate puts a
	copy as far from it again; not moved, the spacing is the one set, as
	before. Ctrl+Shift+D asks for the spacing, then duplicates. Each press
	is one step of the undo stack.
*/
#include "qetapptest.h"
#include "ElementsCollection/xmlelementcollection.h"
#include "factory/elementfactory.h"
#include "qetgraphicsitem/element.h"
#include "qetproject.h"
#include "ui/duplicateoffsetdialog.h"

#include <QCheckBox>
#include <QComboBox>
#include <QKeyEvent>
#include <QSpinBox>
#include <QTimer>
#include <QUndoStack>

class tst_duplicaterepeatmove : public QObject
{
	Q_OBJECT

	private:
		DiagramView *m_view = nullptr;
		Diagram *m_diagram = nullptr;

		QAction *actionFor(const QKeySequence &shortcut)
		{
			for (QAction *a : QetAppTest::editor()->findChildren<QAction *>())
				if (a->shortcut() == shortcut)
					return a;
			return nullptr;
		}
		QAction *duplicateAction()
		{
			return actionFor(QKeySequence(Qt::CTRL | Qt::Key_D));
		}

			/// A new symbol at @p pos, alone selected
		Element *place(const QPointF &pos)
		{
			int state = 0;
			Element *element = ElementFactory::Instance()->createElement(
				ElementsLocation(QStringLiteral("embed://import/box.elmt"), m_diagram->project()),
				nullptr, &state);
			if (state || !element)
				return nullptr;
			m_diagram->addItem(element);
			element->setPos(pos);
			m_diagram->clearSelection();
			element->setSelected(true);
			return element;
		}

			/// The one symbol selected, which Duplicate has just made
		Element *selected()
		{
			Element *found = nullptr;
			for (QGraphicsItem *item : m_diagram->selectedItems()) {
				if (auto *e = dynamic_cast<Element *>(item)) {
					if (found) return nullptr;
					found = e;
				}
			}
			return found;
		}

			/// Press Ctrl+D; @return the copy it made
		Element *duplicate()
		{
			const int steps = m_diagram->undoStack().count();
			duplicateAction()->trigger();
			if (m_diagram->undoStack().count() != steps + 1)
				return nullptr; // one step of the undo stack per press
			return selected();
		}

		QPointF grid() const { return QPointF(Diagram::xGrid, Diagram::yGrid); }

	private slots:
		void initTestCase()
		{
			m_view = QetAppTest::view();
			QVERIFY(m_view);
			m_diagram = m_view->diagram();
			QDomDocument definition;
			definition.setContent(QStringLiteral(
				R"(<definition type="element" link_type="simple" width="20" height="20" hotspot_x="10" hotspot_y="10">
<names><name lang="en">box</name></names>
<description><line x1="0" y1="-5" x2="0" y2="5" style="line-style:normal;line-weight:normal;filling:none;color:black"/></description></definition>)"));
			QVERIFY(m_diagram->project()->embeddedElementCollection()->addElementDefinition(
				QStringLiteral("import"), QStringLiteral("box"), definition.documentElement()));
			QVERIFY(duplicateAction());
				//A spacing already confirmed: no dialog on the first press
			DuplicateOffsetDialog::saveStepOffset(QPoint(1, 0));
		}

			//Off by default, and nothing written while it is
		void offByDefault()
		{
			QVERIFY(!DuplicateOffsetDialog::savedRepeatsMove());
			DuplicateOffsetDialog dialog;
			QVERIFY(!dialog.repeatsMove());
			QVERIFY(!dialog.findChild<QCheckBox *>()->isChecked());
			dialog.save();
			QVERIFY(!QSettings().contains(QStringLiteral("diagrameditor/duplicate_repeats_move")));
		}

			//Off, a moved copy changes nothing: the spacing set, from it
		void offKeepsTheSpacing()
		{
			Element *original = place(QPointF(200, 200));
			QVERIFY(original);
			Element *copy = duplicate();
			QVERIFY(copy && copy != original);
			QCOMPARE(copy->pos(), QPointF(200 + grid().x(), 200));
			copy->setPos(copy->pos() + QPointF(0, 6 * grid().y()));
			Element *next = duplicate();
			QVERIFY(next && next != copy);
			QCOMPARE(next->pos(), copy->pos() + QPointF(grid().x(), 0));
		}

			//On: not moved, as before; moved, the move is repeated, and the
			//run goes on at the new spacing
		void onRepeatsTheMove()
		{
			DuplicateOffsetDialog::saveRepeatsMove(true);
			QVERIFY(DuplicateOffsetDialog::savedRepeatsMove());

			Element *original = place(QPointF(400, 200));
			Element *a = duplicate();
			QVERIFY(a);
			QCOMPARE(a->pos(), QPointF(400 + grid().x(), 200));
			Element *b = duplicate();
			QVERIFY(b);
			QCOMPARE(b->pos(), a->pos() + QPointF(grid().x(), 0));

			b->setPos(b->pos() + QPointF(0, 5 * grid().y()));
			const QPointF spacing = b->pos() - a->pos();
			Element *c = duplicate();
			QVERIFY(c);
			QCOMPARE(c->pos(), b->pos() + spacing);
			Element *d = duplicate();
			QVERIFY(d);
			QCOMPARE(d->pos(), c->pos() + spacing);

				//One press, one undo: only the last copy goes
			m_diagram->undoStack().undo();
			QVERIFY(!d->scene());
			QVERIFY(c->scene() && b->scene() && a->scene() && original->scene());
			m_diagram->undoStack().redo();
			QVERIFY(d->scene());
		}

			//A move with the arrow keys is repeated too
		void arrowKeys()
		{
			Element *original = place(QPointF(200, 500));
			Element *a = duplicate();
			QVERIFY(a);
			const QPointF before = a->pos();
			QKeyEvent press(QEvent::KeyPress, Qt::Key_Down, Qt::NoModifier);
			QApplication::sendEvent(m_view, &press);
			QKeyEvent release(QEvent::KeyRelease, Qt::Key_Down, Qt::NoModifier);
			QApplication::sendEvent(m_view, &release);
			QVERIFY2(a->pos() != before, "the arrow key moved the copy");
			const QPointF spacing = a->pos() - original->pos();
			Element *b = duplicate();
			QVERIFY(b);
			QCOMPARE(b->pos(), a->pos() + spacing);
		}

			//Something else selected, a copy put back on its source, or
			//another spacing: the spacing set
		void otherwiseTheSpacing()
		{
			Element *original = place(QPointF(200, 800));
			Element *a = duplicate();
			QVERIFY(a);
			a->setPos(a->pos() + QPointF(0, 3 * grid().y()));

				//Another selection: the original again
			m_diagram->clearSelection();
			original->setSelected(true);
			Element *b = duplicate();
			QVERIFY(b);
			QCOMPARE(b->pos(), original->pos() + QPointF(grid().x(), 0));

				//Put back on its source
			b->setPos(original->pos());
			Element *c = duplicate();
			QVERIFY(c);
			QCOMPARE(c->pos(), b->pos() + QPointF(grid().x(), 0));

				//Moved, but the spacing changed since
			c->setPos(c->pos() + QPointF(0, 4 * grid().y()));
			DuplicateOffsetDialog::saveStepOffset(QPoint(0, 2));
			Element *d = duplicate();
			QVERIFY(d);
			QCOMPARE(d->pos(), c->pos() + QPointF(0, 2 * grid().y()));
			DuplicateOffsetDialog::saveStepOffset(QPoint(1, 0));
		}

			//Ctrl+Shift+D: the dialog, then a duplicate at the spacing
			//chosen, whatever the copy was moved by; one undo step
		void ctrlShiftD()
		{
			QAction *with_spacing = actionFor(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_D));
			QVERIFY(with_spacing);
			Element *original = place(QPointF(700, 200));
			Element *a = duplicate();
			QVERIFY(a);
			a->setPos(a->pos() + QPointF(0, 7 * grid().y()));

			bool answered = false;
			QTimer::singleShot(0, this, [&answered]() {
				auto *dialog = qobject_cast<DuplicateOffsetDialog *>(QApplication::activeModalWidget());
				if (!dialog) return;
				dialog->findChild<QSpinBox *>()->setValue(3);
				dialog->findChild<QComboBox *>()->setCurrentIndex(DuplicateOffsetDialog::Down);
				answered = true;
				dialog->accept();
			});
			const int steps = m_diagram->undoStack().count();
			with_spacing->trigger();
			QVERIFY(answered);
			QCOMPARE(m_diagram->undoStack().count(), steps + 1);
			Element *b = selected();
			QVERIFY(b && b != a && b != original);
			QCOMPARE(b->pos(), a->pos() + QPointF(0, 3 * grid().y()));
			QCOMPARE(DuplicateOffsetDialog::savedStepOffset(), QPoint(0, 3));

				//Then Ctrl+D goes on at that spacing
			Element *c = duplicate();
			QVERIFY(c);
			QCOMPARE(c->pos(), b->pos() + QPointF(0, 3 * grid().y()));
			DuplicateOffsetDialog::saveStepOffset(QPoint(1, 0));
		}

			//Switched off again: as before
		void switchedOff()
		{
			DuplicateOffsetDialog::saveRepeatsMove(false);
			QVERIFY(!QSettings().contains(QStringLiteral("diagrameditor/duplicate_repeats_move")));
			place(QPointF(700, 800));
			Element *a = duplicate();
			QVERIFY(a);
			a->setPos(a->pos() + QPointF(0, 2 * grid().y()));
			Element *b = duplicate();
			QVERIFY(b);
			QCOMPARE(b->pos(), a->pos() + QPointF(grid().x(), 0));
		}
};

QET_APP_TEST_MAIN(tst_duplicaterepeatmove)
#include "tst_duplicaterepeatmove.moc"
