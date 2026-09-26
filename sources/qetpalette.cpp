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
#include "qetpalette.h"

#include <QApplication>
#include <QColor>
#include <QImage>
#include <QMdiArea>
#include <QStyle>
#include <QTabBar>
#include <QWidget>
#include <cmath>

namespace {
	/**
		Set one role for the Active and Inactive groups (same color: Fusion
		does not draw an inactive window differently, and a differing
		Inactive color is what turned combo box text black on macOS) and
		for the Disabled group.
	*/
	void setRole(QPalette &p, QPalette::ColorRole role,
	             const QColor &active, const QColor &disabled)
	{
		p.setColor(QPalette::Active,   role, active);
		p.setColor(QPalette::Inactive, role, active);
		p.setColor(QPalette::Disabled, role, disabled);
	}

	/// Same color in all three groups.
	void setRole(QPalette &p, QPalette::ColorRole role, const QColor &c)
	{
		setRole(p, role, c, c);
	}

	/// sRGB channel to linear light, as defined by WCAG 2.
	double linear(int channel)
	{
		const double c = channel / 255.0;
		return c <= 0.03928 ? c / 12.92 : std::pow((c + 0.055) / 1.055, 2.4);
	}

	double relativeLuminance(const QColor &c)
	{
		return 0.2126 * linear(c.red())
		     + 0.7152 * linear(c.green())
		     + 0.0722 * linear(c.blue());
	}

	/**
		The selection color shared by both palettes: a blue that reads at
		better than 6:1 under white text. Fusion's own default (48,140,198)
		only reaches 3.7:1.
	*/
	const QColor kHighlight(30, 96, 176);
}

bool QET::Palette::styleIsFusion(const QStyle *style)
{
	return style && style->objectName().compare(QLatin1String("fusion"),
	                                            Qt::CaseInsensitive) == 0;
}

bool QET::Palette::isDark(const QPalette &palette)
{
	return palette.color(QPalette::Active, QPalette::Window).lightness() < 128;
}

void QET::Palette::invertLightness(QImage &image, const QColor &sheet,
                                   const QColor &ink)
{
	if (image.format() != QImage::Format_RGB32)
		image.convertTo(QImage::Format_RGB32);

	// One table per channel maps the inverted value (0 = was white,
	// 255 = was black) onto the sheet..ink span.
	uchar red_of[256], green_of[256], blue_of[256];
	for (int v = 0; v < 256; ++v) {
		red_of[v]   = uchar(sheet.red()   + (ink.red()   - sheet.red())   * v / 255);
		green_of[v] = uchar(sheet.green() + (ink.green() - sheet.green()) * v / 255);
		blue_of[v]  = uchar(sheet.blue()  + (ink.blue()  - sheet.blue())  * v / 255);
	}

	/* Inverting the lightness of an HSL color while keeping its hue and
	   saturation leaves the distance between the highest and the lowest
	   channel unchanged, so it comes down to one offset per pixel:
	   c + 255 - max - min. The offset turns the highest channel into
	   255 - min and the lowest into 255 - max, so no channel can leave
	   the 0..255 range and no clamping is needed. The loop runs on every
	   repaint of a folio, hence the plain integer arithmetic. */
	for (int y = 0; y < image.height(); ++y) {
		quint32 *line = reinterpret_cast<quint32 *>(image.scanLine(y));
		for (int x = 0, width = image.width(); x < width; ++x) {
			const quint32 pixel = line[x];
			const int red   = (pixel >> 16) & 0xff;
			const int green = (pixel >> 8) & 0xff;
			const int blue  = pixel & 0xff;
			int highest = red > green ? red : green;
			int lowest  = red < green ? red : green;
			if (blue > highest) highest = blue;
			if (blue < lowest)  lowest  = blue;
			const int offset = 255 - highest - lowest;
			line[x] = 0xff000000u
			        | (quint32(red_of[red + offset]) << 16)
			        | (quint32(green_of[green + offset]) << 8)
			        | quint32(blue_of[blue + offset]);
		}
	}
}

QColor QET::Palette::gridDotColor(const QColor &sheet, bool inverted)
{
	if (sheet == QColor(Qt::black))
		return Qt::white;
	if (inverted)
		return QColor(sheet.red() * 2 / 3, sheet.green() * 2 / 3, sheet.blue() * 2 / 3);
	return Qt::black;
}

