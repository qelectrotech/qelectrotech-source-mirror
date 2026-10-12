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

class QVBoxLayout;
class QPushButton;
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

		/// The same downsampled image already computed for display --
		/// reused as the live-preview source so a tolerance drag runs
		/// its per-tick colour-key pass against a small image instead
		/// of the full-resolution one, which on a large source (a
		/// scanned schematic background, several Mpx) made every
		/// intermediate slider tick visibly lag. resultPixmap() still
		/// computes the final, committed result from the true
		/// full-resolution source -- only the live preview is
		/// downsampled.
		QImage displayImage() const { return m_displayImage; }

	signals:
		void colorPicked(const QColor &color);

	protected:
		void mousePressEvent(QMouseEvent *event) override;

	private:
		QImage m_source;
		QImage m_displayImage;
		qreal m_displayScale = 1.0;
};

/**
	@brief The ImageTransparentColorDialog class
	Lets the user click directly on a preview of the image to sample one
	or more colors -- each click adds to the set rather than replacing
	the previous pick -- each shown as its own row: a colour swatch, a
	slider for that colour's own tolerance right next to it, and a
	remove button. Each colour keeps its own tolerance rather than
	sharing one: a white background and a grey border rarely need the
	same looseness of match, and forcing one tolerance onto both meant
	either the background left ragged edges or the border ate into
	content near it. A live checkerboard-backed preview of the combined
	result updates as any slider moves. A self-contained modal dialog
	rather than a diagram-level "click the canvas to pick" interaction
	mode, since this needs neither undo-during-drag nor coexistence
	with other tools; it only ever needs a handful of clicks, evaluated
	against a pixmap the caller already has in hand.
*/
class ImageTransparentColorDialog : public QDialog
{
	Q_OBJECT

	public:
		/// One picked colour and the tolerance it's individually keyed
		/// with -- the whole point of this being a struct rather than
		/// two parallel lists is that the two can never drift out of
		/// index alignment with each other.
		struct PickedColor
		{
			QColor color;
			int tolerance;
		};

		/// @param basePixmap the pristine source to pick colours from --
		/// the caller's responsibility to pass the true original, not
		/// an already colour-keyed result, or previously-transparent
		/// areas would show as plain background rather than a pickable
		/// surface, and re-picking the same colour would be a no-op.
		/// @param existingColors colours (with their individual
		/// tolerances) already keyed out of basePixmap in a previous
		/// session, shown as rows from the start rather than forcing
		/// them to be re-picked from scratch.
		explicit ImageTransparentColorDialog(const QPixmap &basePixmap,
				const QList<PickedColor> &existingColors = {}, QWidget *parent = nullptr);

		/// The resulting pixmap: basePixmap unchanged if no colour is
		/// picked, colour-keyed against every picked colour (each at
		/// its own tolerance) otherwise.
		QPixmap resultPixmap() const;
		/// The final set of picked colours and their individual
		/// tolerances, for the caller to remember across dialog
		/// sessions -- may differ from existingColors if any were
		/// added, removed, or had their tolerance adjusted.
		QList<PickedColor> pickedColors() const { return m_pickedColors; }

		/// Public so DiagramImageItem can re-derive its display pixmap
		/// directly (base + crop + these colours) without needing to
		/// re-open this dialog every time the crop region changes --
		/// binary transparency within tolerance, not a smooth falloff:
		/// every pixel within a colour's own `tolerance` (0-100, mapped
		/// onto the maximum possible RGB distance) of that colour
		/// becomes fully transparent, everything else keeps its
		/// existing alpha untouched. Squared distance throughout,
		/// avoiding a sqrt per pixel; breaks out of the inner loop on
		/// the first matching colour, since further matches wouldn't
		/// change the outcome.
		static QImage applyColorKey(const QImage &source, const QList<PickedColor> &keyColors);

	private slots:
		void onColorPicked(const QColor &color);

	private:
		void setToleranceForIndex(int index, int value);
		void removeColor(int index);
		void rebuildSwatches();
		void updatePreview();
		void updateOkEnabled();
		static QPixmap onCheckerboard(const QImage &image);

		QImage              m_sourceImage;
		QImage              m_previewSourceImage;   // downsampled -- see ClickableImageLabel::displayImage()'s comment for why
		QList<PickedColor>  m_pickedColors;
		bool                m_startedWithColors = false;   // whether existingColors was non-empty -- see updateOkEnabled()'s comment for why this matters
		int                 m_lastToleranceUsed = 10;   // seeds a newly-picked colour's own tolerance, so successive picks in one session feel consistent rather than each resetting to some fixed default

		ClickableImageLabel *m_sourceLabel;
		QLabel              *m_previewLabel;
		QVBoxLayout         *m_swatchesLayout;   // one row per picked colour, stacked vertically -- each row now carries its own slider, too wide to lay out side by side the way plain swatches once were
		QLabel              *m_hintLabel;
		QPushButton         *m_okButton;
};

#endif // IMAGE_TRANSPARENT_COLOR_DIALOG_H
