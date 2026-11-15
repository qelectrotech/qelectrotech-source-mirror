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
#include "conductorcolorcodes.h"
#include "conductorproperties.h"
#include "lastusedstyle.h"
#include "ElementsCollection/xmlelementcollection.h"
#include "factory/elementfactory.h"
#include "qetgraphicsitem/conductor.h"
#include "qetgraphicsitem/element.h"
#include "qetgraphicsitem/terminal.h"
#include "qetproject.h"
#include "ui/conductorpropertieswidget.h"

#include <QImage>
#include <QLineEdit>
#include <QPainter>
#include <QSignalSpy>

/**
	The IEC 60757 wire colour codes: picked from a list they set the
	colour text and how the wire is drawn (green-yellow striped); typed
	text stays what was typed.
*/
class tst_conductorcolorcodes : public QObject
{
	Q_OBJECT

	Diagram *m_diagram = nullptr;

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

		//A wire from (100,100) to (300,100)
	Conductor *wire()
	{
		Element *a = place(QPointF(100, 100));
		Element *b = place(QPointF(300, 100));
		if (!a || !b)
			return nullptr;
		auto *conductor = new Conductor(a->terminals().first(), b->terminals().first());
		m_diagram->addItem(conductor);
		return conductor;
	}

		//How many pixels of the drawn wire are close to @a colour
	static int pixelsOf(Conductor *conductor, const QColor &colour)
	{
		const QRectF area = conductor->sceneBoundingRect().adjusted(-2, -2, 2, 2);
		QImage image(area.size().toSize() * 2, QImage::Format_RGB32);
		image.fill(Qt::white);
		QPainter painter(&image);
		conductor->scene()->render(&painter, QRectF(), area);
		painter.end();
		int count = 0;
		for (int y = 0; y < image.height(); ++y)
			for (int x = 0; x < image.width(); ++x) {
				const QColor c = image.pixelColor(x, y);
				if (qAbs(c.red() - colour.red()) < 40
						&& qAbs(c.green() - colour.green()) < 40
						&& qAbs(c.blue() - colour.blue()) < 40)
					++count;
			}
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

		void codesAreFoundWhateverTheCase()
		{
			QVERIFY(ConductorColorCodes::find(QStringLiteral("GNYE")));
			QCOMPARE(ConductorColorCodes::find(QStringLiteral(" gnye "))->code, QStringLiteral("GNYE"));
			QCOMPARE(ConductorColorCodes::find(QStringLiteral("bu"))->code, QStringLiteral("BU"));
			QVERIFY(!ConductorColorCodes::find(QStringLiteral("Bleu")));
			QVERIFY(!ConductorColorCodes::find(QString()));
				//The fourteen colours of IEC 60757 and green-yellow
			QCOMPARE(ConductorColorCodes::all().size(), 15);
		}

		void greenYellowIsStriped()
		{
			ConductorProperties properties;
			ConductorColorCodes::apply(*ConductorColorCodes::find(QStringLiteral("GNYE")), properties);
			QCOMPARE(properties.m_wire_color, QStringLiteral("GNYE"));
			QVERIFY(properties.m_bicolor);
			QCOMPARE(properties.color, QColor(0x00, 0x80, 0x00));
			QCOMPARE(properties.m_color_2, QColor(0xFF, 0xD8, 0x00));
			QVERIFY2(properties.m_dash_size >= 2, "stripes, not a dotted line");

				//A one-colour code afterwards takes the stripes away
			ConductorColorCodes::apply(*ConductorColorCodes::find(QStringLiteral("BU")), properties);
			QCOMPARE(properties.m_wire_color, QStringLiteral("BU"));
			QVERIFY(!properties.m_bicolor);
			QCOMPARE(properties.color, QColor(0x00, 0x00, 0xFF));
		}

		void aCableCoreKeepsItsColourText()
		{
			ConductorProperties properties;
			properties.m_wire_color = QStringLiteral("BN");
			properties.m_cable_uuid = QUuid::createUuid();
			ConductorColorCodes::apply(*ConductorColorCodes::find(QStringLiteral("BU")), properties);
			QCOMPARE(properties.m_wire_color, QStringLiteral("BN"));
		}

		void pickingInTheDialogSetsTextAndDrawing()
		{
			ConductorPropertiesWidget widget;
			widget.setProperties(ConductorProperties());
			QSignalSpy picked(&widget, &ConductorPropertiesWidget::colorCodePicked);
			widget.pickColorCode(QStringLiteral("gnye"));
			QCOMPARE(picked.count(), 1);

			const ConductorProperties properties = widget.properties();
			QCOMPARE(properties.m_wire_color, QStringLiteral("GNYE"));
			QVERIFY(properties.m_bicolor);
			QCOMPARE(properties.color, QColor(0x00, 0x80, 0x00));
			QCOMPARE(properties.m_color_2, QColor(0xFF, 0xD8, 0x00));
		}

		void typedTextIsKept()
		{
				//Free text, and a code typed by hand: both stay text, and
				//the drawn colour is not touched
			for (const QString &typed : {QStringLiteral("Bleu"), QStringLiteral("gnye")})
			{
				ConductorProperties before;
				before.color = Qt::red;
				ConductorPropertiesWidget widget;
				widget.setProperties(before);
				widget.findChild<QLineEdit *>(QStringLiteral("m_wire_color_le"))->setText(typed);
				const ConductorProperties after = widget.properties();
				QCOMPARE(after.m_wire_color, typed);
				QCOMPARE(after.color, QColor(Qt::red));
				QVERIFY(!after.m_bicolor);
			}
		}

		void savedAndOpenedAgain()
		{
			ConductorProperties properties;
			ConductorColorCodes::apply(*ConductorColorCodes::find(QStringLiteral("GNYE")), properties);
			QDomDocument doc;
			QDomElement e = doc.createElement(QStringLiteral("conductor"));
			properties.toXml(e);
				//No new attribute: the code is in the colour text an older
				//QElectroTech already keeps
			QCOMPARE(e.attribute(QStringLiteral("conductor_color")), QStringLiteral("GNYE"));
			ConductorProperties read;
			read.fromXml(e);
			QVERIFY(read == properties);
		}

		void pickGreenYellowAndTheWireDrawsGreenAndYellow()
		{
			Conductor *conductor = wire();
			QVERIFY(conductor);
			const QColor green(0x00, 0x80, 0x00), yellow(0xFF, 0xD8, 0x00);
			QCOMPARE(pixelsOf(conductor, yellow), 0);

			ConductorProperties properties = conductor->properties();
			ConductorColorCodes::apply(*ConductorColorCodes::find(QStringLiteral("GNYE")), properties);
			conductor->setProperties(properties);
			QVERIFY(pixelsOf(conductor, green) > 20);
			QVERIFY(pixelsOf(conductor, yellow) > 20);

				//Blue afterwards: no yellow left
			ConductorColorCodes::apply(*ConductorColorCodes::find(QStringLiteral("BU")), properties);
			conductor->setProperties(properties);
			QCOMPARE(pixelsOf(conductor, yellow), 0);
		}

		void theNextWireTakesTheLastCode()
		{
			LastUsedStyle::setConductorColorCode(QStringLiteral("GNYE"), QColor(0x00, 0x80, 0x00));
			Conductor *conductor = wire();
			QVERIFY(conductor);
			QCOMPARE(conductor->properties().m_wire_color, QStringLiteral("GNYE"));
			QVERIFY(conductor->properties().m_bicolor);

				//A plain colour afterwards ("Other color…", F2): only the
				//drawn colour carries over, as before
			LastUsedStyle::setConductorColor(Qt::red);
			Conductor *next = wire();
			QVERIFY(next);
			QCOMPARE(next->properties().color, QColor(Qt::red));
			QVERIFY(next->properties().m_wire_color.isEmpty());
			QVERIFY(!next->properties().m_bicolor);
		}
};

QET_APP_TEST_MAIN(tst_conductorcolorcodes)
#include "tst_conductorcolorcodes.moc"
