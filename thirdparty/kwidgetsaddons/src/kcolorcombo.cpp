/*
    This file is part of the KDE libraries
    SPDX-FileCopyrightText: 1997 Martin Jones <mjones@kde.org>
    SPDX-FileCopyrightText: 2007 Pino Toscano <pino@kde.org>
    SPDX-FileCopyrightText: 2007 David Jarvie <djarvie@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "kcolorcombo.h"

#include "kabstractcoloredit_p.h"
#include "kcolormimedata_p.h"

#include <QAbstractItemDelegate>
#include <QApplication>
#include <QColorDialog>
#include <QContextMenuEvent>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QStylePainter>

class KColorComboDelegate : public QAbstractItemDelegate
{
    Q_OBJECT
public:
    enum ItemRoles {
        ColorRole = Qt::UserRole + 1,
    };

    enum LayoutMetrics {
        FrameMargin = 3,
    };

    KColorComboDelegate(QObject *parent = nullptr);
    ~KColorComboDelegate() override;

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override;
};

static QBrush k_colorcombodelegate_brush(const QModelIndex &index, int role)
{
    QBrush brush;
    QVariant v = index.data(role);
    if (v.userType() == QMetaType::QBrush) {
        brush = v.value<QBrush>();
    } else if (v.userType() == QMetaType::QColor) {
        brush = QBrush(v.value<QColor>());
    }
    return brush;
}

KColorComboDelegate::KColorComboDelegate(QObject *parent)
    : QAbstractItemDelegate(parent)
{
}

KColorComboDelegate::~KColorComboDelegate()
{
}

void KColorComboDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    // background
    QColor innercolor(Qt::white);
    bool isSelected = (option.state & QStyle::State_Selected);
    bool paletteBrush = (k_colorcombodelegate_brush(index, Qt::BackgroundRole).style() == Qt::NoBrush);
    if (isSelected) {
        innercolor = option.palette.color(QPalette::Highlight);
    } else {
        innercolor = option.palette.color(QPalette::Base);
    }
    // highlight selected item
    QStyleOptionViewItem opt(option);
    opt.showDecorationSelected = true;
    QStyle *style = opt.widget ? opt.widget->style() : QApplication::style();
    style->drawPrimitive(QStyle::PE_PanelItemViewItem, &opt, painter, opt.widget);
    QRect innerrect = option.rect.adjusted(FrameMargin, FrameMargin, -FrameMargin, -FrameMargin);
    // inner color
    QVariant cv = index.data(ColorRole);
    if (cv.userType() == QMetaType::QColor) {
        QColor tmpcolor = cv.value<QColor>();
        if (tmpcolor.isValid()) {
            innercolor = tmpcolor;
            paletteBrush = false;
            painter->setPen(Qt::transparent);
            painter->setBrush(innercolor);
            QPainter::RenderHints tmpHint = painter->renderHints();
            painter->setRenderHint(QPainter::Antialiasing);
            painter->drawRoundedRect(innerrect, 2, 2);
            painter->setRenderHints(tmpHint);
            painter->setBrush(Qt::NoBrush);
        }
    }
    // text
    QVariant tv = index.data(Qt::DisplayRole);
    if (tv.userType() == QMetaType::QString) {
        QString text = tv.toString();
        QColor textColor;
        if (paletteBrush) {
            if (isSelected) {
                textColor = option.palette.color(QPalette::HighlightedText);
            } else {
                textColor = option.palette.color(QPalette::Text);
            }
        } else {
            int unused;
            int v;
            innercolor.getHsv(&unused, &unused, &v);
            if (v > 128) {
                textColor = Qt::black;
            } else {
                textColor = Qt::white;
            }
        }
        painter->setPen(textColor);
        painter->drawText(innerrect.adjusted(1, 1, -1, -1), text);
    }
}

QSize KColorComboDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    Q_UNUSED(index)

    // the width does not matter, as the view will always use the maximum width available
    return QSize(100, option.fontMetrics.height() + 2 * FrameMargin);
}

static const uchar standardPalette[][4] = {
    {255, 255, 255}, // white
    {192, 192, 192}, // light gray
    {160, 160, 160}, // gray
    {128, 128, 128}, // dark gray
    {0, 0, 0}, // black

    {255, 128, 128}, // light red
    {255, 192, 128}, // light orange
    {255, 255, 128}, // light yellow
    {128, 255, 128}, // light green
    {128, 255, 255}, // cyan blue
    {128, 128, 255}, // light blue
    {255, 128, 255}, // light violet
    {255, 0, 0}, // red
    {255, 128, 0}, // orange
    {255, 255, 0}, // yellow
    {0, 255, 0}, // green
    {0, 255, 255}, // light blue
    {0, 0, 255}, // blue
    {255, 0, 255}, // violet
    {128, 0, 0}, // dark red
    {128, 64, 0}, // dark orange
    {128, 128, 0}, // dark yellow
    {0, 128, 0}, // dark green
    {0, 128, 128}, // dark light blue
    {0, 0, 128}, // dark blue
    {128, 0, 128} // dark violet
};

#define STANDARD_PALETTE_SIZE (int(sizeof(standardPalette) / sizeof(*standardPalette)))

static inline QColor standardColor(int i)
{
    const uchar *entry = standardPalette[i];
    return QColor(entry[0], entry[1], entry[2]);
}

class KColorComboPrivate : public KAbstractColorEditPrivate
{
    Q_DECLARE_PUBLIC(KColorCombo)

public:
    KColorComboPrivate(KColorCombo *qq);

    QColor color() const override;
    void setColor(const QColor &color) override;

    void addColors();
    void setCustomColor(const QColor &color, bool lookupInPresets = true);

    // slots
    void slotActivated(int index);
    void slotHighlighted(int index);

    QList<QColor> colorList;
    QColor customColor;
    QColor internalcolor;
};

KColorComboPrivate::KColorComboPrivate(KColorCombo *qq)
    : KAbstractColorEditPrivate(qq)
    , customColor(Qt::white)
{
}

QColor KColorComboPrivate::color() const
{
    Q_Q(const KColorCombo);

    return q->color();
}

void KColorComboPrivate::setColor(const QColor &color)
{
    Q_Q(KColorCombo);

    q->setColor(color);
}

void KColorComboPrivate::setCustomColor(const QColor &color, bool lookupInPresets)
{
    Q_Q(KColorCombo);

    if (lookupInPresets) {
        if (colorList.isEmpty()) {
            for (int i = 0; i < STANDARD_PALETTE_SIZE; ++i) {
                if (standardColor(i) == color) {
                    q->setCurrentIndex(i + 1);
                    internalcolor = color;
                    return;
                }
            }
        } else {
            int i = colorList.indexOf(color);
            if (i >= 0) {
                q->setCurrentIndex(i + 1);
                internalcolor = color;
                return;
            }
        }
    }

    internalcolor = color;
    customColor = color;
    q->setItemData(0, customColor, KColorComboDelegate::ColorRole);
    q->update();
}

KColorCombo::KColorCombo(QWidget *parent)
    : QComboBox(parent)
    , d(new KColorComboPrivate(this))
{
    setAcceptDrops(true);

    setItemDelegate(new KColorComboDelegate(this));
    d->addColors();

    connect(this, &QComboBox::activated, this, [this](int index) {
        d->slotActivated(index);
    });
    connect(this, &QComboBox::highlighted, this, [this](int index) {
        d->slotHighlighted(index);
    });

    // select the white color
    setCurrentIndex(1);
    d->slotActivated(1);

    setMaxVisibleItems(13);
}

KColorCombo::~KColorCombo() = default;

void KColorCombo::setColors(const QList<QColor> &colors)
{
    clear();
    d->colorList = colors;
    d->addColors();
}

QList<QColor> KColorCombo::colors() const
{
    if (d->colorList.isEmpty()) {
        QList<QColor> list;
        list.reserve(STANDARD_PALETTE_SIZE);
        for (int i = 0; i < STANDARD_PALETTE_SIZE; ++i) {
            list += standardColor(i);
        }
        return list;
    } else {
        return d->colorList;
    }
}

void KColorCombo::setColor(const QColor &col)
{
    if (!col.isValid()) {
        return;
    }

    if (count() == 0) {
        d->addColors();
    }

    d->setCustomColor(col, true);
}

QColor KColorCombo::color() const
{
    return d->internalcolor;
}

bool KColorCombo::isCustomColor() const
{
    return d->internalcolor == d->customColor;
}

void KColorCombo::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)
    QStylePainter painter(this);
    painter.setPen(palette().color(QPalette::Text));

    QStyleOptionComboBox opt;
    initStyleOption(&opt);
    painter.drawComplexControl(QStyle::CC_ComboBox, opt);

    QRect frame = style()->subControlRect(QStyle::CC_ComboBox, &opt, QStyle::SC_ComboBoxEditField, this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(Qt::transparent);
    painter.setBrush(QBrush(d->internalcolor));
    painter.drawRoundedRect(frame.adjusted(1, 1, -1, -1), 2, 2);
}

void KColorCombo::contextMenuEvent(QContextMenuEvent *ev)
{
    ev->accept();

    d->showContextMenu(ev->globalPos());
}

void KColorCombo::showEmptyList()
{
    clear();
}

void KColorCombo::dragEnterEvent(QDragEnterEvent *event)
{
    event->setAccepted(KColorMimeData::canDecode(event->mimeData()) && isEnabled());
}

void KColorCombo::dropEvent(QDropEvent *event)
{
    const QColor c = KColorMimeData::fromMimeData(event->mimeData());
    if (c.isValid()) {
        setColor(c);
    }
}

void KColorComboPrivate::slotActivated(int index)
{
    Q_Q(KColorCombo);

    if (index == 0) {
        QColor c = QColorDialog::getColor(customColor, q);
        if (c.isValid()) {
            customColor = c;
            setCustomColor(customColor, false);
        }
    } else if (colorList.isEmpty()) {
        internalcolor = standardColor(index - 1);
    } else {
        internalcolor = colorList[index - 1];
    }

    Q_EMIT q->activated(internalcolor);
}

void KColorComboPrivate::slotHighlighted(int index)
{
    Q_Q(KColorCombo);

    if (index == 0) {
        internalcolor = customColor;
    } else if (colorList.isEmpty()) {
        internalcolor = standardColor(index - 1);
    } else {
        internalcolor = colorList[index - 1];
    }

    Q_EMIT q->highlighted(internalcolor);
}

void KColorComboPrivate::addColors()
{
    Q_Q(KColorCombo);

    q->addItem(KColorCombo::tr("Custom…", "@item:inlistbox Custom color"));

    if (colorList.isEmpty()) {
        for (int i = 0; i < STANDARD_PALETTE_SIZE; ++i) {
            q->addItem(QString());
            q->setItemData(i + 1, standardColor(i), KColorComboDelegate::ColorRole);
        }
    } else {
        for (int i = 0, count = colorList.count(); i < count; ++i) {
            q->addItem(QString());
            q->setItemData(i + 1, colorList[i], KColorComboDelegate::ColorRole);
        }
    }
}

#include "kcolorcombo.moc"
#include "moc_kcolorcombo.cpp"
