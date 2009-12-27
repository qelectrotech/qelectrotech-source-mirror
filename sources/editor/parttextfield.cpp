/*
	Copyright 2006-2009 Xavier Guerrin
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
#include "editorcommands.h"
#include "qetapp.h"

/**
	Constructeur
	@param editor L'editeur d'element concerne
	@param parent Le QGraphicsItem parent de ce champ de texte
	@param scene La scene sur laquelle figure ce champ de texte
*/
PartTextField::PartTextField(QETElementEditor *editor, QGraphicsItem *parent, QGraphicsScene *scene) :
	QGraphicsTextItem(parent, scene),
	CustomElementPart(editor),
	follow_parent_rotations(true),
	rotation_angle_(0.0)
{
	setDefaultTextColor(Qt::black);
	setFont(QETApp::diagramTextsFont());
	setFlags(QGraphicsItem::ItemIsMovable | QGraphicsItem::ItemIsSelectable);
	setPlainText(QObject::tr("_", "default text when adding a textfield in the element editor"));
	infos = new TextFieldEditor(elementEditor(), this);
	infos -> setElementTypeName(name());
}

/// Destructeur
PartTextField::~PartTextField() {
	if (infos -> parentWidget()) return; // le widget sera supprime par son parent
	delete infos;
}

/**
	Importe les proprietes d'un champ de texte depuis un element XML
	@param xml_element Element XML a lire
*/
void PartTextField::fromXml(const QDomElement &xml_element) {
	bool ok;
	int font_size = xml_element.attribute("size").toInt(&ok);
	if (!ok || font_size < 1) font_size = 20;
	
	setFont(QETApp::diagramTextsFont(font_size));
	setPlainText(xml_element.attribute("text"));
	
	qreal default_rotation_angle = 0.0;
	if (QET::attributeIsAReal(xml_element, "rotation", &default_rotation_angle)) {
		setRotationAngle(default_rotation_angle);
	}
	
	setPos(
		xml_element.attribute("x").toDouble(),
		xml_element.attribute("y").toDouble()
	);
	
	follow_parent_rotations = (xml_element.attribute("rotate") == "true");
}

/**
	Exporte le champ de texte en XML
	@param xml_document Document XML a utiliser pour creer l'element XML
	@return un element XML decrivant le champ de texte
*/
const QDomElement PartTextField::toXml(QDomDocument &xml_document) const {
	QDomElement xml_element = xml_document.createElement("input");
	xml_element.setAttribute("x", QString("%1").arg(pos().x()));
	xml_element.setAttribute("y", QString("%1").arg(pos().y()));
	xml_element.setAttribute("text", toPlainText());
	xml_element.setAttribute("size", font().pointSize());
	// angle de rotation du champ de texte
	if (rotationAngle()) {
		xml_element.setAttribute("rotation", QString("%1").arg(rotationAngle()));
	}
	// suivi (ou non) des rotations de l'element parent par le champ de texte
	if (follow_parent_rotations) {
		xml_element.setAttribute("rotate", "true");
	}
	return(xml_element);
}

/**
	@return Le widget permettant d'editer ce champ de texte
*/
QWidget *PartTextField::elementInformations() {
	return(infos);
}

/**
	Retourne la position du texte, l'origine etant le milieu du bord gauche du
	champ
	@return la position du texte
*/
QPointF PartTextField::pos() const {
	return(QGraphicsTextItem::pos() + margin());
}

/**
	Specifie la position du champ de texte
	@param new_pos Nouvelle position
*/
void PartTextField::setPos(const QPointF &new_pos) {
	QGraphicsTextItem::setPos(new_pos - margin());
}

/**
	Specifie la position du champ de texte
	@param x abscisse de la nouvelle position
	@param y ordonnee de la nouvelle position
*/
void PartTextField::setPos(qreal x, qreal y) {
	QGraphicsTextItem::setPos(QPointF(x, y) - margin());
}

/**
	@return l'angle de rotation de ce champ de texte
*/
qreal PartTextField::rotationAngle() const {
	return(rotation_angle_);
}

/**
	@param angle Le nouvel angle de rotation de ce champ de texte
*/
void PartTextField::setRotationAngle(const qreal &angle) {
	rotation_angle_ = QET::correctAngle(angle);
	
	// annule toute rotation precedente
	resetTransform();
	
	QPointF pos_margin = margin();
	QTransform rotation;
	rotation.translate(pos_margin.x(), pos_margin.y());
	rotation.rotate(rotation_angle_);
	rotation.translate(-pos_margin.x(), -pos_margin.y());
	
	QGraphicsTextItem::setTransform(rotation, true);
}

/**
	@return true si le champ de texte suit les rotation de l'element, false
	sinon
*/
bool PartTextField::followParentRotations() {
	return(follow_parent_rotations);
}

/**
	@param  fpr true pour que le champ de texte suive les rotation de
	l'element, false sinon
*/
void PartTextField::setFollowParentRotations(bool fpr) {
	follow_parent_rotations = fpr;
}

/**
	@return le decalage entre l'origine du QGraphicsItem et l'origine du champ de
	texte.
*/
QPointF PartTextField::margin() const {
	return(QPointF(0.0, boundingRect().bottom() / 2.0));
}

