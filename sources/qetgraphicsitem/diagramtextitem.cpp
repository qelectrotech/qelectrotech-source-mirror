/*
	Copyright 2006-2026 QElectroTech Team
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
#include "../ui/labelclearance24.h"
#include "diagramtextitem.h"
#include "../ui/textgeometry23.h"
#include "../qetproject.h"
#include "../ui/languagetext16.h"
#include "../ui/languageeditor21.h"
#include <QJsonDocument>

#include "../diagram.h"
#include "../diagramcommands.h"
#include "../qetapp.h"
#include "../richtext/richtexteditor_p.h"
#include "../textanchor.h"
#include "../utils/qetutils.h"
#include "../QetGraphicsItemModeler/textresizehandles.h"

#include <QAbstractTextDocumentLayout>
#include <QStyleOptionGraphicsItem>
#include <QTextBlock>
#include <QTextLayout>

/**
	@brief DiagramTextItem::DiagramTextItem
	@param parent : parent item
*/
DiagramTextItem::DiagramTextItem(QGraphicsItem *parent) :
	QGraphicsTextItem(parent)
{ build(); }

/**
	@brief DiagramTextItem::DiagramTextItem
	@param text : text to display
	@param parent : parent item
*/
DiagramTextItem::DiagramTextItem(const QString &text, QGraphicsItem *parent) :
	QGraphicsTextItem(text, parent),
	m_mouse_hover(false),
	m_previous_html_text(text)
{ build(); }

/**
	@brief DiagramTextItem::build
	Build this item with default value
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
	setToolTip(tr("Shift-click to add text to the selection; Ctrl+Shift-click to remove it. Hold Ctrl while dragging for free movement."));
    connect(document(),&QTextDocument::contentsChanged,this,&DiagramTextItem::refreshLanguageSiblings23);
    connect(document(),&QTextDocument::contentsChanged,this,[this](){QetLabelClearance24::notify(this);});
    connect(this,&DiagramTextItem::fontChanged,this,[this](){QetLabelClearance24::notify(this);});
}

/**
	@brief DiagramTextItem::diagram
	@return The diagram of this item or 0 if this text isn't in a diagram
*/
Diagram *DiagramTextItem::diagram() const
{
	return(qobject_cast<Diagram *>(scene()));
}

/**
	@brief DiagramTextItem::toXml
	This method do nothing and return an empty DomElement
	This is used to be inherited by child class
	@return
*/
QDomElement DiagramTextItem::toXml(QDomDocument &) const
{
	return QDomElement();
}

