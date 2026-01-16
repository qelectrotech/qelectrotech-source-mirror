/*
	Copyright 2006-2026 The QElectroTech Team
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

#include "../../QPropertyUndoCommand/qpropertyundocommand.h"
#include "../../qetapp.h"
#include "../elementprimitivedecorator.h"
#include "../elementscene.h"
#include "../ui/texteditor.h"

/**
	Constructeur
	@param editor L'editeur d'element concerne
	@param parent Le QGraphicsItem parent de ce texte statique
*/
PartText::PartText(QETElementEditor *editor, QGraphicsItem *parent) :
	QGraphicsTextItem(parent),
	CustomElementPart(editor),
	previous_text()
{
	document() -> setDocumentMargin(1.0);
	setDefaultTextColor(Qt::black);
	setFont(QETApp::diagramTextsFont());
	real_font_size_ = font().pointSize();
	setFlags(QGraphicsItem::ItemIsSelectable
		 | QGraphicsItem::ItemSendsGeometryChanges
		 | QGraphicsItem::ItemIsMovable);
	setAcceptHoverEvents(true);
	setDefaultTextColor(Qt::black);
	setPlainText(QObject::tr(
			     "T",
			     "default text when adding a text in the element editor"));

	adjustItemPosition(1);
	// adjust textfield position after line additions/deletions
	connect(document(),
		SIGNAL(blockCountChanged(int)),
		this,
		SLOT(adjustItemPosition(int)));
	connect(document(),
		SIGNAL(contentsChanged()),
		this,
		SLOT(adjustItemPosition()));
}

/// Destructeur
PartText::~PartText()
{
}

/**
	Redefines setRotation
	@param angle
*/
void PartText::setRotation(qreal angle) {
	qreal diffAngle = qRound((angle - rotation()) * 100.0) / 100.0;
	QGraphicsObject::setRotation(QET::correctAngle(angle, true));
	setPos(QTransform().rotate(diffAngle).map(pos()));
}

void PartText::mirror() {
	// at first: rotate the text:
	QGraphicsObject::setRotation(QET::correctAngle((360-rotation()), true));
	// then see, where we need to re-position depending on text, font ...
	QFontMetrics qfm(font());
	qreal textwidth  = qfm.horizontalAdvance(toPlainText());
	// ... and angle!!!
	qreal rot = qRound(QET::correctAngle(rotation(), true));
	qreal c = qCos(qDegreesToRadians(rot));
	qreal s = qSin(qDegreesToRadians(rot));
	// Now: Move!
	qreal x = (-1) * pos().x() - c * (textwidth);
	qreal y = pos().y() - s * (textwidth);
	setPos(x, y);
}

void PartText::flip() {
	// at first: rotate the text:
	QGraphicsObject::setRotation(QET::correctAngle((360-rotation()), true));
	// then see, where we need to re-position depending on text, font ...
	QFontMetrics qfm(font());
	qreal textheight = realSize() - qfm.descent();
	// ... and angle!!!
	qreal rot = qRound(QET::correctAngle(rotation(), true));
	qreal c = qCos(qDegreesToRadians(rot));
	qreal s = qSin(qDegreesToRadians(rot));
	// Now: Move!
	qreal x = pos().x() - s * (textheight);
	qreal y = (-1) * pos().y() + c * (textheight);
	setPos(x, y);
}


/**
	Importe les proprietes d'un texte statique depuis un element XML
	@param xml_element Element XML a lire
*/
void PartText::fromXml(const QDomElement &xml_element) {
	bool ok;

	if (xml_element.hasAttribute("size")) {
		int font_size = xml_element.attribute("size").toInt(&ok);
		if (!ok || font_size < 1) {
			font_size = 20;
		}
		QFont font_ = this -> font();
		font_.setPointSize(font_size);
		setFont(font_);
	}
	else if (xml_element.hasAttribute("font")) {
		QFont font_;
		font_.fromString(xml_element.attribute("font"));
		setFont(font_);
	}

	setDefaultTextColor(QColor(xml_element.attribute("color", "#000000")));
	setPlainText(xml_element.attribute("text"));
	setPos(xml_element.attribute("x").toDouble(),
			xml_element.attribute("y").toDouble());
	QGraphicsObject::setRotation(QET::correctAngle(xml_element.attribute("rotation", QString::number(0)).toDouble()));
}

