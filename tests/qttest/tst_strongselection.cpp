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
	//Settings > General, "Frame selected symbols clearly": a selected
	//symbol framed in the highlight colour; off, the grey dashes as before
#include "qetapptest.h"
#include "ElementsCollection/xmlelementcollection.h"
#include "factory/elementfactory.h"
#include "qetgraphicsitem/element.h"
#include "qetgraphicsitem/qgraphicsitemutility.h"
#include "qetproject.h"

#include <QPainter>

class tst_strongselection : public QObject
{
	Q_OBJECT

	private:
		Element *m_element = nullptr;

			/// How many pixels of the highlight colour the selected symbol shows
		int highlightPixels()
		{
			QImage image(200, 200, QImage::Format_ARGB32);
			image.fill(Qt::white);
			QPainter painter(&image);
			painter.translate(100, 100);
			QStyleOptionGraphicsItem option;
			m_element->paint(&painter, &option, nullptr);
			painter.end();
			const QColor highlight = QApplication::palette().color(QPalette::Highlight);
			int count = 0;
			for (int y = 0; y < image.height(); ++y)
				for (int x = 0; x < image.width(); ++x)
					if (image.pixelColor(x, y) == highlight)
						++count;
			return count;
		}

	private slots:
		void initTestCase()
		{
			Diagram *diagram = QetAppTest::diagram();
			QVERIFY(diagram);
			QDomDocument definition;
			definition.setContent(QStringLiteral(
				R"(<definition type="element" link_type="simple" width="40" height="40" hotspot_x="20" hotspot_y="20">
<names><name lang="en">box</name></names>
<description><rect x="-10" y="-10" width="20" height="20" style="line-style:normal;line-weight:normal;filling:none;color:black"/></description></definition>)"));
			QVERIFY(diagram->project()->embeddedElementCollection()->addElementDefinition(
				QStringLiteral("import"), QStringLiteral("box"), definition.documentElement()));
			int state = 0;
			m_element = ElementFactory::Instance()->createElement(
				ElementsLocation(QStringLiteral("embed://import/box.elmt"), diagram->project()),
				nullptr, &state);
			QVERIFY(m_element && !state);
			diagram->addItem(m_element);
			m_element->setSelected(true);
		}

		void cleanupTestCase()
		{
			QGIUtility::setStrongSelection(false);
		}

		void offByDefault()
		{
			QVERIFY(!QGIUtility::strongSelection());
			QCOMPARE(highlightPixels(), 0);
		}

		void strongWhenSet()
		{
			QGIUtility::setStrongSelection(true);
			QVERIFY(highlightPixels() > 100);
		}
};

QET_APP_TEST_MAIN(tst_strongselection)
#include "tst_strongselection.moc"
