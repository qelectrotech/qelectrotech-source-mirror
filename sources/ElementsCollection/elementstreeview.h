/*
	Copyright 2006-2016 The QElectroTech Team
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
#ifndef ELEMENTSTREEVIEW_H
#define ELEMENTSTREEVIEW_H

#include <QTreeView>

class ElementsLocation;

/**
 * @brief The ElementsTreeView class
 * This class just reimplement startDrag from QTreeView, for set a custom pixmap.
 * This class must be used when the tree view have an ElementsCollectionModel as model.
 * The pixmap used is the pixmap of the dragged element or a directory pixmap.
 */
class ElementsTreeView : public QTreeView
{
	public:
		ElementsTreeView(QWidget *parent = nullptr);

	protected:
		virtual void startDrag(Qt::DropActions supportedActions);
		virtual void startElementDrag(const ElementsLocation &location);
};

#endif // ELEMENTSTREEVIEW_H
