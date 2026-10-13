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
#include "qetapptest.h"
#include "qetgraphicsitem/diagramimageitem.h"

#include <QDomDocument>

/**
	Space -- the editor's "rotate_selection" action -- on a picture turns
	the picture's own transform (rotationAngle, around its pivot), the
	rotation its file saves. Before #1413 it turned QGraphicsItem's plain
	rotation, which toXml() does not save, so the picture came back
	unrotated once the project was reopened.
*/
class tst_rotateselectionimage : public QObject
{
	Q_OBJECT

	Diagram *m_diagram = nullptr;

	DiagramImageItem *addPicture(const QPointF &pos)
	{
		QPixmap pixmap(100, 60);
		pixmap.fill(Qt::darkGreen);
		auto *picture = new DiagramImageItem(pixmap);
		m_diagram->addItem(picture);
		picture->setPos(pos);
		return picture;
	}

	static QPointF pivotInScene(DiagramImageItem *picture)
	{
		return picture->mapToScene(picture->pivot());
	}

	/// The angle and position a file saves, read back by a new picture.
	static DiagramImageItem *reloaded(DiagramImageItem *picture)
	{
		QDomDocument document;
		auto *copy = new DiagramImageItem();
		copy->fromXml(picture->toXml(document));
		return copy;
	}

	static void trigger(const char *name)
	{
		QAction *a = QetAppTest::action(QString::fromLatin1(name));
		QVERIFY2(a, name);
		a->trigger();
	}

private slots:
	void initTestCase()
	{
		QVERIFY(QetAppTest::editor());
		m_diagram = QetAppTest::diagram();
		QVERIFY(m_diagram);
	}

	void cleanup()
	{
		m_diagram->clearSelection();
		for (QGraphicsItem *item : m_diagram->items())
			if (item->type() == DiagramImageItem::Type)
				delete item;
		m_diagram->undoStack().clear();
	}

	void spaceTurnsThePictureAndTheFileKeepsIt()
	{
		DiagramImageItem *picture = addPicture(QPointF(200, 200));
		const QPointF pivot = pivotInScene(picture);
		picture->setSelected(true);

		trigger("rotate_selection");
		trigger("rotate_selection");

		QCOMPARE(picture->rotationAngle(), 180.0);
		QCOMPARE(picture->rotation(), 0.0);   // not QGraphicsItem's own rotation
		QCOMPARE(pivotInScene(picture), pivot);   // turned in place

		std::unique_ptr<DiagramImageItem> copy(reloaded(picture));
		QCOMPARE(copy->rotationAngle(), 180.0);
		QCOMPARE(copy->pos(), picture->pos());
	}

	void undoAndRedo()
	{
		DiagramImageItem *picture = addPicture(QPointF(200, 200));
		const QPointF pos = picture->pos();
		picture->setSelected(true);
		trigger("rotate_selection");

		// RotateSelectionCommand animates undo and redo after the first
		// time: the values arrive a moment later.
		for (int round = 0; round < 2; ++round)
		{
			m_diagram->undoStack().undo();
			QTRY_COMPARE(picture->rotationAngle(), 0.0);
			QCOMPARE(picture->pos(), pos);
			m_diagram->undoStack().redo();
			QTRY_COMPARE(picture->rotationAngle(), 90.0);
			QCOMPARE(picture->pos(), pos);
		}
	}

	// Turned as a group, each picture turns on itself and its pivot moves
	// a quarter turn around the group's centre (the centre of the
	// selection's bounding box, snapped to the grid).
	void groupTurnMovesEachPivotAroundTheCentre()
	{
		DiagramImageItem *a = addPicture(QPointF(100, 100));
		DiagramImageItem *b = addPicture(QPointF(400, 160));
		a->setSelected(true);
		b->setSelected(true);
		const QPointF centre = Diagram::snapToGrid(
				(a->sceneBoundingRect() | b->sceneBoundingRect()).center());
		const QTransform quarter = QTransform().translate(centre.x(), centre.y())
				.rotate(90).translate(-centre.x(), -centre.y());
		const QPointF expectedA = quarter.map(pivotInScene(a));
		const QPointF expectedB = quarter.map(pivotInScene(b));

		trigger("rotate_group_selection");

		QCOMPARE(a->rotationAngle(), 90.0);
		QCOMPARE(b->rotationAngle(), 90.0);
		QCOMPARE(pivotInScene(a), expectedA);
		QCOMPARE(pivotInScene(b), expectedB);

		std::unique_ptr<DiagramImageItem> copy(reloaded(a));
		QCOMPARE(copy->rotationAngle(), 90.0);
		QCOMPARE(pivotInScene(copy.get()) - copy->pos(), pivotInScene(a) - a->pos());
	}
};

QET_APP_TEST_MAIN(tst_rotateselectionimage)
#include "tst_rotateselectionimage.moc"