/**
	Exporte le texte statique en XML
	@param xml_document Document XML a utiliser pour creer l'element XML
	@return un element XML decrivant le texte statique
*/
const QDomElement PartText::toXml(QDomDocument &xml_document) const
{
	QDomElement xml_element = xml_document.createElement(xmlName());

	qreal x   = (qRound(pos().x() * 100.0) / 100.0);
	qreal y   = (qRound(pos().y() * 100.0) / 100.0);
	qreal rot = (qRound(rotation() * 10.0) /  10.0);
	xml_element.setAttribute("x", QString::number(x));
	xml_element.setAttribute("y", QString::number(y));
	xml_element.setAttribute("text", toPlainText());
	xml_element.setAttribute("font", font().toString());
	xml_element.setAttribute("rotation", QString::number(rot));
	xml_element.setAttribute("color", defaultTextColor().name());

	return(xml_element);
}

/**
	@return Les coordonnees du point situe en bas a gauche du texte.
			The coordinates of the point at the bottom left of the text.
*/
QPointF PartText::margin() const
{
	QFont used_font = font();
	QFontMetrics qfm(used_font);
	qreal document_margin = document() -> documentMargin();

	QPointF margin(
		// margin around the text
		// marge autour du texte
		document_margin,
		// margin above the text + distance between the top of the text and the baseline
		// marge au-dessus du texte + distance entre le plafond du texte et la baseline
		document_margin + qfm.ascent()
	);
	return(margin);
}

/**
	@brief PartText::focusInEvent
	@param e : The QFocusEvent object describing the focus gain.
	Start text edition when the item gains focus.
	@see QGraphicsItem::focusInEvent(QFocusEvent *)
*/
void PartText::focusInEvent(QFocusEvent *e) {
	startEdition();
	QGraphicsTextItem::focusInEvent(e);
}

/**
	@brief PartText::focusOutEvent
	@param e : The QFocusEvent object describing the focus loss.
	End text edition when the item loses focus.
	@see QGraphicsItem::focusOutEvent(QFocusEvent *)
*/
void PartText::focusOutEvent(QFocusEvent *e) {
	QGraphicsTextItem::focusOutEvent(e);
	endEdition();
}

/**
	@brief PartText::keyPressEvent
	Used to handle the escape key when the event is delivered to the field,
	not to the decorator.
	@param event
	@see QGraphicsTextItem::keyPressEvent()
*/
void PartText::keyPressEvent(QKeyEvent *event) {
	if (event -> key() == Qt::Key_Escape) {
		endEdition();
	}
	else {
		QGraphicsTextItem::keyPressEvent(event);
	}
}

/**
	Permet a l'element texte de devenir editable lorsqu'on double-clique dessus
	@param e Le QGraphicsSceneMouseEvent qui decrit le double-clic
*/
void PartText::mouseDoubleClickEvent(QGraphicsSceneMouseEvent *e) {
	QGraphicsTextItem::mouseDoubleClickEvent(e);
	if (e -> button() == Qt::LeftButton) {
		setEditable(true);
	}
}

/**
	Gere les changements intervenant sur cette partie
	@param change Type de changement
	@param value Valeur numerique relative au changement
*/
QVariant PartText::itemChange(GraphicsItemChange change, const QVariant &value) {
	if (change == QGraphicsItem::ItemPositionHasChanged ||
		change == QGraphicsItem::ItemSceneHasChanged ||
		change == QGraphicsItem::ItemSelectedHasChanged) {
			updateCurrentPartEditor();
	}
	return(QGraphicsTextItem::itemChange(change, value));
}

/**
	@return le rectangle delimitant cette partie.
*/
QRectF PartText::boundingRect() const
{
	QRectF r = QGraphicsTextItem::boundingRect();
	r.adjust(0.0, -1.1, 0.0, 0.0);
	return(r);
}

/**
	@return true si cette partie n'est pas pertinente et ne merite pas d'etre
	conservee / enregistree.
	Un texte statique n'est pas pertinent lorsque son texte est vide.
*/
bool PartText::isUseless() const
{
	return(toPlainText().isEmpty());
}

/**
	@return the minimum, margin-less rectangle this part can fit into, in scene
	coordinates. It is different from boundingRect() because it is not supposed
	to imply any margin, and it is different from shape because it is a regular
	rectangle, not a complex shape.
*/
QRectF PartText::sceneGeometricRect() const
{
	return(sceneBoundingRect());
}

