/*
	Copyright 2006-2025 The QElectroTech Team
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

#include "../factory/elementfactory.h"
#include "../qetgraphicsitem/element.h"
#include "../qeticons.h"
#include "elementcollectionitem.h"
#include "elementslocation.h"

#include <QDrag>
#include <QStandardItemModel>

static int MAX_DND_PIXMAP_WIDTH = 500;
static int MAX_DND_PIXMAP_HEIGHT = 375;

/**
	@brief ElementsTreeView::ElementsTreeView
	@param parent
*/
ElementsTreeView::ElementsTreeView(QWidget *parent) :
	QTreeView(parent)
{
	// force du noir sur une alternance de blanc (comme le schema) et de gris
	// clair, avec du blanc sur bleu pas trop fonce pour la selection
	QPalette qp = palette();
	qp.setColor(QPalette::Text,            Qt::black);
	qp.setColor(QPalette::Base,            Qt::white);
	qp.setColor(QPalette::AlternateBase,   QColor("#e8e8e8"));
	qp.setColor(QPalette::Highlight,       QColor("#678db2"));
	qp.setColor(QPalette::HighlightedText, Qt::black);
	setPalette(qp);
}

/**
	@brief ElementsTreeView::startDrag
	Reimplemented from QTreeView
	@param supportedActions
*/
void ElementsTreeView::startDrag(Qt::DropActions supportedActions)
{
	QModelIndex index = currentIndex();

	if (!index.isValid()) {
		QTreeView::startDrag(supportedActions);
		return;
	}

	if (QStandardItemModel *qsim = static_cast<QStandardItemModel *>(model())) {
		if (ElementCollectionItem *eci = static_cast<ElementCollectionItem *>(qsim->itemFromIndex(index))) {
			ElementsLocation loc (eci->collectionPath());
			if (loc.exist()) {
				startElementDrag(loc);
				return;
			}
		}
	}
	QTreeView::startDrag(supportedActions);
}

/**
	@brief ElementsTreeView::startElementDrag
	Build a QDrag according to the content of location
	@param location : location to use for create the content of the QDrag
*/
void ElementsTreeView::startElementDrag(const ElementsLocation &location)
{
	if (! location.exist()) return;

	QScopedPointer<QDrag> drag(new QDrag(this));

	QString location_str = location.toString();
	QMimeData *mime_data = new QMimeData();
	mime_data->setText(location_str);

	if (location.isDirectory())
	{
		mime_data->setData("application/x-qet-category-uri",
				   location_str.toLatin1());
		drag->setPixmap(QET::Icons::Folder.pixmap(22, 22));
	}
	else if (location.isElement())
	{
		mime_data->setData("application/x-qet-element-uri",
				   location_str.toLatin1());

			//Build the element for set the pixmap of the QDrag
		int elmt_creation_state;
		QScopedPointer<Element> temp_elmt(
		    ElementFactory::Instance()->createElement(
			location,
			nullptr,
			&elmt_creation_state));
		if (elmt_creation_state) { return; }

		QPixmap elmt_pixmap(temp_elmt->pixmap());
		QPoint elmt_hotspot(temp_elmt->hotspot());

			//Adjust the size of the pixmap if he is too big
		QPoint elmt_pixmap_size(elmt_pixmap.width(),
					elmt_pixmap.height());
		if (elmt_pixmap.width()
				> MAX_DND_PIXMAP_WIDTH
				|| elmt_pixmap.height()
				> MAX_DND_PIXMAP_HEIGHT)
		{
			elmt_pixmap = elmt_pixmap.scaled(
						MAX_DND_PIXMAP_WIDTH,
						MAX_DND_PIXMAP_HEIGHT,
						Qt::KeepAspectRatio);
			elmt_hotspot = QPoint(
				elmt_hotspot.x() * elmt_pixmap.width() / elmt_pixmap_size.x(),
				elmt_hotspot.y() * elmt_pixmap.height() / elmt_pixmap_size.y()
			);
		}

		drag->setPixmap(elmt_pixmap);
		drag->setHotSpot(elmt_hotspot);
	}

	drag->setMimeData(mime_data);
	drag->exec(Qt::CopyAction);
}
