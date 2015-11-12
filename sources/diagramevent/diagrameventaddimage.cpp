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

#include "diagrameventaddimage.h"
#include "diagram.h"
#include "diagramimageitem.h"
#include "diagramcommands.h"

/**
 * @brief DiagramEventAddImage::DiagramEventAddImage
 * Default constructor
 * @param diagram : diagram where operate this event
 */
DiagramEventAddImage::DiagramEventAddImage(Diagram *diagram) :
    DiagramEventInterface(diagram),
    m_image (nullptr),
    m_is_added (false)
{
    openDialog();
}

/**
 * @brief DiagramEventAddImage::~DiagramEventAddImage
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
 * @brief DiagramEventAddImage::mousePressEvent
 * Action when mouse is pressed
 * @param event : event of mouse pressed
 * @return : true if this event is handled, otherwise false
 */
bool DiagramEventAddImage::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    if (m_image && event -> button() == Qt::LeftButton)
    {
        QPointF pos = event->scenePos();
        pos.rx() -= m_image->boundingRect().width()/2;
        pos.ry() -= m_image->boundingRect().height()/2;
        m_diagram -> undoStack().push (new AddItemCommand<DiagramImageItem *>(m_image, m_diagram, pos));

        foreach (QGraphicsView *view, m_diagram->views())
            view->setContextMenuPolicy((Qt::DefaultContextMenu));

        m_running = false;
        emit finish();
        return true;
    }

    if (m_image && event -> button() == Qt::RightButton)
    {
            m_image -> rotateBy(90);
            return true;
    }

    return false;
}

/**
 * @brief DiagramEventAddImage::mouseMoveEvent
 * Action when mouse move
 * @param event : event of mouse move
 * @return : true if this event is handled, otherwise false
 */
bool DiagramEventAddImage::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
    if (!m_image || event -> buttons() != Qt::NoButton) return false;

    QPointF pos = event->scenePos();

    if (!m_is_added)
    {
        foreach (QGraphicsView *view, m_diagram->views())
            view->setContextMenuPolicy((Qt::NoContextMenu));

        m_diagram -> addItem(m_image);
        m_is_added = true;
    }

    m_image -> setPos(pos - m_image -> boundingRect().center());
    return true;
}

/**
 * @brief DiagramEventAddImage::mouseDoubleClickEvent
 * This method is only use to overwrite double click.
 * When double click, image propertie dialog isn't open.
 * @param event : event of mouse double click.
 * @return : true if this event is handled, otherwise false
 */
bool DiagramEventAddImage::mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event)
{
    Q_UNUSED(event);
    return true;
}

/**
 * @brief DiagramEventAddImage::wheelEvent
 * Action when mouse wheel is rotate
 * @param event: evet of mouse wheel
 * @return : true if this event is handled, otherwise false
 */
bool DiagramEventAddImage::wheelEvent(QGraphicsSceneWheelEvent *event)
{
    if (!m_is_added || !m_image || event -> modifiers() != Qt::CTRL) return false;

    qreal scaling = m_image->scale();
    event->delta() > 1? scaling += 0.01 : scaling -= 0.01;
    if (scaling>0.01 && scaling <= 2)
            m_image->setScale(scaling);

    return true;
}

/**
 * @brief DiagramEventAddImage::isNull
 * @return : true if image can't be loaded, otherwise return false.
 */
bool DiagramEventAddImage::isNull() const
{
    if (!m_image) return true;
    return false;
}

/**
 * @brief DiagramEventAddImage::openDialog
 * Open dialog for select the image to add.
 */
void DiagramEventAddImage::openDialog()
{
    if (m_diagram -> isReadOnly()) return;

        //Open dialog for select image
    QString pathPictures = QStandardPaths::writableLocation(QStandardPaths::PicturesLocation);
    QString fileName = QFileDialog::getOpenFileName(m_diagram->views().isEmpty()? nullptr : m_diagram->views().first(), QObject::tr("Selectionner une image..."), pathPictures, QObject::tr("Image Files (*.png *.jpg *.bmp *.svg)"));

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
