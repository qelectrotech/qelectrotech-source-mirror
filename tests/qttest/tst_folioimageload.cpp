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
#include "diagramcontent.h"
#include "qetgraphicsitem/diagramimageitem.h"

#include <QDomDocument>
#include <QRandomGenerator>

/**
	Diagram::fromXml() decodes a folio's pictures in parallel, then builds
	the items in file order. Every picture must come back with its own
	pixels and position, whatever thread decoded it.
*/
class tst_folioimageload : public QObject
{
	Q_OBJECT

	Diagram *m_diagram = nullptr;

	/// A picture no other one equals: its own size and noise.
	static QImage distinctImage(int n)
	{
		QImage image(40 + 7 * n, 30 + 3 * n, QImage::Format_ARGB32);
		QRandomGenerator random(quint32(n + 1));
		for (int y = 0 ; y < image.height() ; ++y)
			for (int x = 0 ; x < image.width() ; ++x)
				image.setPixel(x, y, random.generate() | 0xFF000000);
		return image;
	}

	static QImage pixelsOf(const DiagramImageItem *item)
	{
		return item->pixmap().toImage()
				.convertToFormat(QImage::Format_ARGB32);
	}

	void removePictures()
	{
			// A selected picture's handles are its children, in items()
			// too: deleting the picture first would leave them dangling.
		m_diagram->clearSelection();
		for (QGraphicsItem *item : m_diagram->items())
			if (item->type() == DiagramImageItem::Type)
				delete item;
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
		removePictures();
		m_diagram->undoStack().clear();
	}

	void everyPictureComesBackWithItsPixels()
	{
		QHash<int, QImage> expected;	// by x position, each its own
		for (int n = 0 ; n < 6 ; ++n) {
			const QImage image = distinctImage(n);
			auto *picture = new DiagramImageItem(QPixmap::fromImage(image));
			m_diagram->addItem(picture);
			picture->setPos(100 + 150 * n, 200);
			picture->setSelected(true);
			expected.insert(qRound(picture->pos().x()), image);
		}
		QDomDocument document = m_diagram->toXml(false);
		removePictures();

		DiagramContent content;
		QVERIFY(m_diagram->fromXml(document, QPointF(), false, &content));

		QCOMPARE(content.m_images.size(), expected.size());
		for (const DiagramImageItem *picture : std::as_const(content.m_images)) {
			const int x = qRound(picture->pos().x());
			QVERIFY2(expected.contains(x), "picture moved");
			QCOMPARE(picture->pos().y(), 200.0);
			QCOMPARE(pixelsOf(picture), expected.value(x));
		}
	}

	void anImageWithoutPictureDoesNotStopTheOthers()
	{
		for (int n = 0 ; n < 3 ; ++n) {
			auto *picture = new DiagramImageItem(
						QPixmap::fromImage(distinctImage(n)));
			m_diagram->addItem(picture);
			picture->setSelected(true);
		}
		QDomDocument document = m_diagram->toXml(false);
		removePictures();
		QDomElement images = document.documentElement()
				.firstChildElement(QStringLiteral("images"));
		QVERIFY(!images.isNull());
		QDomElement empty = document.createElement(QStringLiteral("image"));
		images.insertBefore(empty, images.firstChild());

		DiagramContent content;
		QVERIFY(m_diagram->fromXml(document, QPointF(), false, &content));

		int with_pixels = 0;
		for (const DiagramImageItem *picture : std::as_const(content.m_images))
			if (!picture->pixmap().isNull())
				++with_pixels;
		QCOMPARE(with_pixels, 3);
	}
};

QET_APP_TEST_MAIN(tst_folioimageload)
#include "tst_folioimageload.moc"
