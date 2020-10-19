/*
	Copyright 2006-2020 The QElectroTech Team
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
	@brief PartTerminal::PartTerminal
	@param editor :
	L'editeur d'element concerne
	@param parent :
	Le QGraphicsItem parent de cette borne
*/
PartTerminal::PartTerminal(QETElementEditor *editor, QGraphicsItem *parent) :
	CustomElementGraphicPart(editor, parent)
{
	d = new TerminalData(this);
	d -> m_orientation = Qet::North;
	d -> m_uuid = QUuid::createUuid(); // if part is loaded this uuid will be overwritten, but being sure that terminal has a uuid
	updateSecondPoint();
	setZValue(100000);
}

/// Destructeur
PartTerminal::~PartTerminal()
{
}

/**
	Importe les proprietes d'une borne depuis un element XML
	@param xml_elmt Element XML a lire
*/
void PartTerminal::fromXml(const QDomElement &xml_elmt) {
	d -> fromXml(xml_elmt);
	setPos(d -> m_pos);
	updateSecondPoint();
}

/**
	Exporte la borne en XML
	@param xml_document Document XML a utiliser pour creer l'element XML
	@return un element XML decrivant la borne
*/
const QDomElement PartTerminal::toXml(QDomDocument &xml_document) const
{
	return d -> toXml(xml_document);
}

/**
	Dessine la borne
	@param painter QPainter a utiliser pour rendre le dessin
	@param options Options pour affiner le rendu
	@param widget Widget sur lequel le rendu est effectue
*/
void PartTerminal::paint(
		QPainter *painter,
		const QStyleOptionGraphicsItem *options,
		QWidget *widget)
{
	Q_UNUSED(widget);
	painter -> save();

	// annulation des renderhints
	painter -> setRenderHint(QPainter::Antialiasing,          false);
	painter -> setRenderHint(QPainter::TextAntialiasing,      false);
	painter -> setRenderHint(QPainter::SmoothPixmapTransform, false);

	QPen t;
	t.setWidthF(1.0);

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)	// ### Qt 6: remove
	t.setCosmetic(options && options -> levelOfDetail < 1.0);
#else
#if TODO_LIST
#pragma message("@TODO remove code for QT 6 or later")
#endif
	t.setCosmetic(
				options
				&& options->levelOfDetailFromTransform(
					painter->worldTransform())
				< 1.0);
#endif
	// dessin de la borne en rouge
	t.setColor(isSelected() ? Terminal::neutralColor : Qt::red);
	painter -> setPen(t);
	painter -> drawLine(QPointF(0.0, 0.0), d -> second_point);

	// dessin du point d'amarrage au conducteur en bleu
	t.setColor(isSelected() ? Qt::red : Terminal::neutralColor);
	painter -> setPen(t);
	painter -> setBrush(Terminal::neutralColor);
	painter -> drawPoint(QPointF(0.0, 0.0));
	painter -> restore();

	if (m_hovered)
		drawShadowShape(painter);
}

/**
	@brief PartTerminal::shape
	@return the shape of this item
*/
QPainterPath PartTerminal::shape() const
{
	QPainterPath shape;
	shape.lineTo(d -> second_point);

	QPainterPathStroker pps;
	pps.setWidth(1);

	return (pps.createStroke(shape));
}

/**
	@brief PartTerminal::boundingRect
	@return the bounding rect of this item
*/
QRectF PartTerminal::boundingRect() const
{
	QRectF br(QPointF(0, 0), d -> second_point);
	br = br.normalized();

	qreal adjust = (SHADOWS_HEIGHT + 1) / 2;
	br.adjust(-adjust, -adjust, adjust, adjust);
	return(br);
}

/**
	Definit l'orientation de la borne
	@param ori la nouvelle orientation de la borne
*/
void PartTerminal::setOrientation(Qet::Orientation ori) {
	if (d -> m_orientation == ori) return;
	prepareGeometryChange();
	d -> m_orientation = ori;
	updateSecondPoint();
	emit orientationChanged();
}
/**
	@brief PartTerminal::setName
	@param name
*/
void PartTerminal::setName(QString& name) {
	if (d -> m_name == name) return;
	d -> m_name = name;
	emit nameChanged();
}

void PartTerminal::setNewUuid()
{
	d -> m_uuid = QUuid::createUuid();
}

/**
	Met a jour la position du second point en fonction de la position et de
	l'orientation de la borne.
*/
void PartTerminal::updateSecondPoint()
{
	qreal ts = 4.0; // terminal size
	switch(d -> m_orientation) {
		case Qet::North: d -> second_point = QPointF(0.0,  ts); break;
		case Qet::East : d -> second_point = QPointF(-ts, 0.0); break;
		case Qet::South: d -> second_point = QPointF(0.0, -ts); break;
		case Qet::West : d -> second_point = QPointF(ts,  0.0); break;
	}
}

/**
	@return true si cette partie n'est pas pertinente et ne merite pas d'etre
	conservee / enregistree.
	Une borne est toujours pertinente ; cette fonction renvoie donc
	toujours false
*/
bool PartTerminal::isUseless() const
{
	return(false);
}

/**
	@return the minimum, margin-less rectangle this part can fit into, in scene
	coordinates. It is different from boundingRect() because it is not supposed
	to imply any margin, and it is different from shape because it is a regular
	rectangle, not a complex shape.
*/
QRectF PartTerminal::sceneGeometricRect() const
{
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
	QPointF mapped_point = mapPoints(
		initial_selection_rect, new_selection_rect, QList<QPointF>() << saved_position_).first();
	setPos(mapped_point);
}
