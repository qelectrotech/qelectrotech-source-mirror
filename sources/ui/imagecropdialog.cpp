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
#include "imagecropdialog.h"

#include <QDialogButtonBox>
#include <QLabel>
#include <QMouseEvent>
#include <QPainter>
#include <QPushButton>
#include <QVBoxLayout>

namespace {
	// Fits the dialog comfortably on a normal screen regardless of the
	// source image's own resolution -- same reasoning, and same value,
	// as ImageTransparentColorDialog's identical constant.
	constexpr int MAX_DISPLAY_SIZE = 350;
}

/**
	@brief CropAreaWidget::CropAreaWidget
	@param sourceImage the full-resolution image to crop
	@param initialCropRect a previously-chosen crop rect (in
	sourceImage's own coordinates), or an empty QRect to start covering
	the whole image
	@param parent
*/
CropAreaWidget::CropAreaWidget(const QImage &sourceImage, const QRect &initialCropRect, QWidget *parent) :
	QWidget(parent),
	m_source(sourceImage)
{
	const qreal scaleW = qreal(MAX_DISPLAY_SIZE) / m_source.width();
	const qreal scaleH = qreal(MAX_DISPLAY_SIZE) / m_source.height();
	m_displayScale = qMin(qreal(1.0), qMin(scaleW, scaleH));   // never upscale a small image, only ever shrink a large one

	const QSize displaySize = m_source.size() * m_displayScale;
	m_displayImageRect = QRect(QPoint(0, 0), displaySize);
	setFixedSize(displaySize);

	if (initialCropRect.isEmpty())
	{
		// No previous crop to start from -- covers the whole image, so
		// the user shrinks it from there rather than having to first
		// drag out a rectangle from nothing.
		m_cropRect = m_displayImageRect;
	}
	else
	{
		// Converting an existing, original-space crop rect back into
		// display-space -- the reverse of what cropRect() does going
		// the other way.
		m_cropRect = QRect(
				qRound(initialCropRect.left() * m_displayScale),
				qRound(initialCropRect.top() * m_displayScale),
				qRound(initialCropRect.width() * m_displayScale),
				qRound(initialCropRect.height() * m_displayScale))
				.intersected(m_displayImageRect);
	}
}

/**
	@brief CropAreaWidget::resetToFullImage
*/
void CropAreaWidget::resetToFullImage()
{
	m_cropRect = m_displayImageRect;
	update();
	emit cropRectChanged();
}

/**
	@brief CropAreaWidget::cropRect
	@return the crop rectangle in the original, full-resolution image's
	own coordinates -- m_cropRect itself is display-space only.
*/
QRect CropAreaWidget::cropRect() const
{
	QRect r(
			qRound(m_cropRect.left() / m_displayScale),
			qRound(m_cropRect.top() / m_displayScale),
			qRound(m_cropRect.width() / m_displayScale),
			qRound(m_cropRect.height() / m_displayScale));
	return r.intersected(m_source.rect());
}

/**
	@brief CropAreaWidget::handleAt
	@param pos widget-space position to test
	@return whichever handle (or the rect's own interior, for moving)
	pos is close enough to, or DragMode::None if it's not near anything
*/
CropAreaWidget::DragMode CropAreaWidget::handleAt(const QPoint &pos) const
{
	const int tol = HANDLE_SIZE;   // a generous hit area, larger than the drawn handle itself, since a hit-test exactly matching a small handle's visible size is needlessly hard to land
	auto near = [&](const QPoint &p) { return (pos - p).manhattanLength() <= tol; };

	if (near(m_cropRect.topLeft()))     return DragMode::ResizeNW;
	if (near(m_cropRect.topRight()))    return DragMode::ResizeNE;
	if (near(m_cropRect.bottomLeft()))  return DragMode::ResizeSW;
	if (near(m_cropRect.bottomRight())) return DragMode::ResizeSE;
	if (near(QPoint(m_cropRect.center().x(), m_cropRect.top())))    return DragMode::ResizeN;
	if (near(QPoint(m_cropRect.center().x(), m_cropRect.bottom()))) return DragMode::ResizeS;
	if (near(QPoint(m_cropRect.left(), m_cropRect.center().y())))   return DragMode::ResizeW;
	if (near(QPoint(m_cropRect.right(), m_cropRect.center().y())))  return DragMode::ResizeE;

	if (m_cropRect.contains(pos))
		return DragMode::Move;

	return DragMode::None;
}