double QET::Palette::contrastRatio(const QColor &a, const QColor &b)
{
	double lighter = relativeLuminance(a);
	double darker  = relativeLuminance(b);
	if (lighter < darker)
		std::swap(lighter, darker);
	return (lighter + 0.05) / (darker + 0.05);
}

QPalette QET::Palette::fusionLight()
{
	QPalette p;
	const QColor window(239, 239, 239);
	const QColor text(Qt::black);
	const QColor disabled_text(128, 128, 128);

	setRole(p, QPalette::Window,          window);
	setRole(p, QPalette::WindowText,      text, disabled_text);
	setRole(p, QPalette::Base,            Qt::white, window);
	setRole(p, QPalette::AlternateBase,   QColor(247, 247, 247));
	setRole(p, QPalette::Text,            text, disabled_text);
	setRole(p, QPalette::Button,          window);
	setRole(p, QPalette::ButtonText,      text, disabled_text);
	setRole(p, QPalette::BrightText,      Qt::white);
	setRole(p, QPalette::Highlight,       kHighlight, QColor(145, 145, 145));
	setRole(p, QPalette::HighlightedText, Qt::white);
	setRole(p, QPalette::ToolTipBase,     QColor(255, 255, 220));
	setRole(p, QPalette::ToolTipText,     text);
	setRole(p, QPalette::Link,            QColor(0, 90, 170));
	setRole(p, QPalette::LinkVisited,     QColor(110, 40, 140));
	setRole(p, QPalette::PlaceholderText, QColor(118, 118, 118));
	// The 3D roles Fusion shades frames and gradients with.
	setRole(p, QPalette::Light,           Qt::white);
	setRole(p, QPalette::Midlight,        QColor(202, 202, 202));
	setRole(p, QPalette::Mid,             QColor(184, 184, 184));
	setRole(p, QPalette::Dark,            QColor(159, 159, 159));
	setRole(p, QPalette::Shadow,          QColor(118, 118, 118));
#if QT_VERSION >= QT_VERSION_CHECK(6, 6, 0)
	setRole(p, QPalette::Accent,          kHighlight);
#endif
	return p;
}

QPalette QET::Palette::fusionDark()
{
	QPalette p;
	const QColor window(53, 53, 53);
	const QColor text(220, 220, 220);
	// Fusion lifts a button's fill to about (89,89,89) at the top of its
	// gradient; 175 is the darkest grey that still reads 3:1 on that.
	const QColor disabled_text(175, 175, 175);

	setRole(p, QPalette::Window,          window);
	setRole(p, QPalette::WindowText,      text, disabled_text);
	setRole(p, QPalette::Base,            QColor(30, 30, 30), QColor(45, 45, 45));
	setRole(p, QPalette::AlternateBase,   QColor(45, 45, 45));
	setRole(p, QPalette::Text,            text, disabled_text);
	setRole(p, QPalette::Button,          QColor(66, 66, 66));
	setRole(p, QPalette::ButtonText,      text, disabled_text);
	setRole(p, QPalette::BrightText,      QColor(255, 90, 90));
	setRole(p, QPalette::Highlight,       kHighlight, QColor(80, 80, 80));
	setRole(p, QPalette::HighlightedText, Qt::white);
	setRole(p, QPalette::ToolTipBase,     QColor(66, 66, 66));
	setRole(p, QPalette::ToolTipText,     text);
	setRole(p, QPalette::Link,            QColor(100, 170, 255));
	setRole(p, QPalette::LinkVisited,     QColor(190, 130, 255));
	setRole(p, QPalette::PlaceholderText, QColor(150, 150, 150));
	setRole(p, QPalette::Light,           QColor(90, 90, 90));
	setRole(p, QPalette::Midlight,        QColor(75, 75, 75));
	setRole(p, QPalette::Mid,             QColor(35, 35, 35));
	setRole(p, QPalette::Dark,            QColor(20, 20, 20));
	setRole(p, QPalette::Shadow,          QColor(10, 10, 10));
#if QT_VERSION >= QT_VERSION_CHECK(6, 6, 0)
	setRole(p, QPalette::Accent,          kHighlight);
#endif
	return p;
}

