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
#include "palettegraphicsview.h"

#include "qetpalette.h"
#include "darkimagerendering.h"

#include <QApplication>
#include <QEvent>
#include <QGraphicsItem>
#include <QPaintEvent>
#include <QSet>
#include <QStyleHintReturnMask>
#include <QStyleOptionRubberBand>
#include <QtMath>

#include <algorithm>
#include <iterator>
#include <optional>
#include <vector>

namespace {
	/**
		QGraphicsScene::drawItems() is protected, and QGraphicsView::drawItems()
		hands the scene the viewport only when the painter is on it. The
		view paints into an image, and still needs the scene to get the
		viewport: that is what makes the scene record where it painted
		each item, which is where the item is erased from when it moves.
		Naming the member through a derived class is the standard way to
		a pointer to a protected member; a call through it dispatches to
		the scene's own override, if any.
	*/
	struct SceneAccess : QGraphicsScene
	{
		using DrawItems = void (QGraphicsScene::*)(QPainter *, int, QGraphicsItem *[],
		                                           const QStyleOptionGraphicsItem[], QWidget *);
		static DrawItems drawItemsPointer() { return &SceneAccess::drawItems; }
	};
}

PaletteGraphicsView::PaletteGraphicsView(QWidget *parent) :
	QGraphicsView(parent)
{
	qApp->installEventFilter(this);
}

PaletteGraphicsView::PaletteGraphicsView(QGraphicsScene *scene, QWidget *parent) :
	QGraphicsView(scene, parent)
{
	qApp->installEventFilter(this);
}

/**
	@brief PaletteGraphicsView::invertsLightness
	@return true when the scene is shown with its lightness inverted,
	i.e. when the application palette is dark. The application palette,
	not the view's own: a style sheet on an ancestor (the folio tab
	widget has one) makes QStyleSheetStyle pin the palette of every
	widget under it to the application palette in force when the sheet
	was applied, so after a live light/dark switch palette() is stale.
*/
bool PaletteGraphicsView::invertsLightness() const
{
	return QET::Palette::isDark(QApplication::palette());
}

void PaletteGraphicsView::paintingInverted(bool inverted)
{
	Q_UNUSED(inverted)
}

QET::Palette::SheetInk PaletteGraphicsView::sheetInk() const
{
	return {};
}

PaletteGraphicsView::InkKind PaletteGraphicsView::inkKind(const QGraphicsItem *item) const
{
	Q_UNUSED(item)
	return InkKind::Other;
}

QET::Palette::SheetInk PaletteGraphicsView::paintingInk(const QWidget *widget)
{
	const auto *view = widget ? qobject_cast<const PaletteGraphicsView *>(widget->parentWidget()) : nullptr;
	return view && view->viewport() == widget ? view->m_ink : QET::Palette::SheetInk();
}

/**
	@brief PaletteGraphicsView::eventFilter
	Repaint the whole viewport when the application palette changes. Qt
	sends that change to the application object and then repaints only
	the widgets whose own palette changed with it, which under a style
	sheet is not the case (see invertsLightness()): the scene would then
	repaint only what it updates itself, and the viewport around the
	sheet would keep the colors of the previous palette. The filter sits
	on the application object, the one receiver Qt always notifies.
*/
bool PaletteGraphicsView::eventFilter(QObject *watched, QEvent *event)
{
	if (watched == qApp && event->type() == QEvent::ApplicationPaletteChange)
		viewport()->update();
	return QGraphicsView::eventFilter(watched, event);
}

/**
	@brief PaletteGraphicsView::paintEvent
	Paints as QGraphicsView on a light palette, inverted on a dark one,
	and with the items tinted when sheetInk() is not null.
*/
void PaletteGraphicsView::paintEvent(QPaintEvent *event)
{
	if (invertsLightness() || !sheetInk().isNull())
	{
		paintInverted(event);
		return;
	}
	m_buffer = QImage();
	m_composite = QImage();
	QGraphicsView::paintEvent(event);
}

