/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef PROTECTIVE_EARTH_PATTERN_H
#define PROTECTIVE_EARTH_PATTERN_H

#include "conductorproperties.h"
#include <QStringList>

namespace ProtectiveEarthPattern {

inline bool isPeName(const QString &name)
{
    return name.trimmed().compare(QStringLiteral("PE"), Qt::CaseInsensitive) == 0;
}

// Require an explicit electrical identifier. A colour or a dash style is not
// sufficient: control signals and mechanical lines can use the same style.
inline bool isExplicitPe(const ConductorProperties &properties,
                         const QStringList &terminalNames = {})
{
    if (isPeName(properties.m_function) || isPeName(properties.text)
            || isPeName(properties.m_tension_protocol))
        return true;
    for (const QString &name : terminalNames)
        if (isPeName(name))
            return true;
    return false;
}

// Deliberately copy only the line pattern. In particular this preserves wire
// numbering, colour, width, cross-section, cable assignment and text layout.
inline ConductorProperties withPattern(const ConductorProperties &target,
                                        const ConductorProperties &source)
{
    ConductorProperties result = target;
    result.style = source.style;
    result.m_dash_pattern = source.m_dash_pattern;
    return result;
}

} // namespace ProtectiveEarthPattern
#endif
