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
#include "diagramcommands.h"

/**
 * @brief DiagramImageItem::DiagramImageItem
 * Constructor without pixmap
 * @param parent_item the parent graphics item
 */
DiagramImageItem::DiagramImageItem(QetGraphicsItem *parent_item):
	QetGraphicsItem(parent_item)
{
	setFlags(QGraphicsItem::ItemIsSelectable|QGraphicsItem::ItemIsMovable);
#if QT_VERSION >= 0x040600
	setFlag(QGraphicsItem::ItemSendsGeometryChanges, true);
#endif
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
	setFlags(QGraphicsItem::ItemIsSelectable|QGraphicsItem::ItemIsMovable);
#if QT_VERSION >= 0x040600
	setFlag(QGraphicsItem::ItemSendsGeometryChanges, true);
#endif
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
 * @brief DiagramImageItem::setScale
 * @param scale the value of @scale must be betwen 1 and 200
 */
void DiagramImageItem::PreviewScale(int scale) {
	if (scale >= 1 && scale <= 200) {
		qreal new_scale = scale;
		new_scale /= 100;
		setScale(new_scale);
	}
}

/**
 * @brief Edit the image with ....
 */
void DiagramImageItem::editProperty() {
	if (diagram() -> isReadOnly()) return;
	//the range for scale image and divisor factor
	int min_range = 1;
	int max_range = 200;
	int factor_range = 100;

	//the dialog
	QDialog property_dialog(diagram()->views().at(0));
	property_dialog.setWindowTitle(tr("\311diter les propri\351t\351s d'une image", "window title"));
	//the main layout
	QVBoxLayout dialog_layout(&property_dialog);

	//GroupBox for resizer image
	QGroupBox resize_groupe(tr("Dimension de l'image", "image size"));
	dialog_layout.addWidget(&resize_groupe);
	QHBoxLayout resize_layout(&resize_groupe);

		//slider
	QSlider slider(Qt::Horizontal, &property_dialog);
	slider.setRange(min_range, max_range);
	qreal scale_= scale();
	slider.setValue(scale_*factor_range);
		//spinbox
	QSpinBox spin_box(&property_dialog);
	spin_box.setRange(min_range, max_range);
	spin_box.setValue(scale_*factor_range);
	spin_box.setSuffix(" %");
		//synchro slider with spinbox
	connect(&slider, SIGNAL(valueChanged(int)), &spin_box, SLOT(setValue(int)));
	connect(&slider, SIGNAL(valueChanged(int)), this, SLOT(PreviewScale(int)));
	connect(&spin_box, SIGNAL(valueChanged(int)), &slider, SLOT(setValue(int)));
		//add slider and spinbox to layout
	resize_layout.addWidget(&slider);
	resize_layout.addWidget(&spin_box);
		//check box for disable move
	QCheckBox cb(tr("Verrouiller la position"), &property_dialog);
	cb.setChecked(!is_movable_);
	dialog_layout.addWidget(&cb);

	//dialog button, box
	QDialogButtonBox dbb(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
	dialog_layout.addWidget(&dbb);
	connect(&dbb, SIGNAL(accepted()), &property_dialog, SLOT(accept()));
	connect(&dbb, SIGNAL(rejected()), &property_dialog, SLOT(reject()));

	//dialog is accepted...
	if (property_dialog.exec() == QDialog::Accepted) {
		cb.isChecked() ? is_movable_=false : is_movable_=true;
		qreal new_scale = slider.value();
		new_scale /= factor_range;
		if (scale_ != new_scale) diagram()->undoStack().push(new ItemResizerCommand(this, scale_, new_scale, tr("une image")));
	}
	//...or not
	else setScale(scale_);
	return;
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
	Load the image from this xml element
	@param e xml element that define an image
*/
bool DiagramImageItem::fromXml(const QDomElement &e) {
	if (e.tagName() != "image") return (false);
	QDomNode image_node = e.firstChild();
	if (!image_node.isText()) return (false);

	//load xml image to QByteArray
	QByteArray array;
	array = QByteArray::fromBase64(e.text().toAscii());

	//Set QPixmap from the @array
	QPixmap pixmap;
	pixmap.loadFromData(array);
	setPixmap(pixmap);

	setScale(e.attribute("size").toDouble());
	applyRotation(e.attribute("rotation").toDouble());
	setPos(e.attribute("x").toDouble(), e.attribute("y").toDouble());

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

	//write the pixmap in the xml element after he was been transformed to base64
	QByteArray array;
	QBuffer buffer(&array);
	buffer.open(QIODevice::ReadWrite);
	pixmap_.save(&buffer, "PNG");
	QDomText base64 = document.createTextNode(array.toBase64());
	result.appendChild(base64);

	return(result);
}