/**
	Traduit en coordonnees de la scene un mouvement / vecteur initialement
	exprime en coordonnees locales.
	@param movement Vecteur exprime en coordonnees locales
	@return le meme vecteur, exprime en coordonnees de la scene
*/
QPointF DiagramTextItem::mapMovementToScene(const QPointF &movement) const
{
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
QPointF DiagramTextItem::mapMovementFromScene(const QPointF &movement) const
{
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
QPointF DiagramTextItem::mapMovementToParent(const QPointF &movement) const
{
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
QPointF DiagramTextItem::mapMovementFromParent(const QPointF &movement) const
{
	// on definit deux points sur le parent
	QPointF parent_origin(0.0, 0.0);
	QPointF parent_movement_point(movement);

	// on les mappe sur ce QGraphicsItem
	QPointF local_origin(mapFromParent(parent_origin));
	QPointF local_movement_point(mapFromParent(parent_movement_point));

	// on calcule le vecteur represente par ces deux points
	return(local_movement_point - local_origin);
}

void DiagramTextItem::setFont(const QFont &font)
{
	if (this->font() == font) {
		return;
	}
	else
	{
		prepareAlignment();
		QGraphicsTextItem::setFont(font);
        refreshLanguageSiblings23();
		finishAlignment();
		emit fontChanged(font);
	}
}

void DiagramTextItem::setColor(const QColor& color)
{
	setDefaultTextColor(color);
	emit colorChanged(color);
}

QColor DiagramTextItem::color() const
{
	return defaultTextColor();
}

void DiagramTextItem::setAlignment(const Qt::Alignment &alignment)
{
	if (m_alignment == alignment) return;
	m_alignment = alignment;
	emit alignmentChanged(alignment);
    refreshLanguageSiblings23();
}

Qt::Alignment DiagramTextItem::alignment() const
{
	return m_alignment;
}

/**
	@brief DiagramTextItem::anchorPos
	@return the anchor point of this text, in parent coordinates:
	the point of the text chosen by the alignment (top-left, right edge,
	centre...). For a top-left aligned text this is pos().
	The anchor stays in place when the text changes, see finishAlignment().
*/
QPointF DiagramTextItem::anchorPos() const
{
	return TextAnchor::pos(this, m_alignment);
}

/**
	@brief DiagramTextItem::setAnchorPos
	Move this text so that its anchor point (see anchorPos()) is at anchor.
	@param anchor : in parent coordinates
*/
void DiagramTextItem::setAnchorPos(const QPointF &anchor)
{
	setPos(TextAnchor::itemPosFor(this, m_alignment, anchor));
}

/**
	@brief DiagramTextItem::setAlignmentAtAnchor
	Change the alignment and move the text so that its new anchor point
	is where the previous one was.
	@param alignment
*/
void DiagramTextItem::setAlignmentAtAnchor(const Qt::Alignment &alignment)
{
	const QPointF anchor = anchorPos();
	setAlignment(alignment);
	setAnchorPos(anchor);
}

/**
	@brief DiagramTextItem::frameRect
	@return the rect used to draw a frame around this text
*/
QRectF DiagramTextItem::frameRect() const
{	
		//Get the bounding rectangle of the text
	if(languageHidden21())return {};
	const QTextDocument *display = languageDocument16();
	if (!display) display = document();
	QSizeF size = display->size();
	size.setWidth(display->idealWidth());
		//Remove the margin. Size is exactly the bounding rect of the text
	size.rheight() -= display->documentMargin()*2;
	size.rwidth() -= display->documentMargin()*2;
		//Add a little margin only for a better visual;
	size.rheight() += 2;
	size.rwidth() += 2;
	
		//The pos of the rect
	QPointF pos = boundingRect().center();
	pos.rx() -= size.width()/2;
	pos.ry() -= size.height()/2;
	
	return QRectF(pos, size);
}

/**
	@brief DiagramTextItem::writtenRect
	@return the rect around the lines actually written, in this item's
	coordinates: narrower than boundingRect() for a text kept at a fixed
	width, and placed as the text is aligned. A text with nothing written
	gives its bounding rect, so that it can still be picked.
*/
QRectF DiagramTextItem::writtenRect() const
{
    if (hasLanguageDisplay16()) return boundingRect();
	QRectF written;
	QAbstractTextDocumentLayout *layout = document()->documentLayout();
	for (QTextBlock block = document()->begin(); block.isValid(); block = block.next())
	{
		const QTextLayout *lines = block.layout();
		if (!lines)
			continue;
		const QPointF origin = layout->blockBoundingRect(block).topLeft();
		for (int i = 0; i < lines->lineCount(); ++i)
		{
			const QRectF line = lines->lineAt(i).naturalTextRect().translated(origin);
			if (line.width() > 0)
				written |= line;
		}
	}
	if (written.isEmpty())
		return boundingRect();
		//A little room around the letters, as the frame keeps
	return written.adjusted(-1, -1, 1, 1) & boundingRect();
}

/**
	@brief DiagramTextItem::shape
	@return the lines written as the place to click: the empty part of a
	fixed-width text belongs to whatever lies beneath it. While editing,
	the whole box, so that a click anywhere in it moves the cursor.
*/
QPainterPath DiagramTextItem::shape() const
{
	QPainterPath path;
	path.addRect(isEditing() ? boundingRect() : writtenRect());
	return path;
}

/**
	@brief DiagramTextItem::contains
	QGraphicsTextItem answers from its whole box: answer from shape(), so
	that hovering and picking at a point follow it too.
*/
bool DiagramTextItem::contains(const QPointF &point) const
{
	return shape().contains(point);
}

void DiagramTextItem::setHtml(const QString &text)
{
	QGraphicsTextItem::setHtml(text);
	m_is_html = true;

	m_non_left_alignment = false;
	QTextBlock block = document()->begin();
	while (block.isValid()) {
		Qt::Alignment a = block.blockFormat().alignment();
		if (a & Qt::AlignHCenter || a & Qt::AlignRight || a & Qt::AlignJustify) {
			m_non_left_alignment = true;
			break;
		}
		block = block.next();
	}

		//Room for centred or right-aligned lines, unless the user has
		//given this text a width of its own
	if (m_non_left_alignment && !hasUserTextWidth()) {
		document()->setTextWidth(document()->idealWidth() + 40.0);
	}
}

void DiagramTextItem::setPlainText(const QString &text)
{
	QGraphicsTextItem::setPlainText(text);
	m_is_html = false;
}

bool DiagramTextItem::isHtml() const
{
	return m_is_html;
}

/**
	@brief DiagramTextItem::paint
	Draw this text field. This method draws the text by calling QGraphicsTextItem::paint.
	If text is hovered, this method draws the bounding rect in grey
	@param painter : painter to use
	@param option : style option
	@param widget : widget that is drawn to
*/
void DiagramTextItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
	painter -> setRenderHint(QPainter::Antialiasing, false);
    if(languageHidden21())return;
    if (const auto *translated = languageDocument16()) {
        painter->save();QAbstractTextDocumentLayout::PaintContext context;
        context.palette.setColor(QPalette::Text,defaultTextColor());
        painter->translate(languageOffset22());translated->documentLayout()->draw(painter,context);painter->translate(-languageOffset22());
        if ((option && (option->state & QStyle::State_Selected)) || m_mouse_hover) {
            QPen pen(isSelected()?Qt::black:Qt::gray,0,Qt::DashLine);pen.setCosmetic(true);painter->setPen(pen);painter->setBrush(Qt::NoBrush);painter->drawRect(boundingRect());
        }
        painter->restore();return;
    }

		//Outside editing, the selection is drawn around the lines written,
		//not around the whole width a fixed-width text keeps free
	const bool outline = option && (option->state & QStyle::State_Selected) && !isEditing();
	if (outline)
	{
		QStyleOptionGraphicsItem plain(*option);
		plain.state &= ~(QStyle::State_Selected | QStyle::State_HasFocus);
		QGraphicsTextItem::paint(painter, &plain, widget);

		painter -> save();
		painter -> setBrush(Qt::NoBrush);
		QPen pen(Qt::black, 0, Qt::DashLine);
		pen.setCosmetic(true);
		painter -> setPen(pen);
		painter -> drawRect(writtenRect());
		painter -> restore();
	}
	else
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
		painter -> drawRoundedRect(writtenRect(), 2, 2);

		painter -> restore();
	}

}

/**
	@brief DiagramTextItem::focusInEvent
	@param event
*/
void DiagramTextItem::focusInEvent(QFocusEvent *event)
{
	QGraphicsTextItem::focusInEvent(event);

	setFlag(QGraphicsItem::ItemIsMovable, false);
	
	m_previous_html_text = toHtml();
	m_previous_text = toPlainText();
	refreshTextResizeHandles();
}

/**
	@brief DiagramTextItem::focusOutEvent
	@param event
*/
void DiagramTextItem::focusOutEvent(QFocusEvent *event)
{
	QGraphicsTextItem::focusOutEvent(event);

	if(toPlainText() != m_previous_text)
		emit textEdited(m_previous_text, toPlainText());
	
	QTextCursor cursor = textCursor();
	cursor.clearSelection();
	setTextCursor(cursor);

		//Bad hack to be re-entrant
	setTextInteractionFlags(Qt::NoTextInteraction);

	setFlag(QGraphicsItem::ItemIsMovable, true);
	setFlag(QGraphicsTextItem::ItemIsFocusable, false);
	refreshTextResizeHandles();
}

/**
	@brief DiagramTextItem::itemChange
	Show or hide the resize handles with the selection and the scene.
	@param change
	@param value
	@return
*/
QVariant DiagramTextItem::itemChange(GraphicsItemChange change, const QVariant &value)
{
    if(change==ItemPositionHasChanged||change==ItemRotationHasChanged||change==ItemTransformHasChanged||change==ItemSceneHasChanged)QetLabelClearance24::notify(this);
    if (change == QGraphicsItem::ItemSceneHasChanged) {
        QObject::disconnect(m_language_connection16);
        if (auto *folio=diagram()) if (auto *project=folio->project())
            m_language_connection16=connect(project,&QETProject::languageDataChanged,this,&DiagramTextItem::refreshLanguage16);
        refreshLanguage16();
    }
    if(change==QGraphicsItem::ItemParentChange)refreshLanguageSiblings23();
    if(change==QGraphicsItem::ItemParentHasChanged || change==QGraphicsItem::ItemRotationHasChanged ||
       (change==QGraphicsItem::ItemPositionHasChanged && !isTranslationMode21()))refreshLanguageSiblings23();
		//Also when the text comes back to a scene: Qt keeps it selected
		//across removeItem()/addItem() (undoing a delete) and sends no
		//selection change, and its handles were removed with the scene.
	if (change == QGraphicsItem::ItemSelectedHasChanged
		|| change == QGraphicsItem::ItemSceneHasChanged)
		refreshTextResizeHandles();

	return QGraphicsTextItem::itemChange(change, value);
}

/**
	@brief DiagramTextItem::textResizeHandlesWanted
	@return true when the corner handles to change the width of this text
	should be shown. A text without a "textWidth" property never has them.
*/
bool DiagramTextItem::textResizeHandlesWanted() const
{
	return false;
}

/**
	@brief DiagramTextItem::isEditing
	@return true while the text itself is being typed in
*/
bool DiagramTextItem::isEditing() const
{
	return textInteractionFlags() & Qt::TextEditable;
}

/**
	@brief DiagramTextItem::refreshTextResizeHandles
	Create or remove the corner handles to change the width of this text,
	according to textResizeHandlesWanted().
	Called from itemChange() and when the edition starts or ends, not from
	paint(): moving items from paint() left fragments behind
	(qelectrotech#1002).
*/
void DiagramTextItem::refreshTextResizeHandles()
{
	const bool wanted = scene() && !isTranslationMode21() && textResizeHandlesWanted();
	if (wanted && !m_resize_handles)
	{
		m_resize_handles = new TextResizeHandles(this, QETUtils::graphicsHandlerSize(this));
		connect(m_resize_handles, &TextResizeHandles::resizeFinished,
				this, &DiagramTextItem::pushResizeCommand);
	}
	else if (!wanted && m_resize_handles) {
		removeTextResizeHandles();
	}
}

/**
	@brief DiagramTextItem::removeTextResizeHandles
*/
void DiagramTextItem::removeTextResizeHandles()
{
	delete m_resize_handles;
	m_resize_handles = nullptr;
}

/**
	@brief DiagramTextItem::pushResizeCommand
	Make the width change done with the resize handles undoable. The change
	is already applied, live during the drag.
*/
void DiagramTextItem::pushResizeCommand(qreal old_width, qreal new_width,
										QPointF old_pos, QPointF new_pos)
{
	if (Diagram *diagram_ = diagram())
		diagram_->undoStack().push(new TextResizeCommand(this, old_width, new_width,
														 old_pos, new_pos));
}

/**
	Gere les double-clics sur ce champ de texte.
	@param event un QGraphicsSceneMouseEvent decrivant le double-clic
*/
void DiagramTextItem::mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event) {
    if(isTranslationMode21()){QetLanguageEditor21::editText(this,event->widget());event->accept();return;}
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
	@brief DiagramTextItem::mousePressEvent
	@param event
*/
void DiagramTextItem::mousePressEvent (QGraphicsSceneMouseEvent *event)
{
	m_additive_text_press25 = false;
	if (event->button() == Qt::LeftButton)
	{
		m_first_move = true;
			//Save the pos of item at the beggining of the movement
		m_mouse_to_origin_movement = pos() - event->scenePos();
		if (event->modifiers().testFlag(Qt::ShiftModifier)
			&& !(textInteractionFlags() & Qt::TextEditable)
			&& (flags() & ItemIsSelectable)) {
			m_additive_text_press25 = true;
			m_toggle_text25 = event->modifiers().testFlag(Qt::ControlModifier) && isSelected();
			setSelected(true);
			event->accept();
			return;
		}
	}
	QGraphicsTextItem::mousePressEvent(event);
}

/**
	@brief DiagramTextItem::mouseMoveEvent
	@param event
*/
void DiagramTextItem::mouseMoveEvent(QGraphicsSceneMouseEvent *event) {
	if (textInteractionFlags() & Qt::TextEditable) QGraphicsTextItem::mouseMoveEvent(event);

	else if ((flags() & QGraphicsItem::ItemIsMovable) && (event -> buttons() & Qt::LeftButton)) {
		Diagram *diagram_ = diagram();

		//This is first move, we signal it to parent diagram
		if(diagram_ && m_first_move)
			diagram_->elementsMover().beginMovement(diagram_, this);

		if (diagram_ && diagram_->elementsMover().holds(this)) {
			m_first_move = false;
			event->accept();
			return;
		}

		QPointF old_pos = pos();

		//Set the actual pos
		QPointF new_pos = event->scenePos() + m_mouse_to_origin_movement;
		event->modifiers() == Qt::ControlModifier ? setPos(new_pos) : setPos(Diagram::snapToTextGrid(new_pos));


		//Update the actual movement for other selected item
		if (diagram_)
			diagram_->elementsMover().continueMovement(pos() - old_pos);
	}

	else event -> ignore();

	m_first_move = false;
}

/**
	@brief DiagramTextItem::mouseReleaseEvent
	@param event
*/
void DiagramTextItem::mouseReleaseEvent (QGraphicsSceneMouseEvent *event)
{
		//Signal to diagram movement is finish
	if (diagram() && (event->button() == Qt::LeftButton))
	{
		diagram()->elementsMover().endMovement();
		event->accept();
		if (finishAdditiveSelection25(event)) return;
		if (event->buttonDownScenePos(Qt::LeftButton) != event->scenePos()) {
			return;
		}
	}
	if (event->modifiers() & Qt::ControlModifier && (event->button() == Qt::LeftButton))
	{
		setSelected(!isSelected());
		event->accept();
	}
	else {
		QGraphicsTextItem::mouseReleaseEvent(event);
	}	
}

bool DiagramTextItem::finishAdditiveSelection25(QGraphicsSceneMouseEvent *event)
{
	if (!m_additive_text_press25 || event->button() != Qt::LeftButton) return false;
	if (m_toggle_text25 && m_first_move) setSelected(false);
	m_additive_text_press25 = false;
	event->accept();
	return true;
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
	@brief DiagramTextItem::prepareAlignment
	Call this function before changing the bounding rect of this text.
*/
void DiagramTextItem::prepareAlignment()
{
	m_alignment_rect = boundingRect();
}

/**
	@brief DiagramTextItem::finishAlignment
	Call this function after changing the bounding rect of this text
	to set the position of this text according to the alignment property.
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
	@brief Edit the text with HtmlEditor
*/
void DiagramTextItem::edit()
{
    if(isTranslationMode21()){QetLanguageEditor21::editText(this,scene()&&!scene()->views().isEmpty()?scene()->views().first():nullptr);return;}
	QWidget *parent = nullptr;
	if (scene() && scene()->views().size())
		parent = scene()->views().first();

	qdesigner_internal::RichTextEditorDialog editor(parent);
	editor.setText(toHtml());

	QVector<Qt::Alignment> alignments;
	connect(&editor, &qdesigner_internal::RichTextEditorDialog::applyEditText,
			this, [this, &editor, &alignments](const QString &text) {
		alignments = editor.blockAlignments();
		this->setHtml(text);
	});
	editor.exec();

	if (!alignments.isEmpty()) {
		QTextCursor cursor(document());
		cursor.beginEditBlock();
		QTextBlock block = document()->begin();
		int i = 0;
		while (block.isValid() && i < alignments.size()) {
			Qt::Alignment align = alignments.at(i);
			if (align != Qt::AlignLeft && align != Qt::Alignment()) {
				QTextBlockFormat fmt;
				fmt.setAlignment(align);
				cursor.setPosition(block.position());
				cursor.setBlockFormat(fmt);
			}
			block = block.next();
			i++;
		}
		cursor.endEditBlock();
	}
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
	
	// Add movement instruction for DynamicElementTextItem
	if (inherits("DynamicElementTextItem")) {
		str_ToolTip += tr("\nHold Shift and drag to move this text on its own");
	}
	
	setToolTip(str_ToolTip);
	update();
}

/**
	When mouse over element leave the position
	change m_mouse_hover to false (used in paint() function )
	@param e QGraphicsSceneHoverEvent
*/
void DiagramTextItem::hoverLeaveEvent(QGraphicsSceneHoverEvent *e) {
	Q_UNUSED(e);
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

// Display-only language documents are cached per text/variant/source revision.
void DiagramTextItem::ensureLanguageDocument16() const
{
    if(!m_language_key16.isEmpty())return;
    QVariantMap data;
    if(auto *folio=diagram())if(auto *project=folio->project()) {
        // During XML construction each text receives font, width and position
        // separately. Build its translated layout once, after loading finishes.
        if(project->state()==QETProject::ProjectParsingRunning) {
            m_language_hidden23=false;m_language_offset23={};m_language_alignment23=alignment();return;
        }
        data=project->languageData();
    }
    const QString id=languageId16();
    // Event-driven cache: source edits and project-language changes invalidate
    // it. Rendering no longer serializes HTML/JSON or scans siblings repeatedly.
    m_language_key16=QStringLiteral("ready");
    m_language_variant23=QetLanguageText16::variant(data,id);
    m_language_hidden23=QetLanguages16::hiddenMember21(data,id);
    m_language_document16=QetLanguageText16::document(data,id,document(),font(),property("textWidth").isValid()?property("textWidth").toDouble():-1);
    m_language_offset23=calculateLanguageOffset23(&m_language_alignment23);
    const auto &v=m_language_variant23;
    if(m_language_document16 && !v.contains("align") && !v.contains("alignment") &&
       (m_language_alignment23&Qt::AlignHorizontal_Mask)!=(alignment()&Qt::AlignHorizontal_Mask)) {
        QTextCursor all(m_language_document16.data());all.select(QTextCursor::Document);
        QTextBlockFormat format;format.setAlignment(m_language_alignment23&Qt::AlignHorizontal_Mask);all.mergeBlockFormat(format);
        m_language_offset23=calculateLanguageOffset23(&m_language_alignment23);
    }

}
const QTextDocument *DiagramTextItem::languageDocument16() const
{
    if(isEditing()&&!isTranslationMode21())return nullptr;ensureLanguageDocument16();return m_language_document16.data();
}
bool DiagramTextItem::hasLanguageDisplay16() const {return languageDocument16()!=nullptr;}
QString DiagramTextItem::displayedText16() const {const auto *doc=languageDocument16();return doc?doc->toPlainText():toPlainText();}
QVariantMap DiagramTextItem::languageVariant16() const {ensureLanguageDocument16();return m_language_variant23;}

int DiagramTextItem::languageLineCount16() const {const auto *doc=languageDocument16();return QetLanguageText16::lineCount(doc?doc:document());}
QRectF DiagramTextItem::boundingRect() const {if(languageHidden21())return {};if(const auto *doc=languageDocument16())return QRectF(languageOffset22(),doc->size());return QGraphicsTextItem::boundingRect();}
void DiagramTextItem::refreshLanguage16()
{
    if(m_language_refreshing23)return;
    m_language_refreshing23=true;
    const auto *project=diagram()?diagram()->project():nullptr;
    const bool loading=project&&project->state()==QETProject::ProjectParsingRunning;
    if(!loading)prepareGeometryChange();
    m_language_key16.clear();m_language_document16.clear();
    if(!loading){update();refreshTextResizeHandles();}
    m_language_refreshing23=false;
}

bool DiagramTextItem::isTranslationMode21() const {return QetLanguageEditor21::active(const_cast<DiagramTextItem*>(this));}
bool DiagramTextItem::languageHidden21() const {ensureLanguageDocument16();return m_language_hidden23;}
void DiagramTextItem::refreshLanguageSiblings23() {
    if(auto *folio=diagram())if(auto *project=folio->project())if(project->state()==QETProject::ProjectParsingRunning){refreshLanguage16();return;}
    if(auto *parent=parentItem()) {
        for(auto *item:parent->childItems())if(auto *text=dynamic_cast<DiagramTextItem*>(item))text->refreshLanguage16();
    } else refreshLanguage16();
}
QPointF DiagramTextItem::languageOffset22(Qt::Alignment *resolved) const {
    ensureLanguageDocument16();if(resolved)*resolved=m_language_alignment23;return m_language_offset23;
}

QPointF DiagramTextItem::sourcePosition21() const {if(auto *folio=diagram())if(auto *project=folio->project()){auto data=project->languageData();auto e=QetLanguages16::entry(data,languageId16());if(QetLanguages16::activeLanguage(data)!=QetLanguages16::sourceLanguage(data)&&e.value("enabled").toBool()&&e.contains("baseX"))return {e.value("baseX").toDouble(),e.value("baseY").toDouble()};}return pos();}

// Keep the source text's alignment anchor fixed without moving its item origin.
// This is a local-coordinate display offset: parent transforms and rotation apply
// identically to the source and translation, and manual language moves still work.
QPointF DiagramTextItem::calculateLanguageOffset23(Qt::Alignment *resolved) const {
 if(resolved)*resolved=alignment();
 const auto *display=languageDocument16();if(!display||languageHidden21())return {};
 if(auto *page=diagram())if(auto *project=page->project())if(QetLanguages16::effectiveId21(project->languageData(),languageId16())!=languageId16())return {}; // explicit reviewed blocks already have a top-left layout
 const auto source=QGraphicsTextItem::boundingRect();const auto size=display->size();
 auto a=alignment();const auto v=languageVariant16();const int setting=v.value("anchor22",0).toInt();
 if(setting)a=Qt::Alignment((int(a)&~int(Qt::AlignHorizontal_Mask))|setting);
 qreal referenceX=0;bool inferred=false;
 // Imported drawings often encode right/centre aligned word stacks as several
 // left-aligned objects with manually offset origins. Recognise only close,
 // unrotated descriptive siblings whose source edges form a clear common line.
 if(!setting && (a&Qt::AlignLeft) && parentItem() && qFuzzyIsNull(rotation())) {
  auto *project=diagram()?diagram()->project():nullptr;
  if(project){const auto data=project->languageData();struct Row {const DiagramTextItem *item;QRectF rect;};QList<Row> rows;
   auto sourceRect=[&](const DiagramTextItem *t){auto e=QetLanguages16::entry(data,t->languageId16());QPointF base(e.value("baseX",t->pos().x()).toDouble(),e.value("baseY",t->pos().y()).toDouble());return QetTextGeometry23::trimmedDocumentRect(t->document()).translated(base);};
   const auto own=sourceRect(this);const qreal spacing=qMax<qreal>(12,source.height()*1.1);
   for(auto *sibling:parentItem()->childItems())if(auto *t=dynamic_cast<DiagramTextItem*>(sibling))if(qFuzzyIsNull(t->rotation())&&(t->alignment()&Qt::AlignLeft)&&QetLanguages16::entry(data,t->languageId16()).value("enabled").toBool()&&QetLanguages16::isHumanPhrase(t->toPlainText())&&t->toPlainText().count('\n')==0){auto rect=sourceRect(t);if(qAbs(rect.center().x()-own.center().x())<=qMax(rect.width(),own.width())&&qAbs(rect.top()-own.top())<=spacing*10)rows<<Row{t,rect};}
   std::sort(rows.begin(),rows.end(),[](const Row &a,const Row &b){return a.rect.top()<b.rect.top();});int index=-1;for(int i=0;i<rows.size();++i)if(rows[i].item==this)index=i;
   if(index>=0){int first=index,last=index;while(first>0&&rows[first].rect.top()-rows[first-1].rect.top()>1&&rows[first].rect.top()-rows[first-1].rect.top()<=spacing)--first;while(last+1<rows.size()&&rows[last+1].rect.top()-rows[last].rect.top()>1&&rows[last+1].rect.top()-rows[last].rect.top()<=spacing)++last;
    if(last-first>=1){qreal best=1e9,leftSpread=0;int mode=Qt::AlignLeft;QList<qreal> bestEdges;
     for(auto candidate:{Qt::AlignLeft,Qt::AlignHCenter,Qt::AlignRight}){QList<qreal> edges;for(int i=first;i<=last;++i)edges<< (candidate==Qt::AlignLeft?rows[i].rect.left():candidate==Qt::AlignRight?rows[i].rect.right():rows[i].rect.center().x());std::sort(edges.begin(),edges.end());const qreal spread=edges.last()-edges.first();if(candidate==Qt::AlignLeft)leftSpread=spread;if(spread<best){best=spread;mode=candidate;bestEdges=edges;}}
     if(mode!=Qt::AlignLeft&&best<=3.5&&leftSpread>best+1){a=Qt::Alignment((int(a)&~int(Qt::AlignHorizontal_Mask))|mode);referenceX=bestEdges[bestEdges.size()/2]-QetLanguages16::entry(data,languageId16()).value("baseX",pos().x()).toDouble();inferred=true;}
    }
   }
  }
 }
 if(resolved)*resolved=a;
 qreal x=source.left(),y=source.top();
 if(a&Qt::AlignRight)x=inferred?referenceX-QetTextGeometry23::trimmedDocumentRect(display).right():source.right()-size.width();
 else if(a&Qt::AlignHCenter)x=inferred?referenceX-QetTextGeometry23::trimmedDocumentRect(display).center().x():source.center().x()-size.width()/2;
 if(a&Qt::AlignBottom)y=source.bottom()-size.height();else if(a&Qt::AlignVCenter)y=source.center().y()-size.height()/2;
 return {x,y};
}
