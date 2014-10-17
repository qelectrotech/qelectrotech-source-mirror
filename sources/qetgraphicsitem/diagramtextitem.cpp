/*
	Copyright 2006-2013 QElectroTech Team
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
#include "diagramtextitem.h"
#include "diagramcommands.h"
#include "qetapp.h"
#include "richtext/richtexteditor_p.h"
#include "diagram.h"

/**
	Constructeur
	@param parent Le QGraphicsItem parent du champ de texte
	@param parent_diagram Le schema auquel appartient le champ de texte
*/
DiagramTextItem::DiagramTextItem(QGraphicsItem *parent, Diagram *parent_diagram) :
	QGraphicsTextItem(parent, parent_diagram),
	previous_text_(),
	rotation_angle_(0.0),
	m_first_move (true)
{
	//set Zvalue at 10 to be upper than the DiagramImageItem
	setZValue(10);
	setDefaultTextColor(Qt::black);
	setFont(QETApp::diagramTextsFont());
	setFlags(QGraphicsItem::ItemIsSelectable|QGraphicsItem::ItemIsMovable);
	setNoEditable(false);
#if QT_VERSION >= 0x040600
	setFlag(QGraphicsItem::ItemSendsGeometryChanges, true);
#endif
	connect(this, SIGNAL(lostFocus()), this, SLOT(setNonFocusable()));
}

/**
	Constructeur
	@param text Le texte affiche par le champ de texte
	@param parent Le QGraphicsItem parent du champ de texte
	@param parent_diagram Le schema auquel appartient le champ de texte
*/
DiagramTextItem::DiagramTextItem(const QString &text, QGraphicsItem *parent, Diagram *parent_diagram) :
	QGraphicsTextItem(text, parent, parent_diagram),
	previous_text_(text),
	rotation_angle_(0.0)
{
	//set Zvalue at 10 to be upper than the DiagramImageItem
	setZValue(10);
	setDefaultTextColor(Qt::black);
	setFont(QETApp::diagramTextsFont());
	setFlags(QGraphicsItem::ItemIsSelectable|QGraphicsItem::ItemIsMovable);
	setNoEditable(false);
#if QT_VERSION >= 0x040600
	setFlag(QGraphicsItem::ItemSendsGeometryChanges, true);
#endif
	connect(this, SIGNAL(lostFocus()), this, SLOT(setNonFocusable()));
}

/// Destructeur
DiagramTextItem::~DiagramTextItem() {
}

/**
	@return le Diagram auquel ce texte appartient, ou 0 si ce texte n'est
	rattache a aucun schema
*/
Diagram *DiagramTextItem::diagram() const {
	return(qobject_cast<Diagram *>(scene()));
}

/**
	@return l'angle de rotation actuel de ce texte
*/
qreal DiagramTextItem::rotationAngle() const {
	return(rotation_angle_);
}

/**
	Permet de tourner le texte a un angle donne de maniere absolue.
	Un angle de 0 degres correspond a un texte horizontal non retourne.
	@param rotation Nouvel angle de rotation de ce texte
	@see applyRotation
*/
void DiagramTextItem::setRotationAngle(const qreal &rotation) {
	qreal applied_rotation = QET::correctAngle(rotation);
	applyRotation(applied_rotation - rotation_angle_);
	rotation_angle_ = applied_rotation;
}

/**
	Permet de tourner le texte de maniere relative.
	L'angle added_rotation est ajoute a l'orientation actuelle du texte.
	@param added_rotation Angle a ajouter a la rotation actuelle
	@see applyRotation
*/
void DiagramTextItem::rotateBy(const qreal &added_rotation) {
	qreal applied_added_rotation = QET::correctAngle(added_rotation);
	rotation_angle_ = QET::correctAngle(rotation_angle_ + applied_added_rotation);
	applyRotation(applied_added_rotation);
}

/**
	Traduit en coordonnees de la scene un mouvement / vecteur initialement
	exprime en coordonnees locales.
	@param movement Vecteur exprime en coordonnees locales
	@return le meme vecteur, exprime en coordonnees de la scene
*/
QPointF DiagramTextItem::mapMovementToScene(const QPointF &movement) const {
	// on definit deux points en coordonnees locales
	QPointF local_origin(0.0, 0.0);
	QPointF local_movement_point(movement);
	
	// on les mappe sur la scene
	QPointF scene_origin(mapToScene(local_origin));
	QPointF scene_movement_point(mapToScene(local_movement_point));
	
	// on calcule le vecteur represente par ces deux points
	return(scene_movement_point - scene_origin);
}

