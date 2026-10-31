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
#include "../../sources/qetgraphicsitem/diagramimageitem.h"
#include "../../sources/qetgraphicsitem/qetshapeitem.h"
#include "../../sources/qetmessagebox.h"
#include "../../sources/qetpalette.h"
#include "../../sources/qetproject.h"
#include "../../sources/cable/cablepart.h"
#include "../../sources/scripting/qetscriptapi.h"
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
static bool magenta(const QColor &c) { return c.red() > 180 && c.blue() > 180 && c.green() < 90; }
static bool green(const QColor &c) { return c.green() > 150 && c.red() < 90 && c.blue() < 90; }

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

			//A cable across both conductors: its line and cores in the
			//conductors' red, its label in the texts' yellow
		{
			QRectF wires;
			for (Conductor *c : folio->conductors()) wires |= c->sceneBoundingRect();
			const qreal x = qRound(wires.center().x() / 10.0) * 10.0 + 5;
			QetScriptApi api(&project);
			check(!api.addCable(0, QStringLiteral("-W9"), QStringLiteral("NYM-J 3x1,5"),
					    {QStringLiteral("BN"), QStringLiteral("BU")}, QString(),
					    x, wires.top() - 30, x, wires.bottom() + 30).isEmpty(), "a cable across the conductors");
			setSheet(Qt::black, true, true);
			const QImage cabled = grab(view);
			cabled.save(output + "/cable.png");
			CablePart *part = nullptr;
			for (QGraphicsItem *item : folio->items())
				if (item->type() == CablePart::Type) part = static_cast<CablePart *>(item);
			check(part, "the cable is drawn");
			const QRect area = view.mapFromScene(part->sceneBoundingRect()).boundingRect()
					   .intersected(QRect(QPoint(0, 0), cabled.size()));
			int reds = 0, yellows = 0, whites = 0;
			for (int y = area.top(); y <= area.bottom(); ++y)
				for (int x = area.left(); x <= area.right(); ++x) {
					const QColor c = cabled.pixelColor(x, y);
					reds += red(c); yellows += yellow(c);
					whites += white(c) && c.blue() > 200 && c.red() > 220;
				}
			std::printf("cable: red %d yellow %d white %d\n", reds, yellows, whites);
			check(reds > 30, "its line and cores are red");
			check(yellows > 30, "its label is yellow");

				//With one white ink for all (Dark gray or Black, dark mode
				//off) the label is white too: written in white it counted
				//as gray, and the tint gave it the sheet's color
			setSheet(Qt::black, true, false);
			const QImage mono = grab(view);
			mono.save(output + "/cable-mono.png");
			int mono_whites = 0;
			for (int y = area.top(); y <= area.bottom(); ++y)
				for (int x = area.left(); x <= area.right(); ++x)
					mono_whites += white(mono.pixelColor(x, y));
			std::printf("cable mono: white %d (colored: red %d + yellow %d)\n", mono_whites, reds, yellows);
			check(mono_whites > reds + yellows * 4 / 5, "with one white ink its label shows, white like its line");

				//Picked in the dark mode: magenta, not the red of the
				//conductors around it; nor is a highlighted conductor
				//the light blue of the dashed ones, but green
			setSheet(Qt::black, true, true);
			part->setSelected(true);
			const QImage picked = grab(view);
			int magentas = 0;
			for (int y = area.top(); y <= area.bottom(); ++y)
				for (int x = area.left(); x <= area.right(); ++x)
					magentas += magenta(picked.pixelColor(x, y));
			check(magentas > 30, "a picked cable is magenta in the dark mode");
			part->setSelected(false);
		}
		{
			setSheet(Qt::black, true, true);
			Conductor *c = folio->conductors().first();
			const QRect area = view.mapFromScene(c->sceneBoundingRect()).boundingRect()
					   .intersected(QRect(QPoint(0, 0), view.viewport()->size()));
			auto count = [&](const QImage &image, bool (*test)(const QColor &)) {
				int n = 0;
				for (int y = area.top(); y <= area.bottom(); ++y)
					for (int x = area.left(); x <= area.right(); ++x)
						n += test(image.pixelColor(x, y));
				return n;
			};
			c->setSelected(true);
			const QImage picked = grab(view);
			check(count(picked, magenta) > 20, "a picked conductor is magenta in the dark mode");
			c->setSelected(false);
			c->setHighlighted(Conductor::Normal);
			const QImage lit = grab(view);
			check(count(lit, green) > 20 && count(lit, lightBlue) < count(lit, green) / 4,
			      "a highlighted conductor is green in the dark mode");
			c->setHighlighted(Conductor::None);
			setSheet(Qt::black, true, false);
			c->setSelected(true);
			check(count(grab(view), red) > 20, "with one white ink a pick stays red");
			c->setSelected(false);
		}

			//A white rectangle brought in front of a conductor hides it on
			//the screen as it does in print: the tint keeps the stacking
			//order instead of laying all conductors over the shapes
		{
			Conductor *c = folio->conductors().first();
			const QRectF under = c->sceneBoundingRect().adjusted(-5, -5, 5, 5);
			QetScriptApi api(&project);
			check(api.addShape(0, QStringLiteral("rectangle"), under.left(), under.top(),
			                   under.right(), under.bottom()) >= 0, "a rectangle over a conductor");
			QetShapeItem *shape = nullptr;
			for (QGraphicsItem *item : folio->items())
				if (auto *s = dynamic_cast<QetShapeItem *>(item)) shape = s;
			check(shape, "the rectangle is drawn");
			shape->setBrush(QBrush(Qt::white));
			shape->setZValue(1000);
			setSheet(Qt::black, true, true);
			const QRect inside = view.mapFromScene(under).boundingRect().adjusted(4, 4, -4, -4);
			auto reds = [&](const QImage &image) {
				int n = 0;
				for (int y = inside.top(); y <= inside.bottom(); ++y)
					for (int x = inside.left(); x <= inside.right(); ++x)
						n += red(image.pixelColor(x, y));
				return n;
			};
			const QImage covered = grab(view);
			covered.save(output + "/covered.png");
			std::printf("red under the rectangle: %d\n", reds(covered));
			check(reds(covered) == 0, "a shape in front of a conductor hides it on a tinted sheet");
			shape->setZValue(-1000);
			check(reds(grab(view)) > 20, "and sent behind it, the conductor shows");
			shape->setVisible(false);
		}

			//The ink is the painting view's: printing gets none, and nor
			//does a view outside its paint
		check(PaletteGraphicsView::paintingInk(nullptr).isNull()
		      && PaletteGraphicsView::paintingInk(view.viewport()).isNull(), "no ink outside a view's paint");

			//A picture on a dark custom sheet, on a light palette: its frame
			//and caption in the sheet's light ink, and a picture adapted to
			//the dark theme drawn as it is (a dark gray stays dark), as
			//before there was a tint: inverted against the light palette it
			//came out light gray
		{
			QTemporaryDir pictures;
			QImage gray_picture(40, 40, QImage::Format_RGB32);
			gray_picture.fill(QColor(60, 60, 60));
			const QString path = pictures.filePath("gray.png");
			check(gray_picture.save(path), "a dark gray picture to place");
			QetScriptApi api(&project);
			const QPointF at = view.mapToScene(view.viewport()->rect().center() + QPoint(120, 120));
			check(api.addImage(0, path, at.x(), at.y()) >= 0, "the picture is placed");
			DiagramImageItem *picture = nullptr;
			for (QGraphicsItem *item : folio->items())
				if (item->type() == DiagramImageItem::Type) picture = static_cast<DiagramImageItem *>(item);
			check(picture, "the picture is drawn");
			picture->setAdaptToDarkTheme(true);
			setSheet(Qt::black, true, false);
			const QRect rect = view.mapFromScene(picture->sceneBoundingRect()).boundingRect();
			const QImage adapted = grab(view);
			adapted.save(output + "/picture.png");
			std::printf("adapted picture: %s\n", qPrintable(adapted.pixelColor(rect.center()).name()));
			check(dark(adapted.pixelColor(rect.center())), "an adapted picture is drawn as it is on a light palette");
			picture->setSelected(true);
			const QImage framed = grab(view);
			int frame_whites = 0, frame_pixels = 0;
			for (int x = rect.left() - 1; x <= rect.right() + 1; ++x)
				for (int y : {rect.top() - 1, rect.top(), rect.bottom(), rect.bottom() + 1}) {
					++frame_pixels;
					frame_whites += white(framed.pixelColor(x, y));
				}
			std::printf("picture frame: %d of %d white\n", frame_whites, frame_pixels);
			check(frame_whites > frame_pixels / 8, "its selection frame is light on the dark sheet");
			picture->setSelected(false);
			picture->setVisible(false);	//the folio's QGIManager deletes it
		}

			//Two editor windows share one sheet: the dark mode switched on
			//in one is checked in the other, and switched off there it
			//brings back the sheet from before
		{
			setSheet(Qt::white, false, false);
			DiagramBgColorToolButton a(nullptr), b(nullptr);
			a.darkModeAction()->trigger();
			check(b.darkModeAction()->isChecked(), "on in one window: checked in the other");
			b.darkModeAction()->trigger();
			check(!a.darkModeAction()->isChecked() && !PaletteGraphicsView::customBackgroundColor(),
			      "off in the other: unchecked in both, the sheet from before");
		}

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
