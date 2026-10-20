// SPDX-License-Identifier: GPL-2.0-or-later
// Width handles of a symbol's text, linked against the application's objects.
//
// Dragging a width handle of a symbol's text changes its width and keeps
// the point the text is aligned on in place, one of the nine of its
// alignment: a text aligned top right keeps its top right corner, one
// centred its middle. Before, the text always kept its left edge, so a
// drag on the left handle moved the right edge and the text jumped. A
// text narrowed until it wraps grows away from that point. Undo puts back
// the width and the position together. Checked for four alignments, on
// an upright and on a rotated text.
#include <QApplication>
#include <QFontDatabase>
#include <QGraphicsSceneMouseEvent>
#include <QSettings>
#include <QTemporaryDir>
#include <QTextBlock>
#include <QTextDocument>
#include <QTextLayout>
#include <cstdio>
#include <stdexcept>

#include "../../sources/diagram.h"
#include "../../sources/qetproject.h"
#include "../../sources/qetmessagebox.h"
#include "../../sources/qetgraphicsitem/element.h"
#include "../../sources/qetgraphicsitem/dynamicelementtextitem.h"
#include "../../sources/QetGraphicsItemModeler/qetgraphicshandleritem.h"

static void check(bool value, const char *message)
{
	if (!value) throw std::runtime_error(message);
}

static bool near(const QPointF &a, const QPointF &b)
{
	return qAbs(a.x() - b.x()) < 0.01 && qAbs(a.y() - b.y()) < 0.01;
}

static void mouse(Diagram *scene, QGraphicsItem *item, QEvent::Type type, const QPointF &point)
{
	QGraphicsSceneMouseEvent event(type);
	event.setButton(Qt::LeftButton);
	event.setButtons(type == QEvent::GraphicsSceneMouseRelease ? Qt::NoButton : Qt::LeftButton);
	event.setScenePos(point);
	event.setPos(item->mapFromScene(point));
	scene->sendEvent(item, &event);
}

/// The left or right width handle of text, by where it sits on the text.
static QetGraphicsHandlerItem *handle(DynamicElementTextItem *text, bool left)
{
	QetGraphicsHandlerItem *found = nullptr;
	for (QGraphicsItem *child : text->childItems()) {
		if (child->type() != QetGraphicsHandlerItem::Type) continue;
		const qreal x = child->pos().x();
		if (!found || (left ? x < found->pos().x() : x > found->pos().x()))
			found = static_cast<QetGraphicsHandlerItem *>(child);
	}
	check(found, "the selected symbol's text shows its width handles");
	return found;
}

/// The point the text is aligned on, one of its nine, in the folio.
static QPointF anchor(DynamicElementTextItem *text)
{
	const QRectF br = text->boundingRect();
	const Qt::Alignment a = text->alignment();
	const qreal x = (a & Qt::AlignRight) ? br.right() : (a & Qt::AlignHCenter) ? br.center().x() : br.left();
	const qreal y = (a & Qt::AlignBottom) ? br.bottom() : (a & Qt::AlignVCenter) ? br.center().y() : br.top();
	return text->mapToScene(QPointF(x, y));
}

/// Drag one handle by dx along the text's own baseline, in 30 steps as a
/// mouse does: the text moves on every step to keep its alignment point,
/// so each step must be read the same way as the first.
static void drag(Diagram *scene, DynamicElementTextItem *text, bool left, qreal dx)
{
	QetGraphicsHandlerItem *h = handle(text, left);
	const QPointF start = h->scenePos();
	const QPointF end = start + text->mapToScene(QPointF(dx, 0)) - text->mapToScene(QPointF(0, 0));
	mouse(scene, h, QEvent::GraphicsSceneMousePress, start);
	for (int i = 1; i <= 30; ++i)
		mouse(scene, h, QEvent::GraphicsSceneMouseMove, start + (end - start) * i / 30);
	mouse(scene, h, QEvent::GraphicsSceneMouseRelease, end);
}

/// Every line lies inside the text's width: none runs out of it.
static bool linesInside(DynamicElementTextItem *text)
{
	QTextDocument *doc = text->document();
	const qreal inner = doc->textWidth() - 2 * doc->documentMargin();
	for (QTextBlock b = doc->begin(); b.isValid(); b = b.next())
		for (int i = 0; i < b.layout()->lineCount(); ++i) {
			const QTextLine line = b.layout()->lineAt(i);
			if (line.x() < -0.01 || line.x() + line.naturalTextWidth() > inner + 0.01)
				return false;
		}
	return true;
}