/**
	Traduit en coordonnees locales un mouvement / vecteur initialement
	exprime en coordonnees de la scene.
	@param movement Vecteur exprime en coordonnees de la scene
	@return le meme vecteur, exprime en coordonnees locales
*/
QPointF DiagramTextItem::mapMovementFromScene(const QPointF &movement) const {
	// on definit deux points sur la scene
	QPointF scene_origin(0.0, 0.0);
	QPointF scene_movement_point(movement);
	
	// on les mappe sur ce QGraphicsItem
	QPointF local_origin(mapFromScene(scene_origin));
	QPointF local_movement_point(mapFromScene(scene_movement_point));
	
	// on calcule le vecteur represente par ces deux points
	return(local_movement_point - local_origin);
}

/**
	Traduit en coordonnees de l'item parent un mouvement / vecteur initialement
	exprime en coordonnees locales.
	@param movement Vecteur exprime en coordonnees locales
	@return le meme vecteur, exprime en coordonnees du parent
*/
QPointF DiagramTextItem::mapMovementToParent(const QPointF &movement) const {
	// on definit deux points en coordonnees locales
	QPointF local_origin(0.0, 0.0);
	QPointF local_movement_point(movement);
	
	// on les mappe sur la scene
	QPointF parent_origin(mapToParent(local_origin));
	QPointF parent_movement_point(mapToParent(local_movement_point));
	
	// on calcule le vecteur represente par ces deux points
	return(parent_movement_point - parent_origin);
}

/**
	Traduit en coordonnees locales un mouvement / vecteur initialement
	exprime en coordonnees du parent.
	@param movement Vecteur exprime en coordonnees du parent
	@return le meme vecteur, exprime en coordonnees locales
*/
QPointF DiagramTextItem::mapMovementFromParent(const QPointF &movement) const {
	// on definit deux points sur le parent
	QPointF parent_origin(0.0, 0.0);
	QPointF parent_movement_point(movement);
	
	// on les mappe sur ce QGraphicsItem
	QPointF local_origin(mapFromParent(parent_origin));
	QPointF local_movement_point(mapFromParent(parent_movement_point));
	
	// on calcule le vecteur represente par ces deux points
	return(local_movement_point - local_origin);
}

void DiagramTextItem::setFontSize(int &s) {
	setFont(QETApp::diagramTextsFont(s));
}

/**
	Dessine le champ de texte.
	Cette methode delegue simplement le travail a QGraphicsTextItem::paint apres
	avoir desactive l'antialiasing.
	@param painter Le QPainter a utiliser pour dessiner le champ de texte
	@param option Les options de style pour le champ de texte
	@param widget Le QWidget sur lequel on dessine 
*/
void DiagramTextItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) {
	painter -> setRenderHint(QPainter::Antialiasing, false);
	QGraphicsTextItem::paint(painter, option, widget);
}

/**
	Gere la prise de focus du champ de texte
	@param e Objet decrivant la prise de focus
*/
void DiagramTextItem::focusInEvent(QFocusEvent *e) {
	QGraphicsTextItem::focusInEvent(e);
	
	// empeche le deplacement du texte pendant son edition
	setFlag(QGraphicsItem::ItemIsMovable, false);
	
	// memorise le texte avant que l'utilisateur n'y touche
	previous_text_ = toHtml();
	// cela permettra de determiner si l'utilisateur a modifie le texte a la fin de l'edition
}

/**
	Gere la perte de focus du champ de texte
	@param e Objet decrivant la perte de focus
*/
void DiagramTextItem::focusOutEvent(QFocusEvent *e) {
	QGraphicsTextItem::focusOutEvent(e);
	
	// signale la modification du texte si besoin
	if (toPlainText() != previous_text_) {
		emit(diagramTextChanged(this, previous_text_, toHtml()));
		previous_text_ = toHtml();
	}
	
	// deselectionne le texte
	QTextCursor cursor = textCursor();
	cursor.clearSelection();
	setTextCursor(cursor);
	
	// hack a la con pour etre re-entrant
	setTextInteractionFlags(Qt::NoTextInteraction);
	
	// autorise de nouveau le deplacement du texte
	setFlag(QGraphicsItem::ItemIsMovable, true);
	QTimer::singleShot(0, this, SIGNAL(lostFocus()));
}

