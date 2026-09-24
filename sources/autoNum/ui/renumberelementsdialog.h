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

#ifndef RENUMBERELEMENTSDIALOG_H
#define RENUMBERELEMENTSDIALOG_H

#include <QDialog>
#include <QStringList>

class QComboBox;
class QRadioButton;
class QDialogButtonBox;

/**
 * @brief Simple dialog to pick renumbering scope for elements.
 *
 * The user can choose between:
 *  - all element autonumbering schemes
 *  - one selected scheme title
 */
class RenumberElementsDialog final : public QDialog
{
    Q_OBJECT

public:
    explicit RenumberElementsDialog(const QStringList &scheme_titles, QWidget *parent = nullptr);

    /**
     * @return Empty string if "all" is selected, otherwise the chosen scheme title.
     */
    QString selectedSchemeTitle() const;

private slots:
    void updateUi();

private:
    QStringList m_scheme_titles;
    QRadioButton *m_all_rb = nullptr;
    QRadioButton *m_one_rb = nullptr;
    QComboBox *m_combo = nullptr;
    QDialogButtonBox *m_buttons = nullptr;
};

#endif // RENUMBERELEMENTSDIALOG_H
