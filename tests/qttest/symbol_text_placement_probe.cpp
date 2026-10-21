// SPDX-License-Identifier: GPL-2.0-or-later
// Placing the texts of a symbol at one of nine points, linked against the
// application's objects.
//
// "Place symbol texts" puts the label and the notes of a symbol as one
// block beside its drawing, 3 px away, at one of the nine points of the
// text alignment dialog. Left of the symbol the lines are aligned right,
// so that they hang on it and a longer text grows away from it; right of
// it aligned left; above it centred. The lines keep their order and their
// spacing, a group of texts is placed as one line, a turned symbol is
// placed in the folio's directions, and one undo puts everything back.
#include <QApplication>
#include <QFontDatabase>
#include <QImage>
#include <QPainter>
#include <QSettings>
#include <QTemporaryDir>
#include <QUndoCommand>
#include <cstdio>
#include <stdexcept>

#include "../../sources/diagram.h"
#include "../../sources/qetproject.h"
#include "../../sources/qetmessagebox.h"
#include "../../sources/symboltextplacement.h"
#include "../../sources/qetgraphicsitem/element.h"
#include "../../sources/qetgraphicsitem/dynamicelementtextitem.h"
#include "../../sources/qetgraphicsitem/elementtextitemgroup.h"

static void check(bool value, const char *message)
{
	if (!value) throw std::runtime_error(message);
	std::printf("ok: %s\n", message);
}

static bool same(qreal a, qreal b) { return qAbs(a - b) < 0.6; }

static QRectF body(Element *e) { return e->mapRectToScene(e->drawingRect()); }

static QRectF block(const QList<DynamicElementTextItem *> &texts)
{
	QRectF r;
	for (DynamicElementTextItem *t : texts) r |= t->sceneBoundingRect();
	return r;
}

