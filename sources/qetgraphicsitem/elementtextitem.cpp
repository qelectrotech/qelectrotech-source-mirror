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
#include "elementtextitem.h"
#include "diagram.h"
#include "diagramcommands.h"
#include "element.h"

/**
	Constructeur
	@param parent_element Le QGraphicsItem parent du champ de texte
	@param parent_diagram Le schema auquel appartient le champ de texte
*/
ElementTextItem::ElementTextItem(Element *parent_element, Diagram *parent_diagram) :
	DiagramTextItem(parent_element, parent_diagram),
	parent_element_(parent_element),
	follow_parent_rotations(false),
	original_rotation_angle_(0.0),
	first_move_(true)
{build();}

/**
	Constructeur
	@param parent_element L'element parent du champ de texte
	@param parent_diagram Le schema auquel appartient le champ de texte
	@param text Le texte affiche par le champ de texte
*/
ElementTextItem::ElementTextItem(const QString &text, Element *parent_element, Diagram *parent_diagram) :
	DiagramTextItem(text, parent_element, parent_diagram),
	parent_element_(parent_element),
	follow_parent_rotations(false),
	original_rotation_angle_(0.0),
	first_move_(true)
{build();}

void ElementTextItem::build() {
	setFlag(QGraphicsItem::ItemIsMovable, false);
	setToolTip(tr("Maintenir ctrl pour d\351placer", "tool tip for element text item"));
	adjustItemPosition(1);
	// ajuste la position du QGraphicsItem lorsque le QTextDocument change
	connect(document(), SIGNAL(blockCountChanged(int)), this, SLOT(adjustItemPosition(int)));
	connect(document(), SIGNAL(contentsChanged()),      this, SLOT(adjustItemPosition()));
}

/// Destructeur
ElementTextItem::~ElementTextItem() {
}

/**
	@return L'element parent de ce champ de texte, ou 0 si celui-ci n'en a pas.
*/
Element *ElementTextItem::parentElement() const {
	return(parent_element_);
}

/**
	Modifie la position du champ de texte
	@param pos La nouvelle position du champ de texte
*/
void ElementTextItem::setPos(const QPointF &pos) {
	QGraphicsTextItem::setPos(pos);
}

/**
	Modifie la position du champ de texte
	@param x La nouvelle abscisse du champ de texte
	@param y La nouvelle ordonnee du champ de texte
*/
void ElementTextItem::setPos(qreal x, qreal y) {
	setPos(QPointF(x, y));
}

/**
	@return La position (bidouillee) du champ de texte
*/
QPointF ElementTextItem::pos() const {
	return(QGraphicsTextItem::pos());
}

/**
	Permet de lire le texte a mettre dans le champ a partir d'un element XML.
	Cette methode se base sur la position du champ pour assigner ou non la
	valeur a ce champ.
	@param e L'element XML representant le champ de texte
*/
void ElementTextItem::fromXml(const QDomElement &e) {
	QPointF _pos = pos();
	if (
		qFuzzyCompare(qreal(e.attribute("x").toDouble()), _pos.x()) &&
		qFuzzyCompare(qreal(e.attribute("y").toDouble()), _pos.y())
	) {
		setPlainText(e.attribute("text"));
		
		qreal user_pos_x, user_pos_y;
		if (
			QET::attributeIsAReal(e, "userx", &user_pos_x) &&
			QET::attributeIsAReal(e, "usery", &user_pos_y)
		) {
			setPos(user_pos_x, user_pos_y);
		}
		
		qreal xml_rotation_angle;
		if (QET::attributeIsAReal(e, "userrotation", &xml_rotation_angle)) {
			setRotationAngle(xml_rotation_angle);
		}
	}
}

/**
	@param document Le document XML a utiliser
	@return L'element XML representant ce champ de texte
*/
QDomElement ElementTextItem::toXml(QDomDocument &document) const {
	QDomElement result = document.createElement("input");
	
	result.setAttribute("x", QString("%1").arg(originalPos().x()));
	result.setAttribute("y", QString("%1").arg(originalPos().y()));
	
	if (pos() != originalPos()) {
		result.setAttribute("userx", QString("%1").arg(pos().x()));
		result.setAttribute("usery", QString("%1").arg(pos().y()));
	}
	
	result.setAttribute("text", toPlainText());
	
	if (rotationAngle() != originalRotationAngle()) {
		result.setAttribute("userrotation", QString("%1").arg(rotationAngle()));
	}
	
	return(result);
}

/**
	@param p Position originale / de reference pour ce champ
	Cette position est utilisee lors de l'export en XML
*/
void ElementTextItem::setOriginalPos(const QPointF &p) {
	original_position = p;
}

/**
	@return la position originale / de reference pour ce champ
*/
QPointF ElementTextItem::originalPos() const {
	return(original_position);
}

/**
	Definit l'angle de rotation original de ce champ de texte
	@param rotation_angle un angle de rotation
*/
void ElementTextItem::setOriginalRotationAngle(const qreal &rotation_angle) {
	original_rotation_angle_ = QET::correctAngle(rotation_angle);
}

