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
#include "partterminal.h"
#include "terminal.h"

/**
	Constructeur
	@param editor L'editeur d'element concerne
	@param parent Le QGraphicsItem parent de cette borne
	@param scene La scene sur laquelle figure cette borne
*/
PartTerminal::PartTerminal(QETElementEditor *editor, QGraphicsItem *parent) :
	CustomElementGraphicPart(editor, parent),
	m_orientation(Qet::North)
{
	updateSecondPoint();
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
	m_orientation = Qet::orientationFromString(xml_elmt.attribute("orientation"));
	
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
	xml_element.setAttribute("orientation", Qet::orientationToString(m_orientation));
	// Write name and number to XML
	
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

	if (m_hovered)
		drawShadowShape(p);
}

/**
 * @brief PartTerminal::shape
 * @return the shape of this item
 */
QPainterPath PartTerminal::shape() const
{
	QPainterPath shape;
	shape.lineTo(second_point);

	QPainterPathStroker pps;
	pps.setWidth(1);

	return (pps.createStroke(shape));
}

/**
 * @brief PartTerminal::boundingRect
 * @return the bounding rect of this item
 */
QRectF PartTerminal::boundingRect() const
{
	QRectF br(QPointF(0, 0), second_point);
	br = br.normalized();

	qreal adjust = (SHADOWS_HEIGHT + 1) / 2;
	br.adjust(-adjust, -adjust, adjust, adjust);
	return(br);
}

/**
	Definit l'orientation de la borne
	@param ori la nouvelle orientation de la borne
*/
void PartTerminal::setOrientation(Qet::Orientation ori)
{
	if (m_orientation == ori) return;
	prepareGeometryChange();
	m_orientation = ori;
	updateSecondPoint();
	emit orientationChanged();
}

/**
	Met a jour la position du second point en fonction de la position et de
	l'orientation de la borne.
*/
void PartTerminal::updateSecondPoint() {
	qreal ts = 4.0; // terminal size
	switch(m_orientation) {
		case Qet::North: second_point = QPointF(0.0,  ts); break;
		case Qet::East : second_point = QPointF(-ts, 0.0); break;
		case Qet::South: second_point = QPointF(0.0, -ts); break;
		case Qet::West : second_point = QPointF(ts,  0.0); break;
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
