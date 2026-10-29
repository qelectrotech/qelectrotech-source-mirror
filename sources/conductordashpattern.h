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
#ifndef CONDUCTOR_DASH_PATTERN_H
#define CONDUCTOR_DASH_PATTERN_H

#include <QPen>
#include <QDomElement>
#include <QMetaType>
#include <cmath>

// Lengths are in diagram coordinates, not multiples of conductor width.
// Page scaling scales the whole pattern, just as it scales the conductor.
struct ConductorDashPattern
{
    bool enabled = false;
    double dash = 6.0;
    double gap = 4.0;
    double dot = 1.0;
    double dotGap = 4.0;

    static constexpr double minimum = 0.1;
    static constexpr double maximum = 1000.0;

    bool valid() const
    {
        for (double v : {dash, gap, dot, dotGap})
            if (!std::isfinite(v) || v < minimum || v > maximum)
                return false;
        return true;
    }

    bool apply(QPen &pen, Qt::PenStyle style) const
    {
        pen.setStyle(style);
        if (!enabled || !valid()
                || (style != Qt::DashLine && style != Qt::DashDotLine))
            return false;
        // Qt treats a zero-width cosmetic pen as one pixel wide.
        const qreal width = pen.widthF() > 0 ? pen.widthF() : 1.0;
        if (!std::isfinite(width))
            return false;
        QVector<qreal> pattern {dash / width, gap / width};
        if (style == Qt::DashDotLine)
            pattern << dot / width << dotGap / width;
        // Square caps extend each dash and shorten each gap by the pen width.
        // Flat caps make the requested visible lengths independent of width.
        pen.setCapStyle(Qt::FlatCap);
        pen.setDashPattern(pattern);
        return true;
    }

    bool operator==(const ConductorDashPattern &other) const
    {
        return enabled == other.enabled && dash == other.dash
            && gap == other.gap && dot == other.dot && dotGap == other.dotGap;
    }
    bool operator!=(const ConductorDashPattern &other) const
    { return !(*this == other); }

    void toXml(QDomElement &element) const
    {
        if (*this == ConductorDashPattern()) return;
        element.setAttribute("custom-dash-pattern", enabled ? "true" : "false");
        element.setAttribute("pattern-dash", QString::number(dash, 'g', 17));
        element.setAttribute("pattern-gap", QString::number(gap, 'g', 17));
        element.setAttribute("pattern-dot", QString::number(dot, 'g', 17));
        element.setAttribute("pattern-dot-gap", QString::number(dotGap, 'g', 17));
    }

    static ConductorDashPattern fromXml(const QDomElement &element)
    {
        ConductorDashPattern p;
        p.enabled = element.attribute("custom-dash-pattern", "false") == "true";
        p.dash = element.attribute("pattern-dash", "6").toDouble();
        p.gap = element.attribute("pattern-gap", "4").toDouble();
        p.dot = element.attribute("pattern-dot", "1").toDouble();
        p.dotGap = element.attribute("pattern-dot-gap", "4").toDouble();
        return p.valid() ? p : ConductorDashPattern();
    }
};

Q_DECLARE_METATYPE(ConductorDashPattern)

#endif
