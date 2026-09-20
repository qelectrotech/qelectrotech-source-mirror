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
#ifndef PALETTE_GRAPHICS_VIEW_H
#define PALETTE_GRAPHICS_VIEW_H

#include <QGraphicsView>

class QPainter;

/**
	A QGraphicsView that shows its scene with inverted lightness while its
	palette is dark: white becomes the palette's Base color, black its Text
	color, and colored strokes keep their hue. The scene itself is left as
	drawn, so printing and exporting it still give black on white. On a
	light palette the view paints exactly as QGraphicsView does.

	The view paints through QGraphicsView::render() into an image and blits
	the inverted image. QGraphicsView delivers scene updates straight to
	its viewport when nobody listens to QGraphicsScene::changed(), and in
	that mode the scene clears its "update everything" flag only when the
	items are painted straight onto the viewport, which never happens
	here: from the second QGraphicsScene::update() on, every scene update
	and every item update would wait for an unrelated repaint. So the view
	listens to changed() on every scene it is given, which makes the scene
	clear the flag before it emits. Set the scene through this class, not
	through a QGraphicsView pointer.

	The constructor also forces QGraphicsView::FullViewportUpdate in place
	of the default MinimalViewportUpdate. #954 shipped with the default
	kept, and moving an item then left conductor-shaped ghosts behind on
	both a light and a dark palette, so the cause is shared code, not
	paintInverted(): most likely listening to changed() at all, above,
	changes which of QGraphicsScene's two update paths a view is on, and
	MinimalViewportUpdate's job of turning the scene's reported dirty
	rects into the smallest correct viewport region is where that would
	show up first. FullViewportUpdate removes the need to get that region
	right by repainting the whole viewport on every update; the class's
	own benchmark already shows that cost is small next to a frame budget.
*/
class PaletteGraphicsView : public QGraphicsView
{
	Q_OBJECT

	public:
		explicit PaletteGraphicsView(QWidget *parent = nullptr);
		explicit PaletteGraphicsView(QGraphicsScene *scene, QWidget *parent = nullptr);

		void setScene(QGraphicsScene *scene);
		bool invertsLightness() const;

	protected:
		bool eventFilter(QObject *watched, QEvent *event) override;
		void paintEvent(QPaintEvent *event) override;
		/**
			Called with true right before the scene is rendered for an
			inverted display and with false right after, so a scene can
			adapt what it draws (a softer grid, for instance). Does
			nothing by default.
		*/
		virtual void paintingInverted(bool inverted);

	private:
		void listenToScene(QGraphicsScene *scene);
		void paintInverted(const QRect &area);
		void drawRubberBand(QPainter &painter);

		QMetaObject::Connection m_scene_connection;
};

#endif
