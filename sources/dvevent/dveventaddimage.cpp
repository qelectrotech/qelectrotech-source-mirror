/*
	Copyright 2006-2015 The QElectroTech Team
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
#include "diagramimageitem.h"
#include "diagramcommands.h"
#include "dveventaddimage.h"
#include "diagramview.h"
#include "diagram.h"
#include <QObject>
#include <QStandardPaths>

/**
 * @brief DVEventAddImage::DVEventAddImage
 * Defaultconstructor
 * @param dv : diagram view where operate this event
 */
DVEventAddImage::DVEventAddImage(DiagramView *dv) :
	DVEventInterface (dv),
	m_image (nullptr),
	m_is_added (false)
{
	openDialog();
}

/**
 * @brief DVEventAddImage::~DVEventAddImage
 */
DVEventAddImage::~DVEventAddImage() {
	if (m_running || m_abort) {
		if (m_is_added) m_diagram -> removeItem(m_image);
		delete m_image;
	}
	m_dv -> setContextMenuPolicy(Qt::DefaultContextMenu);
}

/**
 * @brief DVEventAddImage::mousePressEvent
 * Action when mouse is pressed
 * @param event : event of mouse pressed
 * @return : true if this event is managed, otherwise false
 */
bool DVEventAddImage::mousePressEvent(QMouseEvent *event) {
	if (m_image && event -> button() == Qt::LeftButton) {
		QPointF pos = m_dv -> mapToScene(event -> pos());
		pos.rx() -= m_image->boundingRect().width()/2;
		pos.ry() -= m_image->boundingRect().height()/2;
		m_diagram -> undoStack().push (new AddItemCommand<DiagramImageItem *>(m_image, m_diagram, pos));
		m_dv -> setContextMenuPolicy(Qt::DefaultContextMenu);
		m_running = false;
		return true;
	}

	if (m_image && event -> button() == Qt::RightButton) {
		m_image -> rotateBy(90);
		return true;
	}

	return false;
}

/**
 * @brief DVEventAddImage::mouseMoveEvent
 * Action when mouse move
 * @param event : event of mouse move
 * @return : true if this event is managed, otherwise false
 */
bool DVEventAddImage::mouseMoveEvent(QMouseEvent *event) {
	//@m_image isn't loaded, we return true and @m_running return
	//so the diagram view owner of this DVEevent will delete it.
	//if (!m_image) return true;

	if (!m_image || event -> buttons() != Qt::NoButton) return false;

	QPointF pos = m_dv -> mapToScene(event -> pos());

	if (!m_is_added) {
		m_dv -> setContextMenuPolicy(Qt::NoContextMenu);
		m_diagram -> addItem(m_image);
		m_is_added = true;
	}
	m_image -> setPos(pos - m_image -> boundingRect().center());
	return true;
}

/**
 * @brief DVEventAddImage::mouseDoubleClickEvent
 * This methode is only use to overwrite double click.
 * When double clic, image propertie dialog isn't open.
 * @param event : event of mouse double click
 * @return : true if this event is managed, otherwise false
 */
bool DVEventAddImage::mouseDoubleClickEvent(QMouseEvent *event) {
	Q_UNUSED(event);
	return true;
}

/**
 * @brief DVEventAddImage::wheelEvent
 * Action when mouse wheel is rotate
 * @param event : event of mouse wheel
 * @return : true if this event is managed, otherwise false
 */
bool DVEventAddImage::wheelEvent(QWheelEvent *event) {

	if (!m_is_added || !m_image || event -> modifiers() != Qt::CTRL) return false;

	qreal scaling = m_image->scale();
	event->delta() > 1? scaling += 0.01 : scaling -= 0.01;
	if (scaling>0.01 && scaling <= 2)
		m_image->setScale(scaling);

	return true;
}

/**
 * @brief DVEventAddImage::isNull
 * @return true if image can't be loaded, otherwise return false.
 */
bool DVEventAddImage::isNull() const {
	if (!m_image) return true;
	return false;
}

/**
 * @brief DVEventAddImage::openDialog
 * Open dialog for select the image to add.
 */
void DVEventAddImage::openDialog() {
	if (m_diagram -> isReadOnly()) return;

	//Open dialog for select image
	QString pathPictures = QStandardPaths::writableLocation(QStandardPaths::PicturesLocation);
	QString fileName = QFileDialog::getOpenFileName(m_dv, QObject::tr("Selectionner une image..."), pathPictures, QObject::tr("Image Files (*.png *.jpg *.bmp *.svg)"));

	if (fileName.isEmpty()) return;

	QImage image(fileName);
	if(image.isNull()) {
		QMessageBox::critical(m_dv, QObject::tr("Erreur"), QObject::tr("Impossible de charger l'image."));
		return;
	}

	m_image = new DiagramImageItem (QPixmap::fromImage(image));
	m_running = true;
}
