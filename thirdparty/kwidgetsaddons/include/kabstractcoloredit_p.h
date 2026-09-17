/*
    This file is part of the KDE libraries
    SPDX-FileCopyrightText: 2026 Friedrich W. H. Kossebau <kossebau@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KABSTRACTCOLOREDIT_P_H
#define KABSTRACTCOLOREDIT_P_H

#include <QtClassHelperMacros>

class QWidget;
class QAction;
class QColor;
class QPoint;

class KAbstractColorEditPrivate
{
    Q_DECLARE_PUBLIC(QWidget)

public:
    explicit KAbstractColorEditPrivate(QWidget *qq);
    virtual ~KAbstractColorEditPrivate() = default;

public: // API to implement
    virtual QColor color() const = 0;
    virtual void setColor(const QColor &color) = 0;

public:
    void showContextMenu(QPoint globalPos);

public:
    QWidget *const q_ptr;

    QAction *copyAction;
    QAction *pasteAction;
};

#endif
