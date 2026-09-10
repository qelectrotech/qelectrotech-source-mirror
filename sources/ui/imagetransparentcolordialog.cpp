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
#include "imagetransparentcolordialog.h"

#include <QDialogButtonBox>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QMouseEvent>
#include <QPainter>
#include <QPushButton>
#include <QSlider>
#include <QToolButton>
#include <QVBoxLayout>
#include <QVector>

namespace {
	// Fits the dialog comfortably on a normal screen regardless of the
	// source image's own resolution -- a photo straight off a phone
	// would otherwise make this dialog enormous.
	constexpr int MAX_DISPLAY_SIZE = 350;
}

/**
	@brief ClickableImageLabel::ClickableImageLabel
	@param sourceImage the full-resolution image to display and sample from
	@param parent
*/
ClickableImageLabel::ClickableImageLabel(const QImage &sourceImage, QWidget *parent) :
	QLabel(parent),
	m_source(sourceImage)
{
	const qreal scaleW = qreal(MAX_DISPLAY_SIZE) / m_source.width();
	const qreal scaleH = qreal(MAX_DISPLAY_SIZE) / m_source.height();
	m_displayScale = qMin(qreal(1.0), qMin(scaleW, scaleH));   // never upscale a small image, only ever shrink a large one

	m_displayImage = (m_displayScale < 1.0)
			? m_source.scaled(m_source.size() * m_displayScale, Qt::KeepAspectRatio, Qt::SmoothTransformation)
			: m_source;
	setPixmap(QPixmap::fromImage(m_displayImage));
	setCursor(Qt::CrossCursor);
	setToolTip(tr("Cliquez pour choisir une couleur"));
}

/**
	@brief ClickableImageLabel::mousePressEvent
	Maps the click back to the original, full-resolution image before
	sampling -- see the class-level comment for why.
	@param event
*/
void ClickableImageLabel::mousePressEvent(QMouseEvent *event)
{
		// QLabel::pixmap() returns a pointer in Qt5 and a value in Qt6.
		// Qt 5.15 offers the by-value form behind Qt::ReturnByValue; the
		// pointer overload is deprecated there, so take the by-value one
		// on both and the difference reduces to the argument.
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
	const QPixmap label_pixmap = pixmap(Qt::ReturnByValue);
#else
	const QPixmap label_pixmap = pixmap();
#endif

	if (event->button() != Qt::LeftButton || label_pixmap.isNull())
		return;

	// The label may be larger than its pixmap (layout stretching); the
	// pixmap itself is always drawn centered by Qt's default alignment,
	// so the click has to be re-based against the pixmap's own rect
	// within the label, not the label's own top-left.
	const QRect pixmapRect(
			(width() - label_pixmap.width()) / 2,
			(height() - label_pixmap.height()) / 2,
			label_pixmap.width(), label_pixmap.height());
	if (!pixmapRect.contains(event->pos()))
		return;

	const QPoint withinPixmap = event->pos() - pixmapRect.topLeft();
	QPoint originalPos(qRound(withinPixmap.x() / m_displayScale), qRound(withinPixmap.y() / m_displayScale));
	originalPos.setX(qBound(0, originalPos.x(), m_source.width() - 1));
	originalPos.setY(qBound(0, originalPos.y(), m_source.height() - 1));

	emit colorPicked(m_source.pixelColor(originalPos));
}

