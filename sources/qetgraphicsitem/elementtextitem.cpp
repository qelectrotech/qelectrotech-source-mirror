/*
	Copyright 2006-2016 The QElectroTech Team
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
#include "element.h"
#include <QTextDocument>
#include "diagram.h"
#include "diagramcommands.h"

/**
	Constructeur
	@param parent_element Le QGraphicsItem parent du champ de texte
	@param parent_diagram Le schema auquel appartient le champ de texte
*/
ElementTextItem::ElementTextItem(Element *parent_element) :
	DiagramTextItem(parent_element),
	parent_element_(parent_element),
	follow_parent_rotations(false),
	original_rotation_angle_(0.0)
{build();}

/**
	Constructeur
	@param parent_element L'element parent du champ de texte
	@param parent_diagram Le schema auquel appartient le champ de texte
	@param text Le texte affiche par le champ de texte
*/
ElementTextItem::ElementTextItem(const QString &text, Element *parent_element) :
	DiagramTextItem(text, parent_element),
	parent_element_(parent_element),
	follow_parent_rotations(false),
	original_rotation_angle_(0.0)
{build();}

void ElementTextItem::build() {
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
 * @brief ElementTextItem::mouseDoubleClickEvent
 * @param event
 */
void ElementTextItem::mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event)
{
		/*
		 * Before revision 3559, report element haven't got text tagged label
		 * so if parent is a report and haven't got text tagged label,
		 * we know if this text is used has tagged label.
		 */
	bool report_text = false;
	if (parent_element_ -> linkType() & Element::AllReport && !parent_element_ -> taggedText("label"))
	{
			//This is the first of list, so this text is used to display
			//pos of linked report.
		if (parent_element_ -> texts().first() == this)
			report_text = true;
	}

	if ( (tagg_ == "label" || report_text) && !parent_element_ -> isFree())
	{
			//If parent is linked, show the linked element
		if ( parent_element_ -> linkType() & (Element::AllReport | Element::Slave) )
		{
				//Unselect and ungrab mouse to prevent unwanted
				//move when linked element is in the same scene of this.
			setSelected(false);
			ungrabMouse();

				//Show and select the linked element
			Element *linked = parent_element_ -> linkedElements().first();
			if (scene() != linked -> scene())
				linked -> diagram() -> showMe();
			linked -> setSelected(true);

				//Zoom to the linked element
			foreach(QGraphicsView *view, linked -> diagram() -> views()) {
				QRectF fit = linked -> sceneBoundingRect();
				fit.adjust(-200, -200, 200, 200);
				view -> fitInView(fit, Qt::KeepAspectRatioByExpanding);
			}
		}
	}
	else
	{
		DiagramTextItem::mouseDoubleClickEvent(event);
	}
}

/**
 * @brief ElementTextItem::mouseMoveEvent
 * @param e
 */
void ElementTextItem::mouseMoveEvent(QGraphicsSceneMouseEvent *event) {
	if (textInteractionFlags() & Qt::TextEditable) {
		DiagramTextItem::mouseMoveEvent(event);
	} else if ((flags() & QGraphicsItem::ItemIsMovable) && (event -> buttons() & Qt::LeftButton)) {
		QPointF old_pos = pos();

		/*
		 * Use e -> pos() directly will be have for behavior to pos the origin
		 * of the text field to the position pointed by the cursor, that isn't the wanted effect.
		 * Instead of this, we apply to the actual pos,
		 * the vector defined by the movement of cursor since the last pos clicked by left button
		 */
		QPointF movement = event -> pos() - event -> buttonDownPos(Qt::LeftButton);

		/*
		 * the method pos() and setPos() always work with coordinate of parent item
		 * (or scene if there isn't parent) we don't forget to map the movemement to parent
		 * before applyRotation
		 */
		QPointF new_pos = pos() + mapMovementToParent(movement);
		event -> modifiers() == Qt::ControlModifier ? setPos(new_pos) : setPos(Diagram::snapToGrid(new_pos));

		Diagram *diagram_ptr = diagram();
		if (diagram_ptr) {
			if (m_first_move) {
				//We signal the beginning of movement to the parent diagram
				int moved_texts_count = diagram_ptr -> beginMoveElementTexts(this);

				//If there is one texte to move, we highlight the parent element.
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
	} else event -> ignore();

	if (m_first_move) {
		m_first_move = false;
	}
}

/**
 * @brief ElementTextItem::mouseReleaseEvent
 * @param e
 */
void ElementTextItem::mouseReleaseEvent(QGraphicsSceneMouseEvent *event) {
	if (Diagram *diagram_ptr = diagram()) {
		if (parent_element_) {
			if (parent_element_ -> isHighlighted()) {
				parent_element_ -> setHighlighted(false);
			}
		}

		diagram_ptr -> endMoveElementTexts();
	}
	if (!(event -> modifiers() & Qt::ControlModifier)) {
		QGraphicsTextItem::mouseReleaseEvent(event);
	}
}

/**
 * @brief ElementTextItem::hoverEnterEvent
 * @param event
 */
void ElementTextItem::hoverEnterEvent(QGraphicsSceneHoverEvent *event)
{
		/*
		 * Before revision 3559, report element haven't got text tagged label
		 * so if parent is a report and haven't got text tagged label,
		 * we know if this text is used has tagged label.
		 */
	bool report_text = false;
	if (parent_element_ -> linkType() & Element::AllReport && !parent_element_ -> taggedText("label"))
	{
			//This is the first of list, so this text is used to display
			//pos of linked report.
		if (parent_element_ -> texts().first() == this)
			report_text = true;
	}

	if (tagg_ == "label" || report_text)
	{
		if (parent_element_ -> linkType() & (Element::AllReport | Element::Slave) && !parent_element_->isFree()) {

			setDefaultTextColor(Qt::blue);

				//Also color the child text if parent is a slave and linked
			if (parent_element_-> linkType() == Element::Slave && !parent_element_ -> isFree())
				foreach (QGraphicsItem *qgi, childItems())
					if (QGraphicsTextItem *qgti = qgraphicsitem_cast<QGraphicsTextItem *> (qgi))
						qgti -> setDefaultTextColor(Qt::blue);
		}
	}

	DiagramTextItem::hoverEnterEvent(event);
}

/**
 * @brief ElementTextItem::hoverLeaveEvent
 * @param event
 */
void ElementTextItem::hoverLeaveEvent(QGraphicsSceneHoverEvent *event)
{
		/*
		 * Before revision 3559, report element haven't got text tagged label
		 * so if parent is a report and haven't got text tagged label,
		 * we know if this text is used has tagged label.
		 */
	bool report_text = false;
	if (parent_element_ -> linkType() & Element::AllReport && !parent_element_ -> taggedText("label"))
	{
			//This is the first of list, so this text is used to display
			//pos of linked report.
		if (parent_element_ -> texts().first() == this)
			report_text = true;
	}

	if (tagg_ == "label" || report_text)
	{
		if (defaultTextColor() != Qt::black)
			setDefaultTextColor(Qt::black);

			//Also color the child text if parent is a slave and linked
		if (parent_element_-> linkType() == Element::Slave && !parent_element_ -> isFree())
			foreach (QGraphicsItem *qgi, childItems())
				if (QGraphicsTextItem *qgti = qgraphicsitem_cast<QGraphicsTextItem *> (qgi))
					qgti -> setDefaultTextColor(Qt::black);
	}

	DiagramTextItem::hoverLeaveEvent(event);
}
