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
#include "elementstreeview.h"
#include "elementcollectionitem.h"
#include "elementslocation.h"
#include "elementfactory.h"
#include "qeticons.h"
#include "element.h"

#include <QDrag>

static int MAX_DND_PIXMAP_WIDTH = 500;
static int MAX_DND_PIXMAP_HEIGHT = 375;

/**
 * @brief ElementsTreeView::ElementsTreeView
 * @param parent
 */
ElementsTreeView::ElementsTreeView(QWidget *parent) :
	QTreeView(parent)
{}

/**
 * @brief ElementsTreeView::startDrag
 * Reimplemented from QTreeView
 * @param supportedActions
 */
void ElementsTreeView::startDrag(Qt::DropActions supportedActions)
{
	QModelIndex index = currentIndex();

	if (!index.isValid()) {
		QTreeView::startDrag(supportedActions);
		return;
	}

	ElementCollectionItem *eci = static_cast<ElementCollectionItem *>(index.internalPointer());

	if (!eci) {
		QTreeView::startDrag(supportedActions);
		return;
	}

	ElementsLocation loc (eci->collectionPath());
	if (loc.exist())
		startElementDrag(loc);
	else
		QTreeView::startDrag(supportedActions);
}

/**
 * @brief ElementsTreeView::startElementDrag
 * Build a QDrag according to the content of @location
 * @param location : location to use for create the content of the QDrag
 */
void ElementsTreeView::startElementDrag(const ElementsLocation &location)
{
	if (!location.exist())
		return;

	QDrag *drag = new QDrag(this);

	QString location_str = location.toString();
	QMimeData *mime_data = new QMimeData();
	mime_data->setText(location_str);

	if (location.isDirectory())
	{
		mime_data->setData("application/x-qet-category-uri", location_str.toLatin1());
		drag->setPixmap(QET::Icons::Folder.pixmap(22, 22));
	}
	else if (location.isElement())
	{
		mime_data->setData("application/x-qet-element-uri", location_str.toLatin1());

			//Build the element for set the pixmap of the QDrag
		int elmt_creation_state;
		Element *temp_elmt = ElementFactory::Instance()->createElement(location, 0, &elmt_creation_state);
		if (elmt_creation_state)
		{
			delete temp_elmt;
			return;
		}

		QPixmap elmt_pixmap(temp_elmt->pixmap());
		QPoint elmt_hotspot(temp_elmt->hotspot());

			//Adjust the size of the pixmap if he is too big
		QPoint elmt_pixmap_size(elmt_pixmap.width(), elmt_pixmap.height());
		if (elmt_pixmap.width() > MAX_DND_PIXMAP_WIDTH || elmt_pixmap.height() > MAX_DND_PIXMAP_HEIGHT)
		{
			elmt_pixmap = elmt_pixmap.scaled(MAX_DND_PIXMAP_WIDTH, MAX_DND_PIXMAP_HEIGHT, Qt::KeepAspectRatio);
			elmt_hotspot = QPoint(
				elmt_hotspot.x() * elmt_pixmap.width() / elmt_pixmap_size.x(),
				elmt_hotspot.y() * elmt_pixmap.height() / elmt_pixmap_size.y()
			);
		}

		drag->setPixmap(elmt_pixmap);
		drag->setHotSpot(elmt_hotspot);


		delete temp_elmt;
	}

	drag->setMimeData(mime_data);
	drag->exec(Qt::CopyAction);
}
