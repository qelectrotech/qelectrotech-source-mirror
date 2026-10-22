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

#include "qetpalette.h"

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

	On a custom sheet color the scene is not inverted, but a dark sheet
	needs light lines: sheetInk() then gives the colors, and the items
	are drawn on the same off-screen path, each kind on a layer of its
	own that is tinted in its color (QET::Palette::tintLayer()) before it
	goes onto the sheet. The background and the foreground are drawn as
	they are.
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
		/// Yellow symbols and red conductors on a custom sheet, instead
		/// of one light ink for all (see QET::Palette::sheetInk()).
		static void setColoredInk(bool colored) { s_colored_ink = colored; }
		static bool coloredInk() { return s_colored_ink; }

		/// The kinds of item that are tinted each in their own ink.
		enum class InkKind { Untouched, Symbols, Wires, DashedWires, DashDottedWires, Other };
		/// @return the ink the items are shown in, null when they are
		/// shown as drawn. Null by default.
		virtual QET::Palette::SheetInk sheetInk() const;
		/// The ink of the view painting right now, null outside a
		/// tinted paint: an item drawn in several inks (a cable, its
		/// line in one, its label in another) draws the part that does
		/// not take its kind's ink in that ink's own color, which the
		/// tint leaves as it is.
		static const QET::Palette::SheetInk &paintingInk() { return s_painting_ink; }

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
		/// @return which ink \a item is shown in when sheetInk() is not
		/// null: Other by default. The scene draws an item with its
		/// whole top-level item, so all items under one top-level item
		/// must be given the same kind.
		virtual InkKind inkKind(const QGraphicsItem *item) const;
		/// @return the painter the scene really paints with: the
		/// off-screen image's while painting inverted, else \a painter.
		/// For subclasses that draw more in drawBackground().
		QPainter *scenePainter(QPainter *painter)
		{ return m_inverting ? &m_buffer_painter : painter; }

	private:
		void paintInverted(QPaintEvent *event);
		void blitInverted(const QRect &area);
		void drawRubberBand(QPainter &painter);
		void flushLayer(bool invert, const QRect &bounds = QRect());
		void drawTinted(int count, QGraphicsItem *items[],
		                const QStyleOptionGraphicsItem options[]);

		/// The off-screen image the hooks paint into while m_inverting:
		/// the viewport's size, in its coordinates. Kept between paints,
		/// dropped when the view paints on a light palette again.
		QImage m_buffer;
		QImage m_composite;
		QRect m_exposed;
		bool m_layered = false;
		QPainter m_buffer_painter;
		/// True while paintEvent() paints for an inverted display.
		bool m_inverting = false;
		/// The ink of the paint in progress, null when not tinting.
		QET::Palette::SheetInk m_ink;
		/// The layer one kind of item is drawn on before it is tinted.
		QImage m_tint_layer;
		static inline bool s_custom_bg = false;
		static inline bool s_colored_ink = false;
		static inline QET::Palette::SheetInk s_painting_ink;
};

#endif