/**
	@brief ImageTransparentColorDialog::ImageTransparentColorDialog
	@param pixmap the image to pick a transparent colour from
	@param parent
*/
ImageTransparentColorDialog::ImageTransparentColorDialog(const QPixmap &basePixmap,
		const QList<PickedColor> &existingColors, QWidget *parent) :
	QDialog(parent),
	m_sourceImage(basePixmap.toImage()),
	m_pickedColors(existingColors)
{
	setWindowTitle(tr("Couleur transparente"));

	if (!m_pickedColors.isEmpty())
		m_lastToleranceUsed = m_pickedColors.last().tolerance;

	m_sourceLabel = new ClickableImageLabel(m_sourceImage, this);
	m_previewSourceImage = m_sourceLabel->displayImage();
	m_previewLabel = new QLabel(this);

	m_hintLabel = new QLabel(this);
	m_hintLabel->setWordWrap(true);

	auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
	m_okButton = buttons->button(QDialogButtonBox::Ok);
	m_startedWithColors = !m_pickedColors.isEmpty();
	updateOkEnabled();

	auto *grid = new QGridLayout;
	grid->addWidget(new QLabel(tr("Image source")), 0, 0);
	grid->addWidget(new QLabel(tr("Aperçu")), 0, 1);
	grid->addWidget(m_sourceLabel, 1, 0);
	grid->addWidget(m_previewLabel, 1, 1);

	// One row per picked colour, appended/removed as rebuildSwatches()
	// runs -- starts empty either way, populated by the very first
	// rebuildSwatches() call below (including from existingColors, if
	// any were passed in).
	m_swatchesLayout = new QVBoxLayout;

	auto *mainLayout = new QVBoxLayout(this);
	mainLayout->addLayout(grid);
	mainLayout->addWidget(m_hintLabel);
	mainLayout->addLayout(m_swatchesLayout);
	mainLayout->addWidget(buttons);

	connect(m_sourceLabel, &ClickableImageLabel::colorPicked, this, &ImageTransparentColorDialog::onColorPicked);
	connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
	connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

	rebuildSwatches();   // shows existingColors immediately, if any
	updatePreview();     // and the preview already reflects them too, rather than only appearing after the next pick
}

/**
	@brief ImageTransparentColorDialog::onColorPicked
	Adds to the set of picked colours rather than replacing the previous
	pick -- clicking a second, different pixel used to silently discard
	the first choice, with no way to work with more than one colour (a
	white background *and* a grey border, say) in the same pass.
	Skips an exact duplicate rather than adding a second, indistinguishable
	row for it. The new colour is seeded with whatever tolerance was
	last used (not some fixed default), so successive picks in one
	session feel consistent.
	@param color the colour sampled from the source image
*/
void ImageTransparentColorDialog::onColorPicked(const QColor &color)
{
	for (const PickedColor &existing : std::as_const(m_pickedColors))
		if (existing.color == color)
			return;

	m_pickedColors.append({color, m_lastToleranceUsed});
	updateOkEnabled();
	rebuildSwatches();
	updatePreview();
}

/**
	@brief ImageTransparentColorDialog::setToleranceForIndex
	Updates one colour's own tolerance in place -- deliberately doesn't
	call rebuildSwatches(): only a number changed, not which rows exist
	or what order they're in, so recreating every row's widgets (and,
	worse, the very slider currently being dragged) on each tick would
	be both wasteful and liable to interrupt the drag itself.
	@param index which entry in m_pickedColors changed
	@param value its new tolerance, 0-100
*/
void ImageTransparentColorDialog::setToleranceForIndex(int index, int value)
{
	if (index < 0 || index >= m_pickedColors.size())
		return;
	m_pickedColors[index].tolerance = value;
	m_lastToleranceUsed = value;
	updatePreview();
}

/**
	@brief ImageTransparentColorDialog::updateOkEnabled
	OK is disabled only for the one case where accepting would genuinely
	do nothing: a dialog that started with no colours and still has
	none. Removing every colour from a dialog that DID start with some
	is a different, meaningful action -- clearing all transparency back
	to a plain, opaque image -- and has to stay confirmable, not silently
	blocked the same way. Re-run after every add or remove, since either
	can cross the "any colours at all, ever" threshold in either
	direction.
*/
void ImageTransparentColorDialog::updateOkEnabled()
{
	m_okButton->setEnabled(!m_pickedColors.isEmpty() || m_startedWithColors);
}