/**
	Start the user-induced transformation, provided this primitive is contained
	within the \a rect bounding rectangle.
*/
void PartText::startUserTransformation(const QRectF &rect) {
	Q_UNUSED(rect)
	saved_point_ = pos(); // scene coordinates, no need to mapFromScene()
	saved_font_size_ = real_font_size_;
}

/**
	Handle the user-induced transformation from \a initial_selection_rect to \a new_selection_rect
*/
void PartText::handleUserTransformation(const QRectF &initial_selection_rect, const QRectF &new_selection_rect) {
	// let's try the naive approach
	QPointF new_pos = mapPoints(initial_selection_rect, new_selection_rect, QList<QPointF>() << saved_point_).first();
	setPos(new_pos);

	// adjust the font size following the vertical scale factor
	qreal sy = new_selection_rect.height() / initial_selection_rect.height();
	qreal new_font_size = saved_font_size_ * sy;
	setProperty("real_size", qMax(1, qRound(new_font_size)));
}

void PartText::setDefaultTextColor(const QColor &color) {
	if (color != this -> defaultTextColor()) {
		QGraphicsTextItem::setDefaultTextColor(color);
		emit colorChanged(color);
	}
}

void PartText::setPlainText(const QString &text) {
	if (text != this -> toPlainText()) {
		QGraphicsTextItem::setPlainText(text);
		emit plainTextChanged(text);
	}
}

void PartText::setFont(const QFont &font) {
	if (font != this -> font()) {
		QGraphicsTextItem::setFont(font);
		emit fontChanged(font);
	}
}

void PartText::mouseMoveEvent(QGraphicsSceneMouseEvent *event) {
	if((event -> buttons() & Qt::LeftButton) && (flags() & QGraphicsItem::ItemIsMovable)) {
		QPointF pos = event -> scenePos() + (m_origin_pos - event -> buttonDownScenePos(Qt::LeftButton));
		event -> modifiers() == Qt::ControlModifier ? setPos(pos) : setPos(elementScene() -> snapToGrid(pos));
	}
	else {
		QGraphicsObject::mouseMoveEvent(event);
	}
}

void PartText::mousePressEvent(QGraphicsSceneMouseEvent *event) {
	if(event -> button() == Qt::LeftButton)
		m_origin_pos = this -> pos();

	QGraphicsObject::mousePressEvent(event);
}

void PartText::mouseReleaseEvent(QGraphicsSceneMouseEvent *event) {
	if((event -> button() & Qt::LeftButton) &&
		(flags() & QGraphicsItem::ItemIsMovable) &&
		m_origin_pos != pos())
	{
		QPropertyUndoCommand *undo = new QPropertyUndoCommand(this, "pos", QVariant(m_origin_pos), QVariant(pos()));
		undo -> setText(tr("Déplacer un texte"));
		undo -> enableAnimation();
		elementScene() -> undoStack().push(undo);
	}

	QGraphicsObject::mouseReleaseEvent(event);
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
	QPointF origin_offset = margin();

	QTransform base_translation;
	base_translation.translate(-origin_offset.x(), -origin_offset.y());
	setTransform(base_translation, false);
	setTransformOriginPoint(origin_offset);
}

/**
	@param editable Whether this text item should be interactively editable.
*/
void PartText::setEditable(bool editable) {
	if (editable) {
		setFlag(QGraphicsItem::ItemIsFocusable, true);
		setTextInteractionFlags(Qt::TextEditorInteraction);
		setFocus(Qt::MouseFocusReason);
	}
	else {
		setTextInteractionFlags(Qt::NoTextInteraction);
		setFlag(QGraphicsItem::ItemIsFocusable, false);
	}
}

/**
	Start text edition by storing the former value of the text.
*/
void PartText::startEdition()
{
	// !previous_text.isNull() means the text is being edited
	previous_text = toPlainText();
}

/**
	End text edition, potentially generating a ChangePartCommand if the text
	has changed.
*/
void PartText::endEdition()
{
	if (!previous_text.isNull()) {
			// the text was being edited
		QString new_text = toPlainText();
		if (previous_text != new_text) {
			QPropertyUndoCommand *undo = new QPropertyUndoCommand(this, "text", previous_text, new_text);
			undo -> setText(tr("Modifier un champ texte"));
			undoStack().push(undo);
			previous_text = QString();
		}
	}

	// deselectionne le texte
	QTextCursor qtc = textCursor();
	qtc.clearSelection();
	setTextCursor(qtc);

	setEditable(false);
}
