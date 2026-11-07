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
	A conductor between two terminals on the same point, as when two
	symbols are placed against each other: it connects them but is not
	drawn, and a click there goes to the symbols. Its path was the two
	terminals' stubs, drawn over both symbols.
*/
#include "qetapptest.h"
#include "ElementsCollection/xmlelementcollection.h"
#include "factory/elementfactory.h"
#include "qetgraphicsitem/conductor.h"
#include "qetgraphicsitem/element.h"
#include "qetgraphicsitem/terminal.h"
#include "qetproject.h"

#include <QImage>
#include <QPainter>

class tst_touchingconductor : public QObject
{
	Q_OBJECT

	private:
		Diagram *m_diagram = nullptr;

			/// A symbol with one terminal at (@p x, 0), facing @p facing
		Element *symbol(const QString &name, int x, const QString &facing, const QPointF &pos)
		{
			QDomDocument definition;
			definition.setContent(QStringLiteral(
				R"(<definition type="element" link_type="simple" width="20" height="20" hotspot_x="10" hotspot_y="10">
<names><name lang="en">%1</name></names>
<description><line x1="0" y1="0" x2="%2" y2="0" style="line-style:normal;line-weight:normal;filling:none;color:black"/>
<terminal x="%2" y="0" orientation="%3"/></description></definition>)")
					.arg(name).arg(x).arg(facing));
			auto *collection = m_diagram->project()->embeddedElementCollection();
			if (!collection->addElementDefinition(QStringLiteral("import"), name, definition.documentElement()))
				return nullptr;
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

			/// Magenta pixels when the scene's @p rect is drawn (not blue:
			/// the terminals' own colour)
		int magentaIn(const QRectF &rect)
		{
			QImage image(QSize(300, 100), QImage::Format_ARGB32);
			image.fill(Qt::white);
			QPainter painter(&image);
			m_diagram->render(&painter, QRectF(image.rect()), rect);
			painter.end();
			int magenta = 0;
			for (int y = 0; y < image.height(); ++y)
				for (int x = 0; x < image.width(); ++x) {
					const QColor c = image.pixelColor(x, y);
					if (c.red() > 200 && c.blue() > 200 && c.green() < 80)
						++magenta;
				}
			return magenta;
		}

			/// A magenta conductor from @p a's terminal to @p b's
		Conductor *join(Element *a, Element *b)
		{
			auto *conductor = new Conductor(a->terminals().first(), b->terminals().first());
			ConductorProperties properties = conductor->properties();
			properties.color = Qt::magenta;
			properties.cond_size = 2;
			conductor->setProperties(properties);
			m_diagram->addItem(conductor);
			return conductor;
		}

	private slots:
		void initTestCase()
		{
			m_diagram = QetAppTest::diagram();
			QVERIFY(m_diagram);
		}

			//Against each other: connected, not drawn, not clicked
		void touchingIsNotDrawn()
		{
			Element *a = symbol(QStringLiteral("left"), 10, QStringLiteral("e"), QPointF(100, 100));
			Element *b = symbol(QStringLiteral("right"), -10, QStringLiteral("w"), QPointF(120, 100));
			QVERIFY(a && b);
			Conductor *conductor = join(a, b);
			QVERIFY(conductor->endsTouch());
			QVERIFY(a->terminals().first()->conductors().contains(conductor));
			QVERIFY(conductor->shape().isEmpty());
			QCOMPARE(magentaIn(QRectF(80, 90, 60, 20)), 0);
			for (QGraphicsItem *item : m_diagram->items(QPointF(115, 100)))
				QVERIFY(item != conductor);
			delete conductor;
			delete a;
			delete b;
		}

			//Apart: drawn as before
		void apartIsDrawn()
		{
			Element *a = symbol(QStringLiteral("left2"), 10, QStringLiteral("e"), QPointF(100, 200));
			Element *b = symbol(QStringLiteral("right2"), -10, QStringLiteral("w"), QPointF(160, 200));
			QVERIFY(a && b);
			Conductor *conductor = join(a, b);
			QVERIFY(!conductor->endsTouch());
			QVERIFY(!conductor->shape().isEmpty());
			QVERIFY(magentaIn(QRectF(80, 190, 120, 20)) > 0);
			delete conductor;
			delete a;
			delete b;
		}
};

QET_APP_TEST_MAIN(tst_touchingconductor)
#include "tst_touchingconductor.moc"
