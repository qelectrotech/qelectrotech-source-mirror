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
#include <QImage>
#include <QPainter>

/**
	A QGraphicsView that shows its scene with inverted lightness while the
	application palette is dark: white becomes the palette's Base color,
	black its Text color, and colored strokes keep their hue. The scene
	itself is left as drawn, so printing and exporting it still give black
	on white. On a light palette the view paints exactly as QGraphicsView
	does.

	On a dark palette the view still runs QGraphicsView::paintEvent(), with
	the IndirectPainting flag set for the duration of that call, so that
	the background, the items and the foreground come through the
	drawBackground(), drawItems() and drawForeground() hooks. (That flag
	selects Qt's older item-painting algorithm, which first builds a list
	of the exposed items and their style options; it is set only while
	the view paints inverted.) The hooks
	paint into an off-screen image the size of the viewport, in viewport
	coordinates; paintEvent() then inverts the lightness of the exposed
	part of that image and blits it. Going through the real paint event,
	and handing the scene the viewport when the items are drawn, keeps the
	view on QGraphicsView's default update path, which erases a moved item
	where it was last painted, children included, even a child whose
	geometry is set while its parent is painted (a terminal's help lines).
	The alternative, rendering with QGraphicsView::render(), needed a
	receiver on QGraphicsScene::changed() to keep the scene's updates
	flowing, and that receiver puts the scene on its Qt 4.4 compatibility
	path, which erases only the moved item's own old rectangle: a moved
	element then left its terminals' help lines, which span the whole
	sheet, behind at every step (#954).

	The CacheBackground cache mode is not supported on the inverted path.
*/
class PaletteGraphicsView : public QGraphicsView
{
	Q_OBJECT

	public:
		explicit PaletteGraphicsView(QWidget *parent = nullptr);
		explicit PaletteGraphicsView(QGraphicsScene *scene, QWidget *parent = nullptr);

		bool invertsLightness() const;

		static void setCustomBackgroundColor(bool custom) { s_custom_bg = custom; }
		static bool customBackgroundColor() { return s_custom_bg; }

	protected:
		bool eventFilter(QObject *watched, QEvent *event) override;
		void paintEvent(QPaintEvent *event) override;
		void drawBackground(QPainter *painter, const QRectF &rect) override;
		void drawItems(QPainter *painter, int count, QGraphicsItem *items[],
		               const QStyleOptionGraphicsItem options[]) override;
		void drawForeground(QPainter *painter, const QRectF &rect) override;
		/**
			Called with true right before the scene is painted for an
			inverted display and with false right after, so a scene can
			adapt what it draws (a softer grid, for instance). Does
			nothing by default.
		*/
		virtual void paintingInverted(bool inverted);

	private:
		void paintInverted(QPaintEvent *event);
		void blitInverted(const QRect &area);
		void drawRubberBand(QPainter &painter);

		/// The off-screen image the hooks paint into while m_inverting:
		/// the viewport's size, in its coordinates. Kept between paints,
		/// dropped when the view paints on a light palette again.
		QImage m_buffer;
		QPainter m_buffer_painter;
		/// True while paintEvent() paints for an inverted display.
		bool m_inverting = false;
		static inline bool s_custom_bg = false;
};

#endif
