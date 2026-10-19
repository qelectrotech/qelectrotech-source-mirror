// SPDX-License-Identifier: GPL-2.0-or-later
// Width handles of a symbol's text, linked against the application's objects.
//
// Dragging the left handle of a symbol's text changes its width and keeps
// its right edge in place; dragging the right handle keeps its left edge,
// as in a drawing program. Before, the text always kept its left edge, so
// a drag on the left handle moved the right edge and the text jumped.
// Undo puts back the width and the position together. Checked for an
// upright and a rotated text.
#include <QApplication>
#include <QFontDatabase>
#include <QGraphicsSceneMouseEvent>
#include <QSettings>
#include <QTemporaryDir>
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

static QPointF edge(DynamicElementTextItem *text, bool left)
{
	const QRectF br = text->boundingRect();
	return text->mapToScene(QPointF(left ? br.left() : br.right(), br.center().y()));
}

/// Drag one handle by dx along the text's own baseline.
static void drag(Diagram *scene, DynamicElementTextItem *text, bool left, qreal dx)
{
	QetGraphicsHandlerItem *h = handle(text, left);
	const QPointF start = h->scenePos();
	const QPointF end = start + text->mapToScene(QPointF(dx, 0)) - text->mapToScene(QPointF(0, 0));
	mouse(scene, h, QEvent::GraphicsSceneMousePress, start);
	mouse(scene, h, QEvent::GraphicsSceneMouseMove, (start + end) / 2);
	mouse(scene, h, QEvent::GraphicsSceneMouseMove, end);
	mouse(scene, h, QEvent::GraphicsSceneMouseRelease, end);
}

static void checkText(Diagram *scene, Element *element, DynamicElementTextItem *text)
{
	element->setSelected(true);
	const QPointF pos = text->pos();
	const qreal width = text->boundingRect().width();
	const QPointF right = edge(text, false);

	// Left handle 30 to the left: wider, right edge where it was.
	drag(scene, text, true, -30);
	check(text->boundingRect().width() > width + 10, "the left handle widens the text");
	check(near(edge(text, false), right), "dragging the left handle keeps the right edge in place");

	scene->undoStack().undo();
	check(near(text->pos(), pos), "undo puts the text back where it was");
	check(qAbs(text->boundingRect().width() - width) < 0.01, "undo puts the width back");
	scene->undoStack().redo();
	check(near(edge(text, false), right), "redo keeps the right edge in place");
	scene->undoStack().undo();

	// Right handle 30 to the right: wider, left edge where it was.
	element->setSelected(true);
	const QPointF left = edge(text, true);
	drag(scene, text, false, 30);
	check(text->boundingRect().width() > width + 10, "the right handle widens the text");
	check(near(edge(text, true), left), "dragging the right handle keeps the left edge in place");
	scene->undoStack().undo();
	check(near(text->pos(), pos), "undo of the right handle leaves the position alone");
	element->setSelected(false);
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

		checkText(scene, element, text);
		// The same on a text turned by 90 degrees: the fixed edge is
		// found along the text's own baseline, not the folio's x axis.
		text->setRotation(90);
		checkText(scene, element, text);

		std::puts("PASS: a symbol text's width handles keep the opposite edge in place, undo restores both");
	}
	catch (const std::exception &error)
	{
		std::printf("FAIL: %s\n", error.what());
		return 1;
	}
	return 0;
}
