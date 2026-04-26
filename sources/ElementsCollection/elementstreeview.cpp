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
#include "elementstreeview.h"

#include "../factory/elementfactory.h"
#include "../qetgraphicsitem/element.h"
#include "../qeticons.h"
#include "elementcollectionitem.h"
#include "elementslocation.h"
#include "../qetproject.h"
#include "../diagram.h"
#include "xmlelementcollection.h"
#include "../NameList/nameslist.h"
#include <QPainter>
#include <QScopedPointer>
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

#if QT_VERSION < QT_VERSION_CHECK(6, 2, 0)
	QDrag* drag = new QDrag(this);
#else
	QScopedPointer<QDrag> drag(new QDrag(this));
#endif

	QString location_str = location.toString();
	QMimeData *mime_data = new QMimeData();
	mime_data->setText(location_str);

	if (location.isDirectory())
	{
		mime_data->setData("application/x-qet-category-uri",
						   location_str.toLatin1());
		drag->setPixmap(QET::Icons::Folder.pixmap(22, 22));
	}
	else if (location.fileName().endsWith(".qetmak"))
	{
		mime_data->setData("application/x-qet-element-uri", location_str.toLatin1());

		QPixmap macro_pixmap;

		// --- MINI-RENDERER FÜR DAS MAKRO-VORSCHAUBILD ---
		QFile file(location.fileSystemPath());
		if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
			QDomDocument macro_doc;
			if (macro_doc.setContent(&file)) {
				QDomElement root = macro_doc.documentElement();
				if (root.tagName() == "qet_macro") {

					// 1. Unsichtbares Dummy-Projekt erstellen
					QScopedPointer<QETProject> dummy_project(new QETProject());

					// 2. Bauteile in das Dummy-Projekt laden (wie beim echten Drop)
					QDomElement collection_node = root.firstChildElement("collection");
					if (!collection_node.isNull()) {
						QDomNodeList elements = collection_node.elementsByTagName("element");
						for (int i = 0; i < elements.count(); ++i) {
							QDomElement elmt_node = elements.at(i).toElement();
							QString path = elmt_node.attribute("path");
							QDomElement definition = elmt_node.firstChildElement("definition");
							if (!path.isEmpty() && !definition.isNull()) {
								int last_slash = path.lastIndexOf('/');
								QString dir_path = (last_slash != -1) ? path.left(last_slash) : "";
								QString file_name = (last_slash != -1) ? path.mid(last_slash + 1) : path;

								if (!dir_path.isEmpty()) {
									#if QT_VERSION < QT_VERSION_CHECK(5, 14, 0)
									QStringList parts = dir_path.split('/', QString::SkipEmptyParts);
									#else
									QStringList parts = dir_path.split('/', Qt::SkipEmptyParts);
									#endif
									QString current_path = "";
									for (const QString &part : parts) {
										QString parent_path = current_path;
										if (!current_path.isEmpty()) current_path += "/";
										current_path += part;
										if (current_path == "import") continue;
										NamesList empty_names;
										dummy_project->embeddedElementCollection()->createDir(parent_path, part, empty_names);
									}
								}
								dummy_project->embeddedElementCollection()->addElementDefinition(dir_path, file_name, definition);
							}
						}
					}

					Diagram *dummy_diagram = dummy_project->addNewDiagram();

					// 4. Makro auf dem unsichtbaren Blatt zeichnen
					QDomElement diagram_content_node = root.firstChildElement("diagram_content");
					QDomElement diagram_node = diagram_content_node.firstChildElement("diagram");
					if (!diagram_node.isNull()) {
						QDomNodeList instances = diagram_node.elementsByTagName("element");
						for (int i = 0; i < instances.count(); ++i) {
							QDomElement inst = instances.at(i).toElement();
							QString type = inst.attribute("type");
							if (type.startsWith("macro://")) {
								inst.setAttribute("type", type.replace("macro://", "embed://"));
							}
						}

						dummy_diagram->fromXml(diagram_node, QPointF(0, 0), false, nullptr);
						dummy_diagram->clearSelection();

						// 5. "Screenshot" (Pixmap) von den gezeichneten Elementen machen
						QRectF scene_rect = dummy_diagram->itemsBoundingRect();
						if (!scene_rect.isEmpty()) {
							scene_rect.adjust(-5, -5, 5, 5); // Kleiner Rand
							macro_pixmap = QPixmap(scene_rect.size().toSize());
							macro_pixmap.fill(Qt::transparent); // Transparenter Hintergrund

							QPainter painter(&macro_pixmap);
							painter.setRenderHint(QPainter::Antialiasing);
							dummy_diagram->render(&painter, macro_pixmap.rect(), scene_rect);
						}
					}
				}
			}
		}

		if (macro_pixmap.isNull()) {
			macro_pixmap = QET::Icons::Project.pixmap(32, 32);
		}

		// Bild verkleinern, falls das Makro gigantisch groß ist
		if (macro_pixmap.width() > MAX_DND_PIXMAP_WIDTH || macro_pixmap.height() > MAX_DND_PIXMAP_HEIGHT) {
			macro_pixmap = macro_pixmap.scaled(MAX_DND_PIXMAP_WIDTH, MAX_DND_PIXMAP_HEIGHT, Qt::KeepAspectRatio, Qt::SmoothTransformation);
		}

		drag->setPixmap(macro_pixmap);
		// Bild zentriert an die Maus hängen
		drag->setHotSpot(QPoint(macro_pixmap.width() / 2, macro_pixmap.height() / 2));
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
