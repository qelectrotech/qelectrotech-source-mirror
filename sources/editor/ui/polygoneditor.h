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
#ifndef POLYGONEDITOR_H
#define POLYGONEDITOR_H

#include "elementitemeditor.h"

class StyleEditor;
class PartPolygon;
class QTreeWidgetItem;

namespace Ui {
	class PolygonEditor;
}

class PolygonEditor : public ElementItemEditor
{
	Q_OBJECT
	
	public:
	explicit PolygonEditor(QETElementEditor *editor, PartPolygon *part = nullptr, QWidget *parent = nullptr);
	~PolygonEditor() override;
	
	bool setPart(CustomElementPart *part) override;
	CustomElementPart *currentPart() const override;
	QList<CustomElementPart*> currentParts() const override;
	void updateForm() override;
	QVector<QPointF> pointsFromTree();
	bool eventFilter(QObject *watched, QEvent *event) override;
	
	private slots:
	void on_m_close_polygon_cb_stateChanged(int arg1);
	void on_m_points_list_tree_itemChanged(QTreeWidgetItem *item, int column);
	void on_m_points_list_tree_itemSelectionChanged();
	void on_m_add_point_action_triggered();
	void on_m_remove_point_action_triggered();
	
	private:
	/**
			@brief setUpChangeConnections
			Setup the connection from the line(s) to the widget,
			to update it when the line(s) are changed (moved ...)
	*/
	void setUpChangeConnections();
	void disconnectChangeConnections();
	
	private:
	Ui::PolygonEditor *ui;
	StyleEditor *m_style = nullptr;
	PartPolygon *m_part = nullptr;
	QList <QMetaObject::Connection> m_change_connections;
};

#endif // POLYGONEDITOR_H
