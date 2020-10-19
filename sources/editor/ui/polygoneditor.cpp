/*
	Copyright 2006-2020 The QElectroTech Team
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
#include "polygoneditor.h"
#include "ui_polygoneditor.h"
#include "partpolygon.h"
#include "elementscene.h"
#include "qetmessagebox.h"
#include "styleeditor.h"
#include "QPropertyUndoCommand/qpropertyundocommand.h"

/**
	@brief PolygonEditor::PolygonEditor
	@param editor
	@param part
	@param parent
*/
PolygonEditor::PolygonEditor(QETElementEditor *editor,
			     PartPolygon *part,
			     QWidget *parent) :
	ElementItemEditor(editor, parent),
	ui(new Ui::PolygonEditor),
	m_part(part)
{
	ui->setupUi(this);
	m_style = new StyleEditor(editor);
	ui->m_main_layout->insertWidget(0, m_style);
	updateForm();
	ui->m_points_list_tree->installEventFilter(this);
	ui->m_points_list_tree->addAction(ui->m_add_point_action);
	ui->m_points_list_tree->addAction(ui->m_remove_point_action);
}

/**
	@brief PolygonEditor::~PolygonEditor
*/
PolygonEditor::~PolygonEditor()
{
	delete ui;
}

void PolygonEditor::setUpChangeConnections()
{
	m_change_connections << connect(m_part,
					&PartPolygon::polygonChanged,
					this,
					&PolygonEditor::updateForm);
	m_change_connections << connect(m_part,
					&PartPolygon::closedChange,
					this,
					&PolygonEditor::updateForm);
	m_change_connections << connect(m_part,
					&PartPolygon::xChanged,
					this,
					&PolygonEditor::updateForm);
	m_change_connections << connect(m_part,
					&PartPolygon::yChanged,
					this,
					&PolygonEditor::updateForm);
}

void PolygonEditor::disconnectChangeConnections()
{
	for (QMetaObject::Connection c : m_change_connections) {
		disconnect(c);
	}
	 m_change_connections.clear();
}

/**
	@brief PolygonEditor::setPart
	@param new_part
	@return
*/
bool PolygonEditor::setPart(CustomElementPart *new_part)
{
	if (!new_part)
	{
		if (m_part) {
			disconnectChangeConnections();
			m_style->setPart(nullptr);
		}
		m_part = nullptr;
		return(true);
	}
	if (PartPolygon *part_polygon = dynamic_cast<PartPolygon *>(new_part))
	{
		if (m_part == part_polygon) {
			return true;
		}
		if (m_part) {
			disconnectChangeConnections();
		}
		m_part = part_polygon;
		updateForm();
		setUpChangeConnections();
		m_style->setPart(m_part);
		return(true);
	}
	return(false);
}

/**
	@brief PolygonEditor::currentPart
	@return the curent edited part
*/
CustomElementPart *PolygonEditor::currentPart() const
{
	return m_part;
}

QList<CustomElementPart*> PolygonEditor::currentParts() const
{
	return m_style->currentParts();
}

/**
	@brief PolygonEditor::updateForm
	Update the widget
*/
void PolygonEditor::updateForm()
{
	if (!m_part) {
		return;
	}

	ui->m_points_list_tree->clear();

	for(QPointF point : m_part->polygon())
	{
		point = m_part->mapToScene(point);
		QTreeWidgetItem *qtwi = new QTreeWidgetItem();
		qtwi->setData(0, Qt::EditRole, point.x());
		qtwi->setData(1, Qt::EditRole, point.y());
		qtwi -> setFlags(Qt::ItemIsEditable | Qt::ItemIsEnabled);
		ui->m_points_list_tree->addTopLevelItem(qtwi);
	}
	ui->m_close_polygon_cb->setChecked(m_part->isClosed());
	ui->m_remove_point_action->setEnabled(m_part->polygon().size()
					      > 2 ? true : false);
}

/**
	@brief PolygonEditor::pointsFromTree
	@return the point of polygon from the current value of the tree editor
	if part coordinate.
*/
QVector<QPointF> PolygonEditor::pointsFromTree()
{
	QVector<QPointF> points;
	if (!m_part) {
		return points;
	}
	
	for(int i = 0 ; i < ui->m_points_list_tree->topLevelItemCount() ; ++ i)
	{
		QTreeWidgetItem *qtwi = ui->m_points_list_tree->topLevelItem(i);
		bool x_convert_ok, y_convert_ok;
		qreal x = qtwi->data(0, Qt::EditRole).toReal(&x_convert_ok);
		qreal y = qtwi->data(1, Qt::EditRole).toReal(&y_convert_ok);
		if (x_convert_ok && y_convert_ok) {
			points << m_part->mapFromScene(QPointF(x, y));
		}
	}
	return(points);
}

