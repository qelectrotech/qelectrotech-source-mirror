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
 * @brief DiagramTextItem::DiagramTextItem
 * @param parent : parent item
 */
DiagramTextItem::DiagramTextItem(QGraphicsItem *parent) :
	QGraphicsTextItem(parent)
{ build(); }

/**
 * @brief DiagramTextItem::DiagramTextItem
 * @param text : text to display
 * @param parent : parent item
 */
DiagramTextItem::DiagramTextItem(const QString &text, QGraphicsItem *parent) :
	QGraphicsTextItem(text, parent),
	m_mouse_hover(false),
	m_previous_html_text(text)
{ build(); }

/**
 * @brief DiagramTextItem::build
 * Build this item with default value
 */
void DiagramTextItem::build()
{
		//set Zvalue at 10 to be upper than the DiagramImageItem
	setZValue(10);
	setAcceptHoverEvents(true);
	setDefaultTextColor(Qt::black);
	setFont(QETApp::diagramTextsItemFont());
	setFlags(QGraphicsItem::ItemIsSelectable|QGraphicsItem::ItemIsMovable | QGraphicsItem::ItemSendsGeometryChanges);
	setNoEditable(false);
	setToolTip(tr("Maintenir ctrl pour un déplacement libre"));
}

/**
 * @brief DiagramTextItem::diagram
 * @return The diagram of this item or 0 if this text isn't in a diagram
 */
Diagram *DiagramTextItem::diagram() const {
	return(qobject_cast<Diagram *>(scene()));
}

/**
 * @brief DiagramTextItem::toXml
 * This method do nothing and return an empty DomElement
 * This is used to be inherited by child class
 * @return
 */
QDomElement DiagramTextItem::toXml(QDomDocument &) const {
	return QDomElement();
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

void DiagramTextItem::setFontSize(int s)
{
	prepareAlignment();
    setFont(QETApp::diagramTextsFont(s));
	finishAlignment();
	emit fontSizeChanged(s);
}

int DiagramTextItem::fontSize() const
{
    return font().pointSize();
}

void DiagramTextItem::setColor(QColor color)
{
    setDefaultTextColor(color);
	emit colorChanged(color);
}

QColor DiagramTextItem::color() const {
	return defaultTextColor();
}

void DiagramTextItem::setAlignment(const Qt::Alignment &alignment)
{
	m_alignment = alignment;
	emit alignmentChanged(alignment);
}

Qt::Alignment DiagramTextItem::alignment() const
{
	return m_alignment;
}

/**
 * @brief DiagramTextItem::paint
 * Draw this text field. This method draw the text by calling QGraphicsTextItem::paint.
 * If text is hovered, this method draw the bounding rect in grey
 * @param painter : painter to use
 * @param option : style option
 * @param widget : widget where must to draw
 */
void DiagramTextItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
	painter -> setRenderHint(QPainter::Antialiasing, false);
	QGraphicsTextItem::paint(painter, option, widget);

	if (m_mouse_hover && !isSelected())
	{
		painter -> save();

			//Disable renderhints
		painter -> setRenderHint(QPainter::Antialiasing,          false);
		painter -> setRenderHint(QPainter::TextAntialiasing,      false);
		painter -> setRenderHint(QPainter::SmoothPixmapTransform, false);

			//Draw the selected rect in grey
		QPen t;
		t.setColor(Qt::gray);
		t.setStyle(Qt::DashDotLine);
		t.setCosmetic(true);
		painter -> setPen(t);
		painter -> drawRoundRect(boundingRect(), 10, 10);

		painter -> restore();
	}

}

/**
 * @brief DiagramTextItem::focusInEvent
 * @param e
 */
void DiagramTextItem::focusInEvent(QFocusEvent *event)
{
	QGraphicsTextItem::focusInEvent(event);

	setFlag(QGraphicsItem::ItemIsMovable, false);
	
	m_previous_html_text = toHtml();
	m_previous_text = toPlainText();
}

/**
 * @brief DiagramTextItem::focusOutEvent
 * @param event
 */