/**
	@brief CropAreaWidget::applyDrag
	Recomputes m_cropRect from m_dragStartRect plus however far pos has
	moved from m_dragStartPos, according to m_dragMode -- always
	clamped to stay within the displayed image and never smaller than
	MIN_CROP_SIZE, so the rect can never escape its bounds or collapse
	to nothing while being dragged.
	@param pos current widget-space mouse position
*/
void CropAreaWidget::applyDrag(const QPoint &pos)
{
	const QPoint delta = pos - m_dragStartPos;
	QRect r = m_dragStartRect;

	auto clampLeft   = [&](int v) { r.setLeft(qBound(m_displayImageRect.left(), v, r.right() - MIN_CROP_SIZE)); };
	auto clampRight  = [&](int v) { r.setRight(qBound(r.left() + MIN_CROP_SIZE, v, m_displayImageRect.right())); };
	auto clampTop    = [&](int v) { r.setTop(qBound(m_displayImageRect.top(), v, r.bottom() - MIN_CROP_SIZE)); };
	auto clampBottom = [&](int v) { r.setBottom(qBound(r.top() + MIN_CROP_SIZE, v, m_displayImageRect.bottom())); };

	switch (m_dragMode)
	{
		case DragMode::Move:
		{
			// Clamping the translation itself, rather than each edge of
			// the resulting rect independently -- clamping edges one at
			// a time would shrink or distort the rect the instant it
			// hit a boundary, instead of just stopping its movement.
			QRect moved = m_dragStartRect.translated(delta);
			if (moved.left() < m_displayImageRect.left())
				moved.translate(m_displayImageRect.left() - moved.left(), 0);
			if (moved.right() > m_displayImageRect.right())
				moved.translate(m_displayImageRect.right() - moved.right(), 0);
			if (moved.top() < m_displayImageRect.top())
				moved.translate(0, m_displayImageRect.top() - moved.top());
			if (moved.bottom() > m_displayImageRect.bottom())
				moved.translate(0, m_displayImageRect.bottom() - moved.bottom());
			r = moved;
			break;
		}
		case DragMode::ResizeN:  clampTop(m_dragStartRect.top() + delta.y()); break;
		case DragMode::ResizeS:  clampBottom(m_dragStartRect.bottom() + delta.y()); break;
		case DragMode::ResizeE:  clampRight(m_dragStartRect.right() + delta.x()); break;
		case DragMode::ResizeW:  clampLeft(m_dragStartRect.left() + delta.x()); break;
		case DragMode::ResizeNE: clampTop(m_dragStartRect.top() + delta.y()); clampRight(m_dragStartRect.right() + delta.x()); break;
		case DragMode::ResizeNW: clampTop(m_dragStartRect.top() + delta.y()); clampLeft(m_dragStartRect.left() + delta.x()); break;
		case DragMode::ResizeSE: clampBottom(m_dragStartRect.bottom() + delta.y()); clampRight(m_dragStartRect.right() + delta.x()); break;
		case DragMode::ResizeSW: clampBottom(m_dragStartRect.bottom() + delta.y()); clampLeft(m_dragStartRect.left() + delta.x()); break;
		case DragMode::None: return;
	}

	m_cropRect = r;
}

void CropAreaWidget::mousePressEvent(QMouseEvent *event)
{
	if (event->button() != Qt::LeftButton)
		return;
	m_dragMode = handleAt(event->pos());
	m_dragStartPos = event->pos();
	m_dragStartRect = m_cropRect;
}

void CropAreaWidget::mouseMoveEvent(QMouseEvent *event)
{
	if (m_dragMode == DragMode::None)
		return;
	applyDrag(event->pos());
	update();
	emit cropRectChanged();
}

