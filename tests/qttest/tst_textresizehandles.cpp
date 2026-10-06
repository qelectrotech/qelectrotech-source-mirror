/*
	Copyright 2006-2026 The QElectroTech Team
	This file is part of QElectroTech.

	QElectroTech is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 2 of the License, or
	(at your option) any later version.

	QElectroTech is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with QElectroTech.  If not, see <http://www.gnu.org/licenses/>.
*/
#include <QtTest>
#include <QGraphicsScene>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsTextItem>
#include <QSignalSpy>
#include <QTextBlock>
#include <QTextLayout>
#include <QTextOption>
#include <QUndoStack>

#include <cmath>

#include "QetGraphicsItemModeler/qetgraphicshandleritem.h"
#include "QetGraphicsItemModeler/textresizehandles.h"

using TextResize::Corner;

/**
	A text with the "textWidth" property the texts of QElectroTech have.
*/
class WidthText : public QGraphicsTextItem
{
		Q_OBJECT
		Q_PROPERTY(qreal textWidth READ textWidth WRITE setWidth)

	public:
		WidthText(const QString &text) : QGraphicsTextItem(text)
		{
			QTextOption option = document()->defaultTextOption();
			option.setWrapMode(QTextOption::WordWrap);
			document()->setDefaultTextOption(option);
		}
		void setWidth(qreal width) {setTextWidth(width);}
};

/// The number of lines the text is laid out on.
static int lineCount(const QGraphicsTextItem &text)
{
	text.document()->size();
	int lines = 0;
	for (QTextBlock block = text.document()->begin() ; block.isValid() ; block = block.next())
		lines += block.layout()->lineCount();
	return lines;
}

static bool samePoint(const QPointF &a, const QPointF &b)
{
	return qAbs(a.x() - b.x()) < 1e-6 && qAbs(a.y() - b.y()) < 1e-6;
}

static QPointF sceneCorner(const QGraphicsTextItem &text, Corner corner)
{
	return text.mapToScene(TextResize::cornerOf(text.boundingRect(), corner));
}

/// Send a mouse event of type to the handle at corner, as the scene would.
static void sendMouse(QGraphicsScene &scene, TextResizeHandles &handles,
					  Corner corner, QEvent::Type type, const QPointF &scene_pos)
{
	QGraphicsSceneMouseEvent event(type);
	event.setButton(Qt::LeftButton);
	event.setButtons(type == QEvent::GraphicsSceneMouseRelease ? Qt::NoButton : Qt::LeftButton);
	event.setScenePos(scene_pos);
	scene.sendEvent(handles.handle(corner), &event);
}

/// Drag the handle at corner from where it is by offset, in scene coordinates.
static void drag(QGraphicsScene &scene, TextResizeHandles &handles,
				 WidthText &text, Corner corner, const QPointF &offset)
{
	const QPointF start = sceneCorner(text, corner);
	sendMouse(scene, handles, corner, QEvent::GraphicsSceneMousePress, start);
	sendMouse(scene, handles, corner, QEvent::GraphicsSceneMouseMove, start + offset / 2);
	sendMouse(scene, handles, corner, QEvent::GraphicsSceneMouseMove, start + offset);
	sendMouse(scene, handles, corner, QEvent::GraphicsSceneMouseRelease, start + offset);
}

class tst_textresizehandles : public QObject
{
	Q_OBJECT

private slots:
	// Pure geometry: the width given by a drag does not change while the
	// text moves to keep its opposite corner in place.
	void widthForDragUsesThePressTransform()
	{
		WidthText text(QStringLiteral("Motor protection switch"));
		text.setRotation(37);
		text.setPos(10, 10);
		const QTransform press_inverse = text.sceneTransform().inverted();
		const QPointF fixed = TextResize::cornerOf(text.boundingRect(), TextResize::TopLeft);
		const QPointF mouse = text.mapToScene(QPointF(80, 3));

		const qreal width = TextResize::widthForDrag(press_inverse, fixed, mouse, TextResize::BottomRight, 0);
		QCOMPARE(qRound(width * 1000), 80000);

		text.setPos(500, -300);
		QCOMPARE(TextResize::widthForDrag(press_inverse, fixed, mouse, TextResize::BottomRight, 0), width);
	}

