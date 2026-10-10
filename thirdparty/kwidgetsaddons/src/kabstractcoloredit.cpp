/*
    This file is part of the KDE libraries
    SPDX-FileCopyrightText: 2026 Friedrich W. H. Kossebau <kossebau@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "kabstractcoloredit_p.h"

#include "kcolormimedata_p.h"

#include <QAction>
#include <QApplication>
#include <QClipboard>
#include <QGlobalStatic>
#include <QIcon>
#include <QMenu>
#include <QMimeData>

class KColorClipboardWatcher : public QObject
{
    Q_OBJECT

public:
    KColorClipboardWatcher();

    bool isColorOnClipboardAvailable() const;

Q_SIGNALS:
    void colorOnClipboardAvailableChanged(bool available);

private:
    void updateState();

private:
    bool m_colorOnClipboardAvailable = false;
};

KColorClipboardWatcher::KColorClipboardWatcher()
{
    connect(QApplication::clipboard(), &QClipboard::dataChanged, this, &KColorClipboardWatcher::updateState);
    const QMimeData *const mimeData = QApplication::clipboard()->mimeData(QClipboard::Clipboard);
    m_colorOnClipboardAvailable = KColorMimeData::canDecode(mimeData);
}

bool KColorClipboardWatcher::isColorOnClipboardAvailable() const
{
    return m_colorOnClipboardAvailable;
}

void KColorClipboardWatcher::updateState()
{
    const QMimeData *const mimeData = QApplication::clipboard()->mimeData(QClipboard::Clipboard);
    const bool colorOnClipboardAvailable = KColorMimeData::canDecode(mimeData);
    if (colorOnClipboardAvailable == m_colorOnClipboardAvailable) {
        return;
    }
    m_colorOnClipboardAvailable = colorOnClipboardAvailable;
    Q_EMIT colorOnClipboardAvailableChanged(m_colorOnClipboardAvailable);
}

Q_GLOBAL_STATIC(KColorClipboardWatcher, globalClipboardWatcher)

KAbstractColorEditPrivate::KAbstractColorEditPrivate(QWidget *qq)
    : q_ptr(qq)
{
    Q_Q(QWidget);

    copyAction =
        new QAction(QIcon::fromTheme(QIcon::ThemeIcon::EditCopy), QCoreApplication::translate("KAbstractColorEdit", "Copy Color", "@action:inmenu"), q);
    copyAction->setShortcut(QKeySequence::Copy);
    copyAction->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    QObject::connect(copyAction, &QAction::triggered, q, [this]() {
        auto *const mimeData = new QMimeData;
        KColorMimeData::populateMimeData(mimeData, color());
        QApplication::clipboard()->setMimeData(mimeData, QClipboard::Clipboard);
    });
    q->addAction(copyAction); // register shortcuts

    pasteAction =
        new QAction(QIcon::fromTheme(QIcon::ThemeIcon::EditPaste), QCoreApplication::translate("KAbstractColorEdit", "Paste Color", "@action:inmenu"), q);
    pasteAction->setShortcut(QKeySequence::Paste);
    pasteAction->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    QObject::connect(pasteAction, &QAction::triggered, q, [this]() {
        const QColor color = KColorMimeData::fromMimeData(QApplication::clipboard()->mimeData(QClipboard::Clipboard));
        if (color.isValid()) {
            setColor(color);
        }
    });
    pasteAction->setEnabled(globalClipboardWatcher()->isColorOnClipboardAvailable());
    QObject::connect(globalClipboardWatcher(), &KColorClipboardWatcher::colorOnClipboardAvailableChanged, pasteAction, &QAction::setEnabled);
    q->addAction(pasteAction); // register shortcuts
}

void KAbstractColorEditPrivate::showContextMenu(QPoint globalPos)
{
    Q_Q(QWidget);

    auto menu = new QMenu(q);
    menu->setAttribute(Qt::WA_DeleteOnClose);

    menu->addAction(copyAction);
    menu->addAction(pasteAction);

    menu->popup(globalPos);
}

#include "kabstractcoloredit.moc"
