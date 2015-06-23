/*
	Copyright 2006-2013 QElectroTech Team
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
#include "diagram.h"
#include "PropertiesEditor/propertieseditordialog.h"
#include "imagepropertieswidget.h"

/**
 * @brief DiagramImageItem::DiagramImageItem
 * Constructor without pixmap
 * @param parent_item the parent graphics item
 */
DiagramImageItem::DiagramImageItem(QetGraphicsItem *parent_item):
	QetGraphicsItem(parent_item)
{
	setFlags(QGraphicsItem::ItemIsSelectable|QGraphicsItem::ItemIsMovable|QGraphicsItem::ItemSendsGeometryChanges);
}

/**
 * @brief DiagramImageItem::DiagramImageItem
 * Constructor with pixmap
 * @param pixmap the pixmap to be draw
 * @param parent_item the parent graphic item
 */
DiagramImageItem::DiagramImageItem(const QPixmap &pixmap, QetGraphicsItem *parent_item):
	QetGraphicsItem(parent_item),
	pixmap_(pixmap)
{
	setTransformOriginPoint(boundingRect().center());
	setFlags(QGraphicsItem::ItemIsSelectable|QGraphicsItem::ItemIsMovable|QGraphicsItem::ItemSendsGeometryChanges);
}

/**
 * @brief DiagramImageItem::~DiagramImageItem
 * Destructor
 */
DiagramImageItem::~DiagramImageItem() {
}

/**
 * @brief DiagramImageItem::paint
 * Draw the pixmap.
 * @param painter the Qpainter to use for draw the pixmap
 * @param option the style option
 * @param widget the QWidget where we draw the pixmap
 */
void DiagramImageItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) {
	painter -> drawPixmap(pixmap_.rect(),pixmap_);

	Q_UNUSED(option); Q_UNUSED(widget);

	if (isSelected()) {
		painter -> save();
		// Annulation des renderhints
		painter -> setRenderHint(QPainter::Antialiasing,          false);
		painter -> setRenderHint(QPainter::TextAntialiasing,      false);
		painter -> setRenderHint(QPainter::SmoothPixmapTransform, false);
        // Dessin du cadre de selection en noir à partir du boundingrect
		QPen t(Qt::black);
		t.setStyle(Qt::DashLine);
		painter -> setPen(t);
		painter -> drawRect(boundingRect());
		painter -> restore();
	}
}

/**
 * @brief DiagramImageItem::editProperty
 * Open the approriate dialog to edit this image
 */
void DiagramImageItem::editProperty()
{
	if (diagram() -> isReadOnly()) return;
	PropertiesEditorDialog dialog(new ImagePropertiesWidget(this), QApplication::activeWindow());
	dialog.exec();
}

/**
 * @brief DiagramImageItem::setPixmap
 * Set the new pixmap to be draw
 * @param pixmap the new pixmap
 */
void DiagramImageItem::setPixmap(const QPixmap &pixmap) {
	pixmap_ = pixmap;
	setTransformOriginPoint(boundingRect().center());
}

/**
 * @brief DiagramImageItem::boundingRect
 * the outer bounds of the item as a rectangle,
 * if no pixmap are set, return a default QRectF
 * @return a QRectF represent the bounding rectangle
 */
QRectF DiagramImageItem::boundingRect() const {
	if (!pixmap_.isNull()) {
		return (QRectF(pixmap_.rect()));
	} else {
		QRectF bound;
		return (bound);
	}
}

/**
 * @brief DiagramImageItem::name
 * @return the generic name of this item (picture)
 */
QString DiagramImageItem::name() const {
	return tr("une image");
}

/**
	Load the image from this xml element
	@param e xml element that define an image
*/
bool DiagramImageItem::fromXml(const QDomElement &e) {
	if (e.tagName() != "image") return (false);
	QDomNode image_node = e.firstChild();
	if (!image_node.isText()) return (false);

	//load xml image to QByteArray
	QByteArray array;
	array = QByteArray::fromBase64(e.text().toLatin1());

	//Set QPixmap from the @array
	QPixmap pixmap;
	pixmap.loadFromData(array);
	setPixmap(pixmap);

	setScale(e.attribute("size").toDouble());
	applyRotation(e.attribute("rotation").toDouble());
		//We directly call setPos from QGraphicsObject, because QetGraphicsItem will snap to grid
	QGraphicsObject::setPos(e.attribute("x").toDouble(), e.attribute("y").toDouble());
	is_movable_ = (e.attribute("is_movable").toInt());

	return (true);
}

/**
	@param document Le document XML a utiliser
	@return L'element XML representant l'image
*/
QDomElement DiagramImageItem::toXml(QDomDocument &document) const {
	QDomElement result = document.createElement("image");
	//write some attribute
	result.setAttribute("x", QString("%1").arg(pos().x()));
	result.setAttribute("y", QString("%1").arg(pos().y()));
	result.setAttribute("rotation", QString("%1").arg(rotation()));
	result.setAttribute("size", QString("%1").arg(scale()));
	result.setAttribute("is_movable", bool(is_movable_));

	//write the pixmap in the xml element after he was been transformed to base64
	QByteArray array;
	QBuffer buffer(&array);
	buffer.open(QIODevice::ReadWrite);
	pixmap_.save(&buffer, "PNG");
	QDomText base64 = document.createTextNode(array.toBase64());
	result.appendChild(base64);

	return(result);
}