void DiagramTextItem::focusOutEvent(QFocusEvent *event)
{
	QGraphicsTextItem::focusOutEvent(event);

	if (toHtml() != m_previous_html_text)
		emit(diagramTextChanged(this, m_previous_html_text, toHtml()));
	if(toPlainText() != m_previous_text)
		emit textEdited(m_previous_text, toPlainText());
	
	QTextCursor cursor = textCursor();
	cursor.clearSelection();
	setTextCursor(cursor);

		//Bad hack to be re-entrant
	setTextInteractionFlags(Qt::NoTextInteraction);

	setFlag(QGraphicsItem::ItemIsMovable, true);
	setFlag(QGraphicsTextItem::ItemIsFocusable, false);
}

/**
	Gere les double-clics sur ce champ de texte.
	@param event un QGraphicsSceneMouseEvent decrivant le double-clic
*/
void DiagramTextItem::mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event) {
	if (!(textInteractionFlags() & Qt::TextEditable) && !m_no_editable) {
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
	//Save the pos of item at the beggining of the movement
	m_mouse_to_origin_movement = pos() - event->scenePos();
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
			diagram_->elementsMover().beginMovement(diagram_, this);

		QPointF old_pos = pos();

		//Set the actual pos
		QPointF new_pos = event->scenePos() + m_mouse_to_origin_movement;
		event->modifiers() == Qt::ControlModifier ? setPos(new_pos) : setPos(Diagram::snapToGrid(new_pos));


		//Update the actual movement for other selected item
		if (diagram_)
			diagram_->elementsMover().continueMovement(pos() - old_pos);
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
		diagram()->elementsMover().endMovement();

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
 * @brief DiagramTextItem::prepareAlignment
 * Call this function before changing the bounding rect of this text.
 */
void DiagramTextItem::prepareAlignment()
{
	m_alignment_rect = boundingRect();
}

/**
 * @brief DiagramTextItem::finishAlignment
 * Call this function after changing the bouding rect of this text
 * to set the position of this text according to the alignment property.
 */
void DiagramTextItem::finishAlignment()
{
	if(m_block_alignment)
		return;
	
	QTransform transform;
	transform.rotate(this->rotation());
	qreal x,xa, y,ya;
	x=xa=0;
	y=ya=0;

	if(m_alignment &Qt::AlignRight)
	{
		x = m_alignment_rect.right();
		xa = boundingRect().right();
	}
	else if(m_alignment &Qt::AlignHCenter)
	{
		x = m_alignment_rect.center().x();
		xa = boundingRect().center().x();
	}
	
	if(m_alignment &Qt::AlignBottom)
	{
		y = m_alignment_rect.bottom();
		ya = boundingRect().bottom();
	}
	else if(m_alignment &Qt::AlignVCenter)
	{
		y = m_alignment_rect.center().y();
		ya = boundingRect().center().y();
	}

	QPointF p = transform.map(QPointF(x,y));
	QPointF pa = transform.map(QPointF(xa,ya));
	QPointF diff = pa-p;
	
	setPos(this->pos() - diff);
}

/**
 * @brief Edit the text with HtmlEditor
 */
void DiagramTextItem::edit()
{
	QWidget *parent = nullptr;
	if (scene() && scene()->views().size())
		parent = scene()->views().first();

	qdesigner_internal::RichTextEditorDialog editor(parent);
	connect(&editor, &qdesigner_internal::RichTextEditorDialog::applyEditText, [this](QString text) {this->setHtml(text);});
	editor.setText(toHtml());
	editor.exec();
}



/**
	When mouse over element
	change m_mouse_hover to true   (used in paint() function )
	@param e QGraphicsSceneHoverEvent
*/
void DiagramTextItem::hoverEnterEvent(QGraphicsSceneHoverEvent *e) {
	Q_UNUSED(e);

	m_mouse_hover = true;
	QString str_ToolTip = toPlainText();
	setToolTip( str_ToolTip );
	update();
}

/**
	When mouse over element leave the position
	change m_mouse_hover to false (used in paint() function )
	@param e QGraphicsSceneHoverEvent
*/
void DiagramTextItem::hoverLeaveEvent(QGraphicsSceneHoverEvent *e) {
	Q_UNUSED(e);
	//qDebug() << "Leave mouse over";
	m_mouse_hover = false;
	update();
}

/**
	Do nothing default function .
	@param e QGraphicsSceneHoverEvent
*/
void DiagramTextItem::hoverMoveEvent(QGraphicsSceneHoverEvent *e) {
	Q_UNUSED(e);
	QGraphicsTextItem::hoverMoveEvent(e);
}
