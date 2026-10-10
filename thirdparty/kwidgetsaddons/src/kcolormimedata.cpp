/*
    This file is part of the KDE libraries
    SPDX-FileCopyrightText: 1999 Steffen Hansen <hansen@kde.org>
    SPDX-FileCopyrightText: 2005 Joseph Wenninger <jowenn@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "kcolormimedata_p.h"

#include <QColor>
#include <QDrag>
#include <QMimeData>
#include <QPainter>

namespace KColorMimeData
{

void populateMimeData(QMimeData *mimeData, const QColor &color)
{
    mimeData->setColorData(color);
    mimeData->setText(color.name());
}

bool canDecode(const QMimeData *mimeData)
{
    // QClipboard::mimeData() can return nullptr when the clipboard has never
    // been set (e.g. no clipboard manager, or the "offscreen" QPA platform).
    if (!mimeData) {
        return false;
    }
    if (mimeData->hasColor()) {
        return true;
    }
    if (mimeData->hasText()) {
        const QString colorName = mimeData->text();
        if ((colorName.length() >= 4) && (colorName[0] == QLatin1Char('#'))) {
            return true;
        }
    }
    return false;
}

QColor fromMimeData(const QMimeData *mimeData)
{
    if (!mimeData) {
        return QColor();
    }
    if (mimeData->hasColor()) {
        return mimeData->colorData().value<QColor>();
    }
    if (canDecode(mimeData)) {
        return QColor(mimeData->text());
    }
    return QColor();
}

QDrag *createDrag(const QColor &color, QObject *dragsource)
{
    QDrag *drag = new QDrag(dragsource);
    QMimeData *mime = new QMimeData;
    populateMimeData(mime, color);
    drag->setMimeData(mime);
    QPixmap colorpix(25, 20);
    colorpix.fill(color);
    QPainter p(&colorpix);
    p.setPen(Qt::black);
    p.drawRect(0, 0, 24, 19);
    p.end();
    drag->setPixmap(colorpix);
    drag->setHotSpot(QPoint(-5, -7));
    return drag;
}

}
