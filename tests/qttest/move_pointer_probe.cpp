// SPDX-License-Identifier: GPL-2.0-or-later
// The pointer over the folio, linked against the application's objects.
//
// Over a symbol, which a drag moves, the pointer is a four-way arrow; over
// the empty folio the arrow. With the crosshair on, the pointer jumps with
// it from grid point to grid point: the system's pointer is hidden over
// the folio and an arrow, or the four-way arrow, is drawn on the
// crosshair's point instead. All of it only with the setting "Four-way
// arrow and handle catching with the crosshair": off, the default, the
// system's pointer and the handle under the mouse, as before.
#include <QApplication>
#include <QFontDatabase>
#include <QImage>
#include <QMouseEvent>
#include <QSettings>
#include <QTemporaryDir>
#include <cstdio>
#include <stdexcept>

#include "../../sources/diagram.h"
#include "../../sources/diagramview.h"
#include "../../sources/qetgraphicsitem/element.h"
#include "../../sources/qetgraphicsitem/dynamicelementtextitem.h"
#include "../../sources/QetGraphicsItemModeler/qetgraphicshandleritem.h"
#include "../../sources/qetproject.h"
#include "../../sources/qetmessagebox.h"

static void check(bool value, const char *message)
{
	if (!value) throw std::runtime_error(message);
	std::printf("ok: %s\n", message);
}

static void moveTo(DiagramView &view, const QPoint &p)
{
	QMouseEvent event(QEvent::MouseMove, QPointF(p), view.viewport()->mapToGlobal(QPointF(p)),
					  Qt::NoButton, Qt::NoButton, Qt::NoModifier);
	QApplication::sendEvent(view.viewport(), &event);
}

/// How many black pixels lie in @a r of the view
static int blackPixels(const QImage &image, const QRect &r)
{
	int n = 0;
	for (int y = r.top(); y <= r.bottom(); ++y)
		for (int x = r.left(); x <= r.right(); ++x)
			n += image.pixelColor(x, y).lightness() < 40;
	return n;
}

