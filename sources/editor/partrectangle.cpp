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
#include "partrectangle.h"

/**
	Constructeur
	@param editor L'editeur d'element concerne
	@param parent Le QGraphicsItem parent de ce rectangle
	@param scene La scene sur laquelle figure ce rectangle
*/
PartRectangle::PartRectangle(QETElementEditor *editor, QGraphicsItem *parent, QGraphicsScene *scene) : QGraphicsRectItem(parent, scene), CustomElementGraphicPart(editor) {
	setFlags(QGraphicsItem::ItemIsSelectable);
#if QT_VERSION >= 0x040600
	setFlag(QGraphicsItem::ItemSendsGeometryChanges, true);
#endif
	setAcceptedMouseButtons(Qt::LeftButton);
}

/// Destructeur
PartRectangle::~PartRectangle() {
}

/**
	Dessine le rectangle
	@param painter QPainter a utiliser pour rendre le dessin
	@param options Options pour affiner le rendu
	@param widget Widget sur lequel le rendu est effectue
*/
void PartRectangle::paint(QPainter *painter, const QStyleOptionGraphicsItem *options, QWidget *widget) {
	Q_UNUSED(widget);
	applyStylesToQPainter(*painter);
	QPen t = painter -> pen();
	t.setCosmetic(options && options -> levelOfDetail < 1.0);
	if (isSelected()) {
		t.setColor(Qt::red);
	}
	
	// force le type de jointures pour les rectangles
	t.setJoinStyle(Qt::MiterJoin);
	
	// force le dessin avec un trait fin si l'une des dimensions du rectangle est nulle
	if (!rect().width() || !rect().height()) {
		t.setWidth(0);
	}
	
	painter -> setPen(t);
	painter -> drawRect(rect());
	if (isSelected()) {
		painter -> setRenderHint(QPainter::Antialiasing, false);
		painter -> setPen((painter -> brush().color() == QColor(Qt::black) && painter -> brush().isOpaque()) ? Qt::yellow : Qt::blue);
		QPointF center = rect().center();
		painter -> drawLine(QLineF(center.x() - 2.0, center.y(), center.x() + 2.0, center.y()));
		painter -> drawLine(QLineF(center.x(), center.y() - 2.0, center.x(), center.y() + 2.0));
	}
}

/**
	Exporte le rectangle en XML
	@param xml_document Document XML a utiliser pour creer l'element XML
	@return un element XML decrivant le rectangle
*/
const QDomElement PartRectangle::toXml(QDomDocument &xml_document) const {
	QDomElement xml_element = xml_document.createElement("rect");
	QPointF top_left(sceneTopLeft());
	xml_element.setAttribute("x", QString("%1").arg(top_left.x()));
	xml_element.setAttribute("y", QString("%1").arg(top_left.y()));
	xml_element.setAttribute("width",  QString("%1").arg(rect().width()));
	xml_element.setAttribute("height", QString("%1").arg(rect().height()));
	stylesToXml(xml_element);
	return(xml_element);
}

/**
	Importe les proprietes d'une rectangle depuis un element XML
	@param qde Element XML a lire
*/
void PartRectangle::fromXml(const QDomElement &qde) {
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
}

/**
	Specifie la valeur d'une propriete donnee du rectangle
	@param property propriete a modifier. Valeurs acceptees :
		* x : abscisse du coin superieur gauche du rectangle
		* y : ordonnee du coin superieur gauche du rectangle
		* width : largeur du rectangle
		* height : hauteur du rectangle
	@param value Valeur a attribuer a la propriete
*/
void PartRectangle::setProperty(const QString &property, const QVariant &value) {
	CustomElementGraphicPart::setProperty(property, value);
	if (!value.canConvert(QVariant::Double)) return;
	if (property == "x") {
		QRectF current_rect = rect();
		QPointF current_pos = mapToScene(current_rect.topLeft());
		setRect(current_rect.translated(value.toDouble() - current_pos.x(), 0.0));
	} else if (property == "y") {
		QRectF current_rect = rect();
		QPointF current_pos = mapToScene(current_rect.topLeft());
		setRect(current_rect.translated(0.0, value.toDouble() - current_pos.y()));
	} else if (property == "width") {
		qreal new_width = qAbs(value.toDouble());
		QRectF current_rect = rect();
		current_rect.setWidth(new_width);
		setRect(current_rect);
	} else if (property == "height") {
		qreal new_height = qAbs(value.toDouble());
		QRectF current_rect = rect();
		current_rect.setHeight(new_height);
		setRect(current_rect);
	}
	update();
}

