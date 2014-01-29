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
	QGraphicsEllipseItem(parent, scene),
	CustomElementGraphicPart(editor),
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
	Specifie la valeur d'une propriete donnee de l'arc
	@param property propriete a modifier. Valeurs acceptees :
		* x : abscisse du centre de l'ellipse dont fait partie l'arc
		* y : ordonnee du centre de l'ellipse dont fait partie l'arc
		* diameter_h : diametre horizontal de l'ellipse dont fait partie l'arc
		* diameter_v : diametre vertical de l'ellipse dont fait partie l'arc
		* start_angle : angle de depart
		* angle : taille de l'arc de cercle
	@param value Valeur a attribuer a la propriete
*/
void PartArc::setProperty(const QString &property, const QVariant &value) {
	CustomElementGraphicPart::setProperty(property, value);
	if (!value.canConvert(QVariant::Double)) return;
	if (property == "x") {
		QRectF current_rect = rect();
		QPointF current_pos = mapToScene(current_rect.center());
		setRect(current_rect.translated(value.toDouble() - current_pos.x(), 0.0));
	} else if (property == "y") {
		QRectF current_rect = rect();
		QPointF current_pos = mapToScene(current_rect.center());
		setRect(current_rect.translated(0.0, value.toDouble() - current_pos.y()));
	} else if (property == "diameter_h") {
		qreal new_width = qAbs(value.toDouble());
		QRectF current_rect = rect();
		current_rect.translate((new_width - current_rect.width()) / -2.0, 0.0);
		current_rect.setWidth(new_width);
		setRect(current_rect);
	} else if (property == "diameter_v") {
		qreal new_height = qAbs(value.toDouble());
		QRectF current_rect = rect();
		current_rect.translate(0.0, (new_height - current_rect.height()) / -2.0);
		current_rect.setHeight(new_height);
		setRect(current_rect);
	} else if (property == "start_angle") {
		setStartAngle(value.toInt() );
	} else if (property == "angle") {
		setAngle(value.toInt());
	}
	update();
}

/**
	Permet d'acceder a la valeur d'une propriete donnee de l'arc de cercle
	@param property propriete lue. Valeurs acceptees :
		* x : abscisse du centre de l'ellipse dont fait partie l'arc
		* y : ordonnee du centre de l'ellipse dont fait partie l'arc
		* diameter_h : diametre horizontal de l'ellipse dont fait partie l'arc
		* diameter_v : diametre vertical de l'ellipse dont fait partie l'arc
		* start_angle : angle de depart
		* angle : taille de l'arc de cercle
	@return La valeur de la propriete property
*/
QVariant PartArc::property(const QString &property) {
	// appelle la methode property de CustomElementGraphicpart pour les styles
	QVariant style_property = CustomElementGraphicPart::property(property);
	if (style_property != QVariant()) return(style_property);
	
	if (property == "x") {
		return(mapToScene(rect().center()).x());
	} else if (property == "y") {
		return(mapToScene(rect().center()).y());
	} else if (property == "diameter_h") {
		return(rect().width());
	} else if (property == "diameter_v") {
		return(rect().height());
	} else if (property == "start_angle") {
		return(start_angle);
	} else if (property == "angle") {
		return(_angle);
	}
	return(QVariant());
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
	Permet de modifier l'etendue de l'arc de cercle.
	Il s'agit d'un angle, exprime en degres.
	Si l'angle est positif, l'arc s'etendra dans le sens des aiguilles d'une
	montre.
	@param a la nouvelle taille de l'arc de cercle
*/
void PartArc::setAngle(int a) {
	_angle = a;
}

/**
	Permet de modifier la position de depart de l'arc de cercle.
	Il s'agit d'un angle, exprime en degres.
	l'angle "0 degre" est situe a "3 heures".
	@param a la nouvelle taille de l'arc de cercle
*/
void PartArc::setStartAngle(int a) {
	start_angle = a;
}

/**
	@return l'etendue de l'arc de cercle
*/
int PartArc::angle() const {
	return(_angle);
}

/**
	@return la position de depart de l'arc de cercle
*/
int PartArc::startAngle() const {
	return(start_angle);
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