int main(int argc, char **argv)
{
	QApplication app(argc, argv);
	std::freopen(argv[2], "w", stdout);

	QTemporaryDir settings;
	QSettings::setDefaultFormat(QSettings::IniFormat);
	QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, settings.path());
	QCoreApplication::setOrganizationName("QETMovePointerRegression");
	QETProject::setBackupEnabled(false);
	QET::QetMessageBox::setNonInteractive(true);
	QFontDatabase::addApplicationFont(":/fonts/LiberationSans-Regular.ttf");

	try
	{
		QETProject project(QString::fromLocal8Bit(argv[1]));
		check(project.state() == QETProject::Ok, "fixture opens");
		project.buildFolios(project.diagrams());
		Diagram *folio = project.diagrams().first();
		Element *element = folio->elements().value(0);
		check(element, "the fixture has a symbol");

		DiagramView view(folio);
		view.resize(800, 600);
		view.centerOn(element->sceneBoundingRect().center());
		QApplication::processEvents();

		QPoint over;
		for (int dy = -8; dy <= 8 && over.isNull(); ++dy)
			for (int dx = -8; dx <= 8 && over.isNull(); ++dx) {
				const QPoint p = view.mapFromScene(element->sceneBoundingRect().center()) + QPoint(dx, dy);
				if (view.itemAt(p) == element) over = p;
			}
		check(!over.isNull(), "a point of the view over the symbol itself");
		const QPoint empty = view.mapFromScene(folio->border_and_titleblock.insideBorderRect().topLeft() + QPointF(15, 15));
		check(!view.itemAt(empty), "and one over the empty folio");

		// Switched off (the default): nothing changes the pointer, and the
		// crosshair keeps to the grid.
		check(!DiagramView::pointerAids(), "the pointer aids are off by default");
		moveTo(view, over);
		check(!view.viewport()->testAttribute(Qt::WA_SetCursor), "off: over a symbol, the view sets no pointer");
		DiagramView::setCrosshairShown(true);
		moveTo(view, over);
		check(view.drawnPointer() == 0 && !view.viewport()->testAttribute(Qt::WA_SetCursor),
			  "off: with the crosshair too, the system's pointer");
		DiagramView::setCrosshairShown(false);
		DiagramView::setPointerAids(true);

		moveTo(view, over);
		check(view.viewport()->cursor().shape() == Qt::SizeAllCursor, "over a symbol, the four-way arrow");
		check(view.drawnPointer() == 0, "the system's pointer, while the crosshair is off");
		moveTo(view, empty);
		check(view.viewport()->cursor().shape() == Qt::ArrowCursor, "over the empty folio, the arrow");

		DiagramView::setCrosshairShown(true);
		moveTo(view, empty);
		check(view.viewport()->cursor().shape() == Qt::BlankCursor, "with the crosshair, the system's pointer is hidden");
		check(view.drawnPointer() == 1, "an arrow is drawn instead");
		const QPoint tip = view.mapFromScene(view.crosshairPos());
		QImage image = view.viewport()->grab().toImage();
		check(blackPixels(image, QRect(tip + QPoint(1, 4), QSize(4, 8))) > 8, "the arrow stands below and right of the point");
		moveTo(view, over);
		check(view.drawnPointer() == 2, "over a symbol, the drawn pointer is the four-way arrow");

			//A handle near the mouse catches the crosshair, which never
			//reaches it on the grid, and a press there grabs it
		element->setSelected(true);
		QApplication::processEvents();
		DynamicElementTextItem *text = nullptr;
		QetGraphicsHandlerItem *handle = nullptr;
		for (DynamicElementTextItem *t : element->dynamicTextItems())
			for (QGraphicsItem *child : t->childItems())
				if (child->type() == QetGraphicsHandlerItem::Type && child->isVisible()) {
					text = t;
					handle = static_cast<QetGraphicsHandlerItem *>(child);
				}
		check(handle, "the selected symbol's text shows a width handle");
		const QPoint on_handle = view.mapFromScene(handle->scenePos());
		const QPoint beside = on_handle + QPoint(7, 6);
		check(view.itemAt(beside) != handle, "a point beside the handle, off it");
		moveTo(view, beside);
		check(view.crosshairPos() == handle->scenePos(), "the crosshair is caught by the handle near the mouse");
		check(view.drawnPointer() == 1, "and the arrow is drawn on it");
		const QPointF element_pos = element->pos();
		const qreal width_before = text->boundingRect().width();
		auto send = [&](QEvent::Type type, const QPoint &p, Qt::MouseButtons buttons) {
			QMouseEvent event(type, QPointF(p), view.viewport()->mapToGlobal(QPointF(p)),
					  Qt::LeftButton, buttons, Qt::NoModifier);
			QApplication::sendEvent(view.viewport(), &event);
		};
		send(QEvent::MouseButtonPress, beside, Qt::LeftButton);
		for (int step = 1; step <= 10; ++step)
			send(QEvent::MouseMove, beside + QPoint(4 * step, 0), Qt::LeftButton);
		send(QEvent::MouseButtonRelease, beside + QPoint(40, 0), Qt::NoButton);
		QApplication::processEvents();
		std::printf("width %g -> %g\n", width_before, text->boundingRect().width());
		check(qAbs(text->boundingRect().width() - width_before) > 10, "a press beside it grabs the handle: the text's width changes");
		check(element->pos() == element_pos, "and the symbol stays where it was");

			//A press whose release never comes (it went to a menu or a
			//dialog): the next move without the button ends the grab, and
			//the handle catches the crosshair again
		moveTo(view, beside + QPoint(40, 0));
		const QPoint near_handle = view.mapFromScene(handle->scenePos()) + QPoint(7, 6);
		moveTo(view, near_handle);
		check(view.crosshairPos() == handle->scenePos(), "caught again before the lost release");
		send(QEvent::MouseButtonPress, near_handle, Qt::LeftButton);
		moveTo(view, near_handle + QPoint(1, 0));
		check(view.crosshairPos() == handle->scenePos(), "a move without the button ends a grab whose release was lost");
		send(QEvent::MouseButtonRelease, near_handle + QPoint(1, 0), Qt::NoButton);
		element->setSelected(false);
		QApplication::processEvents();

			//After a drag, the pointer of where it ended at once: a
			//rubber band from the empty folio onto the symbol
		moveTo(view, empty);
		check(view.drawnPointer() == 1, "the arrow over the empty folio");
		send(QEvent::MouseButtonPress, empty, Qt::LeftButton);
		for (int step = 1; step <= 10; ++step)
			send(QEvent::MouseMove, empty + (over - empty) * step / 10, Qt::LeftButton);
		send(QEvent::MouseButtonRelease, over, Qt::NoButton);
		check(view.drawnPointer() == 2, "released over the symbol: the four-way arrow without another move");
		folio->clearSelection();

		QEvent leave(QEvent::Leave);
		QApplication::sendEvent(view.viewport(), &leave);
		check(view.drawnPointer() == 0, "it goes with the crosshair when the mouse leaves");
		DiagramView::setCrosshairShown(false);
		moveTo(view, empty);
		check(view.viewport()->cursor().shape() == Qt::ArrowCursor, "crosshair off: the system's arrow again");
		std::puts("PASS");
	}
	catch (const std::exception &error)
	{
		std::printf("FAIL: %s\n", error.what());
		std::fflush(stdout);
		return 1;
	}
	std::fflush(stdout);
	return 0;
}
