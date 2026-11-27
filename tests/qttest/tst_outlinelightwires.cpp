// SPDX-License-Identifier: GPL-2.0-or-later
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
#include "qetapptest.h"
#include "ElementsCollection/xmlelementcollection.h"
#include "factory/elementfactory.h"
#include "qetgraphicsitem/conductor.h"
#include "qetgraphicsitem/element.h"
#include "qetgraphicsitem/terminal.h"
#include "qetproject.h"

#include <QImage>
#include <QPainter>

/**
	The "Outline white and very pale wires in black" setting: a white wire
	is invisible on the white folio; with the setting on it has a black
	outline, and with it off (the default) nothing is drawn differently.
*/
class tst_outlinelightwires : public QObject
{
	Q_OBJECT

	Diagram *m_diagram = nullptr;
		//Each wire gets its own row, so an earlier test's wire is never
		//inside the area a later one renders
	int m_rows = 0;

	Element *place(const QPointF &pos)
	{
		int state = 0;
		Element *element = ElementFactory::Instance()->createElement(
			ElementsLocation(QStringLiteral("embed://import/end.elmt"), m_diagram->project()),
			nullptr, &state);
		if (state || !element)
			return nullptr;
		m_diagram->addItem(element);
		element->setPos(pos);
		return element;
	}

		//A wire of the given colour, 200 units long, on a row of its own
	Conductor *wire(const QColor &colour)
	{
		const qreal y = 100 + 100 * m_rows++;
		Element *a = place(QPointF(100, y));
		Element *b = place(QPointF(300, y));
		if (!a || !b)
			return nullptr;
		auto *conductor = new Conductor(a->terminals().first(), b->terminals().first());
		m_diagram->addItem(conductor);
		ConductorProperties properties = conductor->properties();
		properties.color = colour;
		conductor->setProperties(properties);
		return conductor;
	}

		//How many pixels of the drawn wire are dark. The wire's number
		//text is dark too, so callers compare two counts, never read one
	static int darkPixels(Conductor *conductor)
	{
		const QRectF area = conductor->sceneBoundingRect().adjusted(-2, -2, 2, 2);
		QImage image(area.size().toSize() * 2, QImage::Format_RGB32);
		image.fill(Qt::white);
		QPainter painter(&image);
		conductor->scene()->render(&painter, QRectF(), area);
		painter.end();
		int count = 0;
		for (int y = 0; y < image.height(); ++y)
			for (int x = 0; x < image.width(); ++x)
				if (image.pixelColor(x, y).lightnessF() < 0.2)
					++count;
		return count;
	}

	private slots:
		void initTestCase()
		{
			m_diagram = QetAppTest::diagram();
			QVERIFY(m_diagram);
			QDomDocument doc;
			doc.setContent(QStringLiteral(
				R"(<definition type="element" link_type="simple" width="10" height="10" hotspot_x="0" hotspot_y="5">
<names><name lang="en">end</name></names><description>
<terminal x="0" y="0" orientation="e" name="1"/></description></definition>)"));
			QVERIFY(m_diagram->project()->embeddedElementCollection()->addElementDefinition(
				QStringLiteral("import"), QStringLiteral("end"), doc.documentElement()));
		}

		void cleanup()
		{
			Conductor::setOutlinesLightWires(false);
		}

		void offByDefault()
		{
			QSettings().remove(QStringLiteral("diagrameditor/outline_light_wires"));
			QVERIFY(!Conductor::outlinesLightWires());
		}

		void aWhiteWireGetsAnOutlineOnlyWhenAsked()
		{
			Conductor *conductor = wire(Qt::white);
			QVERIFY(conductor);
			Conductor::setOutlinesLightWires(false);
			const int off = darkPixels(conductor);
			Conductor::setOutlinesLightWires(true);
			const int on = darkPixels(conductor);
				//A 200-unit wire with a one-unit outline each side, at 2x
			QVERIFY2(on > off + 200, qPrintable(QStringLiteral("off %1, on %2").arg(off).arg(on)));
		}

		void aPaleWireGetsOneToo()
		{
			Conductor *conductor = wire(QColor(255, 255, 200));
			QVERIFY(conductor);
			Conductor::setOutlinesLightWires(false);
			const int off = darkPixels(conductor);
			Conductor::setOutlinesLightWires(true);
			QVERIFY(darkPixels(conductor) > off + 200);
		}

		void aRedWireDrawsTheSameEitherWay()
		{
			Conductor *conductor = wire(Qt::red);
			QVERIFY(conductor);
			Conductor::setOutlinesLightWires(false);
			const int off = darkPixels(conductor);
			Conductor::setOutlinesLightWires(true);
			QCOMPARE(darkPixels(conductor), off);
		}

		void theSettingIsKept()
		{
			Conductor::setOutlinesLightWires(true);
			QVERIFY(QSettings().value(QStringLiteral("diagrameditor/outline_light_wires")).toBool());
		}
};

QET_APP_TEST_MAIN(tst_outlinelightwires)
#include "tst_outlinelightwires.moc"