/**
	@brief PaletteGraphicsView::paintInverted
	Run QGraphicsView::paintEvent() with the drawing hooks redirected to
	an off-screen image of the viewport, then invert the lightness of the
	exposed part of that image between the palette's Base and Text colors
	and blit it to the viewport. Inverting the finished rendering turns
	the white sheet dark and the black ink light in one pass, and keeps
	the hue of colored strokes. The image is in viewport coordinates, so
	the hooks paint with the view's own transform and the scene records
	the items' places in the viewport, as it does on a light palette.
	@param event the paint event, for the exposed area
*/
void PaletteGraphicsView::paintInverted(QPaintEvent *event)
{
	const QRect exposed = event->rect().intersected(viewport()->rect());
	if (exposed.isEmpty())
		return;

	const qreal ratio = viewport()->devicePixelRatioF();
	const QSize size(qCeil(viewport()->width() * ratio), qCeil(viewport()->height() * ratio));
	if (m_buffer.size() != size || m_buffer.devicePixelRatio() != ratio)
	{
		m_buffer = QImage(size, QImage::Format_ARGB32_Premultiplied);
		m_buffer.setDevicePixelRatio(ratio);
	}

	m_buffer_painter.begin(&m_buffer);
	// The hooks paint only what the scene draws; what they leave blank is
	// the white sheet, which the inversion turns into the Base color.
	m_buffer_painter.fillRect(exposed, Qt::white);
	m_buffer_painter.setClipRect(exposed);
	m_buffer_painter.setRenderHints(renderHints());
	m_buffer_painter.setWorldTransform(viewportTransform());
	m_exposed = exposed;
	m_layered = false;
	m_ink = sheetInk();
	DarkImageRendering::Context context{&m_buffer_painter, !customBackgroundColor(), [this](bool invert, const QRect &bounds) { flushLayer(invert, bounds); }};
		//Only on a dark palette: a picture "adapted to the dark theme" is
		//inverted against the scene there. Tinting for a dark custom
		//sheet on a light palette draws pictures as they are, as before
		//there was a tint
	std::optional<DarkImageRendering::Scope> rasterScope;
	if (invertsLightness())
		rasterScope.emplace(&context);

	m_inverting = true;
	paintingInverted(!customBackgroundColor());
	const OptimizationFlags flags = optimizationFlags();
	setOptimizationFlag(QGraphicsView::IndirectPainting, true);
	QGraphicsView::paintEvent(event);
	setOptimizationFlags(flags);
	paintingInverted(false);
	m_inverting = false;
	m_ink = QET::Palette::SheetInk();

	if (m_layered) flushLayer(!customBackgroundColor());
	m_buffer_painter.end();
	blitInverted(exposed);
}

void PaletteGraphicsView::flushLayer(bool invert, const QRect &bounds)
{
	const QRect area = bounds.isNull() ? m_exposed : bounds.intersected(m_exposed);
	if (area.isEmpty()) return;
	const qreal ratio = m_buffer.devicePixelRatio();
	const QRect pixels = QRectF(area.topLeft() * ratio, area.size() * ratio).toAlignedRect();
	QImage layer = m_buffer.copy(pixels);
	layer.setDevicePixelRatio(ratio);
	if (invert) {
		const QPalette palette = QApplication::palette();
		QET::Palette::invertLightnessLayer(layer, palette.color(QPalette::Base), palette.color(QPalette::Text));
	}
	if (!m_layered) {
		if (m_composite.size() != m_buffer.size() || m_composite.devicePixelRatio() != ratio) {
			m_composite = QImage(m_buffer.size(), QImage::Format_ARGB32_Premultiplied);
			m_composite.setDevicePixelRatio(ratio);
		}
		m_composite.fill(Qt::transparent);
		m_layered = true;
	}
	QPainter composite(&m_composite);
	composite.drawImage(QPointF(pixels.topLeft()) / ratio, layer);
	paintOnBuffer([&](QPainter &painter) {
		painter.setCompositionMode(QPainter::CompositionMode_Source);
		painter.fillRect(QRectF(QPointF(pixels.topLeft()) / ratio, QSizeF(pixels.size()) / ratio), Qt::transparent);
	});
}

/**
	@brief PaletteGraphicsView::paintOnBuffer
	Run \a paint on the off-screen image's painter in viewport
	coordinates, unclipped and opaque, and give the painter back as it
	was. Qt's active painter and its saved state stack are kept intact:
	replacing or restarting it here would lose ancestor clipping and
	opacity.
*/
void PaletteGraphicsView::paintOnBuffer(const std::function<void(QPainter &)> &paint)
{
	m_buffer_painter.save();
	m_buffer_painter.resetTransform();
	m_buffer_painter.setClipping(false);
	m_buffer_painter.setOpacity(1.0);
	paint(m_buffer_painter);
	m_buffer_painter.restore();
}

