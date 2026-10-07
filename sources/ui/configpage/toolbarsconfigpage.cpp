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
	m_icon_size->addItem(tr("Default"), 0);
	m_icon_size->addItem(tr("Small (16 px)"), 16);
	m_icon_size->addItem(tr("Medium (24 px)"), 24);
	m_icon_size->addItem(tr("Large (32 px)"), 32);
	m_icon_size->addItem(tr("Very large (48 px)"), 48);
	const int size_index = m_icon_size->findData(ToolbarSettings::iconSize());
	m_icon_size->setCurrentIndex(size_index < 0 ? 0 : size_index);

	m_button_style = new QComboBox(this);
	m_button_style->setObjectName(QStringLiteral("buttonStyleCombo"));
	m_button_style->addItem(tr("Icon only"), int(Qt::ToolButtonIconOnly));
	m_button_style->addItem(tr("Text beside the icon"), int(Qt::ToolButtonTextBesideIcon));
	m_button_style->addItem(tr("Text under the icon"), int(Qt::ToolButtonTextUnderIcon));
	m_button_style->setCurrentIndex(m_button_style->findData(int(ToolbarSettings::buttonStyle())));

	m_locked = new QCheckBox(tr("Lock the toolbars (they can no longer be moved)"), this);
	m_locked->setObjectName(QStringLiteral("lockedCheck"));
	m_locked->setChecked(ToolbarSettings::locked());

	auto *form = new QFormLayout();
	form->addRow(tr("Icon size:"), m_icon_size);
	form->addRow(tr("Buttons:"), m_button_style);
	form->addRow(m_locked);
	vlayout->addLayout(form);

	auto *hint = new QLabel(tr("To show or hide a toolbar, right-click on a toolbar."), this);
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
	return tr("Toolbars", "configuration page title");
}

QIcon ToolbarsConfigPage::icon() const
{
	return QET::Icons::ConfigureToolbars;
}
