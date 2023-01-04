/*
    Copyright 2006-2022 The QElectroTech Team
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
#ifndef TERMINALSTRIPDRAWER_H
#define TERMINALSTRIPDRAWER_H

#include <QPointer>
#include "properties/terminalstriplayoutpattern.h"

class QPainter;
class TerminalStrip;

class TerminalStripDrawer
{
    public:
        TerminalStripDrawer(QPointer<TerminalStrip> strip = QPointer<TerminalStrip>(),
                            QSharedPointer<TerminalStripLayoutPattern> layout = QSharedPointer<TerminalStripLayoutPattern>());

        void setStrip(TerminalStrip *strip);
        void paint(QPainter *painter);
        QRectF boundingRect() const;

        void setLayout(QSharedPointer<TerminalStripLayoutPattern> layout);
        bool haveLayout() const;

    private:
        int height() const;
        int width() const;

    private:
        QPointer<TerminalStrip> m_strip;
        QSharedPointer<TerminalStripLayoutPattern> m_pattern;
};

#endif // TERMINALSTRIPDRAWER_H