void CropAreaWidget::mouseReleaseEvent(QMouseEvent *event)
{
	Q_UNUSED(event);
	m_dragMode = DragMode::None;
}

/**
	@brief CropAreaWidget::paintEvent
	Draws the image, dims everything outside the crop rect (four
	strips around it, rather than one overlay with a hole cut out of
	it, since that would need a more involved clip/composition setup
	for no real benefit here), then the rect's own dashed border and
	its eight handles.
*/
void CropAreaWidget::paintEvent(QPaintEvent *)
{
	QPainter painter(this);
	painter.drawImage(m_displayImageRect, m_source);

	const QColor dim(0, 0, 0, 140);
	painter.fillRect(QRect(m_displayImageRect.left(), m_displayImageRect.top(),
			m_displayImageRect.width(), m_cropRect.top() - m_displayImageRect.top()), dim);
	painter.fillRect(QRect(m_displayImageRect.left(), m_cropRect.bottom() + 1,
			m_displayImageRect.width(), m_displayImageRect.bottom() - m_cropRect.bottom()), dim);
	painter.fillRect(QRect(m_displayImageRect.left(), m_cropRect.top(),
			m_cropRect.left() - m_displayImageRect.left(), m_cropRect.height()), dim);
	painter.fillRect(QRect(m_cropRect.right() + 1, m_cropRect.top(),
			m_displayImageRect.right() - m_cropRect.right(), m_cropRect.height()), dim);

	QPen borderPen(Qt::white);
	borderPen.setStyle(Qt::DashLine);
	painter.setPen(borderPen);
	painter.setBrush(Qt::NoBrush);
	painter.drawRect(m_cropRect);

	painter.setPen(Qt::black);
	painter.setBrush(Qt::white);
	auto drawHandle = [&](const QPoint &center) {
		painter.drawRect(QRect(center.x() - HANDLE_SIZE / 2, center.y() - HANDLE_SIZE / 2, HANDLE_SIZE, HANDLE_SIZE));
	};
	drawHandle(m_cropRect.topLeft());
	drawHandle(m_cropRect.topRight());
	drawHandle(m_cropRect.bottomLeft());
	drawHandle(m_cropRect.bottomRight());
	drawHandle(QPoint(m_cropRect.center().x(), m_cropRect.top()));
	drawHandle(QPoint(m_cropRect.center().x(), m_cropRect.bottom()));
	drawHandle(QPoint(m_cropRect.left(), m_cropRect.center().y()));
	drawHandle(QPoint(m_cropRect.right(), m_cropRect.center().y()));
}

/**
	@brief ImageCropDialog::ImageCropDialog
	@param pixmap the image to crop
	@param existingCropRect a crop rectangle remembered from a previous
	session, or an empty QRect if there isn't one yet
	@param parent
*/
ImageCropDialog::ImageCropDialog(const QPixmap &pixmap, const QRect &existingCropRect, QWidget *parent) :
	QDialog(parent)
{
	setWindowTitle(tr("Rogner l'image"));

	m_cropArea = new CropAreaWidget(pixmap.toImage(), existingCropRect, this);

	auto *hint = new QLabel(tr("Faites glisser les poignées, ou l'intérieur du cadre, pour ajuster la zone à conserver."), this);
	hint->setWordWrap(true);

	auto *resetButton = new QPushButton(tr("Réinitialiser"), this);
	resetButton->setToolTip(tr("Revenir à l'image complète, sans rognage"));
	connect(resetButton, &QPushButton::clicked, m_cropArea, &CropAreaWidget::resetToFullImage);

	auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
	buttons->addButton(resetButton, QDialogButtonBox::ResetRole);

	auto *layout = new QVBoxLayout(this);
	layout->addWidget(hint);
	layout->addWidget(m_cropArea, 0, Qt::AlignHCenter);
	layout->addWidget(buttons);

	connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
	connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

/**
	@brief ImageCropDialog::cropRect
	@return the chosen crop rectangle, in the original pixmap's own coordinates
*/
QRect ImageCropDialog::cropRect() const
{
	return m_cropArea->cropRect();
}