/**
	@return l'angle de rotation original de ce champ de texte
*/
qreal ElementTextItem::originalRotationAngle() const {
	return(original_rotation_angle_);
}

/**
	Set the font used to render the text item to \a font.
*/
void ElementTextItem::setFont(const QFont &font) {
	DiagramTextItem::setFont(font);
	adjustItemPosition(1);
}

/**
	Cette methode s'assure que la position de l'ElementTextItem est coherente
	en ajustant :
		* la transformation de base qui permet de considerer que l'origine
	correspond au milieu du bord gauche du champ de texte
		* l'origine utilisee lors des appels a setRotation et setScale
	@param new_block_count Nombre de blocs dans l'ElementTextItem
*/
void ElementTextItem::adjustItemPosition(int new_block_count) {
	Q_UNUSED(new_block_count);
	qreal origin_offset = boundingRect().bottom() / 2.0;
	
	QTransform base_translation;
	base_translation.translate(0.0, -origin_offset);
	setTransform(base_translation, false);
	setTransformOriginPoint(0.0, origin_offset);
}

/**
	Effetue la rotation du texte en elle-meme
	Pour les ElementTextItem, la rotation s'effectue autour du milieu du bord
	gauche du champ de texte.
	@param angle Angle de la rotation a effectuer
*/
void ElementTextItem::applyRotation(const qreal &angle) {
	QGraphicsTextItem::setRotation(QGraphicsTextItem::rotation() + angle);
}

/**
	Gere le clic sur le champ de texte
	@param e Objet decrivant l'evenement souris
*/
void ElementTextItem::mousePressEvent(QGraphicsSceneMouseEvent *e) {
	first_move_ = true;
	if (e -> modifiers() & Qt::ControlModifier) {
		setSelected(true);
		setFlag(QGraphicsItem::ItemIsMovable, true);
	}
	else DiagramTextItem::mousePressEvent(e);
}

/**
	Gere les mouvements de souris lies au champ de texte
	@param e Objet decrivant l'evenement souris
*/
void ElementTextItem::mouseMoveEvent(QGraphicsSceneMouseEvent *e) {
	if (textInteractionFlags() & Qt::TextEditable) {
		DiagramTextItem::mouseMoveEvent(e);
	} else if ((flags() & QGraphicsItem::ItemIsMovable) && (e -> buttons() & Qt::LeftButton)) {
		QPointF old_pos = pos();
		/*
			Utiliser e -> pos() directement aurait pour effet de positionner
			l'origine du champ de texte a la position indiquee par le curseur,
			ce qui n'est pas l'effet recherche
			Au lieu de cela, on applique a la position actuelle le vecteur
			definissant le mouvement effectue depuis la derniere position
			cliquee avec le bouton gauche
		*/
		QPointF movement = e -> pos() - e -> buttonDownPos(Qt::LeftButton);
		
		/*
			Les methodes pos() et setPos() travaillent toujours avec les
			coordonnees de l'item parent (ou de la scene s'il n'y a pas d'item
			parent). On n'oublie donc pas de mapper le mouvement fraichement
			calcule sur l'item parent avant de l'appliquer.
		*/
		QPointF parent_movement = mapMovementToParent(movement);
		setPos(pos() + parent_movement);
		
		Diagram *diagram_ptr = diagram();
		if (diagram_ptr) {
			if (first_move_) {
				// on signale le debut d'un deplacement d'ElementTextItems au schema parent
				int moved_texts_count = diagram_ptr -> beginMoveElementTexts(this);
				
				// s'il n'y a qu'un seul texte deplace, on met en valeur l'element parent
				if (moved_texts_count == 1 && parent_element_) {
					parent_element_ -> setHighlighted(true);
					parent_element_ -> update();
				}
			}
			
			/*
				Comme setPos() n'est pas oblige d'appliquer exactement la
				valeur qu'on lui fournit, on calcule le mouvement reellement
				applique.
			*/
			QPointF effective_movement = pos() - old_pos;
			QPointF scene_effective_movement = mapMovementToScene(mapMovementFromParent(effective_movement));
			
			// on applique le mouvement subi aux autres textes a deplacer
			diagram_ptr -> continueMoveElementTexts(scene_effective_movement);
		}
	} else e -> ignore();
	
	if (first_move_) {
		first_move_ = false;
	}
}

/**
	Gere le relachement de souris
	Cette methode cree un objet d'annulation pour le deplacement
	@param e Objet decrivant l'evenement souris
*/
void ElementTextItem::mouseReleaseEvent(QGraphicsSceneMouseEvent *e) {
	if (Diagram *diagram_ptr = diagram()) {
		// on arrete de mettre en valeur l'element parent
		if (parent_element_) {
			if (parent_element_ -> isHighlighted()) {
				parent_element_ -> setHighlighted(false);
			}
		}
		
		diagram_ptr -> endMoveElementTexts();
	}
	if (!(e -> modifiers() & Qt::ControlModifier)) {
		QGraphicsTextItem::mouseReleaseEvent(e);
	}
	setFlag(QGraphicsItem::ItemIsMovable, false);
}
