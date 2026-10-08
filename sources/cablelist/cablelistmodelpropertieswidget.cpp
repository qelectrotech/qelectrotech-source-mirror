/*
	Copyright 2006-2026 The QElectroTech Team
	This file is part of QElectroTech.

	QElectroTech is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	QElectroTech is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with QElectroTech.  If not, see <http://www.gnu.org/licenses/>.
*/
#include "cablelistmodelpropertieswidget.h"

#include "cablelistmodel.h"
#include "cablequerywidget.h"

#include <QDialog>
#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QPushButton>
#include <QVBoxLayout>

/**
	@brief CableListModelPropertiesWidget::CableListModelPropertiesWidget
	@param model
	@param parent
*/
CableListModelPropertiesWidget::CableListModelPropertiesWidget(
		CableListModel *model,
		QWidget *parent) :
	PropertiesEditorWidget(parent)
{
	auto *layout = new QHBoxLayout(this);
	m_edit_pb = new QPushButton(tr("Modifier les colonnes..."), this);
	m_refresh_pb = new QPushButton(tr("Rafraîchir"), this);
	layout->addWidget(m_edit_pb);
	layout->addWidget(m_refresh_pb);
	layout->addStretch();

	connect(m_edit_pb, &QPushButton::clicked, this, &CableListModelPropertiesWidget::editColumns);
	connect(m_refresh_pb, &QPushButton::clicked, this, &CableListModelPropertiesWidget::refreshRows);

	setModel(model);
}

/**
	@brief CableListModelPropertiesWidget::setModel
	@param model
*/
void CableListModelPropertiesWidget::setModel(CableListModel *model)
{
	m_model = model;
	m_edit_pb->setEnabled(m_model != nullptr);
	m_refresh_pb->setEnabled(m_model != nullptr);
}

/**
	@brief CableListModelPropertiesWidget::editColumns
	Ask for the columns again and put them on the table right away --
	the same immediate, not-undoable way the database table changes its
	query
*/
void CableListModelPropertiesWidget::editColumns()
{
	if (!m_model) {
		return;
	}

	QDialog d(this);
	d.setWindowTitle(tr("Colonnes de la liste des câbles"));
	auto *layout = new QVBoxLayout(&d);
	auto *widget = new CableQueryWidget(&d);
	widget->setFields(m_model->fields());
	layout->addWidget(widget);

	auto *button_box = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
	layout->addWidget(button_box);
	connect(button_box, &QDialogButtonBox::accepted, &d, &QDialog::accept);
	connect(button_box, &QDialogButtonBox::rejected, &d, &QDialog::reject);

	if (d.exec()) {
		m_model->setFields(widget->selectedKeys());
	}
}

/**
	@brief CableListModelPropertiesWidget::refreshRows
*/
void CableListModelPropertiesWidget::refreshRows()
{
	if (m_model) {
		m_model->refresh();
	}
}
