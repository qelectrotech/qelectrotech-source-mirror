/*
	Copyright 2006-2007 Xavier Guerrin
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
#include "parttextfield.h"
#include "textfieldeditor.h"


PartTextField::PartTextField(QETElementEditor *editor, QGraphicsItem *parent, QGraphicsScene *scene) :
	QGraphicsTextItem(parent, scene),
	CustomElementPart(editor),
	follow_parent_rotations(true)
{
	setDefaultTextColor(Qt::black);
	setFlags(QGraphicsItem::ItemIsMovable | QGraphicsItem::ItemIsSelectable);
	setPlainText(QObject::tr("_"));
	infos = new TextFieldEditor(elementEditor(), this);
	infos -> setElementTypeName(name());
}

PartTextField::~PartTextField() {
	delete infos;
}

void PartTextField::fromXml(const QDomElement &xml_element) {
	bool ok;
	int font_size = xml_element.attribute("size").toInt(&ok);
	if (!ok || font_size < 1) font_size = 20;
	
	setFont(QFont(QString("Sans Serif"), font_size));
	setPlainText(xml_element.attribute("text"));
	setPos(
		xml_element.attribute("x").toDouble(),
		xml_element.attribute("y").toDouble()
	);
	
	follow_parent_rotations = (xml_element.attribute("rotate") == "true");
}

const QDomElement PartTextField::toXml(QDomDocument &xml_document) const {
	QDomElement xml_element = xml_document.createElement("input");
	xml_element.setAttribute("x", QString("%1").arg((scenePos() + margin()).x()));
	xml_element.setAttribute("y", QString("%1").arg((scenePos() + margin()).y()));
	xml_element.setAttribute("text", toPlainText());
	xml_element.setAttribute("size", font().pointSize());
	if (follow_parent_rotations) xml_element.setAttribute("rotate", "true");
	return(xml_element);
}

QWidget *PartTextField::elementInformations() {
	return(infos);
}

/**
	Retourne la position du texte, l'origine etant le point en bas a gauche du
	texte (et pas du cadre)
	@return la position du texte
*/
QPointF PartTextField::pos() const {
	return(QGraphicsTextItem::pos() + margin());
}

void PartTextField::setPos(const QPointF &left_corner_pos) {
	QGraphicsTextItem::setPos(left_corner_pos - margin());
}

void PartTextField::setPos(qreal x, qreal y) {
	QGraphicsTextItem::setPos(QPointF(x, y) - margin());
}

bool PartTextField::followParentRotations() {
	return(follow_parent_rotations);
}

void PartTextField::setFollowParentRotations(bool fpr) {
	follow_parent_rotations = fpr;
}

/**
	@return Les coordonnees du point situe en bas a gauche du texte.
*/
QPointF PartTextField::margin() const {
	QFont used_font = font();
	QFontMetrics qfm(used_font);
	QPointF margin(
		(boundingRect().width () - qfm.width(toPlainText())) / 2.0,
		((boundingRect().height() - used_font.pointSizeF()) / 3.0) + used_font.pointSizeF()
	);
	return(margin);
}

/**
	Permet a l'element texte de redevenir deplacable a la fin de l'edition de texte
	@param e Le QFocusEvent decrivant la perte de focus
*/
void PartTextField::focusOutEvent(QFocusEvent *e) {
	QGraphicsTextItem::focusOutEvent(e);
	setFlags(QGraphicsItem::ItemIsMovable | QGraphicsItem::ItemIsSelectable);
}

/**
	Permet a l'element texte de devenir editable lorsqu'on double-clique dessus
	@param e Le QGraphicsSceneMouseEvent qui decrit le double-clic
*/
void PartTextField::mouseDoubleClickEvent(QGraphicsSceneMouseEvent *e) {
	setFlags(QGraphicsItem::ItemIsMovable | QGraphicsItem::ItemIsSelectable | QGraphicsItem::ItemIsFocusable);
	setTextInteractionFlags(Qt::TextEditorInteraction);
	QGraphicsTextItem::mouseDoubleClickEvent(e);
	setFocus(Qt::MouseFocusReason);
}

void PartTextField::setProperty(const QString &property, const QVariant &value) {
	if (property == "x") {
		if (!value.canConvert(QVariant::Double)) return;
		setPos(value.toDouble(), pos().y());
	} else if (property == "y") {
		if (!value.canConvert(QVariant::Double)) return;
		setPos(pos().x(), value.toDouble());
	} else if (property == "size") {
		if (!value.canConvert(QVariant::Int)) return;
		setFont(QFont(font().family(), value.toInt()));
	} else if (property == "text") {
		setPlainText(value.toString());
	} else if (property == "rotate") {
		follow_parent_rotations = value.toBool();
	}
}

QVariant PartTextField::property(const QString &property) {
	if (property == "x") {
		return((scenePos() + margin()).x());
	} else if (property == "y") {
		return((scenePos() + margin()).y());
	} else if (property == "size") {
		return(font().pointSize());
	} else if (property == "text") {
		return(toPlainText());
	} else if (property == "rotate") {
		return(follow_parent_rotations);
	}
	return(QVariant());
}

QVariant PartTextField::itemChange(GraphicsItemChange change, const QVariant &value) {
	if (scene()) {
		if (change == QGraphicsItem::ItemPositionChange || change == QGraphicsItem::ItemSelectedChange) {
			infos -> updateForm();
		}
	}
	return(QGraphicsTextItem::itemChange(change, value));
}

QRectF PartTextField::boundingRect() const {
	QRectF r = QGraphicsTextItem::boundingRect();
	r.adjust(0.0, -2.0, 0.0, 0.0);
	return(r);
}
