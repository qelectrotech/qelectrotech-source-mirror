// SPDX-License-Identifier: GPL-2.0-or-later
// The folio-wide crosshair, linked against the application's objects.
//
// With the Display option on, two lines cross the whole folio through the
// grid point under the mouse, to see at a glance whether parts are in
// line. They are drawn on screen only: no item is added to the folio, so
// nothing of them is saved, exported or printed. They go when the mouse
// leaves the view and when the option is off, and the option is kept in
// the settings.
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

/// How many pixels of the crosshair colour lie along one row of the view
static int bluePixels(const QImage &image, int row)
{
	int n = 0;
	for (int x = 0; x < image.width(); ++x) {
		const QColor c = image.pixelColor(x, row);
		if (c.blue() > 150 && c.red() < 80) ++n;
	}
	return n;
}

int main(int argc, char **argv)
{
	QApplication app(argc, argv);
	std::freopen(argv[2], "w", stdout);

	QTemporaryDir settings;
	QSettings::setDefaultFormat(QSettings::IniFormat);
	QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, settings.path());
	QCoreApplication::setOrganizationName("QETCrosshairRegression");
	QETProject::setBackupEnabled(false);
	QET::QetMessageBox::setNonInteractive(true);
	QFontDatabase::addApplicationFont(":/fonts/LiberationSans-Regular.ttf");

	try
	{
		QETProject project(QString::fromLocal8Bit(argv[1]));
		check(project.state() == QETProject::Ok, "fixture opens");
		project.buildFolios(project.diagrams());
		Diagram *folio = project.diagrams().first();

		DiagramView view(folio);
		view.resize(800, 600);
		view.centerOn(folio->border_and_titleblock.insideBorderRect().center());
		QApplication::processEvents();
		const int items = folio->items().size();

		check(!DiagramView::crosshairShown(), "off by default");
		const QPoint mouse(403, 297);
		moveTo(view, mouse);
		check(!view.crosshairVisible(), "no crosshair while the option is off");

		DiagramView::setCrosshairShown(true);
		moveTo(view, mouse);
		check(view.crosshairVisible(), "the crosshair follows the mouse once on");
		const QPointF at = view.crosshairPos();
		check(at == Diagram::snapToGrid(view.mapToScene(mouse)), "it stands on the grid point under the mouse");
		check(folio->items().size() == items, "nothing is added to the folio");

		const QImage image = view.viewport()->grab().toImage();
		const int row = view.mapFromScene(at).y();
		check(bluePixels(image, row) > image.width() / 2, "a line crosses the view through that point");
		check(bluePixels(image, row + 20) < image.width() / 4, "and not elsewhere");

		QEvent leave(QEvent::Leave);
		QApplication::sendEvent(view.viewport(), &leave);
		check(!view.crosshairVisible(), "it goes when the mouse leaves the view");

		DiagramView::setCrosshairShown(false);
		moveTo(view, mouse);
		check(!view.crosshairVisible(), "and when the option is switched off");
		check(!QSettings().value("diagrameditor/crosshair").toBool(), "the option is kept in the settings");
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
