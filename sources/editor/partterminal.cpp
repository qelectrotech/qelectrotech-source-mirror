/*
	Copyright 2006-2013 The QElectroTech Team
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
#include "partterminal.h"
#include "terminal.h"

/**
	Constructeur
	@param editor L'editeur d'element concerne
	@param parent Le QGraphicsItem parent de cette borne
	@param scene La scene sur laquelle figure cette borne
*/
PartTerminal::PartTerminal(QETElementEditor *editor, QGraphicsItem *parent, QGraphicsScene *scene) :
	CustomElementPart(editor),
	QGraphicsItem(parent, scene),
	_orientation(QET::North)
{
	updateSecondPoint();
	setFlags(QGraphicsItem::ItemIsSelectable);
#if QT_VERSION >= 0x040600
	setFlag(QGraphicsItem::ItemSendsGeometryChanges, true);
#endif
	setZValue(100000);
}

/// Destructeur
PartTerminal::~PartTerminal() {
}

/**
	Importe les proprietes d'une borne depuis un element XML
	@param xml_elmt Element XML a lire
*/
void PartTerminal::fromXml(const QDomElement &xml_elmt) {
	// lit la position de la borne
	qreal term_x = 0.0, term_y = 0.0;
	QET::attributeIsAReal(xml_elmt, "x", &term_x);
	QET::attributeIsAReal(xml_elmt, "y", &term_y);
	setPos(QPointF(term_x, term_y));
	
	// lit l'orientation de la borne
	_orientation = QET::orientationFromString(xml_elmt.attribute("orientation"));
	
	// Read number and name of terminal from XML
	number_ = xml_elmt.attribute("number");
	name_ = xml_elmt.attribute("name");
	
	updateSecondPoint();
}

/**
	Exporte la borne en XML
	@param xml_document Document XML a utiliser pour creer l'element XML
	@return un element XML decrivant la borne
*/
const QDomElement PartTerminal::toXml(QDomDocument &xml_document) const {
	QDomElement xml_element = xml_document.createElement("terminal");
	
	// ecrit la position de la borne
	xml_element.setAttribute("x", QString("%1").arg(scenePos().x()));
	xml_element.setAttribute("y", QString("%1").arg(scenePos().y()));
	
	// ecrit l'orientation de la borne
	xml_element.setAttribute("orientation", orientationToString(_orientation));
	// Write name and number to XML
	xml_element.setAttribute("number", number_);
	xml_element.setAttribute("name", name_);
	
	return(xml_element);
}

/**
	Dessine la borne
	@param p QPainter a utiliser pour rendre le dessin
	@param options Options pour affiner le rendu
	@param widget Widget sur lequel le rendu est effectue
*/
void PartTerminal::paint(QPainter *p, const QStyleOptionGraphicsItem *options, QWidget *widget) {
	Q_UNUSED(widget);
	p -> save();
	
	// annulation des renderhints
	p -> setRenderHint(QPainter::Antialiasing,          false);
	p -> setRenderHint(QPainter::TextAntialiasing,      false);
	p -> setRenderHint(QPainter::SmoothPixmapTransform, false);
	
	QPen t;
	t.setWidthF(1.0);
	t.setCosmetic(options && options -> levelOfDetail < 1.0);
	
	// dessin de la borne en rouge
	t.setColor(isSelected() ? Terminal::neutralColor : Qt::red);
	p -> setPen(t);
	p -> drawLine(QPointF(0.0, 0.0), second_point);
	
	// dessin du point d'amarrage au conducteur en bleu
	t.setColor(isSelected() ? Qt::red : Terminal::neutralColor);
	p -> setPen(t);
	p -> setBrush(Terminal::neutralColor);
	p -> drawPoint(QPointF(0.0, 0.0));
	p -> restore();
}

/**
	@return le rectangle delimitant cette partie.
*/
QRectF PartTerminal::boundingRect() const {
	QPointF p1, p2;
	if (second_point.x() <= 0.0 && second_point.y() <= 0.0) {
		p1 = second_point;
		p2 = QPointF(0.0, 0.0);
	} else {
		p1 = QPointF(0.0, 0.0);
		p2 = second_point;
	}
	QRectF br;
	br.setTopLeft    (p1 - QPointF(2.0, 2.0));
	br.setBottomRight(p2 + QPointF(2.0, 2.0));
	return(br);
}

/**
	@return L'orientation de la borne
*/
QET::Orientation PartTerminal::orientation() const {
	return(_orientation);
}

