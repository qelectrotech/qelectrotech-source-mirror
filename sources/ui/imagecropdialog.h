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
#ifndef IMAGE_CROP_DIALOG_H
#define IMAGE_CROP_DIALOG_H

#include <QDialog>
#include <QImage>
#include <QRect>
#include <QWidget>

class QMouseEvent;
class QPaintEvent;
class QPushButton;

/**
	@brief The CropAreaWidget class
	Displays the image scaled to fit a reasonable dialog size, with a
	draggable, resizable crop rectangle overlaid on top -- the area
	outside it dimmed, the usual visual convention for "this part goes
	away" during a crop, so it's never ambiguous which region survives.
	All internal geometry (the crop rect, drag handling, hit-testing)
	works in display-space pixel coordinates; only cropRect() converts
	back to the original, full-resolution image's own coordinates for
	the caller, the same original-vs-display distinction already used
	for colour sampling in ImageTransparentColorDialog.
*/
class CropAreaWidget : public QWidget
{
	Q_OBJECT

	public:
		/// @param sourceImage the full, uncropped image to crop from
		/// @param initialCropRect a previously-chosen crop rectangle, in
		/// sourceImage's own coordinates, to start from -- an empty
		/// (default-constructed) QRect means "no previous crop", i.e.
		/// start covering the whole image.
		explicit CropAreaWidget(const QImage &sourceImage, const QRect &initialCropRect = QRect(), QWidget *parent = nullptr);

		/// The current crop rectangle, in the original image's own
		/// coordinates (not display-space) -- always clamped to the
		/// image's own bounds and never degenerate (zero width/height).
		QRect cropRect() const;

	public slots:
		/// Resets the crop rectangle back to covering the entire image --
		/// the direct answer to "I want to undo the crop, not just
		/// adjust it", since dragging every handle back out by hand to
		/// exactly the original extent is needlessly fiddly.
		void resetToFullImage();

	signals:
		void cropRectChanged();

	protected:
		void paintEvent(QPaintEvent *event) override;
		void mousePressEvent(QMouseEvent *event) override;
		void mouseMoveEvent(QMouseEvent *event) override;
		void mouseReleaseEvent(QMouseEvent *event) override;

	private:
		enum class DragMode { None, Move, ResizeN, ResizeS, ResizeE, ResizeW, ResizeNE, ResizeNW, ResizeSE, ResizeSW };

		DragMode handleAt(const QPoint &pos) const;
		void applyDrag(const QPoint &pos);

		QImage  m_source;
		qreal   m_displayScale = 1.0;
		QRect   m_displayImageRect;   // where the (possibly scaled) pixmap actually sits within this widget
		QRect   m_cropRect;           // in display-space, relative to m_displayImageRect's own origin

		DragMode m_dragMode = DragMode::None;
		QPoint   m_dragStartPos;
		QRect    m_dragStartRect;

		static constexpr int HANDLE_SIZE = 8;
		static constexpr int MIN_CROP_SIZE = 16;
};

/**
	@brief The ImageCropDialog class
	Self-contained modal dialog wrapping CropAreaWidget with OK/Cancel --
	the same reasoning as ImageTransparentColorDialog: this needs
	neither undo-during-drag nor coexistence with other tools, only a
	rectangle the user drags out against a pixmap already in hand.
*/
class ImageCropDialog : public QDialog
{
	Q_OBJECT

	public:
		/// @param pixmap the full, uncropped image to crop from
		/// @param existingCropRect a crop rectangle remembered from a
		/// previous session, shown from the start rather than resetting
		/// to the full image every time the dialog is reopened -- an
		/// empty QRect means there isn't one yet.
		explicit ImageCropDialog(const QPixmap &pixmap, const QRect &existingCropRect = QRect(), QWidget *parent = nullptr);

		/// The chosen crop rectangle, in the original pixmap's own
		/// coordinates. Only meaningful after the dialog was accepted.
		QRect cropRect() const;

	private:
		CropAreaWidget *m_cropArea;
};

#endif // IMAGE_CROP_DIALOG_H
