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

#include <QStyle>
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
