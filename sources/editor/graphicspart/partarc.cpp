/*
	Copyright 2006-2014 The QElectroTech Team
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
#include "partarc.h"

/**
	Constructeur
	@param editor L'editeur d'element concerne
	@param parent Le QGraphicsItem parent de cet arc
	@param scene La scene sur laquelle figure cet arc
*/
PartArc::PartArc(QETElementEditor *editor, QGraphicsItem *parent, QGraphicsScene *scene) :
	CustomElementGraphicPart(editor),
	QGraphicsEllipseItem(parent, scene),
	_angle(-90),
	start_angle(0)
{
	setFlags(QGraphicsItem::ItemIsSelectable);
#if QT_VERSION >= 0x040600
	setFlag(QGraphicsItem::ItemSendsGeometryChanges, true);
#endif
	setAcceptedMouseButtons(Qt::LeftButton);
}

/// Destructeur
PartArc::~PartArc() {
}

/**
	Dessine l'arc de cercle
	@param painter QPainter a utiliser pour rendre le dessin
	@param options Options pour affiner le rendu
	@param widget Widget sur lequel le rendu est effectue
*/
void PartArc::paint(QPainter *painter, const QStyleOptionGraphicsItem *options, QWidget *widget) {
	Q_UNUSED(widget);
	applyStylesToQPainter(*painter);
	// enleve systematiquement la couleur de fond
	painter -> setBrush(Qt::NoBrush);
	QPen t = painter -> pen();
	t.setCosmetic(options && options -> levelOfDetail < 1.0);
	painter -> setPen(t);
	
	if (isSelected()) {
		// dessine l'ellipse en noir
		painter -> drawEllipse(rect());
		
		// dessine l'arc en rouge
		t.setColor(Qt::red);
		painter -> setPen(t);
	}
	
	painter -> drawArc(rect(), start_angle * 16, _angle * 16);
	if (isSelected()) {
		// dessine la croix au centre de l'ellipse
		painter -> setRenderHint(QPainter::Antialiasing, false);
		painter -> setPen((painter -> brush().color() == QColor(Qt::black) && painter -> brush().isOpaque()) ? Qt::yellow : Qt::blue);
		QPointF center = rect().center();
		painter -> drawLine(QLineF(center.x() - 2.0, center.y(), center.x() + 2.0, center.y()));
		painter -> drawLine(QLineF(center.x(), center.y() - 2.0, center.x(), center.y() + 2.0));
	}
}

/**
	Exporte l'arc de cercle en XML
	@param xml_document Document XML a utiliser pour creer l'element XML
	@return un element XML decrivant l'arc de cercle
*/
const QDomElement PartArc::toXml(QDomDocument &xml_document) const {
	QDomElement xml_element = xml_document.createElement("arc");
	QPointF top_left(sceneTopLeft());
	xml_element.setAttribute("x", QString("%1").arg(top_left.x()));
	xml_element.setAttribute("y", QString("%1").arg(top_left.y()));
	xml_element.setAttribute("width",  QString("%1").arg(rect().width()));
	xml_element.setAttribute("height", QString("%1").arg(rect().height()));
	xml_element.setAttribute("start", QString("%1").arg(start_angle));
	xml_element.setAttribute("angle", QString("%1").arg(_angle));
	stylesToXml(xml_element);
	return(xml_element);
}

/**
	Importe les proprietes d'un arc de cercle depuis un element XML
	@param qde Element XML a lire
*/
void PartArc::fromXml(const QDomElement &qde) {
	stylesFromXml(qde);
	setRect(
		QRectF(
			mapFromScene(
				qde.attribute("x", "0").toDouble(),
				qde.attribute("y", "0").toDouble()
			),
			QSizeF(
				qde.attribute("width",  "0").toDouble(),
				qde.attribute("height", "0").toDouble()
			)
		)
	);
	setStartAngle(qde.attribute("start", "0").toInt());
	setAngle(qde.attribute("angle", "-90").toInt());
}