/// The width changed by exactly as much as the mouse moved.
static void checkGrowth(DynamicElementTextItem *text, qreal before, qreal moved, const char *message)
{
	check(qAbs(text->textWidth() - before - moved) < 0.5, message);
}

static void checkText(Diagram *scene, Element *element, DynamicElementTextItem *text, Qt::Alignment alignment)
{
	text->setTextWidth(-1);
	text->setAlignment(alignment);
	element->setSelected(true);
	const QPointF pos = text->pos();
	const qreal width = text->boundingRect().width();
	const QPointF fixed = anchor(text);
	const qreal frame = text->frameRect().width();

	// Left handle 30 outwards: wider, the alignment point where it was.
	drag(scene, text, true, -30);
	checkGrowth(text, frame, 30, "the left handle widens the text by as much as the mouse moved");
	check(near(anchor(text), fixed), "dragging the left handle keeps the alignment point in place");
	scene->undoStack().undo();
	check(near(text->pos(), pos), "undo puts the text back where it was");
	check(qAbs(text->boundingRect().width() - width) < 0.01, "undo puts the width back");
	scene->undoStack().redo();
	check(near(anchor(text), fixed), "redo keeps the alignment point in place");
	scene->undoStack().undo();

	// Right handle 30 outwards: the same point stays.
	element->setSelected(true);
	drag(scene, text, false, 30);
	checkGrowth(text, frame, 30, "the right handle widens the text by as much as the mouse moved");
	check(near(anchor(text), fixed), "dragging the right handle keeps the alignment point in place");
	scene->undoStack().undo();
	check(near(text->pos(), pos), "undo of the right handle puts the text back");

	// Narrowed until it wraps onto more lines: it grows away from the
	// alignment point, a text aligned at the top downwards, so that it
	// does not move into what stands above it.
	element->setSelected(true);
	const qreal height = text->boundingRect().height();
	drag(scene, text, false, 25 - width);
	check(text->boundingRect().height() > height + 5, "narrowing the text wraps it onto more lines");
	check(linesInside(text), "a text is not narrower than its widest word: no line runs out of it");
	check(near(anchor(text), fixed), "a text that wraps keeps its alignment point in place");
	scene->undoStack().undo();
	check(near(text->pos(), pos), "undo puts the wrapped text back");
	element->setSelected(false);
}

static void checkAll(Diagram *scene, Element *element, DynamicElementTextItem *text)
{
	checkText(scene, element, text, Qt::AlignLeft | Qt::AlignTop);
	checkText(scene, element, text, Qt::AlignRight | Qt::AlignTop);
	checkText(scene, element, text, Qt::AlignHCenter | Qt::AlignVCenter);
	checkText(scene, element, text, Qt::AlignRight | Qt::AlignBottom);
	checkText(scene, element, text, Qt::AlignLeft | Qt::AlignBottom);
}

int main(int argc, char **argv)
{
	QApplication app(argc, argv);
	std::freopen(argv[2], "w", stdout);

	QTemporaryDir settings;
	QSettings::setDefaultFormat(QSettings::IniFormat);
	QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, settings.path());
	QCoreApplication::setOrganizationName("QETElementTextResizeRegression");
	QETProject::setBackupEnabled(false);
	QET::QetMessageBox::setNonInteractive(true);
	QFontDatabase::addApplicationFont(":/fonts/LiberationSans-Regular.ttf");

	try
	{
		QETProject project(QString::fromLocal8Bit(argv[1]));
		check(project.state() == QETProject::Ok, "fixture opens");
		project.buildFolios(project.diagrams());
		Diagram *scene = project.diagrams().first();

		DynamicElementTextItem *text = nullptr;
		Element *element = nullptr;
		for (Element *e : scene->elements())
			for (DynamicElementTextItem *t : e->dynamicTextItems())
				if (!text) { text = t; element = e; }
		check(text, "the fixture has a symbol with a text");
			// Several words, so that narrowing it makes it wrap
		text->setTextFrom(DynamicElementTextItem::UserText);
		text->setText(QStringLiteral("Sockets bathroom first floor, 5"));

		checkAll(scene, element, text);
		// The same on a text turned by 90 degrees: the fixed point is
		// found along the text's own baseline, not the folio's x axis.
		text->setRotation(90);
		checkAll(scene, element, text);

		std::puts("PASS: a symbol text's width handles keep its alignment point in place, undo restores both");
	}
	catch (const std::exception &error)
	{
		std::printf("FAIL: %s\n", error.what());
		return 1;
	}
	return 0;
}
