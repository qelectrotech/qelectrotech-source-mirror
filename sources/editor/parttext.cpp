/*
	Copyright 2006-2010 Xavier Guerrin
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
#include "parttext.h"
#include "texteditor.h"
#include "editorcommands.h"
#include "elementscene.h"
#include "qetapp.h"

/**
	Constructeur
	@param editor L'editeur d'element concerne
	@param parent Le QGraphicsItem parent de ce texte statique
	@param scene La scene sur laquelle figure ce texte statique
*/
PartText::PartText(QETElementEditor *editor, QGraphicsItem *parent, ElementScene *scene) :
	QGraphicsTextItem(parent, scene),
	CustomElementPart(editor)
{
#if QT_VERSION >= 0x040500
	document() -> setDocumentMargin(1.0);
#endif
	setDefaultTextColor(Qt::black);
	setFont(QETApp::diagramTextsFont());
	setFlags(QGraphicsItem::ItemIsMovable | QGraphicsItem::ItemIsSelectable);
#if QT_VERSION >= 0x040600
	setFlag(QGraphicsItem::ItemSendsGeometryChanges, true);
#endif
	setPlainText(QObject::tr("T", "default text when adding a text in the element editor"));
}

/// Destructeur
PartText::~PartText() {
}

/**
	Importe les proprietes d'un texte statique depuis un element XML
	@param xml_element Element XML a lire
*/
void PartText::fromXml(const QDomElement &xml_element) {
	bool ok;
	int font_size = xml_element.attribute("size").toInt(&ok);
	if (!ok || font_size < 1) font_size = 20;
	
	setFont(QETApp::diagramTextsFont(font_size));
	setPlainText(xml_element.attribute("text"));
	setPos(
		xml_element.attribute("x").toDouble(),
		xml_element.attribute("y").toDouble()
	);
	known_position_ = pos();
}

/**
	Exporte le texte statique en XML
	@param xml_document Document XML a utiliser pour creer l'element XML
	@return un element XML decrivant le texte statique
*/
const QDomElement PartText::toXml(QDomDocument &xml_document) const {
	QDomElement xml_element = xml_document.createElement("text");
	xml_element.setAttribute("x", QString("%1").arg(pos().x()));
	xml_element.setAttribute("y", QString("%1").arg(pos().y()));
	xml_element.setAttribute("text", toPlainText());
	xml_element.setAttribute("size", font().pointSize());
	return(xml_element);
}

/**
	Retourne la position du texte, l'origine etant le point a gauche du texte,
	sur la baseline de la premiere ligne
	@return la position du texte
*/
QPointF PartText::pos() const {
	return(QGraphicsTextItem::pos() + margin());
}

/**
	Specifie la position du texte statique
	@param left_corner_pos Nouvelle position
*/
void PartText::setPos(const QPointF &left_corner_pos) {
	QGraphicsTextItem::setPos(left_corner_pos - margin());
}

/**
	Specifie la position du texte statique
	@param x abscisse de la nouvelle position
	@param y ordonnee de la nouvelle position
*/
void PartText::setPos(qreal x, qreal y) {
	QGraphicsTextItem::setPos(QPointF(x, y) - margin());
}

/**
	@return Les coordonnees du point situe en bas a gauche du texte.
*/
QPointF PartText::margin() const {
	QFont used_font = font();
	QFontMetrics qfm(used_font);
	
	// marge du texte
#if QT_VERSION >= 0x040500
	qreal document_margin = document() -> documentMargin();
#else
	// il semblerait qu'avant Qt 4.5, ceci vaille non pas 4.0 mais 2.0
	qreal document_margin = 2.0;
#endif
	
	QPointF margin(
		// marge autour du texte
		document_margin,
		// marge au-dessus du texte + distance entre le plafond du texte et la baseline
		document_margin + qfm.ascent()
	);
	return(margin);
}