/**
	Gere les double-clics sur ce champ de texte.
	@param event un QGraphicsSceneMouseEvent decrivant le double-clic
*/
void DiagramTextItem::mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event) {
	if (!(textInteractionFlags() & Qt::TextEditable) && !no_editable) {
		// rend le champ de texte editable
		setTextInteractionFlags(Qt::TextEditorInteraction);
		
		// edite le champ de texte
		setFocus(Qt::MouseFocusReason);
	} else {
		QGraphicsTextItem::mouseDoubleClickEvent(event);
	}
}

/**
 * @brief DiagramTextItem::mousePressEvent
 * @param event
 */
void DiagramTextItem::mousePressEvent (QGraphicsSceneMouseEvent *event) {
	m_first_move = true;
	if (event -> modifiers() & Qt::ControlModifier) {
		setSelected(!isSelected());
	}
	QGraphicsTextItem::mousePressEvent(event);
}

/**
 * @brief DiagramTextItem::mouseMoveEvent
 * @param event
 */
void DiagramTextItem::mouseMoveEvent(QGraphicsSceneMouseEvent *event) {
	if (textInteractionFlags() & Qt::TextEditable) QGraphicsTextItem::mouseMoveEvent(event);

	else if ((flags() & QGraphicsItem::ItemIsMovable) && (event -> buttons() & Qt::LeftButton)) {
		Diagram *diagram_ = diagram();

		//This is first move, we signal it to parent diagram
		if(diagram_ && m_first_move)
			diagram_ -> beginMoveElements(this);

		QPointF old_pos = pos();

		//Save the pos of item at the beggining of the movement
		if (m_first_move)
			m_mouse_to_origin_movement = old_pos - event->buttonDownScenePos(Qt::LeftButton);

		//Set the actual pos
		setPos(event->scenePos() + m_mouse_to_origin_movement);

		//Update the actual movement for other selected item
		if (diagram_)
			diagram_ -> continueMoveElements(pos() - old_pos);
	}

	else event -> ignore();

	m_first_move = false;
}

/**
 * @brief DiagramTextItem::mouseReleaseEvent
 * @param event
 */
void DiagramTextItem::mouseReleaseEvent (QGraphicsSceneMouseEvent *event) {
	//Signal to diagram movement is finish
	if (diagram())
		diagram() -> endMoveElements();

	if (!(event -> modifiers() & Qt::ControlModifier))
		QGraphicsTextItem::mouseReleaseEvent(event);
}

/**
	Effectue la rotation du texte en elle-meme
	Pour les DiagramTextItem, la rotation s'effectue autour du point (0, 0).
	Cette methode peut toutefois etre redefinie dans des classes filles
	@param angle Angle de la rotation a effectuer
*/
void DiagramTextItem::applyRotation(const qreal &angle) {
	setRotation(QET::correctAngle(rotation()+angle));
}

/**
	Change la position du champ de texte en veillant a ce qu'il
	reste sur la grille du schema auquel il appartient.
	@param p Nouvelles coordonnees de l'element
*/
void DiagramTextItem::setPos(const QPointF &p) {
	if (p == pos()) return;
	// pas la peine de positionner sur la grille si l'element n'est pas sur un Diagram
	if (scene()) {
		// arrondit l'abscisse a 10 px pres
		int p_x = qRound(p.x() / (Diagram::xGrid * 1.0)) * Diagram::xGrid;
		// arrondit l'ordonnee a 10 px pres
		int p_y = qRound(p.y() / (Diagram::yGrid * 1.0)) * Diagram::yGrid;
		QGraphicsTextItem::setPos(p_x, p_y);
	} else QGraphicsTextItem::setPos(p);
}

/**
	Change la position du champ de texte en veillant a ce que l'il
	reste sur la grille du schema auquel il appartient.
	@param x Nouvelle abscisse de l'element
	@param y Nouvelle ordonnee de l'element
*/
void DiagramTextItem::setPos(qreal x, qreal y) {
	setPos(QPointF(x, y));
}

/// Rend le champ de texte non focusable
void DiagramTextItem::setNonFocusable() {
	setFlag(QGraphicsTextItem::ItemIsFocusable, false);
}


/**
 * @brief DiagramTextItem::setHtmlText
 * @param txt
 */
void DiagramTextItem::setHtmlText(const QString &txt) {
	setHtml( txt );
}

/**
 * @brief Edit the text with HtmlEditor
 */
void DiagramTextItem::edit() {
	//Open the HtmlEditor
	qdesigner_internal::RichTextEditorDialog *editor = new qdesigner_internal::RichTextEditorDialog();
	// connect the in/out
	connect(editor, SIGNAL(applyEditText(const QString &)), this, SLOT(setHtmlText(const QString &)));
	// load the Html txt
	editor->setText( toHtml() );
	// show
	editor->show();
}