/**
	@brief ImageTransparentColorDialog::removeColor
	Removes one colour from the set -- the counterpart onColorPicked()
	was missing entirely before: picking the wrong pixel by mistake had
	no way to undo except cancelling the whole dialog and starting over.
	@param index position in m_pickedColors to remove
*/
void ImageTransparentColorDialog::removeColor(int index)
{
	if (index < 0 || index >= m_pickedColors.size())
		return;

	m_pickedColors.removeAt(index);
	updateOkEnabled();
	rebuildSwatches();
	updatePreview();
}

/**
	@brief ImageTransparentColorDialog::rebuildSwatches
	Rebuilds the list of picked-colour rows from scratch against the
	current m_pickedColors -- simpler and safer than trying to
	incrementally add/remove individual rows in sync with the list,
	given the list only ever changes one colour at a time and is never
	large enough for a full rebuild to be a real cost. Each row is a
	colour swatch (a plain, round, non-interactive indicator -- there's
	nothing left to click it FOR, now that each row carries its own
	slider instead of one shared slider needing a row selected first),
	that row's own tolerance slider, and a small "x" button that
	removes it.
*/
void ImageTransparentColorDialog::rebuildSwatches()
{
	QLayoutItem *item;
	while ((item = m_swatchesLayout->takeAt(0)) != nullptr)
	{
		delete item->widget();
		delete item;
	}

	for (int i = 0; i < m_pickedColors.size(); ++i)
	{
		const PickedColor &pc = m_pickedColors.at(i);

		auto *row = new QWidget(this);
		auto *rowLayout = new QHBoxLayout(row);
		rowLayout->setContentsMargins(0, 0, 0, 0);

		auto *swatch = new QLabel(row);
		swatch->setFixedSize(24, 24);
		swatch->setStyleSheet(QStringLiteral(
				"background-color: rgb(%1,%2,%3); border: 1px solid palette(mid); border-radius: 12px;")
				.arg(pc.color.red()).arg(pc.color.green()).arg(pc.color.blue()));
		swatch->setToolTip(tr("rgb(%1, %2, %3)").arg(pc.color.red()).arg(pc.color.green()).arg(pc.color.blue()));

		auto *slider = new QSlider(Qt::Horizontal, row);
		slider->setRange(0, 100);
		slider->setValue(pc.tolerance);
		slider->setToolTip(tr("Tolérance pour cette couleur"));
		connect(slider, &QSlider::valueChanged, this, [this, i](int value) { setToleranceForIndex(i, value); });

		auto *removeButton = new QToolButton(row);
		removeButton->setText(QStringLiteral("×"));
		removeButton->setToolTip(tr("Retirer cette couleur"));
		connect(removeButton, &QToolButton::clicked, this, [this, i]() { removeColor(i); });

		rowLayout->addWidget(swatch);
		rowLayout->addWidget(slider);
		rowLayout->addWidget(removeButton);
		m_swatchesLayout->addWidget(row);
	}

	m_hintLabel->setText(m_pickedColors.isEmpty()
			? tr("Cliquez sur l'image pour choisir une couleur")
			: tr("Cliquez sur l'image pour ajouter une couleur. Ajustez la tolérance de chaque couleur avec son curseur, ou cliquez sur × pour la retirer."));
}

/**
	@brief ImageTransparentColorDialog::updatePreview
	Recomputes the checkerboard-backed preview against the current set
	of picked colours, each at its own tolerance. Runs against
	m_previewSourceImage (the same downsampled copy ClickableImageLabel
	already computed for its own display, not the full-resolution
	m_sourceImage) -- this fires on every tolerance slider tick, not
	just on release, and a full-resolution O(width*height*colours) pass
	per tick was visibly laggy on a large source image. resultPixmap(),
	below, still computes the final, committed result at full
	resolution -- only this live preview is downsampled. Always
	re-evaluates every picked colour from scratch against
	m_previewSourceImage, not any already-keyed result, so adjusting a
	tolerance or adding/removing a colour never compounds successive
	passes.
*/
void ImageTransparentColorDialog::updatePreview()
{
	const QImage keyed = applyColorKey(m_previewSourceImage, m_pickedColors);
	m_previewLabel->setPixmap(onCheckerboard(keyed));
}