	// The minimum width is the longest word: a text that narrow does not
	// break any word, one narrower would.
	void minimumWidthIsTheLongestWord()
	{
		WidthText text(QStringLiteral("a Motorschutzschalter b"));
		const qreal minimum = TextResize::minimumWidth(text.document());

		WidthText word(QStringLiteral("Motorschutzschalter"));
		QVERIFY(qAbs(minimum - word.boundingRect().width()) < 0.5);

		text.setWidth(minimum);
		QVERIFY(qAbs(text.boundingRect().width() - minimum) < 0.5);
		QCOMPARE(lineCount(text), 3);
	}

	// The minimum width never cuts a word, even for a document that would
	// otherwise break a word anywhere.
	void minimumWidthKeepsWords()
	{
		QGraphicsTextItem text(QStringLiteral("a Motorschutzschalter"));
		QTextOption option = text.document()->defaultTextOption();
		option.setWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);
		text.document()->setDefaultTextOption(option);

		QGraphicsTextItem word(QStringLiteral("Motorschutzschalter"));
		QVERIFY(qAbs(TextResize::minimumWidth(text.document()) - word.boundingRect().width()) < 0.5);
	}

	// pinCorner() puts a corner where it was, for each corner, rotation,
	// rotation point and inside a rotated parent.
	void pinCornerKeepsTheCorner()
	{
		QGraphicsRectItem parent(0, 0, 10, 10);
		parent.setRotation(30);
		for (bool with_parent : {false, true})
			for (int c = TextResize::TopLeft ; c <= TextResize::BottomLeft ; ++c)
				for (qreal angle : {0.0, 90.0, 37.0})
					for (bool centre : {false, true}) {
						WidthText text(QStringLiteral("Motor protection switch"));
						if (with_parent)
							text.setParentItem(&parent);
						text.setRotation(angle);
						text.setTransformOriginPoint(centre ? text.boundingRect().center() : QPointF());
						const Corner corner = Corner(c);
						const QPointF before = sceneCorner(text, corner);

						text.setWidth(70);
						if (centre)
							text.setTransformOriginPoint(text.boundingRect().center());
						TextResize::pinCorner(&text, corner, before);
						QVERIFY2(samePoint(sceneCorner(text, corner), before),
								 qPrintable(QStringLiteral("corner %1 angle %2 centre %3 parent %4")
											.arg(c).arg(angle).arg(centre).arg(with_parent)));
						text.setParentItem(nullptr);
					}
	}

	// Dragging a handle keeps the opposite corner in place, wraps the text
	// and reports the change once, on release.
	void dragKeepsTheOppositeCorner()
	{
		for (int c = TextResize::TopLeft ; c <= TextResize::BottomLeft ; ++c)
			for (qreal angle : {0.0, 90.0, 37.0}) {
				QGraphicsScene scene;
				auto *text = new WidthText(QStringLiteral("Motor protection switch Q12"));
				scene.addItem(text);
				text->setPos(100, 100);
				text->setRotation(angle);
				auto *handles = new TextResizeHandles(text);
				QSignalSpy finished(handles, &TextResizeHandles::resizeFinished);

				const Corner corner = Corner(c);
				const QPointF fixed = sceneCorner(*text, TextResize::opposite(corner));
				const qreal natural = text->boundingRect().width();
				const QPointF start_pos = text->pos();

					//Narrower by 40 along the text, whatever its rotation;
					//a vertical move of the mouse is ignored.
				QTransform turn;
				turn.rotate(angle);
				const qreal sign = TextResize::isLeft(corner) ? 1 : -1;
				drag(scene, *handles, *text, corner, turn.map(QPointF(sign * 40, 25)));

				const QString what = QStringLiteral("corner %1 angle %2").arg(c).arg(angle);
				QCOMPARE(text->textWidth(), qreal(qRound(natural - 40)));
				QVERIFY2(lineCount(*text) > 1, qPrintable(what));
				QVERIFY2(samePoint(sceneCorner(*text, TextResize::opposite(corner)), fixed), qPrintable(what));

				QCOMPARE(finished.count(), 1);
				const QList<QVariant> args = finished.first();
				QCOMPARE(args.at(0).toReal(), qreal(-1));
				QCOMPARE(args.at(1).toReal(), text->textWidth());
				QCOMPARE(args.at(2).toPointF(), start_pos);
				QCOMPARE(args.at(3).toPointF(), text->pos());

					//The handles follow the new box
				QVERIFY(samePoint(handles->handle(corner)->scenePos(), sceneCorner(*text, corner)));
			}
	}

	// The handles still work after the text left its scene and came back,
	// as undoing a delete does: Qt drops scene event filters on removal.
	void dragAfterLeavingTheScene()
	{
		QGraphicsScene scene;
		auto *text = new WidthText(QStringLiteral("Motor protection switch Q12"));
		scene.addItem(text);
		auto *handles = new TextResizeHandles(text);

		scene.removeItem(text);
		scene.addItem(text);

		const QPointF start_pos = text->pos();
		drag(scene, *handles, *text, TextResize::BottomRight, QPointF(-40, 0));
		QVERIFY(text->textWidth() > 0);
		QCOMPARE(text->pos(), start_pos);
		delete text;
	}

	// The handles also work when they are created before the text is in a
	// scene.
	void dragWhenCreatedOutsideAScene()
	{
		QGraphicsScene scene;
		auto *text = new WidthText(QStringLiteral("Motor protection switch Q12"));
		auto *handles = new TextResizeHandles(text);
		scene.addItem(text);

		drag(scene, *handles, *text, TextResize::BottomRight, QPointF(-40, 0));
		QVERIFY(text->textWidth() > 0);
	}

	// A text is never made narrower than its longest word.
	void dragStopsAtTheLongestWord()
	{
		QGraphicsScene scene;
		auto *text = new WidthText(QStringLiteral("a Motorschutzschalter b"));
		scene.addItem(text);
		auto *handles = new TextResizeHandles(text);
		drag(scene, *handles, *text, TextResize::BottomRight, QPointF(-1000, 0));
		QCOMPARE(text->textWidth(), std::ceil(TextResize::minimumWidth(text->document())));
	}

	// A press and release without moving changes nothing.
	void clickChangesNothing()
	{
		QGraphicsScene scene;
		auto *text = new WidthText(QStringLiteral("Motor protection switch"));
		scene.addItem(text);
		auto *handles = new TextResizeHandles(text);
		QSignalSpy finished(handles, &TextResizeHandles::resizeFinished);
		drag(scene, *handles, *text, TextResize::TopRight, QPointF());
		QCOMPARE(finished.count(), 0);
		QCOMPARE(text->textWidth(), qreal(-1));
	}

	// Double-clicking a handle goes back to the automatic width.
	void doubleClickResetsToAutomaticWidth()
	{
		QGraphicsScene scene;
		auto *text = new WidthText(QStringLiteral("Motor protection switch"));
		scene.addItem(text);
		text->setWidth(60);
		auto *handles = new TextResizeHandles(text);
		QSignalSpy finished(handles, &TextResizeHandles::resizeFinished);

		sendMouse(scene, *handles, TextResize::BottomLeft, QEvent::GraphicsSceneMouseDoubleClick,
				  sceneCorner(*text, TextResize::BottomLeft));
		QCOMPARE(text->textWidth(), qreal(-1));
		QCOMPARE(finished.count(), 1);
		QCOMPARE(finished.first().at(0).toReal(), qreal(60));
		QCOMPARE(finished.first().at(1).toReal(), qreal(-1));

			//Already automatic: nothing to undo
		sendMouse(scene, *handles, TextResize::BottomLeft, QEvent::GraphicsSceneMouseDoubleClick,
				  sceneCorner(*text, TextResize::BottomLeft));
		QCOMPARE(finished.count(), 1);
	}

	// One undo step gives back the width and the position, one redo
	// applies them again.
	void undoRestoresWidthAndPosition()
	{
		QGraphicsScene scene;
		auto *text = new WidthText(QStringLiteral("Motor protection switch Q12"));
		scene.addItem(text);
		text->setPos(100, 100);
		text->setRotation(90);
		auto *handles = new TextResizeHandles(text);
		QUndoStack stack;
		connect(handles, &TextResizeHandles::resizeFinished, &stack,
				[&stack, text](qreal ow, qreal nw, QPointF op, QPointF np) {
			stack.push(new TextResizeCommand(text, ow, nw, op, np));
		});

		drag(scene, *handles, *text, TextResize::TopLeft, QPointF(0, 40));
		QCOMPARE(stack.count(), 1);
		const qreal new_width = text->textWidth();
		const QPointF new_pos = text->pos();
		QVERIFY(new_pos != QPointF(100, 100));

		stack.undo();
		QCOMPARE(text->textWidth(), qreal(-1));
		QCOMPARE(text->pos(), QPointF(100, 100));
		stack.redo();
		QCOMPARE(text->textWidth(), new_width);
		QCOMPARE(text->pos(), new_pos);
	}
};

QTEST_MAIN(tst_textresizehandles)
#include "tst_textresizehandles.moc"
