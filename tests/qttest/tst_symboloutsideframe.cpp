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
	A symbol that draws beyond its frame (its size and hotspot) is drawn
	whole: the view repaints only within an item's bounding rect, so the
	part outside the frame was cut off at some zooms and left out at others.
*/
#include "qetapptest.h"
#include "ElementsCollection/xmlelementcollection.h"
#include "factory/elementfactory.h"
#include "qetgraphicsitem/element.h"
#include "qetproject.h"

#include <QImage>
#include <QPainter>

class tst_symboloutsideframe : public QObject
{
	Q_OBJECT

	private:
		Diagram *m_diagram = nullptr;

			/// A symbol of frame (-10, -10) to (10, 10) with @p drawing
		Element *symbol(const QString &name, const QString &drawing)
		{
			QDomDocument definition;
			definition.setContent(QStringLiteral(
				R"(<definition type="element" link_type="simple" width="20" height="20" hotspot_x="10" hotspot_y="10">
<names><name lang="en">%1</name></names>
<description>%2<terminal x="0" y="-10" orientation="n"/></description></definition>)")
					.arg(name, drawing));
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
			element->setPos(200, 200);
			return element;
		}

			/// Black pixels when the scene's @p rect is drawn
		int inkIn(const QRectF &rect)
		{
			QImage image(QSize(200, 200), QImage::Format_ARGB32);
			image.fill(Qt::white);
			QPainter painter(&image);
			m_diagram->render(&painter, QRectF(image.rect()), rect);
			painter.end();
			int ink = 0;
			for (int y = 0; y < image.height(); ++y)
				for (int x = 0; x < image.width(); ++x)
					if (qGray(image.pixel(x, y)) < 128)
						++ink;
			return ink;
		}

	private slots:
		void initTestCase()
		{
			m_diagram = QetAppTest::diagram();
			QVERIFY(m_diagram);
		}

			//Drawn within its frame: the bounding rect is the frame, as before
		void withinTheFrame()
		{
			Element *element = symbol(QStringLiteral("within"),
				QStringLiteral(R"(<line x1="0" y1="-10" x2="0" y2="5" style="line-style:normal;line-weight:normal;filling:none;color:black"/>)"));
			QVERIFY(element);
			QCOMPARE(element->boundingRect(), QRectF(-10, -10, 20, 20));
			delete element;
		}

			//A circle reaching 11 below the frame: covered, and drawn
		void beyondTheFrame()
		{
			Element *element = symbol(QStringLiteral("beyond"),
				QStringLiteral(R"(<ellipse x="-8" y="5" width="16" height="16" style="line-style:normal;line-weight:normal;filling:none;color:black"/>)"));
			QVERIFY(element);
			const QRectF rect = element->boundingRect();
			QVERIFY2(rect.contains(QPointF(0, 21)), qPrintable(QStringLiteral("%1 %2 %3 %4")
					 .arg(rect.left()).arg(rect.top()).arg(rect.width()).arg(rect.height())));
			QVERIFY(rect.contains(QRectF(-10, -10, 20, 20)));
				//Only the part below the frame is drawn, as when the view
				//repaints just that strip
			QVERIFY(inkIn(QRectF(185, 212, 30, 12)) > 0);
			delete element;
		}
};

QET_APP_TEST_MAIN(tst_symboloutsideframe)
#include "tst_symboloutsideframe.moc"
