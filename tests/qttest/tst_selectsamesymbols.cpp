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
	Edit > Select the same symbols: every symbol of the folio drawn from
	the definition of a selected one is selected, the rest deselected;
	with no symbol selected the action is disabled and nothing changes.
*/
#include "qetapptest.h"
#include "ElementsCollection/xmlelementcollection.h"
#include "factory/elementfactory.h"
#include "qetgraphicsitem/conductor.h"
#include "qetgraphicsitem/element.h"
#include "qetproject.h"

class tst_selectsamesymbols : public QObject
{
	Q_OBJECT

	private:
		Diagram *m_diagram = nullptr;
		QAction *m_action = nullptr;
		Element *m_a1 = nullptr, *m_a2 = nullptr, *m_a3 = nullptr, *m_b1 = nullptr;
		Conductor *m_conductor = nullptr;

		Element *place(const QString &name, const QPointF &pos)
		{
			int state = 0;
			Element *element = ElementFactory::Instance()->createElement(
				ElementsLocation(QStringLiteral("embed://import/%1.elmt").arg(name), m_diagram->project()),
				nullptr, &state);
			if (state || !element)
				return nullptr;
			m_diagram->addItem(element);
			element->setPos(pos);
			return element;
		}

		bool define(const QString &name)
		{
			QDomDocument definition;
			definition.setContent(QStringLiteral(
				R"(<definition type="element" link_type="simple" width="20" height="20" hotspot_x="10" hotspot_y="10">
<names><name lang="en">%1</name></names>
<description><line x1="0" y1="-5" x2="0" y2="5" style="line-style:normal;line-weight:normal;filling:none;color:black"/>
<terminal x="0" y="-5" orientation="n"/></description></definition>)").arg(name));
			return m_diagram->project()->embeddedElementCollection()->addElementDefinition(
				QStringLiteral("import"), name, definition.documentElement());
		}

		QList<Element *> selectedElements() const
		{
			QList<Element *> list;
			for (QGraphicsItem *item : m_diagram->selectedItems())
				if (auto *element = qgraphicsitem_cast<Element *>(item))
					list << element;
			return list;
		}

	private slots:
		void initTestCase()
		{
			m_diagram = QetAppTest::diagram();
			QVERIFY(m_diagram);
			QVERIFY(define(QStringLiteral("a")));
			QVERIFY(define(QStringLiteral("b")));
			QVERIFY(m_a1 = place(QStringLiteral("a"), QPointF(100, 100)));
			QVERIFY(m_a2 = place(QStringLiteral("a"), QPointF(200, 100)));
			QVERIFY(m_a3 = place(QStringLiteral("a"), QPointF(300, 100)));
			QVERIFY(m_b1 = place(QStringLiteral("b"), QPointF(400, 100)));
			m_conductor = new Conductor(m_a1->terminals().first(), m_b1->terminals().first());
			m_diagram->addItem(m_conductor);
			m_action = QetAppTest::action(QStringLiteral("select_same_symbols"));
			QVERIFY(m_action);
		}

			//One "a" selected: all three, and the "b" and the wire deselected
		void selectsTheSameKind()
		{
			m_diagram->clearSelection();
			m_a2->setSelected(true);
			m_conductor->setSelected(true);
			QCoreApplication::processEvents();
			QVERIFY(m_action->isEnabled());
			m_action->trigger();
			const QList<Element *> selected = selectedElements();
			QCOMPARE(selected.size(), 3);
			QVERIFY(selected.contains(m_a1) && selected.contains(m_a2) && selected.contains(m_a3));
			QVERIFY(!m_b1->isSelected());
			QVERIFY(!m_conductor->isSelected());
		}

			//An "a" and the "b" selected: both kinds
		void selectsEveryKindSelected()
		{
			m_diagram->clearSelection();
			m_a1->setSelected(true);
			m_b1->setSelected(true);
			m_action->trigger();
			QCOMPARE(selectedElements().size(), 4);
		}

			//Only a wire selected: disabled, and the selection stays
		void noSymbolNoChange()
		{
			m_diagram->clearSelection();
			m_conductor->setSelected(true);
			QCoreApplication::processEvents();
			QVERIFY(!m_action->isEnabled());
			m_diagram->selectSameSymbols();
			QVERIFY(m_conductor->isSelected());
			QVERIFY(selectedElements().isEmpty());
		}
};

QET_APP_TEST_MAIN(tst_selectsamesymbols)
#include "tst_selectsamesymbols.moc"
