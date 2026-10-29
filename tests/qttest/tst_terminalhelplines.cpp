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
	The shaded lines from a dragged symbol's terminals: shown by default,
	as always, and none with the setting off.
*/
#include "qetapptest.h"
#include "diagram.h"
#include "diagramview.h"
#include "projectview.h"
#include "qetgraphicsitem/element.h"
#include "qetgraphicsitem/terminal.h"

#include <QGraphicsLineItem>
#include <QImage>
#include <QPainter>

class TstTerminalHelpLines : public QObject
{
	Q_OBJECT

	QTemporaryDir m_dir;

		/// Paint the symbol, which is when a terminal makes its line
	static void paint(Diagram *diagram, Element *element)
	{
		QImage image(400, 400, QImage::Format_ARGB32);
		QPainter painter(&image);
		diagram->render(&painter, QRectF(), element->sceneBoundingRect().adjusted(-20, -20, 20, 20));
	}

	static int lines(Diagram *diagram)
	{
		int n = 0;
		for (QGraphicsItem *item : diagram->items())
			if (item->type() == QGraphicsLineItem::Type)
				++n;
		return n;
	}

	private slots:
		void shownByDefaultAndSwitchedOff()
		{
			QVERIFY(DiagramView::terminalHelpLinesShown());
			QETDiagramEditor *editor = QetAppTest::editor();
			editor->resize(1400, 1000);
			editor->show();
			QVERIFY(QTest::qWaitForWindowExposed(editor));
			const QString copy = m_dir.filePath("p.qet");
			QVERIFY(QFile::copy(QFINDTESTDATA("fixtures/every_feature.qet"), copy));
			QVERIFY(editor->openAndAddProject(copy, false));
			DiagramView *view = editor->currentProjectView()->currentDiagram();
			Diagram *diagram = view->diagram();
			Element *element = nullptr;
				//A line is drawn only from a terminal without a wire
			for (Element *e : diagram->elements())
				for (Terminal *t : e->terminals())
					if (t->conductors().isEmpty() && !element)
						element = e;
			QVERIFY(element);
			view->centerOn(element);
			const int before = lines(diagram);

			for (Terminal *t : element->terminals()) t->drawHelpLine(true);
			paint(diagram, element);
			QVERIFY(lines(diagram) > before);
			for (Terminal *t : element->terminals()) t->drawHelpLine(false);
			QCOMPARE(lines(diagram), before);

			DiagramView::setTerminalHelpLinesShown(false);
			for (Terminal *t : element->terminals()) t->drawHelpLine(true);
			paint(diagram, element);
			QCOMPARE(lines(diagram), before);
			for (Terminal *t : element->terminals()) t->drawHelpLine(false);
			DiagramView::setTerminalHelpLinesShown(true);
		}
};

QET_APP_TEST_MAIN(TstTerminalHelpLines)

#include "tst_terminalhelplines.moc"