/**
	Permet a l'element texte de redevenir deplacable a la fin de l'edition de texte
	@param e Le QFocusEvent decrivant la perte de focus
*/
void PartTextField::focusOutEvent(QFocusEvent *e) {
	QGraphicsTextItem::focusOutEvent(e);
	if (previous_text != toPlainText()) {
		undoStack().push(
			new ChangePartCommand(
				TextFieldEditor::tr("contenu") + " " + name(),
				this,
				"text",
				previous_text,
				toPlainText()
			)
		);
		previous_text = toPlainText();
	}
	
	// deselectionne le texte
	QTextCursor qtc = textCursor();
	qtc.clearSelection();
	setTextCursor(qtc);
	
	setTextInteractionFlags(Qt::NoTextInteraction);
	setFlags(QGraphicsItem::ItemIsMovable | QGraphicsItem::ItemIsSelectable);
}

/**
	Permet a l'element texte de devenir editable lorsqu'on double-clique dessus
	@param e Le QGraphicsSceneMouseEvent qui decrit le double-clic
*/
void PartTextField::mouseDoubleClickEvent(QGraphicsSceneMouseEvent *e) {
	setFlags(QGraphicsItem::ItemIsMovable | QGraphicsItem::ItemIsSelectable | QGraphicsItem::ItemIsFocusable);
	setTextInteractionFlags(Qt::TextEditorInteraction);
	previous_text = toPlainText();
	QGraphicsTextItem::mouseDoubleClickEvent(e);
	setFocus(Qt::MouseFocusReason);
}

/**
	Specifie la valeur d'une propriete donnee du champ de texte
	@param property propriete a modifier. Valeurs acceptees :
		* x : abscisse de la position
		* y : ordonnee de la position
		* size : taille du texte
		* text : texte
		* rotate : suivi de la rotation de l'element parent
	@param value Valeur a attribuer a la propriete
*/
void PartTextField::setProperty(const QString &property, const QVariant &value) {
	if (property == "x") {
		if (!value.canConvert(QVariant::Double)) return;
		setPos(value.toDouble(), pos().y());
	} else if (property == "y") {
		if (!value.canConvert(QVariant::Double)) return;
		setPos(pos().x(), value.toDouble());
	} else if (property == "size") {
		if (!value.canConvert(QVariant::Int)) return;
		setFont(QETApp::diagramTextsFont(value.toInt()));
	} else if (property == "text") {
		setPlainText(value.toString());
	} else if (property == "rotation angle") {
		setRotationAngle(value.toDouble());
	} else if (property == "rotate") {
		follow_parent_rotations = value.toBool();
	}
	update();
}

/**
	Permet d'acceder a la valeur d'une propriete donnee du champ de texte
	@param property propriete lue. Valeurs acceptees :
		* x : abscisse de la position
		* y : ordonnee de la position
		* size : taille du texte
		* text : texte
		* rotate : suivi de la rotation de l'element parent
	@return La valeur de la propriete property
*/
QVariant PartTextField::property(const QString &property) {
	if (property == "x") {
		return(pos().x());
	} else if (property == "y") {
		return(pos().y());
	} else if (property == "size") {
		return(font().pointSize());
	} else if (property == "text") {
		return(toPlainText());
	} else if (property == "rotation angle") {
		return(rotation_angle_);
	} else if (property == "rotate") {
		return(follow_parent_rotations);
	}
	return(QVariant());
}

/**
	Gere les changements intervenant sur cette partie
	@param change Type de changement
	@param value Valeur numerique relative au changement
*/
QVariant PartTextField::itemChange(GraphicsItemChange change, const QVariant &value) {
	if (scene()) {
		if (change == QGraphicsItem::ItemPositionChange || change == QGraphicsItem::ItemSelectedChange) {
			infos -> updateForm();
		}
	}
	return(QGraphicsTextItem::itemChange(change, value));
}

/**
	@return le rectangle delimitant cette partie.
*/
QRectF PartTextField::boundingRect() const {
	QRectF r = QGraphicsTextItem::boundingRect();
	r.adjust(0.0, -1.1, 0.0, 0.0);
	return(r);
}

/**
	@return true si cette partie n'est pas pertinente et ne merite pas d'etre
	conservee / enregistree.
	Un champ de texte est toujours pertinent ; cette fonction renvoie donc
	toujours false
*/
bool PartTextField::isUseless() const {
	return(false);
}

/**
	Dessine le texte statique.
	@param painter QPainter a utiliser pour effectuer le rendu
	@param qsogi   Pptions de dessin
	@param widget  Widget sur lequel on dessine (facultatif)
*/
void PartTextField::paint(QPainter *painter, const QStyleOptionGraphicsItem *qsogi, QWidget *qw) {
	QGraphicsTextItem::paint(painter, qsogi, qw);
	
#ifdef QET_DEBUG_EDITOR_TEXTS
	painter -> setPen(Qt::blue);
	painter -> drawRect(boundingRect());
	
	painter -> setPen(Qt::red);
	drawPoint(painter, QPointF(0, 0));
	
	painter -> setPen(QColor("#800000"));
	drawPoint(painter, mapFromScene(pos()));
#endif
}

#ifdef QET_DEBUG_EDITOR_TEXTS
/**
	Dessine deux petites fleches pour mettre un point en valeur
	@param painter QPainter a utiliser pour effectuer le rendu
	@param point   Point a dessiner
*/
void PartTextField::drawPoint(QPainter *painter, const QPointF &point) {
	qreal px = point.x();
	qreal py = point.y();
	qreal size_1 = 5.0;
	qreal size_2 = 1.0;
	painter -> drawLine(px, py, px + size_1, py);
	painter -> drawLine(px + size_1 - size_2, py - size_2, px + size_1, py);
	painter -> drawLine(px + size_1 - size_2, py + size_2, px + size_1, py);
	painter -> drawLine(px, py, px, py + size_1);
	painter -> drawLine(px, py + size_1, px - size_2, py + size_1 - size_2);
	painter -> drawLine(px, py + size_1, px + size_2, py + size_1 - size_2);
}
#endif