int main(int argc, char **argv)
{
	QApplication app(argc, argv);
	std::freopen(argv[2], "w", stdout);

	QTemporaryDir settings;
	QSettings::setDefaultFormat(QSettings::IniFormat);
	QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, settings.path());
	QCoreApplication::setOrganizationName("QETSymbolTextPlacementRegression");
	QETProject::setBackupEnabled(false);
	QET::QetMessageBox::setNonInteractive(true);
	QFontDatabase::addApplicationFont(":/fonts/LiberationSans-Regular.ttf");

	try
	{
		QETProject project(QString::fromLocal8Bit(argv[1]));
		check(project.state() == QETProject::Ok, "fixture opens");
		project.buildFolios(project.diagrams());
		Diagram *scene = project.diagrams().first();
		Element *element = scene->elements().first();
		check(element && !element->drawingRect().isEmpty(), "the fixture has a drawn symbol");
		check(element->drawingRect().width() <= element->boundingRect().width(),
			  "the drawing is no wider than the frame");

			//The symbol's own texts and three of ours, to the right of it
		for (DynamicElementTextItem *t : element->dynamicTextItems()) t->setVisible(false);
		QList<DynamicElementTextItem *> texts;
		const QStringList lines {"-FI1", "40 A / 30 mA, type A", "FI UG"};
		for (int i = 0; i < lines.size(); ++i) {
			auto *t = new DynamicElementTextItem(element);
			t->setTextFrom(DynamicElementTextItem::UserText);
			t->setText(lines[i]);
			t->setPos(80 + 5 * i, -20 + 12 * i);
			element->addDynamicTextItem(t);
			texts << t;
		}
		QList<QPointF> start;
		for (DynamicElementTextItem *t : texts) start << t->pos();
		const QRectF before = block(texts);

		// Left of the symbol, halfway up: aligned right, 3 px away.
		QUndoCommand *undo = SymbolTextPlacement::place({element}, Qt::AlignLeft | Qt::AlignVCenter);
		check(undo, "placing gives one undo step");
		scene->undoStack().push(undo);
		QRectF b = block(texts);
		check(same(b.right(), body(element).left() - SymbolTextPlacement::gap), "left: the block ends 3 px before the drawing");
		check(same(b.center().y(), body(element).center().y()), "left: halfway up the drawing");
		bool right_aligned = true, keeps_order = true;
		for (int i = 0; i < texts.size(); ++i) {
			right_aligned &= same(texts[i]->sceneBoundingRect().right(), b.right());
			right_aligned &= bool(texts[i]->alignment() & Qt::AlignRight);
			if (i) keeps_order &= same(texts[i]->sceneBoundingRect().top() - texts[i-1]->sceneBoundingRect().top(), 12);
		}
		check(right_aligned, "left: the lines are aligned right, on the screen and as their alignment");
		check(keeps_order, "the lines keep their order and their spacing");
		check(same(b.height(), before.height()), "the block keeps its height");

		scene->undoStack().undo();
		bool back = true;
		for (int i = 0; i < texts.size(); ++i) back &= texts[i]->pos() == start[i] && !(texts[i]->alignment() & Qt::AlignRight);
		check(back, "one undo puts every text and its alignment back");
		scene->undoStack().redo();

		// Right of it: aligned left. Above it: centred.
		scene->undoStack().push(SymbolTextPlacement::place({element}, Qt::AlignRight | Qt::AlignVCenter));
		b = block(texts);
		check(same(b.left(), body(element).right() + SymbolTextPlacement::gap), "right: the block starts 3 px after the drawing");
		check(same(texts[1]->sceneBoundingRect().left(), b.left()) && (texts[1]->alignment() & Qt::AlignLeft),
			  "right: the lines are aligned left");
		scene->undoStack().push(SymbolTextPlacement::place({element}, Qt::AlignHCenter | Qt::AlignTop));
		b = block(texts);
		check(same(b.bottom(), body(element).top() - SymbolTextPlacement::gap), "above: the block ends 3 px above the drawing");
		check(same(texts[0]->sceneBoundingRect().center().x(), body(element).center().x()), "above: the lines are centred on it");

		// A turned symbol: left in the folio is still left.
		element->setRotation(90);
		scene->undoStack().push(SymbolTextPlacement::place({element}, Qt::AlignLeft | Qt::AlignVCenter));
		b = block(texts);
		check(same(b.right(), body(element).left() - SymbolTextPlacement::gap), "turned symbol: the block is left of it in the folio");
		element->setRotation(0);

		// A group of texts is one line of the block, aligned like it.
		ElementTextItemGroup *group = element->addTextGroup(QStringLiteral("g"));
		element->addTextToGroup(texts[1], group);
		element->addTextToGroup(texts[2], group);
		scene->undoStack().push(SymbolTextPlacement::place({element}, Qt::AlignLeft | Qt::AlignVCenter));
		QRectF with_group = texts[0]->sceneBoundingRect() | group->sceneBoundingRect();
		check(same(with_group.right(), body(element).left() - SymbolTextPlacement::gap)
			  && same(group->sceneBoundingRect().right(), with_group.right()),
			  "a group is placed as one line of the block");
		check(group->alignment() == Qt::AlignRight, "and is aligned right too");

		// A group whose texts lie freely (AlignJustify, the default of the
		// "Label and comment" group): placing it lays its texts out anew,
		// and one undo puts them back where they lay, not only the group.
		scene->undoStack().undo();
		group->setAlignment(Qt::AlignJustify);
		texts[1]->setPos(0, 0);
		texts[2]->setPos(5, 12);
		const QPointF group_pos = group->pos();
		scene->undoStack().push(SymbolTextPlacement::place({element}, Qt::AlignRight | Qt::AlignVCenter));
		check(texts[2]->pos() != QPointF(5, 12), "placing lays the group's texts out anew");
		scene->undoStack().undo();
		check(group->alignment() == Qt::AlignJustify && group->pos() == group_pos,
			  "undo puts the group's alignment and place back");
		check(texts[1]->pos() == QPointF(0, 0) && texts[2]->pos() == QPointF(5, 12),
			  "undo puts the group's texts back where they lay");
		scene->undoStack().redo();
		check(group->alignment() == Qt::AlignLeft && texts[2]->pos() != QPointF(5, 12),
			  "redo places them again");

		// A symbol with no texts: nothing to undo.
		Element *other = nullptr;
		for (Element *e : scene->elements()) if (e != element) other = e;
		if (other) {
			for (DynamicElementTextItem *t : other->dynamicTextItems()) t->setVisible(false);
			check(SymbolTextPlacement::place({other}, Qt::AlignLeft) == nullptr, "a symbol without texts gives no undo step");
		}
		// The icons show where the texts go: bars left of the grey square
		// for "left", right of it for "right", above it for "above".
		auto dark = [](const QImage &img, const QRect &r) {
			int n = 0;
			for (int y = r.top(); y <= r.bottom(); ++y)
				for (int x = r.left(); x <= r.right(); ++x) {
					const QColor c = img.pixelColor(x, y);
					if (c.alpha() > 128 && c.lightness() < 60) ++n;
				}
			return n;
		};
		const QImage left = SymbolTextPlacement::icon(Qt::AlignLeft | Qt::AlignVCenter).pixmap(24, 24).toImage();
		const QImage right = SymbolTextPlacement::icon(Qt::AlignRight | Qt::AlignVCenter).pixmap(24, 24).toImage();
		const QImage above = SymbolTextPlacement::icon(Qt::AlignHCenter | Qt::AlignTop).pixmap(24, 24).toImage();
		check(dark(left, QRect(0, 8, 7, 8)) > 10 && dark(left, QRect(17, 0, 7, 24)) == 0, "the icon for left has its bars left of the symbol");
		check(dark(right, QRect(17, 8, 7, 8)) > 10 && dark(right, QRect(0, 0, 7, 24)) == 0, "the icon for right has them right of it");
		check(dark(above, QRect(6, 0, 12, 7)) > 10 && dark(above, QRect(0, 17, 24, 7)) == 0, "the icon for above has them above it");

			//All nine side by side, to look at
		QImage sheet(3 * 52, 3 * 52, QImage::Format_ARGB32);
		sheet.fill(Qt::white);
		QPainter p(&sheet);
		const Qt::Alignment nine[] = {
			Qt::AlignLeft | Qt::AlignTop, Qt::AlignHCenter | Qt::AlignTop, Qt::AlignRight | Qt::AlignTop,
			Qt::AlignLeft | Qt::AlignVCenter, Qt::AlignCenter, Qt::AlignRight | Qt::AlignVCenter,
			Qt::AlignLeft | Qt::AlignBottom, Qt::AlignHCenter | Qt::AlignBottom, Qt::AlignRight | Qt::AlignBottom};
		for (int i = 0; i < 9; ++i)
			p.drawPixmap((i % 3) * 52 + 2, (i / 3) * 52 + 2, SymbolTextPlacement::icon(nine[i]).pixmap(48, 48));
		p.end();
		if (argc > 3) sheet.save(QString::fromLocal8Bit(argv[3]) + "/icons.png");

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
