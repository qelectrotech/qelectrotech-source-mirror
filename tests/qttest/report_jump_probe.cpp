// SPDX-License-Identifier: GPL-2.0-or-later
// Jumping between the two ends of a folio report, linked against the
// application's objects.
//
// A linked folio report shows a pointing hand under the mouse, on its
// arrow and on the text that shows its label, and a double click on
// either goes to the other end. The end jumped to is highlighted for a
// moment, its texts too, independently of the hover highlight, which the mouse leaving
// the end jumped from would clear at once. An unlinked report shows no
// hand and has nowhere to jump to.
#include <QApplication>
#include <QEventLoop>
#include <QFontDatabase>
#include <QGraphicsSceneHoverEvent>
#include <QGraphicsSceneMouseEvent>
#include <QSettings>
#include <QTemporaryDir>
#include <QTimer>
#include <cstdio>
#include <stdexcept>

#include "../../sources/diagram.h"
#include "../../sources/qetproject.h"
#include "../../sources/qetmessagebox.h"
#include "../../sources/qetgraphicsitem/element.h"
#include "../../sources/qetgraphicsitem/dynamicelementtextitem.h"

static void check(bool value, const char *message)
{
	if (!value) throw std::runtime_error(message);
}

static void hover(Diagram *scene, QGraphicsItem *item, QEvent::Type type)
{
	QGraphicsSceneHoverEvent event(type);
	event.setScenePos(item->sceneBoundingRect().center());
	event.setPos(item->boundingRect().center());
	scene->sendEvent(item, &event);
}

static void doubleClick(Diagram *scene, QGraphicsItem *item)
{
	QGraphicsSceneMouseEvent event(QEvent::GraphicsSceneMouseDoubleClick);
	event.setButton(Qt::LeftButton);
	event.setButtons(Qt::LeftButton);
	event.setScenePos(item->sceneBoundingRect().center());
	event.setPos(item->boundingRect().center());
	scene->sendEvent(item, &event);
}

static void wait(int ms)
{
	QEventLoop loop;
	QTimer::singleShot(ms, &loop, &QEventLoop::quit);
	loop.exec();
}

static bool showsHand(const QGraphicsItem *item)
{
	return item->hasCursor() && item->cursor().shape() == Qt::PointingHandCursor;
}

int main(int argc, char **argv)
{
	QApplication app(argc, argv);
	std::freopen(argv[2], "w", stdout);

	QTemporaryDir settings;
	QSettings::setDefaultFormat(QSettings::IniFormat);
	QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, settings.path());
	QCoreApplication::setOrganizationName("QETReportJumpRegression");
	QETProject::setBackupEnabled(false);
	QET::QetMessageBox::setNonInteractive(true);
	QFontDatabase::addApplicationFont(":/fonts/LiberationSans-Regular.ttf");

	try
	{
		QETProject project(QString::fromLocal8Bit(argv[1]));
		check(project.state() == QETProject::Ok, "fixture opens");
		check(project.diagrams().size() == 2, "fixture has two folios");
		project.buildFolios(project.diagrams());
		Diagram *one = project.diagrams().at(0);
		Diagram *two = project.diagrams().at(1);

		// Folio one: a going report linked to folio two, and one alone.
		Element *going = nullptr, *alone = nullptr;
		for (Element *element : one->elements()) {
			if (!(element->linkType() & Element::AllReport)) continue;
			(element->linkedElements().isEmpty() ? alone : going) = element;
		}
		check(going && alone, "a linked and an unlinked report on folio one");
		Element *coming = going->linkedElements().first();
		check(coming->diagram() == two, "the linked end is on folio two");

		check(going->jumpTarget() == coming, "a linked report jumps to its other end");
		check(alone->jumpTarget() == nullptr, "an unlinked report has nowhere to jump to");

		// The arrow: a pointing hand while the mouse is over it, only
		// when linked.
		hover(one, going, QEvent::GraphicsSceneHoverEnter);
		check(showsHand(going), "pointing hand over a linked report");
		hover(one, going, QEvent::GraphicsSceneHoverLeave);
		check(!going->hasCursor(), "hand gone when the mouse leaves");
		hover(one, alone, QEvent::GraphicsSceneHoverEnter);
		check(!showsHand(alone), "no hand over an unlinked report");
		hover(one, alone, QEvent::GraphicsSceneHoverLeave);

		// Double click on the arrow: the other end is selected and
		// highlighted, and stays highlighted when the mouse leaves the
		// end jumped from.
		hover(one, going, QEvent::GraphicsSceneHoverEnter);
		doubleClick(one, going);
		check(coming->isSelected(), "the end jumped to is selected");
		check(coming->isShownAsJumpTarget(), "the end jumped to is highlighted");
		{
			bool texts_shaded = !coming->dynamicTextItems().isEmpty();
			for (DynamicElementTextItem *text : coming->dynamicTextItems())
				texts_shaded &= text->isJumpHighlighted();
			check(texts_shaded, "and its texts are shaded too");
		}
		hover(one, going, QEvent::GraphicsSceneHoverLeave);
		check(coming->isShownAsJumpTarget(), "leaving the report keeps the jump highlight");
		wait(3300);
		check(!coming->isShownAsJumpTarget(), "the jump highlight goes after a moment");
		{
			bool texts_plain = true;
			for (DynamicElementTextItem *text : coming->dynamicTextItems())
				texts_plain &= !text->isJumpHighlighted();
			check(texts_plain, "and so does the shading of its texts");
		}

		// The text showing the label: a pointing hand, and a double click
		// jumps back.
		DynamicElementTextItem *label = nullptr;
		for (DynamicElementTextItem *text : coming->dynamicTextItems())
			if (text->linkedElementShown()) label = text;
		check(label, "the coming report has a text that leads to its other end");
		check(label->linkedElementShown() == going, "the text leads to the going report");
		hover(two, label, QEvent::GraphicsSceneHoverEnter);
		check(showsHand(label), "pointing hand over the label text");
		hover(two, label, QEvent::GraphicsSceneHoverLeave);
		check(!label->hasCursor(), "hand gone when the mouse leaves the text");
		doubleClick(two, label);
		check(going->isSelected(), "a double click on the text jumps back");
		check(going->isShownAsJumpTarget(), "the end jumped back to is highlighted");

		std::puts("PASS: linked folio reports show a hand and jump to their highlighted other end");
	}
	catch (const std::exception &error)
	{
		std::printf("FAIL: %s\n", error.what());
		return 1;
	}
	return 0;
}
