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
#ifndef IMAGE_TRANSPARENT_COLOR_DIALOG_H
#define IMAGE_TRANSPARENT_COLOR_DIALOG_H

#include <QDialog>
#include <QImage>
#include <QColor>
#include <QLabel>
#include <QList>

class QSlider;
class QPushButton;
class QHBoxLayout;
class QMouseEvent;

/**
	@brief The ClickableImageLabel class
	Displays an image scaled to fit a reasonable dialog size, but always
	samples the picked color from the original, full-resolution image at
	the corresponding coordinate -- never from the (possibly smoothly
	interpolated, and therefore color-blended) scaled-down pixmap being
	displayed, which would make the picked color subtly wrong for
	exactly the pixels near an edge where it matters most.
*/
class ClickableImageLabel : public QLabel
{
	Q_OBJECT

	public:
		explicit ClickableImageLabel(const QImage &sourceImage, QWidget *parent = nullptr);

	signals:
		void colorPicked(const QColor &color);

	protected:
		void mousePressEvent(QMouseEvent *event) override;

	private:
		QImage m_source;
		qreal m_displayScale = 1.0;
};

/**
	@brief The ImageTransparentColorDialog class
	Lets the user click directly on a preview of the image to sample one
	or more colors -- each click adds to the set rather than replacing
	the previous pick, shown as a row of removable swatches -- adjust a
	shared tolerance, and see a live checkerboard-backed preview of the
	result before committing. A self-contained modal dialog rather than
	a diagram-level "click the canvas to pick" interaction mode, since
	this needs neither undo-during-drag nor coexistence with other
	tools; it only ever needs a handful of clicks, evaluated against a
	pixmap the caller already has in hand.
*/
class ImageTransparentColorDialog : public QDialog
{
	Q_OBJECT

	public:
		/// @param basePixmap the pristine source to pick colours from --
		/// the caller's responsibility to pass the true original, not
		/// an already colour-keyed result, or previously-transparent
		/// areas would show as plain background rather than a pickable
		/// surface, and re-picking the same colour would be a no-op.
		/// @param existingColors colours already keyed out of basePixmap
		/// in a previous session, shown as swatches from the start
		/// rather than forcing them to be re-picked from scratch.
		/// @param existingTolerance the tolerance from that previous
		/// session, if any.
		explicit ImageTransparentColorDialog(const QPixmap &basePixmap, const QList<QColor> &existingColors = {},
				int existingTolerance = 10, QWidget *parent = nullptr);

		/// The resulting pixmap: basePixmap unchanged if no colour is
		/// picked, colour-keyed against every picked colour otherwise.
		QPixmap resultPixmap() const;
		/// The final set of picked colours, for the caller to remember
		/// across dialog sessions -- may differ from existingColors if
		/// any were added or removed.
		QList<QColor> pickedColors() const { return m_pickedColors; }
		/// The final tolerance, for the same reason.
		int tolerance() const { return m_tolerance; }

		/// Public so DiagramImageItem can re-derive its display pixmap
		/// directly (base + crop + these colours) without needing to
		/// re-open this dialog every time the crop region changes --
		/// binary transparency within tolerance, not a smooth falloff:
		/// every pixel within `tolerance` (0-100, mapped onto the
		/// maximum possible RGB distance) of *any* of keyColors becomes
		/// fully transparent, everything else keeps its existing alpha
		/// untouched. Squared distance throughout, avoiding a sqrt per
		/// pixel; breaks out of the inner loop on the first matching
		/// colour, since further matches wouldn't change the outcome.
		static QImage applyColorKey(const QImage &source, const QList<QColor> &keyColors, int tolerance);

	private slots:
		void onColorPicked(const QColor &color);
		void onToleranceChanged(int value);

	private:
		void removeColor(const QColor &color);
		void rebuildSwatches();
		void updatePreview();
		static QPixmap onCheckerboard(const QImage &image);

		QImage        m_sourceImage;
		QList<QColor> m_pickedColors;
		int           m_tolerance = 10;

		ClickableImageLabel *m_sourceLabel;
		QLabel              *m_previewLabel;
		QHBoxLayout         *m_swatchesLayout;
		QLabel              *m_hintLabel;
		QSlider             *m_toleranceSlider;
		QPushButton         *m_okButton;
};

#endif // IMAGE_TRANSPARENT_COLOR_DIALOG_H
