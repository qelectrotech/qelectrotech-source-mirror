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

#include "renumberelementsdialog.h"

#include <QComboBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QRadioButton>
#include <QVBoxLayout>

RenumberElementsDialog::RenumberElementsDialog(const QStringList &scheme_titles, QWidget *parent)
    : QDialog(parent)
    , m_scheme_titles(scheme_titles)
{
    setWindowTitle(tr("Renumber element(s)"));
    setModal(true);

    auto *vl = new QVBoxLayout(this);

    auto *scope = new QGroupBox(tr("Scope"), this);
    auto *scope_l = new QVBoxLayout(scope);
    m_all_rb = new QRadioButton(tr("All schemes"), scope);
    m_one_rb = new QRadioButton(tr("One scheme"), scope);
    scope_l->addWidget(m_all_rb);
    scope_l->addWidget(m_one_rb);
    vl->addWidget(scope);

    auto *form = new QFormLayout();
    m_combo = new QComboBox(this);
    m_combo->addItems(m_scheme_titles);
    form->addRow(new QLabel(tr("Scheme:"), this), m_combo);
    vl->addLayout(form);

    m_buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(m_buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(m_buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    vl->addWidget(m_buttons);

    m_all_rb->setChecked(true);
    updateUi();

    connect(m_all_rb, &QRadioButton::toggled, this, &RenumberElementsDialog::updateUi);
    connect(m_one_rb, &QRadioButton::toggled, this, &RenumberElementsDialog::updateUi);
}

QString RenumberElementsDialog::selectedSchemeTitle() const
{
    if (m_all_rb && m_all_rb->isChecked()) return QString();
    return m_combo ? m_combo->currentText() : QString();
}

void RenumberElementsDialog::updateUi()
{
    const bool one = m_one_rb && m_one_rb->isChecked();
    if (m_combo) m_combo->setEnabled(one);
}
