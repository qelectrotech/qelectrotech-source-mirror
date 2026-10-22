// SPDX-License-Identifier: GPL-2.0-or-later
// The ink of a dark custom sheet, linked against the application's objects.
//
// On a custom dark sheet the folio's black lines are shown light: white,
// or with "Black, yellow symbols, red conductors" the symbols and texts in
// yellow, the conductors with their texts in red and the rest in white, with white grid
// dots and a white crosshair. Only the screen changes: the scene itself
// still renders black lines, so printing and exporting are as before.
#include <QApplication>
#include <QFontDatabase>
#include <QImage>
#include <QMouseEvent>
#include <QPainter>
#include <QSettings>
#include <QTemporaryDir>
#include <cstdio>
#include <stdexcept>

#include "../../sources/diagram.h"
#include "../../sources/diagramview.h"
#include "../../sources/qetgraphicsitem/conductor.h"
#include "../../sources/qetgraphicsitem/element.h"
#include "../../sources/qetmessagebox.h"
#include "../../sources/qetpalette.h"
#include "../../sources/qetproject.h"
#include "../../sources/ui/diagrambgcolorbutton.h"
#include "../../sources/utils/qetsettings.h"

static void check(bool value, const char *message)
{
	if (!value) throw std::runtime_error(message);
	std::printf("ok: %s\n", message);
}

static void setSheet(const QColor &color, bool custom, bool colored)
{
	PaletteGraphicsView::setCustomBackgroundColor(custom);
	PaletteGraphicsView::setColoredInk(colored);
	Diagram::background_color = color;
}

static QImage grab(DiagramView &view)
{
	view.viewport()->update();
	QApplication::processEvents();
	return view.viewport()->grab().toImage().convertToFormat(QImage::Format_RGB32);
}

static bool dark(const QColor &c) { return c.red() < 90 && c.green() < 90 && c.blue() < 90; }
static bool yellow(const QColor &c) { return c.red() > 140 && c.green() > 140 && c.blue() < 90 && qAbs(c.red() - c.green()) < 40; }
static bool red(const QColor &c) { return c.red() > 140 && c.green() < 90 && c.blue() < 90; }
static bool lightBlue(const QColor &c) { return c.blue() > 180 && c.green() > 120 && c.red() < 170; }
static bool white(const QColor &c) { return c.red() > 140 && c.green() > 140 && c.blue() > 140; }

/// The share of the pixels of \a mask that pass \a test in \a image
static double share(const QImage &image, const QList<QPoint> &mask, bool (*test)(const QColor &))
{
	if (mask.isEmpty()) return 0;
	int n = 0;
	for (const QPoint &p : mask)
		if (test(image.pixelColor(p))) ++n;
	return double(n) / mask.size();
}

