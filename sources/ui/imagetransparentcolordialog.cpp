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

	const QImage displayImage = (m_displayScale < 1.0)
			? m_source.scaled(m_source.size() * m_displayScale, Qt::KeepAspectRatio, Qt::SmoothTransformation)
			: m_source;
	setPixmap(QPixmap::fromImage(displayImage));
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
	if (event->button() != Qt::LeftButton || pixmap().isNull())
		return;

	// The label may be larger than its pixmap (layout stretching); the
	// pixmap itself is always drawn centered by Qt's default alignment,
	// so the click has to be re-based against the pixmap's own rect
	// within the label, not the label's own top-left.
	const QRect pixmapRect(
			(width() - pixmap().width()) / 2,
			(height() - pixmap().height()) / 2,
			pixmap().width(), pixmap().height());
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
ImageTransparentColorDialog::ImageTransparentColorDialog(const QPixmap &basePixmap, const QList<QColor> &existingColors,
		int existingTolerance, QWidget *parent) :
	QDialog(parent),
	m_sourceImage(basePixmap.toImage()),
	m_pickedColors(existingColors),
	m_tolerance(existingTolerance)
{
	setWindowTitle(tr("Couleur transparente"));

	m_sourceLabel = new ClickableImageLabel(m_sourceImage, this);
	m_previewLabel = new QLabel(this);

	m_hintLabel = new QLabel(this);

	m_toleranceSlider = new QSlider(Qt::Horizontal, this);
	m_toleranceSlider->setRange(0, 100);
	m_toleranceSlider->setValue(m_tolerance);

	auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
	m_okButton = buttons->button(QDialogButtonBox::Ok);
	m_okButton->setEnabled(!m_pickedColors.isEmpty());   // nothing to apply until at least one colour has been picked

	auto *grid = new QGridLayout;
	grid->addWidget(new QLabel(tr("Image source")), 0, 0);
	grid->addWidget(new QLabel(tr("Aperçu")), 0, 1);
	grid->addWidget(m_sourceLabel, 1, 0);
	grid->addWidget(m_previewLabel, 1, 1);

	// An empty row to start with if existingColors is empty --
	// rebuildSwatches() below populates it either way (including from
	// existingColors on the first call), and again as colours get
	// added or removed.
	m_swatchesLayout = new QHBoxLayout;

	auto *colorRow = new QHBoxLayout;
	colorRow->addWidget(m_hintLabel);
	colorRow->addStretch();
	colorRow->addLayout(m_swatchesLayout);

	auto *toleranceRow = new QHBoxLayout;
	toleranceRow->addWidget(new QLabel(tr("Tolérance")));
	toleranceRow->addWidget(m_toleranceSlider);

	auto *mainLayout = new QVBoxLayout(this);
	mainLayout->addLayout(grid);
	mainLayout->addLayout(colorRow);
	mainLayout->addLayout(toleranceRow);
	mainLayout->addWidget(buttons);

	connect(m_sourceLabel, &ClickableImageLabel::colorPicked, this, &ImageTransparentColorDialog::onColorPicked);
	connect(m_toleranceSlider, &QSlider::valueChanged, this, &ImageTransparentColorDialog::onToleranceChanged);
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
	swatch for it.
	@param color the colour sampled from the source image
*/
void ImageTransparentColorDialog::onColorPicked(const QColor &color)
{
	if (m_pickedColors.contains(color))
		return;

	m_pickedColors.append(color);
	m_okButton->setEnabled(true);
	rebuildSwatches();
	updatePreview();
}

/**
	@brief ImageTransparentColorDialog::removeColor
	Removes one colour from the set -- the counterpart onColorPicked()
	was missing entirely before: picking the wrong pixel by mistake had
	no way to undo except cancelling the whole dialog and starting over.
	@param color the colour to remove
*/
void ImageTransparentColorDialog::removeColor(const QColor &color)
{
	m_pickedColors.removeAll(color);
	m_okButton->setEnabled(!m_pickedColors.isEmpty());
	rebuildSwatches();
	updatePreview();
}

/**
	@brief ImageTransparentColorDialog::rebuildSwatches
	Rebuilds the row of picked-colour swatches from scratch against the
	current m_pickedColors -- simpler and safer than trying to
	incrementally add/remove individual widgets in sync with the list,
	given the list only ever changes one colour at a time and is never
	large enough for a full rebuild to be a real cost.
*/
void ImageTransparentColorDialog::rebuildSwatches()
{
	QLayoutItem *item;
	while ((item = m_swatchesLayout->takeAt(0)) != nullptr)
	{
		delete item->widget();
		delete item;
	}

	for (const QColor &color : std::as_const(m_pickedColors))
	{
		auto *swatch = new QPushButton(this);
		swatch->setFixedSize(24, 24);
		swatch->setStyleSheet(QStringLiteral("background-color: rgb(%1,%2,%3); border: 1px solid palette(mid);")
				.arg(color.red()).arg(color.green()).arg(color.blue()));
		swatch->setToolTip(tr("rgb(%1, %2, %3) -- cliquer pour retirer").arg(color.red()).arg(color.green()).arg(color.blue()));
		connect(swatch, &QPushButton::clicked, this, [this, color]() { removeColor(color); });
		m_swatchesLayout->addWidget(swatch);
	}

	m_hintLabel->setText(m_pickedColors.isEmpty()
			? tr("Cliquez sur l'image pour choisir une couleur")
			: tr("Cliquez sur l'image pour ajouter une couleur, ou sur une pastille pour la retirer"));
}

/**
	@brief ImageTransparentColorDialog::onToleranceChanged
	@param value the new tolerance, 0-100
*/
void ImageTransparentColorDialog::onToleranceChanged(int value)
{
	m_tolerance = value;
	if (!m_pickedColors.isEmpty())
		updatePreview();
}

/**
	@brief ImageTransparentColorDialog::updatePreview
	Recomputes the checkerboard-backed preview against the current set
	of picked colours and the shared tolerance. Always runs against
	m_sourceImage (the original, full-resolution image), not any
	already-keyed result -- so adjusting the tolerance, or adding or
	removing a colour, re-evaluates every picked colour from scratch
	each time rather than compounding successive passes.
*/
void ImageTransparentColorDialog::updatePreview()
{
	const QImage keyed = applyColorKey(m_sourceImage, m_pickedColors, m_tolerance);
	m_previewLabel->setPixmap(onCheckerboard(keyed));
}

/**
	@brief ImageTransparentColorDialog::resultPixmap
	@return the colour-keyed pixmap against every picked colour, or the
	original pixmap unchanged if none were ever picked (the Ok button
	stays disabled until at least one is, so this is mostly a defensive
	fallback).
*/
QPixmap ImageTransparentColorDialog::resultPixmap() const
{
	if (m_pickedColors.isEmpty())
		return QPixmap::fromImage(m_sourceImage);
	return QPixmap::fromImage(applyColorKey(m_sourceImage, m_pickedColors, m_tolerance));
}

/**
	@brief ImageTransparentColorDialog::applyColorKey
	Binary transparency within tolerance, not a smooth falloff: every
	pixel within `tolerance` (0-100, mapped onto the maximum possible
	RGB distance) of *any* of keyColors becomes fully transparent,
	everything else keeps its existing alpha untouched. Squared distance
	throughout, avoiding a sqrt per pixel; breaks out of the inner loop
	on the first matching colour, since further matches wouldn't change
	the outcome.
	@param source the image to key
	@param keyColors the colours to make transparent
	@param tolerance 0 (exact match only) to 100 (everything)
	@return the resulting image, always in Format_ARGB32
*/
QImage ImageTransparentColorDialog::applyColorKey(const QImage &source, const QList<QColor> &keyColors, int tolerance)
{
	QImage result = source.convertToFormat(QImage::Format_ARGB32);
	if (keyColors.isEmpty())
		return result;

	QVector<QRgb> keys;
	keys.reserve(keyColors.size());
	for (const QColor &c : keyColors)
		keys.append(c.rgb());

	const qint64 threshold = qint64(tolerance) * tolerance * 3 * 255 * 255 / (100 * 100);

	for (int y = 0; y < result.height(); ++y)
	{
		QRgb *line = reinterpret_cast<QRgb *>(result.scanLine(y));
		for (int x = 0; x < result.width(); ++x)
		{
			const QRgb px = line[x];
			for (const QRgb &key : keys)
			{
				const int dr = qRed(px) - qRed(key), dg = qGreen(px) - qGreen(key), db = qBlue(px) - qBlue(key);
				const qint64 distSq = qint64(dr) * dr + qint64(dg) * dg + qint64(db) * db;
				if (distSq <= threshold)
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
