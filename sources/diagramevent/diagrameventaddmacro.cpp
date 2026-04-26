/*
 *   Copyright 2006-2026 The QElectroTech Team
 *   This file is part of QElectroTech.
 */
#include "diagrameventaddmacro.h"

#include "../diagram.h"
#include "../qetapp.h"
#include "../qetdiagrameditor.h"
#include "../qetproject.h"
#include "../ElementsCollection/xmlelementcollection.h"
#include "../nameslist.h"
#include "../diagramcommands.h"
#include "../diagramcontent.h"
#include <QFile>
#include <QDebug>
#include <QGraphicsSceneMouseEvent>
#include <QStatusBar>
#include <QPainter>

DiagramEventAddMacro::DiagramEventAddMacro(const ElementsLocation &location, Diagram *diagram, QPointF pos) :
DiagramEventInterface(diagram),
m_location(location),
m_preview_item(nullptr)
{
	if (loadMacro()) {
		init();

		QScopedPointer<QETProject> dummy_project(new QETProject());
		QDomElement root = m_macro_doc.documentElement();

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
		QDomElement diagram_node = root.firstChildElement("diagram_content").firstChildElement("diagram");

		if (!diagram_node.isNull()) {
			dummy_diagram->fromXml(diagram_node, QPointF(0, 0), false, nullptr);

			QRectF scene_rect = dummy_diagram->itemsBoundingRect();
			if (!scene_rect.isEmpty()) {
				QPixmap pixmap(scene_rect.toAlignedRect().size());
				pixmap.fill(Qt::transparent);
				QPainter painter(&pixmap);
				painter.setRenderHint(QPainter::Antialiasing);
				dummy_diagram->render(&painter, QRectF(QPointF(0,0), scene_rect.size()), scene_rect);

				m_preview_item = new QGraphicsPixmapItem(pixmap);
				m_preview_item->setOffset(scene_rect.topLeft());
			}
		}

		if (m_preview_item) {
			m_preview_item->setPos(Diagram::snapToGrid(pos));
			m_preview_item->setOpacity(0.6);
			m_diagram->addItem(m_preview_item);
			m_running = true;
		}

		if (!diagram->views().isEmpty()) {
			const auto qde = QETApp::diagramEditorAncestorOf(diagram->views().at(0));
			if (qde) {
				m_status_bar = qde->statusBar();
			}
		} else {
			m_status_bar.clear();
		}
	}
}

DiagramEventAddMacro::~DiagramEventAddMacro()
{
	if (m_preview_item) {
		m_diagram->removeItem(m_preview_item);
		delete m_preview_item;
	}

	if (m_status_bar) {
		m_status_bar->clearMessage();
	}

	for (auto view : m_diagram->views())
		view->setContextMenuPolicy(Qt::DefaultContextMenu);
}

void DiagramEventAddMacro::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
	if (m_preview_item) {
		const auto pos_{Diagram::snapToGrid(event->scenePos())};
		m_preview_item->setPos(pos_);

		if (m_status_bar) {
			m_status_bar->showMessage(QString("x %1 : y %2 (Makro-Anker)").arg(QString::number(pos_.x()), QString::number(pos_.y())));
		}
	}
	event->setAccepted(true);
}

void DiagramEventAddMacro::mousePressEvent(QGraphicsSceneMouseEvent *event) {
	event->setAccepted(true);
}

void DiagramEventAddMacro::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
	if (m_preview_item) {
		if (event->button() == Qt::RightButton) {
			m_diagram->removeItem(m_preview_item);
			delete m_preview_item;
			m_preview_item = nullptr;
			m_running = false;
			emit finish();
		}
		else if (event->button() == Qt::LeftButton) {
			addMacro(Diagram::snapToGrid(event->scenePos()));
		}
	}
	event->setAccepted(true);
}

void DiagramEventAddMacro::mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event)
{
	if (m_preview_item && (event->button() == Qt::LeftButton)) {
		m_diagram->removeItem(m_preview_item);
		delete m_preview_item;
		m_preview_item = nullptr;
		m_running = false;
		emit finish();
	}
	event->setAccepted(true);
}

void DiagramEventAddMacro::keyPressEvent(QKeyEvent *event)
{
	DiagramEventInterface::keyPressEvent(event);
}

void DiagramEventAddMacro::init()
{
	foreach(QGraphicsView *view, m_diagram->views())
		view->setContextMenuPolicy(Qt::NoContextMenu);
}

bool DiagramEventAddMacro::loadMacro()
{
	QFile file(m_location.fileSystemPath());
	if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
		qDebug() << "Error: Macro file could not be read:" << m_location.fileSystemPath();
		return false;
	}

	if (!m_macro_doc.setContent(&file)) {
		qDebug() << "Error: Invalid XML in macro.";
		return false;
	}

	QDomElement root = m_macro_doc.documentElement();
	if (root.tagName() != "qet_macro") return false;

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
						m_diagram->project()->embeddedElementCollection()->createDir(parent_path, part, empty_names);
					}
				}
				m_diagram->project()->embeddedElementCollection()->addElementDefinition(dir_path, file_name, definition);
			}
		}
	}

	QDomElement diagram_node = root.firstChildElement("diagram_content").firstChildElement("diagram");
	if (!diagram_node.isNull()) {
		QDomNodeList instances = diagram_node.elementsByTagName("element");
		for (int i = 0; i < instances.count(); ++i) {
			QDomElement inst = instances.at(i).toElement();
			QString type = inst.attribute("type");
			if (type.startsWith("macro://")) {
				inst.setAttribute("type", type.replace("macro://", "embed://"));
			}
		}
	}

	return true;
}

void DiagramEventAddMacro::addMacro(QPointF final_pos)
{
	QDomElement root = m_macro_doc.documentElement();
	QDomElement diagram_node = root.firstChildElement("diagram_content").firstChildElement("diagram");

	if (!diagram_node.isNull()) {
		QDomElement cloned_node = diagram_node.cloneNode(true).toElement();

		QPointF target_pos = final_pos;
		if (m_preview_item) {
			target_pos += m_preview_item->offset();
		}

		DiagramContent pasted_content;

		m_diagram->fromXml(cloned_node, target_pos, false, &pasted_content);
		m_diagram->refreshContents();

		m_diagram->undoStack().push(new PasteDiagramCommand(m_diagram, pasted_content));
	}
}
