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

#ifndef CABINETLAYOUTSOURCEWIDGET_H
#define CABINETLAYOUTSOURCEWIDGET_H

#include <QWidget>

class QETProject;
class QTreeView;
class CabinetLayoutSourceModel;
class Diagram;

/**
	@brief The CabinetLayoutSourceWidget class
	Dock-widget content: a tree of the elements placed in the current
	project, grouped by folio, that can be dragged onto a cabinet
	layout folio ("disposition des armoires") to create a
	scale-accurate box for that device.
	@see CabinetLayoutSourceModel
*/
class CabinetLayoutSourceWidget : public QWidget
{
	Q_OBJECT

	public:
	explicit CabinetLayoutSourceWidget(QWidget *parent = nullptr);
	~CabinetLayoutSourceWidget() override = default;

	void setProject(QETProject *project);

	/**
		@brief setActiveDiagram
		Forwarded to the underlying CabinetLayoutSourceModel so it
		can grey out devices already placed for the active folio's
		view (front/side). Call whenever the active folio changes.
	*/
	void setActiveDiagram(Diagram *diagram);

private:
	QTreeView *m_tree_view;
	CabinetLayoutSourceModel *m_model = nullptr;
};

#endif // CABINETLAYOUTSOURCEWIDGET_H