/**
	@brief ImageTransparentColorDialog::resultPixmap
	@return the colour-keyed pixmap against every picked colour, each at
	its own tolerance, or the original pixmap unchanged if none were
	ever picked (the Ok button stays disabled until at least one is, so
	this is mostly a defensive fallback).
*/
QPixmap ImageTransparentColorDialog::resultPixmap() const
{
	if (m_pickedColors.isEmpty())
		return QPixmap::fromImage(m_sourceImage);
	return QPixmap::fromImage(applyColorKey(m_sourceImage, m_pickedColors));
}

/**
	@brief ImageTransparentColorDialog::applyColorKey
	Binary transparency within tolerance, not a smooth falloff: every
	pixel within a colour's own `tolerance` (0-100, mapped onto the
	maximum possible RGB distance) of that colour becomes fully
	transparent, everything else keeps its existing alpha untouched --
	each of keyColors is checked against its own threshold, not one
	shared across all of them, since a background colour and a border
	colour rarely call for the same looseness of match. Squared
	distance throughout, avoiding a sqrt per pixel; breaks out of the
	inner loop on the first matching colour, since further matches
	wouldn't change the outcome.
	@param source the image to key
	@param keyColors the colours (each with its own tolerance) to make transparent
	@return the resulting image, always in Format_ARGB32
*/
QImage ImageTransparentColorDialog::applyColorKey(const QImage &source, const QList<PickedColor> &keyColors)
{
	QImage result = source.convertToFormat(QImage::Format_ARGB32);
	if (keyColors.isEmpty())
		return result;

	struct KeyEntry { QRgb rgb; qint64 threshold; };
	QVector<KeyEntry> keys;
	keys.reserve(keyColors.size());
	for (const PickedColor &pc : keyColors)
	{
		const qint64 threshold = qint64(pc.tolerance) * pc.tolerance * 3 * 255 * 255 / (100 * 100);
		keys.append({pc.color.rgb(), threshold});
	}

	for (int y = 0; y < result.height(); ++y)
	{
		QRgb *line = reinterpret_cast<QRgb *>(result.scanLine(y));
		for (int x = 0; x < result.width(); ++x)
		{
			const QRgb px = line[x];
			for (const KeyEntry &key : std::as_const(keys))
			{
				const int dr = qRed(px) - qRed(key.rgb), dg = qGreen(px) - qGreen(key.rgb), db = qBlue(px) - qBlue(key.rgb);
				const qint64 distSq = qint64(dr) * dr + qint64(dg) * dg + qint64(db) * db;
				if (distSq <= key.threshold)
				{
					line[x] = qRgba(qRed(px), qGreen(px), qBlue(px), 0);
					break;
				}
			}
		}
	}
	return result;
}

/**
	@brief ImageTransparentColorDialog::onCheckerboard
	Composites `image` over a light/dark checkerboard, the standard
	visual convention for showing where an image is transparent --
	without this, a fully keyed-out region would just show whatever
	widget background happens to be behind it, easy to misread as "still
	opaque, just white" rather than "correctly transparent".
	@param image the (possibly partially transparent) image to composite
	@return a checkerboard-backed pixmap ready to display
*/
QPixmap ImageTransparentColorDialog::onCheckerboard(const QImage &image)
{
	const int cell = 8;
	QPixmap board(image.size());
	QPainter painter(&board);
	for (int y = 0; y < image.height(); y += cell)
	{
		for (int x = 0; x < image.width(); x += cell)
		{
			const bool dark = ((x / cell) + (y / cell)) % 2 == 0;
			painter.fillRect(x, y, cell, cell, dark ? QColor(200, 200, 200) : QColor(255, 255, 255));
		}
	}
	painter.drawImage(0, 0, image);
	painter.end();
	return board;
}
