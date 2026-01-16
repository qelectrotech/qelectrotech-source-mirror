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
#ifndef TERMINALSTRIPLAYOUTPATTERNXML_H
#define TERMINALSTRIPLAYOUTPATTERNXML_H

#include <QDomElement>
#include <QSharedPointer>

class TerminalStripLayoutPattern;

/**
 * @brief The TerminalStripLayoutPatternXml class
 * A class with static function used to save/restor a
 * @class TerminalStripLayoutPattern to xml
 */
class TerminalStripLayoutPatternXml
{
    public:
        static QDomElement toXml(const QVector<QSharedPointer<TerminalStripLayoutPattern>> &patterns, QDomDocument &document);
        static QVector<QSharedPointer<TerminalStripLayoutPattern>> fromXml(const QDomElement &element);

        static QDomElement toXml (const QSharedPointer<TerminalStripLayoutPattern> &pattern, QDomDocument &document);
        static void fromXml(QSharedPointer<TerminalStripLayoutPattern> &layout, const QDomElement &element);        
};

#endif // TERMINALSTRIPLAYOUTPATTERNXML_H
