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

#include "cabinetlayoutsourcewidget.h"
#include "cabinetlayoutsourcemodel.h"

#include <QTreeView>
#include <QVBoxLayout>
#include <QHeaderView>

CabinetLayoutSourceWidget::CabinetLayoutSourceWidget(QWidget *parent) :
	QWidget(parent)
{
	m_tree_view = new QTreeView(this);
	m_tree_view->setHeaderHidden(true);
	m_tree_view->setDragEnabled(true);
	m_tree_view->setDragDropMode(QAbstractItemView::DragOnly);
	m_tree_view->header()->setStretchLastSection(true);

	auto *layout = new QVBoxLayout(this);
	layout->setContentsMargins(0, 0, 0, 0);
	layout->addWidget(m_tree_view);
	setLayout(layout);
}

/**
	@brief CabinetLayoutSourceWidget::setProject
	Point the tree at @a project. Passing nullptr (e.g. when the last
	project of the editor is closed) clears the tree.
*/
void CabinetLayoutSourceWidget::setProject(QETProject *project)
{
	if (m_model) {
		m_tree_view->setModel(nullptr);
		delete m_model;
		m_model = nullptr;
	}

	if (!project)
		return;

	m_model = new CabinetLayoutSourceModel(project, this);
	m_tree_view->setModel(m_model);
	m_tree_view->expandAll();
	connect(m_model, &QAbstractItemModel::rowsInserted,
			m_tree_view, &QTreeView::expandAll);
}

/**
	@brief CabinetLayoutSourceWidget::setActiveDiagram
	See header.
*/
void CabinetLayoutSourceWidget::setActiveDiagram(Diagram *diagram)
{
	if (m_model)
		m_model->setActiveDiagram(diagram);
}