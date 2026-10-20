// SPDX-License-Identifier: GPL-2.0-or-later
// The pointer over the folio, linked against the application's objects.
//
// Over a symbol, which a drag moves, the pointer is a four-way arrow; over
// the empty folio the arrow. With the crosshair on, the pointer jumps with
// it from grid point to grid point: the system's pointer is hidden over
// the folio and an arrow, or the four-way arrow, is drawn on the
// crosshair's point instead.
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
