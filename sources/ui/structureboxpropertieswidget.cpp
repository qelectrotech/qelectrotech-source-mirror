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

#include "../QPropertyUndoCommand/qpropertyundocommand.h"
#include "../diagram.h"
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
	layout->addRow(tr("Plant (=)"), m_plant_edit);
	layout->addRow(tr("Location (+)"), m_location_edit);
	layout->addRow(tr("Prefix (-)"), m_prefix_edit);

	updateUi();
}

void StructureBoxPropertiesWidget::apply()
{
	if (!m_item)
		return;

		//One undo step for all that changed, as the other item editors do
	QUndoCommand *undo = nullptr;
	const auto change = [&](const char *property, const QString &old_value, const QString &new_value) {
		if (old_value == new_value) return;
		if (!undo) undo = new QUndoCommand(tr("Modify a structure box"));
		new QPropertyUndoCommand(m_item, property, old_value, new_value, undo);
	};
	change("plant",    m_item->plant(),    m_plant_edit->text());
	change("location", m_item->location(), m_location_edit->text());
	change("prefix",   m_item->prefix(),   m_prefix_edit->text());
	if (!undo) return;
	if (m_item->diagram())
		m_item->diagram()->undoStack().push(undo);
	else {
		undo->redo();
		delete undo;
	}
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

/**
	@brief StructureBoxPropertiesWidget::setLiveEdit
	In the Selection properties panel there is no Apply button: a value is
	applied when its field is left, as the other item editors do.
	@param live_edit
	@return always true
*/
bool StructureBoxPropertiesWidget::setLiveEdit(bool live_edit)
{
	if (live_edit == m_live_edit)
		return true;
	m_live_edit = live_edit;

	for (const auto &connection : std::as_const(m_edit_connections))
		disconnect(connection);
	m_edit_connections.clear();

	if (m_live_edit) {
		for (QLineEdit *edit : {m_plant_edit, m_location_edit, m_prefix_edit})
			m_edit_connections << connect(edit, &QLineEdit::editingFinished,
										  this, &StructureBoxPropertiesWidget::apply);
	}
	return true;
}
