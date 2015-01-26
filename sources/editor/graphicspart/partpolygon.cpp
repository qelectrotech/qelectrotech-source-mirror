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
#include "partpolygon.h"
#include "qet.h"

/**
	Constructeur
	@param editor L'editeur d'element concerne
	@param parent Le QGraphicsItem parent de ce polygone
	@param scene La scene sur laquelle figure ce polygone
*/
PartPolygon::PartPolygon(QETElementEditor *editor, QGraphicsItem *parent, QGraphicsScene *scene) : 
	CustomElementGraphicPart(editor),
	QGraphicsPolygonItem(parent, scene),
	m_closed(false)
{
	setFlags(QGraphicsItem::ItemIsSelectable);
#if QT_VERSION >= 0x040600
	setFlag(QGraphicsItem::ItemSendsGeometryChanges, true);
#endif
	setAcceptedMouseButtons(Qt::LeftButton);
}

/// Destructeur
PartPolygon::~PartPolygon() {
}

/**
	Importe les proprietes d'un polygone depuis un element XML
	@param qde Element XML a lire
*/
void PartPolygon::fromXml(const QDomElement &qde) {
	stylesFromXml(qde);
	int i = 1;
	while(true) {
		if (
			QET::attributeIsAReal(qde, QString("x%1").arg(i)) &&\
			QET::attributeIsAReal(qde, QString("y%1").arg(i))
		) ++ i;
		else break;
	}
	
	QPolygonF temp_polygon;
	for (int j = 1 ; j < i ; ++ j) {
		temp_polygon << QPointF(
			qde.attribute(QString("x%1").arg(j)).toDouble(),
			qde.attribute(QString("y%1").arg(j)).toDouble()
		);
	}
	setPolygon(temp_polygon);
	
	m_closed = qde.attribute("closed") != "false";
}

/**
	Exporte le polygone en XML
	@param xml_document Document XML a utiliser pour creer l'element XML
	@return un element XML decrivant le polygone
*/
const QDomElement PartPolygon::toXml(QDomDocument &xml_document) const {
	QDomElement xml_element = xml_document.createElement("polygon");
	int i = 1;
	foreach(QPointF point, polygon()) {
		point = mapToScene(point);
		xml_element.setAttribute(QString("x%1").arg(i), QString("%1").arg(point.x()));
		xml_element.setAttribute(QString("y%1").arg(i), QString("%1").arg(point.y()));
		++ i;
	}
	if (!m_closed) xml_element.setAttribute("closed", "false");
	stylesToXml(xml_element);
	return(xml_element);
}

/**
	Dessine le polygone
	@param painter QPainter a utiliser pour rendre le dessin
	@param options Options pour affiner le rendu
	@param widget Widget sur lequel le rendu est effectue
*/
void PartPolygon::paint(QPainter *painter, const QStyleOptionGraphicsItem *options, QWidget *widget) {
	Q_UNUSED(widget);
	applyStylesToQPainter(*painter);
	QPen t = painter -> pen();
	t.setCosmetic(options && options -> levelOfDetail < 1.0);
	if (isSelected()) t.setColor(Qt::red);
	painter -> setPen(t);
	if (m_closed) painter -> drawPolygon(polygon());
	else painter -> drawPolyline(polygon());
}

/**
	Gere les changements intervenant sur cette partie
	@param change Type de changement
	@param value Valeur numerique relative au changement
*/
QVariant PartPolygon::itemChange(GraphicsItemChange change, const QVariant &value) {
	if (scene()) {
		if (change == QGraphicsItem::ItemPositionChange || change == QGraphicsItem::ItemPositionHasChanged) {
			updateCurrentPartEditor();
		}
	}
	return(QGraphicsPolygonItem::itemChange(change, value));
}

/**
	@return true si cette partie n'est pas pertinente et ne merite pas d'etre
	conservee / enregistree.
	Un polygone est pertinent des lors qu'il possede deux points differents.
*/
bool PartPolygon::isUseless() const {
	QPolygonF poly(polygon());
	
	if (polygon().count() < 2) return(true);
	
	QPointF previous_point;
	for (int i = 1 ; i < poly.count() ; ++ i) {
		if (poly[i] != poly[i-1]) return(false);
	}
	
	return(true);
}

/**
	@return the minimum, margin-less rectangle this part can fit into, in scene
	coordinates. It is different from boundingRect() because it is not supposed
	to imply any margin, and it is different from shape because it is a regular
	rectangle, not a complex shape.
*/
QRectF PartPolygon::sceneGeometricRect() const {
	return(mapToScene(polygon().boundingRect()).boundingRect());
}

/**
	Start the user-induced transformation, provided this primitive is contained
	within the \a initial_selection_rect bounding rectangle.
*/
void PartPolygon::startUserTransformation(const QRectF &initial_selection_rect) {
	Q_UNUSED(initial_selection_rect)
	saved_points_ = mapToScene(polygon()).toList();
}

/**
	Handle the user-induced transformation from \a initial_selection_rect to \a new_selection_rect
*/
void PartPolygon::handleUserTransformation(const QRectF &initial_selection_rect, const QRectF &new_selection_rect) {
	QList<QPointF> mapped_points = mapPoints(initial_selection_rect, new_selection_rect, saved_points_);
	setPolygon(mapFromScene(QPolygonF(mapped_points.toVector())));
}

/**
	@reimp CustomElementPart::preferredScalingMethod
	This method is called by the decorator when it needs to determine the best
	way to interactively scale a primitive. It is typically called when only a
	single primitive is being scaled.
	This reimplementation systematically returns QET::RoundScaleRatios.
*/
QET::ScalingMethod PartPolygon::preferredScalingMethod() const {
	return(QET::RoundScaleRatios);
}

/**
 * @brief PartPolygon::addPoint
 * Add new point to polygon
 * @param point
 */
void PartPolygon::addPoint(const QPointF &point) {
	QPolygonF poly = polygon();
	poly << point;
	setPolygon(poly);
}

/**
 * @brief PartPolygon::setLastPoint
 * Set the last point of polygon to @point
 * @param point
 */
void PartPolygon::setLastPoint(const QPointF &point) {
	QPolygonF poly = polygon();

	if (poly.size())
		poly.pop_back();

	poly << point;
	setPolygon(poly);
}

/**
 * @brief PartPolygon::removeLastPoint
 * Remove the last point of polygon
 */
void PartPolygon::removeLastPoint() {
	QPolygonF poly = polygon();

	if (poly.size())
		poly.pop_back();

	setPolygon(poly);
}

/**
	@return le rectangle delimitant cette partie.
*/
QRectF PartPolygon::boundingRect() const {
	qreal adjust = 1.5;
	QRectF r(QGraphicsPolygonItem::boundingRect());
	r.adjust(-adjust, -adjust, adjust, adjust);
	return(r);
}
