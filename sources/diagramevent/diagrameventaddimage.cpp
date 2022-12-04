/*
	Copyright 2006-2021 The QElectroTech Team
	This file is part of QElectroTech.

	QElectroTech is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 2 of the License, or
	(at your option) any later version.

	QElectroTech is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with QElectroTech. If not, see <http://www.gnu.org/licenses/>.
*/

#include "diagrameventaddimage.h"

#include "../diagram.h"
#include "../undocommand/addgraphicsobjectcommand.h"
#include "../qetgraphicsitem/diagramimageitem.h"

/**
	@brief DiagramEventAddImage::DiagramEventAddImage
	Default constructor
	@param diagram : diagram where operate this event
*/
DiagramEventAddImage::DiagramEventAddImage(Diagram *diagram) :
	DiagramEventInterface(diagram),
	m_image (nullptr),
	m_is_added (false)
{
	openDialog();
}

/**
	@brief DiagramEventAddImage::~DiagramEventAddImage
*/
DiagramEventAddImage::~DiagramEventAddImage()
{
	if (m_running || m_abort)
	{
		if (m_is_added) m_diagram->removeItem(m_image);
		delete m_image;
	}

	foreach (QGraphicsView *view, m_diagram->views())
		view->setContextMenuPolicy((Qt::DefaultContextMenu));
}

/**
	@brief DiagramEventAddImage::mousePressEvent
	Action when mouse is pressed
	@param event : event of mouse pressed
*/
void DiagramEventAddImage::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
	if (m_image && event -> button() == Qt::LeftButton)
	{
		QPointF pos = event->scenePos();
		pos.rx() -= m_image->boundingRect().width()/2;
		pos.ry() -= m_image->boundingRect().height()/2;
		m_diagram -> undoStack().push (new AddGraphicsObjectCommand(m_image, m_diagram, pos));
		
		for (QGraphicsView *view : m_diagram->views()) {
			view->setContextMenuPolicy((Qt::DefaultContextMenu));
		}
		
		m_running = false;
		emit finish();
		event->setAccepted(true);
	}
	else if (m_image && event -> button() == Qt::RightButton)
	{
		m_image->setRotation(m_image->rotation() + 90);
		event->setAccepted(true);
	}
}

/**
	@brief DiagramEventAddImage::mouseMoveEvent
	Action when mouse move
	@param event : event of mouse move
*/
void DiagramEventAddImage::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
	if (!m_image || event->buttons() != Qt::NoButton) {
		return;
	}
	
	QPointF pos = event->scenePos();
	
	if (!m_is_added)
	{
		for (QGraphicsView *view : m_diagram->views()) {
			view->setContextMenuPolicy((Qt::NoContextMenu));
		}
		
		m_diagram->addItem(m_image);
		m_is_added = true;
	}
	
	m_image->setPos(pos - m_image->boundingRect().center());
	event->setAccepted(true);
}

/**
	@brief DiagramEventAddImage::mouseDoubleClickEvent
	This method is used only to overwrite double click.
	When double click, image properties dialog isn't open.
	@param event : event of mouse double click.
*/
void DiagramEventAddImage::mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event) {
	event->setAccepted(true);
}

/**
	@brief DiagramEventAddImage::wheelEvent
	Action when mouse wheel is rotated
	@param event: evet of mouse wheel
*/
void DiagramEventAddImage::wheelEvent(QGraphicsSceneWheelEvent *event)
{
	if (!m_is_added || !m_image || event -> modifiers() != Qt::CTRL) {
		return;
	}
	
	qreal scaling = m_image->scale();
	event->delta() > 1? scaling += 0.01 : scaling -= 0.01;
	if (scaling>0.01 && scaling <= 2) {
		m_image->setScale(scaling);
	}
	
	event->setAccepted(true);
}

/**
	@brief DiagramEventAddImage::isNull
	@return : true if image can't be loaded, otherwise return false.
*/
bool DiagramEventAddImage::isNull() const
{
	if (!m_image) return true;
	return false;
}

/**
	@brief DiagramEventAddImage::openDialog
	Open dialog to select the image to add.
*/
void DiagramEventAddImage::openDialog()
{
	if (m_diagram -> isReadOnly()) return;
	
	//Open dialog to select image
	QString pathPictures = QStandardPaths::writableLocation(QStandardPaths::PicturesLocation);
	QString fileName = QFileDialog::getOpenFileName(m_diagram->views().isEmpty()? nullptr : m_diagram->views().first(), QObject::tr("Selectionner une image..."), pathPictures, QObject::tr("Image Files (*.png *.jpg  *.jpeg *.bmp *.svg)"));
	
	if (fileName.isEmpty()) return;
	
	QImage image(fileName);
	if(image.isNull())
	{
		QMessageBox::critical(m_diagram->views().isEmpty()? nullptr : m_diagram->views().first(), QObject::tr("Erreur"), QObject::tr("Impossible de charger l'image."));
		return;
	}
	
	m_image = new DiagramImageItem (QPixmap::fromImage(image));
	m_running = true;
}