/**
	@brief PaletteGraphicsView::drawBackground
	Into the off-screen image while painting inverted, else as
	QGraphicsView.
*/
void PaletteGraphicsView::drawBackground(QPainter *painter, const QRectF &rect)
{
	QGraphicsView::drawBackground(m_inverting ? &m_buffer_painter : painter, rect);
}

/**
	@brief PaletteGraphicsView::drawItems
	Into the off-screen image while painting inverted, with the viewport
	as the scene's widget (see SceneAccess), tinted when there is an ink
	(see drawTinted()), else as QGraphicsView.
*/
void PaletteGraphicsView::drawItems(QPainter *painter, int count, QGraphicsItem *items[],
                                    const QStyleOptionGraphicsItem options[])
{
	if (m_inverting && scene() && !m_ink.isNull())
		drawTinted(count, items, options);
	else if (m_inverting && scene())
		(scene()->*SceneAccess::drawItemsPointer())(&m_buffer_painter, count, items, options, viewport());
	else
		QGraphicsView::drawItems(painter, count, items, options);
}

/// Items of one kind drawn together, and the part of the viewport they cover
struct PaletteGraphicsView::TintBatch
{
	InkKind kind = InkKind::Other;
	std::vector<QGraphicsItem *> items;
	std::vector<QStyleOptionGraphicsItem> options;
	QRect area;
};

/**
	@brief PaletteGraphicsView::drawTinted
	Draw the items kind by kind (see inkKind()): those left untouched
	straight into the off-screen image, the others each kind on a clear
	layer that is then tinted in its ink and laid onto the image.

	The items come bottom first, and keep that order where they overlap:
	an item joins the batch of its kind unless it overlaps a batch that
	is laid on later; then the batches so far are drawn first. Items
	that do not overlap need no order, so a folio is drawn in a few
	batches however its kinds alternate in the stack. Each batch is
	tinted only over the part of the viewport its items cover, and an
	item's kind is asked once, for its top-level item, which the scene
	draws with all its children.
*/
void PaletteGraphicsView::drawTinted(int count, QGraphicsItem *items[],
                                     const QStyleOptionGraphicsItem options[])
{
	static constexpr InkKind order[] = {InkKind::Untouched, InkKind::Other, InkKind::DashDottedWires,
	                                    InkKind::DashedWires, InkKind::Wires, InkKind::Symbols};
	constexpr int kinds = int(std::size(order));
	TintBatch batches[kinds];
	for (int k = 0; k < kinds; ++k) batches[k].kind = order[k];

	const auto drawAll = [&]() {
		for (TintBatch &batch : batches) {
			if (batch.items.empty()) continue;
			drawBatch(batch);
			batch.items.clear();
			batch.options.clear();
			batch.area = QRect();
		}
	};

	const QTransform to_viewport = viewportTransform();
	QSet<const QGraphicsItem *> seen;
	for (int i = 0; i < count; ++i)
	{
		QGraphicsItem *top = items[i]->topLevelItem();
		if (seen.contains(top)) continue;
		seen.insert(top);

		const int k = int(std::find(std::begin(order), std::end(order), inkKind(top)) - std::begin(order));
			//Two pixels around for antialiasing and wide pens
		const QRect area = to_viewport.mapRect(top->sceneBoundingRect()
		                                       | top->mapRectToScene(top->childrenBoundingRect()))
		                   .toAlignedRect().adjusted(-2, -2, 2, 2).intersected(m_exposed);
		for (int later = k + 1; later < kinds; ++later)
			if (batches[later].area.intersects(area)) {
				drawAll();
				break;
			}
		batches[k].items.push_back(top);
		batches[k].options.push_back(options[i]);
		batches[k].area |= area;
	}
	drawAll();
}

