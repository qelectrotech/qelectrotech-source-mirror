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
	A connected terminal drawn in the color of its conductor (a setting,
	off by default): in red, it stands out within a wire of another color.
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

class tst_terminalcolor : public QObject
{
	Q_OBJECT

	private:
		Diagram *m_diagram = nullptr;
		Element *m_left = nullptr;
		Element *m_right = nullptr;

			/// A symbol with one terminal at (@p x, 0), facing @p facing
		Element *symbol(const QString &name, int x, const QString &facing, const QPointF &pos)
		{
			QDomDocument definition;
			definition.setContent(QStringLiteral(
				R"(<definition type="element" link_type="simple" width="20" height="20" hotspot_x="10" hotspot_y="10">
<names><name lang="en">%1</name></names>
<description><terminal x="%2" y="0" orientation="%3"/></description></definition>)")
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

			/// Pixels of the left terminal's stub, (106, 100) to (110, 100), short
			/// of the conductor's end, that are red and that are magenta
		QPair<int, int> stub()
		{
			QImage image(QSize(30, 60), QImage::Format_ARGB32);
			image.fill(Qt::white);
			QPainter painter(&image);
			m_diagram->render(&painter, QRectF(image.rect()), QRectF(106, 97, 3, 6));
			painter.end();
			int red = 0, magenta = 0;
			for (int y = 0; y < image.height(); ++y)
				for (int x = 0; x < image.width(); ++x) {
					const QColor c = image.pixelColor(x, y);
					if (c.red() > 200 && c.green() < 80)
						++(c.blue() > 200 ? magenta : red);
				}
			return {red, magenta};
		}

	private slots:
		void initTestCase()
		{
			m_diagram = QetAppTest::diagram();
			QVERIFY(m_diagram);
			m_left = symbol(QStringLiteral("left"), 10, QStringLiteral("e"), QPointF(100, 100));
			m_right = symbol(QStringLiteral("right"), -10, QStringLiteral("w"), QPointF(160, 100));
			QVERIFY(m_left && m_right);
		}

			//Off by default: red, connected or not
		void redByDefault()
		{
			QVERIFY(!Terminal::takesConductorColor());
			QVERIFY(stub().first > 0);
			auto *conductor = new Conductor(m_left->terminals().first(), m_right->terminals().first());
			ConductorProperties properties = conductor->properties();
			properties.color = Qt::magenta;
			conductor->setProperties(properties);
			m_diagram->addItem(conductor);
			QVERIFY(stub().first > 0);
			QCOMPARE(stub().second, 0);
			delete conductor;
		}

			//On: the conductor's color once connected, red again without it
		void conductorColor()
		{
			Terminal::setTakesConductorColor(true);
			QVERIFY(stub().first > 0);
			auto *conductor = new Conductor(m_left->terminals().first(), m_right->terminals().first());
			ConductorProperties properties = conductor->properties();
			properties.color = Qt::magenta;
			conductor->setProperties(properties);
			m_diagram->addItem(conductor);
			QCOMPARE(stub().first, 0);
			QVERIFY(stub().second > 0);
			delete conductor;
			QVERIFY(stub().first > 0);
			Terminal::setTakesConductorColor(false);
		}
};

QET_APP_TEST_MAIN(tst_terminalcolor)
#include "tst_terminalcolor.moc"