QPalette QET::Palette::withPlatformAccent(QPalette palette, const QPalette &platform)
{
	const QColor highlight = platform.color(QPalette::Active, QPalette::Highlight);
	const QColor highlighted_text = platform.color(QPalette::Active, QPalette::HighlightedText);
	if (!highlight.isValid() || !highlighted_text.isValid())
		return palette;
	if (contrastRatio(highlight, highlighted_text) < 4.5)
		return palette;

	palette.setColor(QPalette::Active,   QPalette::Highlight, highlight);
	palette.setColor(QPalette::Inactive, QPalette::Highlight, highlight);
	palette.setColor(QPalette::Active,   QPalette::HighlightedText, highlighted_text);
	palette.setColor(QPalette::Inactive, QPalette::HighlightedText, highlighted_text);
#if QT_VERSION >= QT_VERSION_CHECK(6, 6, 0)
	palette.setColor(QPalette::Active,   QPalette::Accent, highlight);
	palette.setColor(QPalette::Inactive, QPalette::Accent, highlight);
#endif
	return palette;
}

QPalette QET::Palette::forFusion(const QPalette &platform)
{
	return withPlatformAccent(isDark(platform) ? fusionDark() : fusionLight(),
	                          platform);
}

bool QET::Palette::isLineArt(const QImage &image)
{
	const QImage source = image.convertToFormat(QImage::Format_ARGB32);
	int visible = 0;
	int saturated = 0;
	for (int y = 0; y < source.height(); ++y)
	{
		const QRgb *line = reinterpret_cast<const QRgb *>(source.constScanLine(y));
		for (int x = 0; x < source.width(); ++x)
		{
			if (qAlpha(line[x]) <= 64)
				continue;
			++visible;
			const QColor color(line[x]);
			if (color.hslSaturationF() > 0.25 && color.value() > 60)
				++saturated;
		}
	}
	return visible > 0 && saturated < visible * 0.20;
}

QImage QET::Palette::invertedLightness(const QImage &image)
{
	// Lightness of pure black after inversion: the dark palette's text.
	const qreal ink = 220.0 / 255.0;
	QImage result = image.convertToFormat(QImage::Format_ARGB32);
	qreal darkest = 1.0;
	for (int y = 0; y < result.height(); ++y)
	{
		const QRgb *line = reinterpret_cast<const QRgb *>(result.constScanLine(y));
		for (int x = 0; x < result.width(); ++x)
			if (qAlpha(line[x]) > 64)
				darkest = qMin(darkest, QColor(line[x]).lightnessF());
	}
	const qreal span = qMax(1.0 - darkest, 1e-6);
	for (int y = 0; y < result.height(); ++y)
	{
		QRgb *line = reinterpret_cast<QRgb *>(result.scanLine(y));
		for (int x = 0; x < result.width(); ++x)
		{
			const int alpha = qAlpha(line[x]);
			if (alpha == 0)
				continue;
			const QColor color(line[x]);
			const qreal lightness = qBound(0.0, ink * (1.0 - (color.lightnessF() - darkest) / span), 1.0);
			QColor out = QColor::fromHslF(qMax(color.hslHueF(), 0.0), color.hslSaturationF(), lightness);
			out.setAlpha(alpha);
			line[x] = out.rgba();
		}
	}
	return result;
}

QPixmap QET::Palette::forPalette(const QPixmap &pixmap, const QPalette &palette)
{
	if (pixmap.isNull() || !isDark(palette))
		return pixmap;
	const QImage image = pixmap.toImage();
	if (!isLineArt(image))
		return pixmap;
	QPixmap result = QPixmap::fromImage(invertedLightness(image));
	result.setDevicePixelRatio(pixmap.devicePixelRatio());
	return result;
}

void QET::Palette::refreshStyleSheets()
{
	// Setting the same sheet again is not a no-op: QWidget::setStyleSheet()
	// asks QStyleSheetStyle to repolish the widget, which recomputes its
	// palette from the application palette now in force.
	const QWidgetList widgets = QApplication::allWidgets();
	for (QWidget *widget : widgets)
		if (!widget->styleSheet().isEmpty())
			widget->setStyleSheet(widget->styleSheet());

	// Force an immediate repaint on tab bars and MDI areas so their text
	// updates together with the rest of the UI, not one event loop later.
	for (QWidget *widget : widgets)
		if (qobject_cast<QTabBar *>(widget) || qobject_cast<QMdiArea *>(widget))
			widget->update();
}
