// SPDX-License-Identifier: GPL-2.0-or-later
// Drawing aids, linked against the application's objects.
//
// A symbol whose definition says drawing_aid="true", such as an
// interruption point that stops the automatic connections, steers the
// drawing only:
// - on screen it is light grey;
// - it is never printed nor exported: a folio painted to an image leaves
//   it out, while the terminals next to it are there;
// - View > Show can hide it, and hidden it still stops the automatic
//   connections;
// - its information (label and all) cannot be edited.
#include <QApplication>
#include <QFontDatabase>
#include <QImage>
#include <QSettings>
#include <QTemporaryDir>
#include <cstdio>
#include <stdexcept>

#include "../../sources/autoconnect.h"
#include "../../sources/diagram.h"
#include "../../sources/diagramview.h"
#include "../../sources/elementsmover.h"
#include "../../sources/qetproject.h"
#include "../../sources/qetmessagebox.h"
#include "../../sources/shownkinds.h"
#include "../../sources/qetgraphicsitem/element.h"
#include "../../sources/qetgraphicsitem/terminal.h"
#include "../../sources/ui/elementinfowidget.h"

static void check(bool value, const char *message)
{
	if (!value) throw std::runtime_error(message);
	std::printf("ok: %s\n", message);
}

/// How dark the darkest pixel in @a r is, 255 for none drawn
static int darkest(const QImage &image, const QRect &r)
{
	int least = 255;
	for (int y = qMax(0, r.top()); y <= qMin(image.height() - 1, r.bottom()); ++y)
		for (int x = qMax(0, r.left()); x <= qMin(image.width() - 1, r.right()); ++x)
			least = qMin(least, image.pixelColor(x, y).lightness());
	return least;
}

static void move(Diagram *diagram, Element *e, const QPointF &by)
{
	diagram->clearSelection();
	e->setSelected(true);
	ElementsMover &mover = diagram->elementsMover();
	mover.beginMovement(diagram);
	mover.continueMovement(by);
	mover.endMovement();
	diagram->clearSelection();
}

int main(int argc, char **argv)
{
	QApplication app(argc, argv);
	std::freopen(argv[2], "w", stdout);

	QTemporaryDir settings;
	QSettings::setDefaultFormat(QSettings::IniFormat);
	QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, settings.path());
	QCoreApplication::setOrganizationName("QETDrawingAidRegression");
	QETProject::setBackupEnabled(false);
	QET::QetMessageBox::setNonInteractive(true);
	QFontDatabase::addApplicationFont(":/fonts/LiberationSans-Regular.ttf");

	try
	{
		QETProject project(QString::fromLocal8Bit(argv[1]));
		check(project.state() == QETProject::Ok, "fixture opens");
		project.buildFolios(project.diagrams());
		Diagram *folio = project.diagrams().first();
		Element *aid = nullptr, *terminal = nullptr;
		for (Element *e : folio->elements())
			(e->terminals().isEmpty() ? aid : terminal) = e;
		check(aid && aid->isDrawingAid(), "the interruption point is a drawing aid");
		check(terminal && !terminal->isDrawingAid(), "a terminal is not");

		// Painted to an image, as printing and exporting do: not there.
		const QRectF border = folio->border_and_titleblock.borderAndTitleBlockRect();
		QImage image(border.size().toSize() + QSize(2 * int(Diagram::margin), 2 * int(Diagram::margin)),
					 QImage::Format_ARGB32);
		image.fill(Qt::white);
		folio->setDisplayGrid(false);	//as exports do
		folio->toPaintDevice(image, image.width(), image.height());
		folio->setDisplayGrid(true);
		auto onImage = [&](Element *e) {
			return e->sceneBoundingRect().translated(Diagram::margin, Diagram::margin).toAlignedRect();
		};
		check(darkest(image, onImage(terminal)) < 80, "a terminal is printed");
		check(darkest(image, onImage(aid)) > 240, "the drawing aid is not");

		// On screen: light grey.
		DiagramView view(folio);
		view.resize(800, 600);
		view.setRenderHint(QPainter::Antialiasing, false);
		view.centerOn(aid->sceneBoundingRect().center());
		folio->setDisplayGrid(false);	//its dots are darker than the aid
		QApplication::processEvents();
		const QImage screen = view.viewport()->grab().toImage();
		folio->setDisplayGrid(true);
		const QRect on_screen = view.mapFromScene(aid->sceneBoundingRect()).boundingRect();
		const int grey = darkest(screen, on_screen);
		check(grey > 100 && grey < 240, "on screen it is drawn, light grey");

		// Hidden with View > Show, and hidden it still stops the line.
		Terminal *aE = nullptr, *bW = nullptr;
		QList<Element *> row;
		for (Element *e : folio->elements())
			if (!e->terminals().isEmpty() && e->pos().y() < 300) row << e;
		std::sort(row.begin(), row.end(), [](Element *a, Element *b) { return a->pos().x() < b->pos().x(); });
		for (Terminal *t : row[0]->terminals()) if (t->name() == "E") aE = t;
		for (Terminal *t : row[1]->terminals()) if (t->name() == "W") bW = t;
		move(folio, aid, QPointF((aE->dockConductor().x() + bW->dockConductor().x()) / 2, aE->dockConductor().y()) - aid->pos());
		check(AutoConnect::partner(aE) == nullptr, "between two terminals it stops the line");
		ShownKinds::setShown(ShownKinds::DrawingAids, false);
		ShownKinds::apply(folio, ShownKinds::DrawingAids);
		check(!aid->isVisible(), "View > Show hides the drawing aids");
		check(AutoConnect::partner(aE) == nullptr, "hidden, it still stops the line");
		ShownKinds::setShown(ShownKinds::DrawingAids, true);
		ShownKinds::apply(folio, ShownKinds::DrawingAids);
		check(aid->isVisible(), "and shows them again");

		// Its information cannot be edited.
		ElementInfoWidget info(aid);
		check(!info.isEnabled(), "the information of a drawing aid cannot be edited");
		ElementInfoWidget other(terminal);
		check(other.isEnabled(), "that of a terminal can");
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
