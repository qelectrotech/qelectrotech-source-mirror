/*
    This file is part of the KDE libraries
    SPDX-FileCopyrightText: 1997 Martin Jones <mjones@kde.org>
    SPDX-FileCopyrightText: 1999 Cristian Tibirna <ctibirna@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "kcolorbutton.h"

#include "kabstractcoloredit_p.h"
#include "kcolormimedata_p.h"

#include <QApplication>
#include <QColorDialog>
#include <QDrag>
#include <QMimeData>
#include <QMouseEvent>
#include <QPainter>
#include <QPointer>
#include <QStyle>
#include <QStyleOptionButton>
#include <qdrawutil.h>

class KColorButtonPrivate : public KAbstractColorEditPrivate
{
    Q_DECLARE_PUBLIC(KColorButton)

public:
    KColorButtonPrivate(KColorButton *qq);

    QColor color() const override;
    void setColor(const QColor &color) override;

    void chooseColor();
    void colorChosen();

    QColor m_defaultColor;
    bool m_bdefaultColor : 1;
    bool m_alphaChannel : 1;

    QColor col;
    QPoint mPos;

    QPointer<QColorDialog> dialogPtr;

    void initStyleOption(QStyleOptionButton *opt) const;
};

KColorButtonPrivate::KColorButtonPrivate(KColorButton *qq)
    : KAbstractColorEditPrivate(qq)
{
    Q_Q(KColorButton);

    m_bdefaultColor = false;
    m_alphaChannel = false;
    q->setAcceptDrops(true);

    QObject::connect(q, &KColorButton::clicked, q, [this]() {
        chooseColor();
    });
}

QColor KColorButtonPrivate::color() const
{
    Q_Q(const KColorButton);

    return q->color();
}

void KColorButtonPrivate::setColor(const QColor &color)
{
    Q_Q(KColorButton);

    q->setColor(color);
}

KColorButton::KColorButton(QWidget *parent)
    : QPushButton(parent)
    , d(new KColorButtonPrivate(this))
{
}

KColorButton::KColorButton(const QColor &c, QWidget *parent)
    : KColorButton(parent)
{
    d->col = c;
}

KColorButton::KColorButton(const QColor &c, const QColor &defaultColor, QWidget *parent)
    : KColorButton(c, parent)
{
    setDefaultColor(defaultColor);
}

KColorButton::~KColorButton() = default;

QColor KColorButton::color() const
{
    return d->col;
}

void KColorButton::setColor(const QColor &c)
{
    if (d->col != c) {
        d->col = c;
        update();
        Q_EMIT changed(d->col);
    }
}

void KColorButton::setAlphaChannelEnabled(bool alpha)
{
    d->m_alphaChannel = alpha;
}

bool KColorButton::isAlphaChannelEnabled() const
{
    return d->m_alphaChannel;
}

QColor KColorButton::defaultColor() const
{
    return d->m_defaultColor;
}

void KColorButton::setDefaultColor(const QColor &c)
{
    d->m_bdefaultColor = c.isValid();
    d->m_defaultColor = c;
}

void KColorButtonPrivate::initStyleOption(QStyleOptionButton *opt) const
{
    Q_Q(const KColorButton);

    opt->initFrom(q);
    opt->state |= q->isDown() ? QStyle::State_Sunken : QStyle::State_Raised;
    opt->features = QStyleOptionButton::None;
    if (q->isDefault()) {
        opt->features |= QStyleOptionButton::DefaultButton;
    }
    opt->text.clear();
    opt->icon = QIcon();
}

void KColorButton::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    QStyle *style = QWidget::style();

    // First, we need to draw the bevel.
    QStyleOptionButton butOpt;
    d->initStyleOption(&butOpt);
    style->drawControl(QStyle::CE_PushButtonBevel, &butOpt, &painter, this);

    // OK, now we can muck around with drawing out pretty little color box
    // First, sort out where it goes
    QRect labelRect = style->subElementRect(QStyle::SE_PushButtonContents, &butOpt, this);
    int shift = style->pixelMetric(QStyle::PM_ButtonMargin, &butOpt, this) / 2;
    labelRect.adjust(shift, shift, -shift, -shift);
    int x;
    int y;
    int w;
    int h;
    labelRect.getRect(&x, &y, &w, &h);

    if (isChecked() || isDown()) {
        x += style->pixelMetric(QStyle::PM_ButtonShiftHorizontal, &butOpt, this);
        y += style->pixelMetric(QStyle::PM_ButtonShiftVertical, &butOpt, this);
    }

    QColor fillCol = isEnabled() ? d->col : palette().color(backgroundRole());
    qDrawShadePanel(&painter, x, y, w, h, palette(), true, 1, nullptr);
    if (fillCol.isValid()) {
        const QRect rect(x + 1, y + 1, w - 2, h - 2);
        if (fillCol.alpha() < 255) {
            QPixmap chessboardPattern(16, 16);
            QPainter patternPainter(&chessboardPattern);
            patternPainter.fillRect(0, 0, 8, 8, Qt::black);
            patternPainter.fillRect(8, 8, 8, 8, Qt::black);
            patternPainter.fillRect(0, 8, 8, 8, Qt::white);
            patternPainter.fillRect(8, 0, 8, 8, Qt::white);
            patternPainter.end();
            painter.fillRect(rect, QBrush(chessboardPattern));
        }
        painter.fillRect(rect, fillCol);
    }

    if (hasFocus()) {
        QRect focusRect = style->subElementRect(QStyle::SE_PushButtonFocusRect, &butOpt, this);
        QStyleOptionFocusRect focusOpt;
        focusOpt.initFrom(this);
        focusOpt.rect = focusRect;
        focusOpt.backgroundColor = palette().window().color();
        style->drawPrimitive(QStyle::PE_FrameFocusRect, &focusOpt, &painter, this);
    }
}

QSize KColorButton::sizeHint() const
{
    QStyleOptionButton opt;
    d->initStyleOption(&opt);
    return style()->sizeFromContents(QStyle::CT_PushButton, &opt, QSize(40, 15), this);
}

QSize KColorButton::minimumSizeHint() const
{
    QStyleOptionButton opt;
    d->initStyleOption(&opt);
    return style()->sizeFromContents(QStyle::CT_PushButton, &opt, QSize(3, 3), this);
}

void KColorButton::dragEnterEvent(QDragEnterEvent *event)
{
    event->setAccepted(KColorMimeData::canDecode(event->mimeData()) && isEnabled());
}

void KColorButton::dropEvent(QDropEvent *event)
{
    QColor c = KColorMimeData::fromMimeData(event->mimeData());
    if (c.isValid()) {
        setColor(c);
    }
}

#if KWIDGETSADDONS_BUILD_DEPRECATED_SINCE(6, 29)
void KColorButton::keyPressEvent(QKeyEvent *e)
{
    QPushButton::keyPressEvent(e);
}
#endif

void KColorButton::mousePressEvent(QMouseEvent *e)
{
    d->mPos = e->pos();
    QPushButton::mousePressEvent(e);
}

void KColorButton::mouseMoveEvent(QMouseEvent *e)
{
    if ((e->buttons() & Qt::LeftButton) && (e->pos() - d->mPos).manhattanLength() > QApplication::startDragDistance()) {
        KColorMimeData::createDrag(color(), this)->exec(Qt::CopyAction);
        setDown(false);
    }
}

void KColorButton::contextMenuEvent(QContextMenuEvent *ev)
{
    ev->accept();

    d->showContextMenu(ev->globalPos());
}

void KColorButtonPrivate::chooseColor()
{
    Q_Q(KColorButton);

    QColorDialog *dialog = dialogPtr.data();
    if (dialog) {
        dialog->show();
        dialog->raise();
        dialog->activateWindow();
        return;
    }

    dialog = new QColorDialog(q);
    dialog->setCurrentColor(q->color());
    dialog->setOption(QColorDialog::ShowAlphaChannel, m_alphaChannel);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    QObject::connect(dialog, &QDialog::accepted, q, [this]() {
        colorChosen();
    });
    dialogPtr = dialog;
    dialog->show();
}

void KColorButtonPrivate::colorChosen()
{
    Q_Q(KColorButton);

    QColorDialog *dialog = dialogPtr.data();
    if (!dialog) {
        return;
    }

    if (dialog->selectedColor().isValid()) {
        q->setColor(dialog->selectedColor());
    } else if (m_bdefaultColor) {
        q->setColor(m_defaultColor);
    }
}

#include "moc_kcolorbutton.cpp"