bool PolygonEditor::eventFilter(QObject *watched, QEvent *event)
{
	if (watched == ui->m_points_list_tree &&
		event->type() == QEvent::FocusOut &&
		m_part)
	{
		m_part->resetAllHandlerColor();
		return true;
	}
	
	return false;
}

/**
	@brief PolygonEditor::on_m_close_polygon_cb_stateChanged
*/
void PolygonEditor::on_m_close_polygon_cb_stateChanged(int arg1)
{
	Q_UNUSED(arg1);
	
	if (!m_part) {
		return;
	}
	bool close = ui->m_close_polygon_cb->isChecked();
	if (close != m_part->isClosed())
	{
		QPropertyUndoCommand *undo =
				new QPropertyUndoCommand(m_part,
							 "closed",
							 m_part->property("closed"),
							 close);
		undo->setText(tr("Modifier un polygone"));
		undoStack().push(undo);
	}
}

/**
	@brief PolygonEditor::on_m_points_list_tree_itemChanged
	Update the polygon according to the current value of the tree editor
*/
void PolygonEditor::on_m_points_list_tree_itemChanged(QTreeWidgetItem *item,
						      int column)
{
	Q_UNUSED(item);
	Q_UNUSED(column);
	
	if (!m_part) {
		return;
	}
	
	QPolygonF points = pointsFromTree();
	if (points.count() < 2)
	{
		QET::QetMessageBox::warning(this,
					    tr("Erreur",
					       "message box title"),
					    tr("Le polygone doit comporter au moins deux points.",
					       "message box content"));
		return;
	}
	
	if (points != m_part->polygon())
	{
		QPropertyUndoCommand *undo = new QPropertyUndoCommand(
					m_part,
					"polygon",
					m_part->property("polygon"),
					points);
		undo->setText(tr("Modifier un polygone"));
		undoStack().push(undo);
	}
}

/**
	@brief PolygonEditor::on_m_points_list_tree_itemSelectionChanged
	Used to change the color of the current selected point.
*/
void PolygonEditor::on_m_points_list_tree_itemSelectionChanged()
{
	//Prevent when selection change but the widget ins't focused
	if (!ui->m_points_list_tree->hasFocus()) {
		return;
	}
	
	QTreeWidgetItem *qtwi = ui->m_points_list_tree->currentItem();
	if (!qtwi || !m_part) {
		return;
	}
	
	m_part->resetAllHandlerColor();
	int index = ui->m_points_list_tree->indexOfTopLevelItem(qtwi);
	
	//We need to check if index isn't out of range of polygon
	//this case can occur when user remove the last point of the polygon
	//with the context menu of the tree widget
	if(index >= 0 &&
	   index < m_part->polygon().size())
	{
		m_part->setHandlerColor(m_part->polygon().at(index), QColor(0, 255, 128));
	}
}

void PolygonEditor::on_m_add_point_action_triggered()
{
	QTreeWidgetItem *qtwi = ui->m_points_list_tree->currentItem();
	if (!qtwi || !m_part) {
		return;
	}
	
	int index = ui->m_points_list_tree->indexOfTopLevelItem(qtwi);
	QPolygonF new_polygon = m_part->polygon();
	
	//Special case when user add a point after the last point of the polygon
	if (index == m_part->polygon().size()-1)
	{
		QPointF p = m_part->polygon().last();
		p.rx()+=20;
		p.ry()+=20;
		new_polygon.append(p);
	}
	else
	{
		QPointF p = m_part->polygon().at(index) +
				m_part->polygon().at(index+1);
		p/=2;
		new_polygon.insert(index+1, p);
	}
	
	//Wrap the undo for avoid to merge the undo commands when user add several points.
	QUndoCommand *undo = new QUndoCommand(tr("Ajouter un point à un polygone"));
	new QPropertyUndoCommand(m_part, "polygon", m_part->polygon(), new_polygon, undo);
	elementScene()->undoStack().push(undo);
	
	m_part->resetAllHandlerColor();
	m_part->setHandlerColor(m_part->polygon().at(index+1), QColor(0, 255, 128));
}

void PolygonEditor::on_m_remove_point_action_triggered()
{
	QTreeWidgetItem *qtwi = ui->m_points_list_tree->currentItem();
	if (!qtwi || !m_part) {
		return;
	}
	
	QPolygonF new_polygon = m_part->polygon();
	new_polygon.removeAt(ui->m_points_list_tree->indexOfTopLevelItem(qtwi));
	
	//Wrap the undo for avoid to merge the undo commands when user remove several points.
	QUndoCommand *undo = new QUndoCommand(tr("Supprimer un point d'un polygone"));
	new QPropertyUndoCommand(m_part, "polygon", m_part->polygon(), new_polygon, undo);
	elementScene()->undoStack().push(undo);
}
