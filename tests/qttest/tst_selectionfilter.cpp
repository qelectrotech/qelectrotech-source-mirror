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
	The selection filter (status bar): what a click, a selection rectangle
	and Select All pick. Everything by default, as before; Automatic
	leaves a symbol's texts to their symbol; the others pick one kind, and
	a click goes through what is left out to what lies under it.
*/
#include "qetapptest.h"
#include "ElementsCollection/xmlelementcollection.h"
#include "factory/elementfactory.h"
#include "qetgraphicsitem/conductor.h"
#include "qetgraphicsitem/dynamicelementtextitem.h"
#include "qetgraphicsitem/element.h"
#include "qetgraphicsitem/independenttextitem.h"
#include "qetproject.h"

#include <QMouseEvent>

class tst_selectionfilter : public QObject
{
	Q_OBJECT

	private:
		DiagramView *m_view = nullptr;
		Diagram *m_diagram = nullptr;
		Element *m_a = nullptr, *m_b = nullptr;
		DynamicElementTextItem *m_label = nullptr;
		IndependentTextItem *m_free = nullptr;
		Conductor *m_conductor = nullptr;

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
			return element;
		}

		void send(QEvent::Type type, const QPointF &scene, Qt::MouseButton button,
				  Qt::MouseButtons buttons, Qt::KeyboardModifiers modifiers = Qt::NoModifier)
		{
			const QPointF pos = m_view->mapFromScene(scene);
			QMouseEvent event(type, pos, m_view->viewport()->mapToGlobal(pos),
							  button, buttons, modifiers);
			QApplication::sendEvent(m_view->viewport(), &event);
		}

		void click(const QPointF &scene, Qt::KeyboardModifiers modifiers = Qt::NoModifier)
		{
			m_diagram->clearSelection();
			send(QEvent::MouseButtonPress, scene, Qt::LeftButton, Qt::LeftButton, modifiers);
			send(QEvent::MouseButtonRelease, scene, Qt::LeftButton, Qt::NoButton, modifiers);
		}

			/// A selection rectangle over the whole drawing
		void band()
		{
			m_diagram->clearSelection();
			const QPointF from(100, 100), to(500, 400);
			send(QEvent::MouseButtonPress, from, Qt::LeftButton, Qt::LeftButton);
			for (int i = 1; i <= 10; ++i)
				send(QEvent::MouseMove, from + (to - from) * i / 10.0, Qt::NoButton, Qt::LeftButton);
			send(QEvent::MouseButtonRelease, to, Qt::LeftButton, Qt::NoButton);
		}

		QPointF labelPoint() const { return m_label->sceneBoundingRect().center(); }

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
<description><rect x="-10" y="-10" width="20" height="20" style="line-style:normal;line-weight:normal;filling:white;color:black"/>
<terminal x="0" y="-10" orientation="n"/></description></definition>)"));
			QVERIFY(m_diagram->project()->embeddedElementCollection()->addElementDefinition(
				QStringLiteral("import"), QStringLiteral("box"), definition.documentElement()));
			QVERIFY(m_a = place(QPointF(200, 200)));
			QVERIFY(m_b = place(QPointF(400, 200)));
			m_label = new DynamicElementTextItem(m_a);
			m_label->setText(QStringLiteral("LABEL"));
			m_a->addDynamicTextItem(m_label);
			m_label->setPos(20, -5);
			m_free = new IndependentTextItem(QStringLiteral("free"));
			m_diagram->addItem(m_free);
				//Over symbol b
			m_free->setPos(390, 190);
			m_conductor = new Conductor(m_a->terminals().first(), m_b->terminals().first());
			m_diagram->addItem(m_conductor);
			m_view->resize(900, 700);
			m_view->centerOn(300, 250);
			QCOMPARE(Diagram::selectionFilter(), Diagram::SelectionFilter::All);
		}

		void cleanup()
		{
			Diagram::setSelectionFilter(Diagram::SelectionFilter::All);
		}

			//By default everything, a symbol's text by itself too
		void everythingByDefault()
		{
			band();
			QVERIFY(m_a->isSelected() && m_b->isSelected());
			QVERIFY(m_label->isSelected());
			QVERIFY(m_free->isSelected());
			QVERIFY(m_conductor->isSelected());
		}

			//Automatic: the rectangle leaves the symbol's text out, a click on
			//it picks the symbol, Shift still the text alone
		void automaticLeavesTextsToTheirSymbol()
		{
			Diagram::setSelectionFilter(Diagram::SelectionFilter::Automatic);
			band();
			QVERIFY(m_a->isSelected() && m_b->isSelected());
			QVERIFY(!m_label->isSelected());
			QVERIFY(m_free->isSelected());
			QVERIFY(m_conductor->isSelected());

			click(labelPoint());
			QVERIFY(m_a->isSelected());
			QVERIFY(!m_label->isSelected());

			click(labelPoint(), Qt::ShiftModifier);
			QVERIFY(m_label->isSelected());
			QVERIFY(!m_a->isSelected());

			QCOMPARE(m_diagram->pickAt(labelPoint(), m_view->transform()), static_cast<QGraphicsItem *>(m_a));
		}

			//Symbols: a click on the free text over b goes to b
		void symbolsOnly()
		{
			Diagram::setSelectionFilter(Diagram::SelectionFilter::Symbols);
			band();
			QVERIFY(m_a->isSelected() && m_b->isSelected());
			QVERIFY(!m_label->isSelected() && !m_free->isSelected() && !m_conductor->isSelected());

			click(m_free->sceneBoundingRect().center());
			QVERIFY(m_b->isSelected());
			QVERIFY(!m_free->isSelected());
		}

		void conductorsOnly()
		{
			Diagram::setSelectionFilter(Diagram::SelectionFilter::Conductors);
			m_diagram->selectAll();
			const QList<QGraphicsItem *> selected = m_diagram->selectedItems();
			QCOMPARE(selected.size(), 1);
			QCOMPARE(selected.first(), static_cast<QGraphicsItem *>(m_conductor));
				//A symbol is left out: a click on it picks nothing
			click(QPointF(200, 200));
			QVERIFY(m_diagram->selectedItems().isEmpty());
		}

			//Free texts: the one over b, not b under it
		void freeTextsOnly()
		{
			Diagram::setSelectionFilter(Diagram::SelectionFilter::FreeTexts);
			band();
			const QList<QGraphicsItem *> selected = m_diagram->selectedItems();
			QCOMPARE(selected.size(), 1);
			QCOMPARE(selected.first(), static_cast<QGraphicsItem *>(m_free));
		}

			//Symbol texts: a click picks the text, not its symbol
		void symbolTextsOnly()
		{
			Diagram::setSelectionFilter(Diagram::SelectionFilter::SymbolTexts);
			band();
			QCOMPARE(m_diagram->selectedItems(), QList<QGraphicsItem *>{m_label});
			click(labelPoint());
			QVERIFY(m_label->isSelected());
			QVERIFY(!m_a->isSelected());
		}

			//Kept in the settings
		void kept()
		{
			Diagram::setSelectionFilter(Diagram::SelectionFilter::Symbols);
			QCOMPARE(QSettings().value(QStringLiteral("diagrameditor/selection_filter")).toInt(),
					 int(Diagram::SelectionFilter::Symbols));
		}
};

QET_APP_TEST_MAIN(tst_selectionfilter)
#include "tst_selectionfilter.moc"
