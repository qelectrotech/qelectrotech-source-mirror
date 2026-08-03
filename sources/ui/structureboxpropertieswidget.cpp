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
#include "structureboxpropertieswidget.h"

#include "../qetgraphicsitem/structureboxitem.h"

#include <QFormLayout>
#include <QLineEdit>

StructureBoxPropertiesWidget::StructureBoxPropertiesWidget(StructureBoxItem *item, QWidget *parent) :
	PropertiesEditorWidget(parent),
	m_item(item),
	m_plant_edit(new QLineEdit(this)),
	m_location_edit(new QLineEdit(this)),
	m_prefix_edit(new QLineEdit(this))
{
	auto layout = new QFormLayout(this);
	layout->addRow(tr("Installation (=)"), m_plant_edit);
	layout->addRow(tr("Localisation (+)"), m_location_edit);
	layout->addRow(tr("Préfixe (-)"), m_prefix_edit);

	updateUi();
}

void StructureBoxPropertiesWidget::apply()
{
	if (!m_item)
		return;

	m_item->setPlant(m_plant_edit->text());
	m_item->setLocation(m_location_edit->text());
	m_item->setPrefix(m_prefix_edit->text());
}

void StructureBoxPropertiesWidget::reset()
{
	updateUi();
}

void StructureBoxPropertiesWidget::updateUi()
{
	if (!m_item)
		return;

	m_plant_edit->setText(m_item->plant());
	m_location_edit->setText(m_item->location());
	m_prefix_edit->setText(m_item->prefix());
}