/**
	@brief PaletteGraphicsView::drawBatch
	Draw \a batch: untouched items straight into the off-screen image,
	the others on the layer, tinted in their kind's ink over the area
	they cover and laid onto the image.
*/
void PaletteGraphicsView::drawBatch(TintBatch &batch)
{
	const auto draw = SceneAccess::drawItemsPointer();
	if (batch.kind == InkKind::Untouched) {
		(scene()->*draw)(&m_buffer_painter, int(batch.items.size()), batch.items.data(),
		                 batch.options.data(), viewport());
		return;
	}
	if (batch.area.isEmpty())
		return;

	const qreal ratio = m_buffer.devicePixelRatio();
	const QRect pixels = QRectF(batch.area.topLeft() * ratio, batch.area.size() * ratio).toAlignedRect();
	if (m_tint_layer.size() != m_buffer.size() || m_tint_layer.devicePixelRatio() != ratio) {
		m_tint_layer = QImage(m_buffer.size(), QImage::Format_ARGB32_Premultiplied);
		m_tint_layer.setDevicePixelRatio(ratio);
	}
	QPainter layer(&m_tint_layer);
	layer.setCompositionMode(QPainter::CompositionMode_Source);
	layer.fillRect(batch.area, Qt::transparent);
	layer.setCompositionMode(QPainter::CompositionMode_SourceOver);
	layer.setClipRect(batch.area);
	layer.setRenderHints(renderHints());
	layer.setWorldTransform(viewportTransform());
	(scene()->*draw)(&layer, int(batch.items.size()), batch.items.data(),
	                 batch.options.data(), viewport());
	layer.end();

	QImage part = m_tint_layer.copy(pixels);
	QColor ink = m_ink.other;
	switch (batch.kind) {
		case InkKind::Symbols:         ink = m_ink.symbols; break;
		case InkKind::Wires:           ink = m_ink.wires; break;
		case InkKind::DashedWires:     ink = m_ink.dashed_wires; break;
		case InkKind::DashDottedWires: ink = m_ink.dash_dotted_wires; break;
		default: break;
	}
	QET::Palette::tintLayer(part, m_ink.sheet, ink);
	part.setDevicePixelRatio(ratio);
	paintOnBuffer([&](QPainter &painter) {
		painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
		painter.drawImage(QPointF(pixels.topLeft()) / ratio, part);
	});
}

/**
	@brief PaletteGraphicsView::drawForeground
	Into the off-screen image while painting inverted, else as
	QGraphicsView.
*/
void PaletteGraphicsView::drawForeground(QPainter *painter, const QRectF &rect)
{
	QGraphicsView::drawForeground(m_inverting ? &m_buffer_painter : painter, rect);
}

/**
	@brief PaletteGraphicsView::blitInverted
	Invert the lightness of \a area of the off-screen image and draw it on
	the viewport, then the selection rubber band on top: the one
	QGraphicsView::paintEvent() drew went under the blit.
	@param area the part of the viewport to blit, in viewport coordinates
*/
void PaletteGraphicsView::blitInverted(const QRect &area)
{
	const qreal ratio = m_buffer.devicePixelRatio();
	const QRect pixels = QRectF(area.topLeft() * ratio, area.size() * ratio).toAlignedRect();
	QImage part = (m_layered ? m_composite : m_buffer).copy(pixels);
	part.setDevicePixelRatio(ratio);
	// The application palette, for the reason given in invertsLightness().
	const QPalette application_palette = QApplication::palette();
	if (!m_layered && !customBackgroundColor()) QET::Palette::invertLightness(part, application_palette.color(QPalette::Base),
	                              application_palette.color(QPalette::Text));

	QPainter painter(viewport());
	painter.drawImage(QPointF(pixels.topLeft()) / ratio, part);
	drawRubberBand(painter);
}

/**
	@brief PaletteGraphicsView::drawRubberBand
	Draw the selection rubber band the way QGraphicsView::paintEvent does,
	after the inversion, in the palette colors.
	@param painter a painter on the viewport
*/
void PaletteGraphicsView::drawRubberBand(QPainter &painter)
{
	const QRect band = rubberBandRect();
	if (band.isNull())
		return;

	QStyleOptionRubberBand option;
	option.initFrom(viewport());
	option.rect = band;
	option.shape = QRubberBand::Rectangle;

	QStyleHintReturnMask mask;
	if (viewport()->style()->styleHint(QStyle::SH_RubberBand_Mask, &option,
	                                   viewport(), &mask))
		painter.setClipRegion(mask.region, Qt::IntersectClip);
	viewport()->style()->drawControl(QStyle::CE_RubberBand, &option,
	                                 &painter, viewport());
}
