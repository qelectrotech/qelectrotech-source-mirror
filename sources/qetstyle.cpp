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
#include "qetstyle.h"

#include <QApplication>
#include <QImage>
#include <QPainter>
#include <QPixmap>
#include <QStyleOption>
#include <QWidget>

#include "qetpalette.h"

/**
	@brief QETStyle::QETStyle
	@param base the style to wrap; this object takes ownership of it.
*/
QETStyle::QETStyle(QStyle *base) :
	QProxyStyle(base)
{
	setObjectName(base ? base->objectName() : QString());
}

/**
	@brief QETStyle::isLineArt
	The same rule misc/make_icon_themes.py uses to pick the icons that
	get a dark variant: an icon is line art when fewer than 20% of its
	visible pixels are saturated.
*/
bool QETStyle::isLineArt(const QImage &image)
{
	const QImage source = image.convertToFormat(QImage::Format_ARGB32);
	int visible = 0;
	int saturated = 0;
	for (int y = 0; y < source.height(); ++y)
	{
		const QRgb *line = reinterpret_cast<const QRgb *>(source.constScanLine(y));
		for (int x = 0; x < source.width(); ++x)
		{
			const QRgb pixel = line[x];
			if (qAlpha(pixel) <= 64)
				continue;
			++visible;
			const QColor color(pixel);
			if (color.hslSaturationF() > 0.25 && color.value() > 60)
				++saturated;
		}
	}
	return visible > 0 && saturated < visible * 0.20;
}

/**
	@brief QETStyle::hoverColor
	The palette's highlight color is the accent users already know from
	selections. On a dark palette it is too dark to read on a hovered
	button face, so it is lightened, a step at a time, until it reaches
	3:1 (WCAG 1.4.11) against the Light role: Fusion paints a hovered
	auto-raise button with a gradient that runs from Button up to about
	that color, and the icon has to read on the lightest part of it.
*/
QColor QETStyle::hoverColor(const QPalette &palette)
{
	const QColor face = palette.color(QPalette::Active, QPalette::Light);
	QColor ink = palette.color(QPalette::Active, QPalette::Highlight);
	// 3.5 rather than 3.0: the top of Fusion's hover gradient is a shade
	// lighter than the Light role, so the icon needs some headroom there.
	for (int step = 0; step < 20 && QET::Palette::contrastRatio(ink, face) < 3.5; ++step)
		ink = ink.lighter(110);
	return ink;
}

/**
	@brief QETStyle::tinted
	@return pixmap with every pixel set to color, alpha kept.
*/
QPixmap QETStyle::tinted(const QPixmap &pixmap, const QColor &color)
{
	QImage image = pixmap.toImage().convertToFormat(QImage::Format_ARGB32_Premultiplied);
	QPainter painter(&image);
	painter.setCompositionMode(QPainter::CompositionMode_SourceIn);
	painter.fillRect(image.rect(), color);
	painter.end();

	QPixmap result = QPixmap::fromImage(image);
	result.setDevicePixelRatio(pixmap.devicePixelRatio());
	return result;
}

/**
	@brief QETStyle::menuIcon
	An icon for a highlighted menu item: Normal as the original, Active
	as the original in the HighlightedText color when it is line art.
	Cached per source icon, size and device pixel ratio, since menus
	repaint on every pointer move.
*/
QIcon QETStyle::menuIcon(const QIcon &icon, const QPalette &palette, int size, qreal dpr) const
{
	const QString key = QString("%1/%2/%3/%4")
	                    .arg(icon.cacheKey()).arg(size).arg(dpr)
	                    .arg(palette.color(QPalette::Active, QPalette::HighlightedText).name());
	auto it = menu_icons_.constFind(key);
	if (it != menu_icons_.constEnd())
		return *it;

	const QPixmap normal = icon.pixmap(QSize(size, size), dpr, QIcon::Normal);
	QIcon result;
	result.addPixmap(normal, QIcon::Normal);
	result.addPixmap(isLineArt(normal.toImage())
	                 ? tinted(normal, palette.color(QPalette::Active, QPalette::HighlightedText))
	                 : normal,
	                 QIcon::Active);
	result.addPixmap(icon.pixmap(QSize(size, size), dpr, QIcon::Disabled), QIcon::Disabled);
	menu_icons_.insert(key, result);
	return result;
}

/**
	@brief QETStyle::drawControl
	CE_MenuItem with an icon and a highlight: swap the icon for menuIcon()
	so the base style's QIcon::Active request gets an icon that reads on
	the highlight bar. Everything else goes to the base style.
*/
void QETStyle::drawControl(ControlElement element,
                           const QStyleOption *option,
                           QPainter *painter,
                           const QWidget *widget) const
{
	if (element == CE_MenuItem)
	{
		const auto *item = qstyleoption_cast<const QStyleOptionMenuItem *>(option);
		if (item && !item->icon.isNull()
		    && (item->state & State_Selected) && (item->state & State_Enabled))
		{
			QStyleOptionMenuItem copy = *item;
			const int size = pixelMetric(PM_SmallIconSize, option, widget);
			const qreal dpr = widget ? widget->devicePixelRatio()
			                         : QApplication::instance() ? qApp->devicePixelRatio() : 1.0;
			copy.icon = menuIcon(item->icon, item->palette, size, dpr);
			QProxyStyle::drawControl(element, &copy, painter, widget);
			return;
		}
	}
	QProxyStyle::drawControl(element, option, painter, widget);
}

/**
	@brief QETStyle::generatedIconPixmap
	QIcon::Active: a line-art icon comes back tinted in hoverColor(),
	alpha kept, so the hovered icon differs from the normal one on both
	light and dark palettes. Colored icons and every
	other mode go to the base style.
*/
QPixmap QETStyle::generatedIconPixmap(QIcon::Mode mode,
                                      const QPixmap &pixmap,
                                      const QStyleOption *option) const
{
	if (mode != QIcon::Active || pixmap.isNull())
		return QProxyStyle::generatedIconPixmap(mode, pixmap, option);

	if (!isLineArt(pixmap.toImage()))
		return QProxyStyle::generatedIconPixmap(mode, pixmap, option);

	const QPalette palette = option ? option->palette : QApplication::palette();
	return tinted(pixmap, hoverColor(palette));
}