/**
	@return le coin superieur gauche du rectangle dans lequel s'inscrit
	l'ellipse dont fait partie cet arc, dans les coordonnees de la scene.
*/
QPointF PartArc::sceneTopLeft() const {
	return(mapToScene(rect().topLeft()));
}

/**
 * @brief PartArc::setX
 * @param x is the center of the rect bounding this ellipse
 */
void PartArc::setX(const qreal x) {
	QRectF current_rect = rect();
	QPointF current_pos = mapToScene(current_rect.center());
	setRect(current_rect.translated(x - current_pos.x(), 0.0));
}

/**
 * @brief PartArc::setY
 * @param y is the center of the rect bounding this ellipse
 */
void PartArc::setY(const qreal y) {
	QRectF current_rect = rect();
	QPointF current_pos = mapToScene(current_rect.center());
	setRect(current_rect.translated(0.0, y - current_pos.y()));
}

/**
 * @brief PartArc::setWidth
 * @param w is the width of the rect bounding this ellipse
 */
void PartArc::setWidth(const qreal w) {
	qreal new_width = qAbs(w);
	QRectF current_rect = rect();
	current_rect.translate((new_width - current_rect.width()) / -2.0, 0.0);
	current_rect.setWidth(new_width);
	setRect(current_rect);
}

/**
 * @brief PartArc::setHeight
 * @param h is the heigth of the rect bounding this ellipse
 */
void PartArc::setHeight(const qreal h) {
	qreal new_height = qAbs(h);
	QRectF current_rect = rect();
	current_rect.translate(0.0, (new_height - current_rect.height()) / -2.0);
	current_rect.setHeight(new_height);
	setRect(current_rect);
}

/**
	Gere les changements intervenant sur cette partie
	@param change Type de changement
	@param value Valeur numerique relative au changement
*/
QVariant PartArc::itemChange(GraphicsItemChange change, const QVariant &value) {
	if (scene()) {
		if (change == QGraphicsItem::ItemPositionChange || change == QGraphicsItem::ItemPositionHasChanged) {
			updateCurrentPartEditor();
		}
	}
	return(QGraphicsEllipseItem::itemChange(change, value));
}

/**
	@return true si cette partie n'est pas pertinente et ne merite pas d'etre
	conservee / enregistree.
	Un arc est pertinent des lors que ses dimensions et son etendue ne sont
	pas nulles.
*/
bool PartArc::isUseless() const {
	return(rect().isNull() || !angle());
}

/**
	@return the minimum, margin-less rectangle this part can fit into, in scene
	coordinates. It is different from boundingRect() because it is not supposed
	to imply any margin, and it is different from shape because it is a regular
	rectangle, not a complex shape.
*/
QRectF PartArc::sceneGeometricRect() const {
	return(mapToScene(rect()).boundingRect());
}

/**
	Start the user-induced transformation, provided this primitive is contained
	within the \a initial_selection_rect bounding rectangle.
*/
void PartArc::startUserTransformation(const QRectF &initial_selection_rect) {
	Q_UNUSED(initial_selection_rect)
	saved_points_.clear();
	saved_points_ << mapToScene(rect().topLeft()) << mapToScene(rect().bottomRight());
}

/**
	Handle the user-induced transformation from \a initial_selection_rect to \a new_selection_rect
*/
void PartArc::handleUserTransformation(const QRectF &initial_selection_rect, const QRectF &new_selection_rect) {
	QList<QPointF> mapped_points = mapPoints(initial_selection_rect, new_selection_rect, saved_points_);
	setRect(QRectF(mapFromScene(mapped_points.at(0)), mapFromScene(mapped_points.at(1))));
}

/**
	@return le rectangle delimitant cette partie.
*/
QRectF PartArc::boundingRect() const {
	qreal adjust = 1.5;
	QRectF r(QGraphicsEllipseItem::boundingRect().normalized());
	r.adjust(-adjust, -adjust, adjust, adjust);
	return(r);
}