int main(int argc, char **argv)
{
	QApplication app(argc, argv);
	std::freopen(argv[2], "w", stdout);
	const QString output = QString::fromLocal8Bit(argv[3]);

	QTemporaryDir settings;
	QSettings::setDefaultFormat(QSettings::IniFormat);
	QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, settings.path());
	QCoreApplication::setOrganizationName("QETSheetInkRegression");
	QETProject::setBackupEnabled(false);
	QET::QetMessageBox::setNonInteractive(true);
	QFontDatabase::addApplicationFont(":/fonts/LiberationSans-Regular.ttf");
	QApplication::setPalette(QET::Palette::fusionLight());

	try
	{
		QETProject project(QString::fromLocal8Bit(argv[1]));
		check(project.state() == QETProject::Ok, "fixture opens");
		project.buildFolios(project.diagrams());
		Diagram *folio = project.diagrams().first();
		check(!folio->elements().isEmpty() && !folio->conductors().isEmpty(), "the folio has symbols and conductors");

		DiagramView view(folio);
		view.resize(900, 700);
		QRectF parts;
		for (Element *e : folio->elements()) parts |= e->sceneBoundingRect();
		view.fitInView(parts.adjusted(-20, -20, 20, 20), Qt::KeepAspectRatio);
		QApplication::processEvents();

			//Where each kind draws black on a white sheet, the other hidden
		setSheet(Qt::white, true, false);
		const auto show = [&](bool elements, bool conductors) {
			for (Element *e : folio->elements()) e->setVisible(elements);
			for (Conductor *c : folio->conductors()) c->setVisible(conductors);
		};
		show(true, false);
		const QImage symbols_only = grab(view);
		show(false, true);
		const QImage wires_only = grab(view);
		show(false, false);
		const QImage sheet_only = grab(view);
		show(true, true);
		const QImage plain = grab(view);
		plain.save(output + "/white.png");
		QList<QPoint> symbol_mask, wire_mask, blank, grid;
		for (int y = 0; y < plain.height(); ++y)
			for (int x = 0; x < plain.width(); ++x) {
				const bool g = dark(sheet_only.pixelColor(x, y));
				const bool s = dark(symbols_only.pixelColor(x, y)) && !g;
				const bool w = dark(wires_only.pixelColor(x, y)) && !g;
				if (s && !w) symbol_mask << QPoint(x, y);
				else if (w && !s) wire_mask << QPoint(x, y);
				else if (g && x > 0 && y > 0 && x + 1 < plain.width() && y + 1 < plain.height()
				         && !dark(sheet_only.pixelColor(x - 1, y)) && !dark(sheet_only.pixelColor(x + 1, y))
				         && !dark(sheet_only.pixelColor(x, y - 1)) && !dark(sheet_only.pixelColor(x, y + 1)))
					grid << QPoint(x, y);  //a lone dot
				else if (!s && !w && !g && white(plain.pixelColor(x, y))) blank << QPoint(x, y);
			}
		std::printf("symbol pixels %lld, conductor pixels %lld\n", qint64(symbol_mask.size()), qint64(wire_mask.size()));
		check(symbol_mask.size() > 200 && wire_mask.size() > 100, "both kinds draw something");
		check(share(plain, symbol_mask, dark) > 0.9 && share(plain, wire_mask, dark) > 0.9,
		      "a white custom sheet keeps the lines black");

		setSheet(Qt::black, true, true);
		QImage colored = grab(view);
		colored.save(output + "/colored.png");
		std::printf("yellow %.3f red %.3f\n", share(colored, symbol_mask, yellow), share(colored, wire_mask, red));
		check(share(colored, symbol_mask, yellow) > 0.85, "symbols, their texts and the free texts are yellow");
		check(share(colored, wire_mask, red) > 0.85, "conductors and their texts are red");
		check(share(colored, blank, dark) > 0.99, "the sheet is black");
		check(grid.size() > 20 && share(colored, grid, white) > 0.9, "the grid dots are white");

		DiagramView::setCrosshairShown(true);
		const QPoint mouse(451, 349);
		QMouseEvent move(QEvent::MouseMove, QPointF(mouse), view.viewport()->mapToGlobal(QPointF(mouse)),
		                 Qt::NoButton, Qt::NoButton, Qt::NoModifier);
		QApplication::sendEvent(view.viewport(), &move);
		check(view.crosshairVisible(), "the crosshair is shown");
		const QImage cross = grab(view);
		cross.save(output + "/crosshair.png");
		const int row = view.mapFromScene(view.crosshairPos()).y();
		int whites = 0;
		for (int r = row - 1; r <= row + 1; ++r) {
			int n = 0;
			for (int x = 0; x < cross.width(); ++x) if (white(cross.pixelColor(x, r))) ++n;
			whites = qMax(whites, n);
		}
		check(whites > cross.width() / 2, "the crosshair is white");
		DiagramView::setCrosshairShown(false);

		QApplication::setPalette(QET::Palette::fusionDark());
		colored = grab(view);
		check(share(colored, symbol_mask, yellow) > 0.85 && share(colored, wire_mask, red) > 0.85,
		      "the same on a dark palette");
		QApplication::setPalette(QET::Palette::fusionLight());

		setSheet(Qt::black, true, false);
		const QImage plain_dark = grab(view);
		plain_dark.save(output + "/black.png");
		check(share(plain_dark, symbol_mask, white) > 0.85 && share(plain_dark, wire_mask, white) > 0.85,
		      "a plain black sheet shows the lines white");

		setSheet(QColor(0x40, 0x40, 0x40), true, false);
		const QImage gray = grab(view);
		check(share(gray, symbol_mask, white) > 0.85, "so does dark gray");
		check(share(gray, grid, white) > 0.9, "with white grid dots");

		setSheet(Qt::white, false, false);
		check(grab(view) == plain, "the system sheet is drawn as before");

			//A dashed conductor (a neutral) and a dash-dotted one (a
			//protective conductor), each drawn alone on a white sheet
		const QList<Conductor *> conductors = folio->conductors();
		check(conductors.size() >= 2, "two conductors to restyle");
		const Qt::PenStyle styles[2] = {Qt::DashLine, Qt::DashDotLine};
		QList<QPoint> style_masks[2];
		for (int i = 0; i < 2; ++i) {
			ConductorProperties properties = conductors[i]->properties();
			properties.style = styles[i];
			conductors[i]->setProperties(properties);
		}
		setSheet(Qt::white, true, false);
		for (int i = 0; i < 2; ++i) {
			for (Element *e : folio->elements()) e->setVisible(false);
			for (Conductor *c : folio->conductors()) c->setVisible(c == conductors[i]);
			const QImage alone = grab(view);
			for (int y = 0; y < alone.height(); ++y)
				for (int x = 0; x < alone.width(); ++x)
					if (dark(alone.pixelColor(x, y)) && !dark(sheet_only.pixelColor(x, y)))
						style_masks[i] << QPoint(x, y);
		}
		show(true, true);
		check(style_masks[0].size() > 50 && style_masks[1].size() > 50, "both draw something");
		setSheet(Qt::black, true, true);
		const QImage styled = grab(view);
		styled.save(output + "/styled.png");
		check(share(styled, style_masks[0], lightBlue) > 0.8, "a dashed conductor is light blue");
		check(share(styled, style_masks[1], yellow) > 0.8, "a dash-dotted conductor is yellow");


			//The scene itself renders the same lines whatever the sheet
		const auto render = [&]() {
			QImage image(600, 400, QImage::Format_ARGB32);
			image.fill(Qt::transparent);
			QPainter painter(&image);
			folio->render(&painter, QRectF(), parts);
			return image;
		};
		setSheet(Qt::white, true, false);
		const QImage before = render();
		setSheet(Qt::white, true, true);
		check(render() == before, "printing and exporting are not tinted");

		setSheet(Qt::white, false, false);
		DiagramBgColorToolButton button(nullptr);
		QAction *dark_mode = button.darkModeAction();
		check(dark_mode && dark_mode->isCheckable() && !dark_mode->isChecked(), "View > Dark mode, off");
		dark_mode->trigger();
		check(PaletteGraphicsView::coloredInk() && PaletteGraphicsView::customBackgroundColor()
		      && Diagram::background_color == QColor(Qt::black), "on: the black sheet with colored lines");
		check(QetSettings::sheetBackground().colored_ink, "kept in the settings");
		dark_mode->trigger();
		check(!PaletteGraphicsView::coloredInk() && !PaletteGraphicsView::customBackgroundColor(),
		      "off: back to the sheet before");

		QetSettings::setSheetBackground({Qt::black, true, true});
		check(QetSettings::sheetBackground().colored_ink, "the choice is kept in the settings");
		QetSettings::setSheetBackground({Qt::white, false, true});
		check(!QetSettings::sheetBackground().colored_ink, "and only for a custom sheet");
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
