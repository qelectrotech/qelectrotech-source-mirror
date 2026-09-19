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

#include <QApplication>
#include <QEvent>
#include <QPainter>
#include <QPaintEvent>
#include <QStyleHintReturnMask>
#include <QStyleOptionRubberBand>
#include <QtMath>

PaletteGraphicsView::PaletteGraphicsView(QWidget *parent) :
	QGraphicsView(parent)
{
	qApp->installEventFilter(this);
}

PaletteGraphicsView::PaletteGraphicsView(QGraphicsScene *scene, QWidget *parent) :
	QGraphicsView(scene, parent)
{
	qApp->installEventFilter(this);
	listenToScene(scene);
}

/**
	@brief PaletteGraphicsView::setScene
	Same as QGraphicsView::setScene, and keeps the scene's updates flowing
	(see the class description).
*/
void PaletteGraphicsView::setScene(QGraphicsScene *scene)
{
	QGraphicsView::setScene(scene);
	listenToScene(scene);
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

/**
	@brief PaletteGraphicsView::listenToScene
	Connect a receiver to the scene's changed() signal, dropping the one
	on the previous scene. Any receiver does; this one has nothing to do.
	The connection dies with the view.
*/
void PaletteGraphicsView::listenToScene(QGraphicsScene *scene)
{
	disconnect(m_scene_connection);
	m_scene_connection = QMetaObject::Connection();
	if (scene)
		m_scene_connection = connect(scene, &QGraphicsScene::changed,
		                             this, [](const QList<QRectF> &) {});
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
	Paints as QGraphicsView on a light palette, inverted on a dark one.
*/
void PaletteGraphicsView::paintEvent(QPaintEvent *event)
{
	if (invertsLightness())
		paintInverted(event->rect());
	else
		QGraphicsView::paintEvent(event);
}

/**
	@brief PaletteGraphicsView::paintInverted
	Render \a area of the viewport into an off-screen image, invert the
	lightness of that image between the palette's Base and Text colors and
	blit it to the viewport. Inverting the finished rendering turns the
	white sheet dark and the black ink light in one pass, and keeps the
	hue of colored strokes.
	@param area the part of the viewport to repaint, in viewport coordinates
*/
void PaletteGraphicsView::paintInverted(const QRect &area)
{
	const QRect rect = area.intersected(viewport()->rect());
	if (rect.isEmpty())
		return;

	const qreal ratio = viewport()->devicePixelRatioF();
	QImage buffer(qCeil(rect.width() * ratio), qCeil(rect.height() * ratio),
	              QImage::Format_RGB32);
	buffer.setDevicePixelRatio(ratio);
	// render() paints only what the scene draws; what it leaves blank is
	// the white sheet, which the inversion turns into the Base color.
	buffer.fill(Qt::white);

	QPainter buffer_painter(&buffer);
	buffer_painter.setRenderHints(renderHints());
	paintingInverted(true);
	render(&buffer_painter, QRectF(QPointF(0, 0), QSizeF(rect.size())),
	       rect, Qt::IgnoreAspectRatio);
	paintingInverted(false);
	buffer_painter.end();

	// The application palette, for the reason given in invertsLightness().
	const QPalette application_palette = QApplication::palette();
	QET::Palette::invertLightness(buffer, application_palette.color(QPalette::Base),
	                              application_palette.color(QPalette::Text));

	QPainter painter(viewport());
	painter.drawImage(rect.topLeft(), buffer);
	drawRubberBand(painter);
}

/**
	@brief PaletteGraphicsView::drawRubberBand
	Draw the selection rubber band the way QGraphicsView::paintEvent does.
	Rendering the view into an off-screen image skips it, so it is drawn
	here instead, after the inversion, in the palette colors.
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