/**
	Permet d'acceder a la valeur d'une propriete donnee du rectangle
	@param property propriete lue. Valeurs acceptees :
		* x : abscisse du coin superieur gauche du rectangle
		* y : ordonnee du coin superieur gauche du rectangle
		* width : largeur du rectangle
		* height : hauteur du rectangle
	@return La valeur de la propriete property
*/
QVariant PartRectangle::property(const QString &property) {
	// appelle la methode property de CustomElementGraphicpart pour les styles
	QVariant style_property = CustomElementGraphicPart::property(property);
	if (style_property != QVariant()) return(style_property);
	
	if (property == "x") {
		return(mapToScene(rect().topLeft()).x());
	} else if (property == "y") {
		return(mapToScene(rect().topLeft()).y());
	} else if (property == "width") {
		return(rect().width());
	} else if (property == "height") {
		return(rect().height());
	}
	return(QVariant());
}

/**
	Gere les changements intervenant sur cette partie
	@param change Type de changement
	@param value Valeur numerique relative au changement
*/
QVariant PartRectangle::itemChange(GraphicsItemChange change, const QVariant &value) {
	if (scene()) {
		if (change == QGraphicsItem::ItemPositionChange || change == QGraphicsItem::ItemPositionHasChanged) {
			updateCurrentPartEditor();
		}
	}
	return(QGraphicsRectItem::itemChange(change, value));
}

/**
	@return le coin superieur gauche du rectangle, dans les coordonnees de la
	scene.
*/
QPointF PartRectangle::sceneTopLeft() const {
	return(mapToScene(rect().topLeft()));
}

/**
	@return true si cette partie n'est pas pertinente et ne merite pas d'etre
	conservee / enregistree.
	Un rectangle est pertinent des lors que ses dimensions ne sont pas nulles.
*/
bool PartRectangle::isUseless() const {
	return(rect().isNull());
}

/**
	@return the minimum, margin-less rectangle this part can fit into, in scene
	coordinates. It is different from boundingRect() because it is not supposed
	to imply any margin, and it is different from shape because it is a regular
	rectangle, not a complex shape.
*/
QRectF PartRectangle::sceneGeometricRect() const {
	return(mapToScene(rect()).boundingRect());
}

/**
	Start the user-induced transformation, provided this primitive is contained
	within the \a initial_selection_rect bounding rectangle.
*/
void PartRectangle::startUserTransformation(const QRectF &initial_selection_rect) {
	Q_UNUSED(initial_selection_rect)
	// we keep track of our own rectangle at the moment in scene coordinates too
	saved_points_.clear();
	saved_points_ << mapToScene(rect().topLeft()) << mapToScene(rect().bottomRight());
}

/**
	Handle the user-induced transformation from \a initial_selection_rect to \a new_selection_rect
*/
void PartRectangle::handleUserTransformation(const QRectF &initial_selection_rect, const QRectF &new_selection_rect) {
	QList<QPointF> mapped_points = mapPoints(initial_selection_rect, new_selection_rect, saved_points_);
	setRect(QRectF(mapFromScene(mapped_points.at(0)), mapFromScene(mapped_points.at(1))));
}

/**
	@return le rectangle delimitant cette partie.
*/
QRectF PartRectangle::boundingRect() const {
	qreal adjust = 1.5;
	QRectF r(QGraphicsRectItem::boundingRect().normalized());
	r.adjust(-adjust, -adjust, adjust, adjust);
	return(r);
}
