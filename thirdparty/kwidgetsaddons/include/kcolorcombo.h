/*
    This file is part of the KDE libraries
    SPDX-FileCopyrightText: 1997 Martin Jones <mjones@kde.org>
    SPDX-FileCopyrightText: 2007 David Jarvie <djarvie@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/
//-----------------------------------------------------------------------------
// KDE color selection combo box

// layout management added Oct 1997 by Mario Weilguni
// <mweilguni@sime.com>

#ifndef KCOLORCOMBO_H
#define KCOLORCOMBO_H

#include <QComboBox>
#include <QList>

#include <kwidgetsaddons_export.h>

#include <memory>

/*!
 * \class KColorCombo
 * \inmodule KWidgetsAddons
 *
 * \brief Combobox for colors.
 *
 * The combobox provides some preset colors to be selected, and an entry to
 * select a custom color using a color dialog.
 *
 * \image kcolorcombo.png "KColorCombo Widget"
 */
class KWIDGETSADDONS_EXPORT KColorCombo : public QComboBox
{
    Q_OBJECT

    /*!
     * \property KColorCombo::color
     */
    Q_PROPERTY(QColor color READ color WRITE setColor NOTIFY activated USER true)

    /*!
     * \property KColorCombo::colors
     */
    Q_PROPERTY(QList<QColor> colors READ colors WRITE setColors)

public:
    /*!
     * Constructs a color combo box.
     */
    explicit KColorCombo(QWidget *parent = nullptr);
    ~KColorCombo() override;

    /*!
     * Selects the color \a col.
     */
    void setColor(const QColor &col);

    /*!
     * Returns the currently selected color.
     */
    QColor color() const;

    /*!
     * Find whether the currently selected color is a custom color selected
     * using a color dialog.
     */
    bool isCustomColor() const;

    /*!
     * Set a custom list of colors to choose from, in place of the standard
     * list.
     *
     * \a colors list of colors. If empty, the selection list reverts to
     *             the standard list.
     */
    void setColors(const QList<QColor> &colors);

    /*!
     * Return the list of colors available for selection.
     */
    QList<QColor> colors() const;

    /*!
     * Clear the color list and don't show it, till the next setColor() call
     */
    void showEmptyList();

Q_SIGNALS:
    /*!
     * Emitted when a new color box has been selected.
     */
    void activated(const QColor &col);
    /*!
     * Emitted when a new item has been highlighted.
     */
    void highlighted(const QColor &col);

protected:
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dropEvent(QDropEvent *event) override;
    void paintEvent(QPaintEvent *event) override;
    void contextMenuEvent(QContextMenuEvent *) override;

private:
    friend class KColorComboPrivate;
    std::unique_ptr<class KColorComboPrivate> const d;

    Q_DISABLE_COPY(KColorCombo)
};

#endif // KCOLORCOMBO_H