/**
	Permet a l'element texte de redevenir deplacable a la fin de l'edition de texte
	@param e Le QFocusEvent decrivant la perte de focus
*/
void PartText::focusOutEvent(QFocusEvent *e) {
	QGraphicsTextItem::focusOutEvent(e);
	if (previous_text != toPlainText()) {
		undoStack().push(
			new ChangePartCommand(
				TextEditor::tr("contenu") + " " + name(),
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
	setFlag(QGraphicsItem::ItemIsFocusable, false);
}

/**
	Permet a l'element texte de devenir editable lorsqu'on double-clique dessus
	@param e Le QGraphicsSceneMouseEvent qui decrit le double-clic
*/
void PartText::mouseDoubleClickEvent(QGraphicsSceneMouseEvent *e) {
	setFlag(QGraphicsItem::ItemIsFocusable, true);
	setTextInteractionFlags(Qt::TextEditorInteraction);
	previous_text = toPlainText();
	QGraphicsTextItem::mouseDoubleClickEvent(e);
	setFocus(Qt::MouseFocusReason);
}

/**
	Specifie la valeur d'une propriete donnee du texte statique
	@param property propriete a modifier. Valeurs acceptees :
		* x : abscisse de la position
		* y : ordonnee de la position
		* size : taille du texte
		* text : texte
	@param value Valeur a attribuer a la propriete
*/
void PartText::setProperty(const QString &property, const QVariant &value) {
	if (property == "x") {
		if (!value.canConvert(QVariant::Double)) return;
		setPos(value.toDouble(), pos().y());
	} else if (property == "y") {
		if (!value.canConvert(QVariant::Double)) return;
		setPos(pos().x(), value.toDouble());
	} else if (property == "size") {
		if (!value.canConvert(QVariant::Int)) return;
		setFont(QETApp::diagramTextsFont(value.toInt()));
		adjustItemPosition(0);
	} else if (property == "text") {
		setPlainText(value.toString());
	}
	update();
}

/**
	Permet d'acceder a la valeur d'une propriete donnee du texte statique
	@param property propriete lue. Valeurs acceptees :
		* x : abscisse de la position
		* y : ordonnee de la position
		* size : taille du texte
		* text : texte
	@return La valeur de la propriete property
*/
QVariant PartText::property(const QString &property) {
	if (property == "x") {
		return((scenePos() + margin()).x());
	} else if (property == "y") {
		return((scenePos() + margin()).y());
	} else if (property == "size") {
		return(font().pointSize());
	} else if (property == "text") {
		return(toPlainText());
	}
	return(QVariant());
}

/**
	Gere les changements intervenant sur cette partie
	@param change Type de changement
	@param value Valeur numerique relative au changement
*/
QVariant PartText::itemChange(GraphicsItemChange change, const QVariant &value) {
	if (change == QGraphicsItem::ItemPositionHasChanged || change == QGraphicsItem::ItemSceneHasChanged) {
		// memorise la nouvelle position "officielle" du champ de texte
		// cette information servira a le recentrer en cas d'ajout / retrait de lignes
		known_position_ = pos();
		updateCurrentPartEditor();
	} else if (change == QGraphicsItem::ItemSelectedHasChanged) {
		if (value.toBool() == true) {
			updateCurrentPartEditor();
		}
	}
	return(QGraphicsTextItem::itemChange(change, value));
}

/**
	@return le rectangle delimitant cette partie.
*/
QRectF PartText::boundingRect() const {
	QRectF r = QGraphicsTextItem::boundingRect();
	r.adjust(0.0, -1.1, 0.0, 0.0);
	return(r);
}

/**
	@return true si cette partie n'est pas pertinente et ne merite pas d'etre
	conservee / enregistree.
	Un texte statique n'est pas pertinent lorsque son texte est vide.
*/
bool PartText::isUseless() const {
	return(toPlainText().isEmpty());
}

/**
	Dessine le texte statique.
	@param painter QPainter a utiliser pour effectuer le rendu
	@param qsogi   Pptions de dessin
	@param widget  Widget sur lequel on dessine (facultatif)
*/
void PartText::paint(QPainter *painter, const QStyleOptionGraphicsItem *qsogi, QWidget *widget) {
	QGraphicsTextItem::paint(painter, qsogi, widget);
	
#ifdef QET_DEBUG_EDITOR_TEXTS
	painter -> setPen(Qt::blue);
	painter -> drawRect(boundingRect());
	
	painter -> setPen(Qt::red);
	drawPoint(painter, QPointF(0, 0));
	
	painter -> setPen(Qt::green);
	drawPoint(painter, mapFromScene(pos()));
#endif
}

/**
	Cette methode s'assure que la position du champ de texte est coherente
	en repositionnant son origine (c-a-d le milieu du bord gauche du champ de
	texte) a la position originale. Cela est notamment utile lorsque le champ
	de texte est agrandi ou retreci verticalement (ajout ou retrait de lignes).
	@param new_block_count Nombre de blocs dans le PartText
*/
void PartText::adjustItemPosition(int new_block_count) {
	Q_UNUSED(new_block_count);
	setPos(known_position_);
}

#ifdef QET_DEBUG_EDITOR_TEXTS
/**
	Dessine deux petites fleches pour mettre un point en valeur
	@param painter QPainter a utiliser pour effectuer le rendu
	@param point   Point a dessiner
*/
void PartText::drawPoint(QPainter *painter, const QPointF &point) {
	qreal px = point.x();
	qreal py = point.y();
	qreal size_1 = 5.0;
	qreal size_2 = 1.0;
	painter -> drawLine(QLineF(px, py, px + size_1, py));
	painter -> drawLine(QLineF(px + size_1 - size_2, py - size_2, px + size_1, py));
	painter -> drawLine(QLineF(px + size_1 - size_2, py + size_2, px + size_1, py));
	painter -> drawLine(QLineF(px, py, px, py + size_1));
	painter -> drawLine(QLineF(px, py + size_1, px - size_2, py + size_1 - size_2));
	painter -> drawLine(QLineF(px, py + size_1, px + size_2, py + size_1 - size_2));
}
#endif
