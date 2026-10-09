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
#include "toolbarsconfigpage.h"

#include "../../qeticons.h"
#include "../../toolbarsettings.h"

#include <QCheckBox>
#include <QComboBox>
#include <QFormLayout>
#include <QFrame>
#include <QLabel>
#include <QVBoxLayout>

ToolbarsConfigPage::ToolbarsConfigPage(QWidget *parent) :
	ConfigPage(parent)
{
	auto *vlayout = new QVBoxLayout(this);
	vlayout->addWidget(new QLabel(title(), this));
	auto *line = new QFrame(this);
	line->setFrameShape(QFrame::HLine);
	vlayout->addWidget(line);

	m_icon_size = new QComboBox(this);
	m_icon_size->setObjectName(QStringLiteral("iconSizeCombo"));
	m_icon_size->addItem(tr("Par défaut"), 0);
	m_icon_size->addItem(tr("Petites (16 px)"), 16);
	m_icon_size->addItem(tr("Moyennes (24 px)"), 24);
	m_icon_size->addItem(tr("Grandes (32 px)"), 32);
	m_icon_size->addItem(tr("Très grandes (48 px)"), 48);
	const int size_index = m_icon_size->findData(ToolbarSettings::iconSize());
	m_icon_size->setCurrentIndex(size_index < 0 ? 0 : size_index);

	m_button_style = new QComboBox(this);
	m_button_style->setObjectName(QStringLiteral("buttonStyleCombo"));
	m_button_style->addItem(tr("Icône seule"), int(Qt::ToolButtonIconOnly));
	m_button_style->addItem(tr("Texte à côté de l'icône"), int(Qt::ToolButtonTextBesideIcon));
	m_button_style->addItem(tr("Texte sous l'icône"), int(Qt::ToolButtonTextUnderIcon));
	m_button_style->setCurrentIndex(m_button_style->findData(int(ToolbarSettings::buttonStyle())));

	m_locked = new QCheckBox(tr("Verrouiller les barres d'outils (elles ne peuvent plus être déplacées)"), this);
	m_locked->setObjectName(QStringLiteral("lockedCheck"));
	m_locked->setChecked(ToolbarSettings::locked());

	auto *form = new QFormLayout();
	form->addRow(tr("Taille des icônes :"), m_icon_size);
	form->addRow(tr("Boutons :"), m_button_style);
	form->addRow(m_locked);
	vlayout->addLayout(form);

	auto *hint = new QLabel(tr("Pour afficher ou masquer une barre d'outils, faites un clic droit sur une barre d'outils."), this);
	hint->setWordWrap(true);
	vlayout->addWidget(hint);
	vlayout->addStretch();
}

void ToolbarsConfigPage::applyConf()
{
	ToolbarSettings::save(m_icon_size->currentData().toInt(),
			      Qt::ToolButtonStyle(m_button_style->currentData().toInt()),
			      m_locked->isChecked());
	ToolbarSettings::applyToAll();
}

QString ToolbarsConfigPage::title() const
{
	return tr("Barres d'outils", "configuration page title");
}

QIcon ToolbarsConfigPage::icon() const
{
	return QET::Icons::ConfigureToolbars;
}