/**
	Definit l'orientation de la borne
	@param ori la nouvelle orientation de la borne
*/
void PartTerminal::setOrientation(QET::Orientation ori) {
	prepareGeometryChange();
	_orientation = ori;
	updateSecondPoint();
}

/**
	@return Number of terminal
*/
QString PartTerminal::number() const {
	return(number_);
}

/**
	set Number of Terminal
	@param num number of terminal
*/
void PartTerminal::setNumber(const QString &num) {
	number_ = num;
}

/**
	@return Name of terminal
*/
QString PartTerminal::nameOfTerminal() const {
	return(name_);
}

/**
	set Name of Terminal
	@param na Name of terminal
*/
void PartTerminal::setName(const QString &na) {
	name_ = na;
}

/**
	Specifie la valeur d'une propriete donnee de la borne
	@param property propriete a modifier. Valeurs acceptees :
		* x : abscisse de la borne
		* y : ordonnee de la borne
		* orientation : orientation de la borne
	@param value Valeur a attribuer a la propriete
*/
void PartTerminal::setProperty(const QString &property, const QVariant &value) {
	if (property == "x") {
		if (!value.canConvert(QVariant::Double)) return;
		setPos(value.toDouble(), pos().y());
	} else if (property == "y") {
		if (!value.canConvert(QVariant::Double)) return;
		setPos(pos().x(), value.toDouble());
	} else if (property == "orientation") {
		if (!value.canConvert(QVariant::Int)) return;
		setOrientation(static_cast<QET::Orientation>(value.toInt()));
	} else if (property == "number") {
		if (!value.canConvert(QVariant::String)) return;
		setNumber(value.toString());
	} else if (property == "name") {
		if (!value.canConvert(QVariant::String)) return;
		setName(value.toString());
	}
}

/**
	Permet d'acceder a la valeur d'une propriete donnee de la borne
	@param property propriete lue. Valeurs acceptees :
		* x : abscisse de la borne
		* y : ordonnee de la borne
		* orientation : orientation de la borne
	@return La valeur de la propriete property
*/
QVariant PartTerminal::property(const QString &property) {
	if (property == "x") {
		return(scenePos().x());
	} else if (property == "y") {
		return(scenePos().y());
	} else if (property == "orientation") {
		return(_orientation);
	} else if (property == "number") {
		return(number_);
	} else if (property == "name") {
		return(name_);
	}
	return(QVariant());
}

/**
	Gere les changements intervenant sur cette partie
	@param change Type de changement
	@param value Valeur numerique relative au changement
*/
QVariant PartTerminal::itemChange(GraphicsItemChange change, const QVariant &value) {
	if (scene()) {
		if (change == QGraphicsItem::ItemPositionChange || change == QGraphicsItem::ItemPositionHasChanged) {
			updateCurrentPartEditor();
		}
	}
	return(QGraphicsItem::itemChange(change, value));
}

/**
	Met a jour la position du second point en fonction de la position et de
	l'orientation de la borne.
*/
void PartTerminal::updateSecondPoint() {
	qreal ts = 4.0; // terminal size
	switch(_orientation) {
		case QET::North: second_point = QPointF(0.0,  ts); break;
		case QET::East : second_point = QPointF(-ts, 0.0); break;
		case QET::South: second_point = QPointF(0.0, -ts); break;
		case QET::West : second_point = QPointF(ts,  0.0); break;
	}
}

/**
	@return true si cette partie n'est pas pertinente et ne merite pas d'etre
	conservee / enregistree.
	Une borne est toujours pertinente ; cette fonction renvoie donc
	toujours false
*/
bool PartTerminal::isUseless() const {
	return(false);
}

/**
	@return the minimum, margin-less rectangle this part can fit into, in scene
	coordinates. It is different from boundingRect() because it is not supposed
	to imply any margin, and it is different from shape because it is a regular
	rectangle, not a complex shape.
*/
QRectF PartTerminal::sceneGeometricRect() const {
	return(sceneBoundingRect());
}

/**
	Start the user-induced transformation, provided this primitive is contained
	within the \a initial_selection_rect bounding rectangle.
*/
void PartTerminal::startUserTransformation(const QRectF &initial_selection_rect) {
	Q_UNUSED(initial_selection_rect)
	saved_position_ = scenePos();
}

/**
	Handle the user-induced transformation from \a initial_selection_rect to \a new_selection_rect
*/
void PartTerminal::handleUserTransformation(const QRectF &initial_selection_rect, const QRectF &new_selection_rect) {
	QPointF mapped_point = mapPoints(initial_selection_rect, new_selection_rect, QList<QPointF>() << saved_position_).first();
	setPos(mapped_point);
}
