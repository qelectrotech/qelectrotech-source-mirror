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
	Duplicate (Ctrl+D) learning its offset from the move of the copy, as a
	drawing program does -- a setting, off by default. Switched on, the
	spacing of the dialog is set aside: the first copy lands 4 grid steps
	right and down, and once the copy just made is moved, that move is the
	offset of the next copies, kept when something else is selected and
	when QET is closed. Switching the setting on starts afresh. Off,
	Duplicate is as before. Ctrl+Shift+D opens the dialog (Configure
	duplication...), without duplicating. Each press of Ctrl+D is one step
	of the undo stack.
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
			select(element);
			return element;
		}

		void select(Element *element)
		{
			m_diagram->clearSelection();
			element->setSelected(true);
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

			/// Press @p key on the folio
		void key(Qt::Key key)
		{
			QKeyEvent press(QEvent::KeyPress, key, Qt::NoModifier);
			QApplication::sendEvent(m_view, &press);
			QKeyEvent release(QEvent::KeyRelease, key, Qt::NoModifier);
			QApplication::sendEvent(m_view, &release);
		}

		QPointF grid() const { return QPointF(Diagram::xGrid, Diagram::yGrid); }
		QPointF firstOffset() const { return 4 * grid(); }

		QPointF learned()
		{
			QPointF offset;
			return DuplicateOffsetDialog::learnedOffset(&offset) ? offset : QPointF();
		}

			/// Answer the dialog Ctrl+Shift+D opens: @p repeat checks the
			/// box, else @p spacing and @p direction are set
		void answerDialog(bool *answered, int spacing, int direction, bool repeat)
		{
			QTimer::singleShot(0, this, [=]() {
				auto *dialog = qobject_cast<DuplicateOffsetDialog *>(QApplication::activeModalWidget());
				if (!dialog) return;
				dialog->findChild<QCheckBox *>()->setChecked(repeat);
				if (!repeat) {
					dialog->findChild<QSpinBox *>()->setValue(spacing);
					dialog->findChild<QComboBox *>()->setCurrentIndex(direction);
				}
				*answered = true;
				dialog->accept();
			});
		}

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

			//Off by default, and nothing written while it is; checked, the
			//spacing is set aside, not cleared
		void offByDefault()
		{
			QVERIFY(!DuplicateOffsetDialog::savedRepeatsMove());
			DuplicateOffsetDialog dialog;
			QVERIFY(!dialog.repeatsMove());
			auto *box = dialog.findChild<QCheckBox *>();
			auto *spacing = dialog.findChild<QSpinBox *>();
			auto *direction = dialog.findChild<QComboBox *>();
			QVERIFY(!box->isChecked());
			QVERIFY(spacing->isEnabled() && direction->isEnabled());
			box->setChecked(true);
			QVERIFY(!spacing->isEnabled() && !direction->isEnabled());
			QCOMPARE(dialog.stepOffset(), QPoint(1, 0));
			box->setChecked(false);
			QVERIFY(spacing->isEnabled() && direction->isEnabled());
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
			place(QPointF(200, 1500)); // another selection: still nothing learned
			QVERIFY(learned().isNull());
		}

			//Switched on: what was learned before is forgotten, the first
			//copy lands 4 grid steps right and down
		void switchingOnStartsAfresh()
		{
			DuplicateOffsetDialog::saveLearnedOffset(QPointF(70, 0));
			DuplicateOffsetDialog dialog;
			dialog.findChild<QCheckBox *>()->setChecked(true);
			dialog.save();
			QVERIFY(DuplicateOffsetDialog::savedRepeatsMove());
			QVERIFY(learned().isNull());

			Element *original = place(QPointF(400, 200));
			Element *a = duplicate();
			QVERIFY(a);
			QCOMPARE(a->pos(), original->pos() + firstOffset());
				//Saved again as it is: what was learned stays
			DuplicateOffsetDialog::saveLearnedOffset(QPointF(70, 0));
			DuplicateOffsetDialog again;
			QVERIFY(again.repeatsMove());
			again.save();
			QCOMPARE(learned(), QPointF(70, 0));
		}

			//The move of the copy is the offset of the next ones, which
			//go on at it unmoved; one press, one undo
		void learnsTheMove()
		{
			Element *original = place(QPointF(200, 2000));
			DuplicateOffsetDialog::forgetLearnedOffset();
			Element *a = duplicate();
			QVERIFY(a);
			QCOMPARE(a->pos(), original->pos() + firstOffset());
			a->setPos(original->pos() + QPointF(6 * grid().x(), 0));
			Element *b = duplicate();
			QVERIFY(b);
			QCOMPARE(b->pos(), a->pos() + QPointF(6 * grid().x(), 0));
			QCOMPARE(learned(), QPointF(6 * grid().x(), 0));
			Element *c = duplicate();
			QVERIFY(c);
			QCOMPARE(c->pos(), b->pos() + QPointF(6 * grid().x(), 0));

			m_diagram->undoStack().undo();
			QVERIFY(!c->scene());
			QVERIFY(b->scene() && a->scene() && original->scene());
			m_diagram->undoStack().redo();
			QVERIFY(c->scene());
		}

			//Learned as soon as something else is selected, and kept for
			//that other symbol
		void keptForAnotherSelection()
		{
			Element *x = place(QPointF(200, 2500));
			DuplicateOffsetDialog::forgetLearnedOffset();
			Element *other = place(QPointF(800, 2500));
			select(x);
			Element *a = duplicate();
			QVERIFY(a);
			a->setPos(x->pos() + QPointF(0, 8 * grid().y()));
			QVERIFY(learned().isNull());
			select(other);
			QCOMPARE(learned(), QPointF(0, 8 * grid().y()));
			Element *b = duplicate();
			QVERIFY(b);
			QCOMPARE(b->pos(), other->pos() + QPointF(0, 8 * grid().y()));
		}

			//Moves of anything but the copy just made teach nothing, nor
			//does a copy put back on its source
		void otherMovesAreNotLearned()
		{
			Element *x = place(QPointF(200, 3500));
			DuplicateOffsetDialog::saveLearnedOffset(QPointF(0, 80));
			Element *a = duplicate();
			QVERIFY(a);
			QCOMPARE(a->pos(), x->pos() + QPointF(0, 80));
				//The original moved
			select(x);
			x->setPos(x->pos() + QPointF(30, 0));
			Element *b = duplicate();
			QVERIFY(b);
			QCOMPARE(b->pos(), x->pos() + QPointF(0, 80));
			QCOMPARE(learned(), QPointF(0, 80));
				//The copy before the last one moved: forgotten already
			a->setPos(a->pos() + QPointF(50, 0));
			select(a);
			QCOMPARE(learned(), QPointF(0, 80));
				//The last copy put back on its source
			Element *c = duplicate();
			QVERIFY(c);
			c->setPos(a->pos());
			select(x);
			QCOMPARE(learned(), QPointF(0, 80));
		}

			//A move with the arrow keys is learned too
		void arrowKeys()
		{
			Element *original = place(QPointF(200, 4500));
			DuplicateOffsetDialog::forgetLearnedOffset();
			Element *a = duplicate();
			QVERIFY(a);
			const QPointF before = a->pos();
			key(Qt::Key_Down);
			QVERIFY2(a->pos() != before, "the arrow key moved the copy");
			const QPointF spacing = a->pos() - original->pos();
			Element *b = duplicate();
			QVERIFY(b);
			QCOMPARE(b->pos(), a->pos() + spacing);
			QCOMPARE(learned(), spacing);
		}

			//Ctrl+Shift+D is Configure duplication...: the dialog, saved,
			//and no copy; usable with nothing selected
		void ctrlShiftD()
		{
			QAction *configure = actionFor(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_D));
			QVERIFY(configure);
			m_diagram->clearSelection();
			QVERIFY(configure->isEnabled());
			Element *original = place(QPointF(1200, 200));

				//Unchecked: the spacing chosen is saved, nothing is copied
			bool answered = false;
			answerDialog(&answered, 3, DuplicateOffsetDialog::Down, false);
			int steps = m_diagram->undoStack().count();
			configure->trigger();
			QVERIFY(answered);
			QCOMPARE(m_diagram->undoStack().count(), steps);
			QCOMPARE(selected(), original);
			QCOMPARE(DuplicateOffsetDialog::savedStepOffset(), QPoint(0, 3));
			QVERIFY(!DuplicateOffsetDialog::savedRepeatsMove());
				//Then Ctrl+D at that spacing
			Element *a = duplicate();
			QVERIFY(a && a != original);
			QCOMPARE(a->pos(), original->pos() + QPointF(0, 3 * grid().y()));

				//Checked there: afresh, still nothing copied
			DuplicateOffsetDialog::saveLearnedOffset(QPointF(70, 0));
			answered = false;
			answerDialog(&answered, 0, 0, true);
			steps = m_diagram->undoStack().count();
			configure->trigger();
			QVERIFY(answered);
			QCOMPARE(m_diagram->undoStack().count(), steps);
			QVERIFY(DuplicateOffsetDialog::savedRepeatsMove());
			QVERIFY(learned().isNull());
			Element *b = duplicate();
			QVERIFY(b && b != a);
			QCOMPARE(b->pos(), a->pos() + firstOffset());
			DuplicateOffsetDialog::saveStepOffset(QPoint(1, 0));
		}

			//The command that duplicated after the dialog is gone: one
			//command per key, nothing else on Ctrl+Shift+D
		void noDuplicateWithSpacing()
		{
			int on_ctrl_shift_d = 0;
			for (QAction *a : QetAppTest::editor()->findChildren<QAction *>()) {
				QVERIFY(!a->text().contains(QStringLiteral("another spacing")));
				if (a->shortcut() == QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_D))
					++on_ctrl_shift_d;
			}
			QCOMPARE(on_ctrl_shift_d, 1);
		}

			//Switched off again: as before
		void switchedOff()
		{
			DuplicateOffsetDialog::saveRepeatsMove(false);
			QVERIFY(!QSettings().contains(QStringLiteral("diagrameditor/duplicate_repeats_move")));
			place(QPointF(1200, 1500));
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
