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
#include "parttextfield.h"
#include "textfieldeditor.h"
#include "editorcommands.h"
#include "elementprimitivedecorator.h"
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
	previous_text(),
	decorator_(0)
{
	setDefaultTextColor(Qt::black);
	setFont(QETApp::diagramTextsFont());
	real_font_size_ = font().pointSize();
	setFlags(QGraphicsItem::ItemIsSelectable);
#if QT_VERSION >= 0x040600
	setFlag(QGraphicsItem::ItemSendsGeometryChanges, true);
#endif
	setAcceptHoverEvents(true);
	setPlainText(QObject::tr("_", "default text when adding a textfield in the element editor"));
	
	adjustItemPosition(1);
	// adjust textfield position after line additions/deletions
	connect(document(), SIGNAL(blockCountChanged(int)), this, SLOT(adjustItemPosition(int)));
	connect(document(), SIGNAL(contentsChanged()),      this, SLOT(adjustItemPosition()));
}

/// Destructeur
PartTextField::~PartTextField() {
}

/**
	Importe les proprietes d'un champ de texte depuis un element XML
	@param xml_element Element XML a lire
*/
void PartTextField::fromXml(const QDomElement &xml_element) {
	bool ok;
	int font_size = xml_element.attribute("size").toInt(&ok);
	if (!ok || font_size < 1) font_size = 20;
	
	setProperty("size", font_size);
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
	@return l'angle de rotation de ce champ de texte
*/
qreal PartTextField::rotationAngle() const {
	return(rotation());
}

/**
	@param angle Le nouvel angle de rotation de ce champ de texte
*/
void PartTextField::setRotationAngle(const qreal &angle) {
	setRotation(QET::correctAngle(angle));
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
	Handle context menu events.
	@param event Object describing the context menu event to handle.
*/
void PartTextField::contextMenuEvent(QGraphicsSceneContextMenuEvent *event) {
	Q_UNUSED(event);
}

/**
	Handle events generated when the mouse hovers over the decorator.
	@param event Object describing the hover event.
*/
void PartTextField::hoverMoveEvent(QGraphicsSceneHoverEvent *event) {
	// force the cursor when the text is being edited
	if (hasFocus() && decorator_) {
		decorator_ -> setCursor(Qt::IBeamCursor);
	}
	QGraphicsTextItem::hoverMoveEvent(event);
}

/**
	@reimp QGraphicsItem::sceneEventFilter(QGraphicsItem *, QEvent *).
	Intercepts events before they reach the watched target, i.e. typically the
	primitives decorator.
	This method mainly works with key strokes (F2, escape) and double clicks to
	begin or end text edition.
*/
bool PartTextField::sceneEventFilter(QGraphicsItem *watched, QEvent *event) {
	if (watched != decorator_) return(false);
	
	QPointF event_scene_pos = QET::graphicsSceneEventPos(event);
	if (!event_scene_pos.isNull()) {
		if (contains(mapFromScene(event_scene_pos))) {
			if (hasFocus()) {
				return sceneEvent(event); // manually deliver the event to this item
				return(true); // prevent this event from being delivered to any item
			} else {
				if (event -> type() == QEvent::GraphicsSceneMouseDoubleClick) {
					mouseDoubleClickEvent(static_cast<QGraphicsSceneMouseEvent *>(event));
				}
			}
		}
	}
	else if (event -> type() == QEvent::KeyRelease || event -> type() == QEvent::KeyPress) {
		// Intercept F2 and escape keystrokes to focus in and out
		QKeyEvent *key_event = static_cast<QKeyEvent *>(event);
		if (!hasFocus() && key_event -> key() == Qt::Key_F2) {
			setEditable(true);
			QTextCursor qtc = textCursor();
			qtc.setPosition(qMax(0, document()->characterCount() - 1));
			setTextCursor(qtc);
		} else if (hasFocus() && key_event -> key() == Qt::Key_Escape) {
			endEdition();
		}
		if (hasFocus()) {
			sceneEvent(event); // manually deliver the event to this item
			return(true); // prevent this event from being delivered to any item
		}
	}
	return(false);
}

/*
	@reimp QGraphicsItem::focusInEvent(QFocusEvent *)
	@param e The QFocusEvent object describing the focus gain.
	Start text edition when the item gains focus.
*/
void PartTextField::focusInEvent(QFocusEvent *e) {
	startEdition();
	QGraphicsTextItem::focusInEvent(e);
}

/**
	Permet a l'element texte de redevenir deplacable a la fin de l'edition de texte
	@param e Le QFocusEvent decrivant la perte de focus
*/
void PartTextField::focusOutEvent(QFocusEvent *e) {
	QGraphicsTextItem::focusOutEvent(e);
	endEdition();
}

/**
	@reimp QGraphicsTextItem::keyPressEvent()
	Used to handle the escape key when the event is delivered to the field, not
	to the decorator.
*/
void PartTextField::keyPressEvent(QKeyEvent *event) {
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
void PartTextField::mouseDoubleClickEvent(QGraphicsSceneMouseEvent *e) {
	QGraphicsTextItem::mouseDoubleClickEvent(e);
	if (e -> button() == Qt::LeftButton) {
		setEditable(true);
	}
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
		real_font_size_ = value.toInt();
	} else if (property == "real_size") {
		if (!value.canConvert(QVariant::Double)) return;
		setFont(QETApp::diagramTextsFont(value.toInt()));
		real_font_size_ = value.toDouble();
	} else if (property == "text") {
		setPlainText(value.toString());
	} else if (property == "rotation angle") {
		setRotationAngle(value.toDouble());
	} else if (property == "rotate") {
		follow_parent_rotations = value.toBool();
	}
	// adjust item position, especially useful when changing text or size
	adjustItemPosition();
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
	} else if (property == "real_size") {
		return(real_font_size_);
	} else if (property == "text") {
		return(toPlainText());
	} else if (property == "rotation angle") {
		return(rotation());
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
	if (change == QGraphicsItem::ItemPositionHasChanged || change == QGraphicsItem::ItemSceneHasChanged) {
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
	@return the minimum, margin-less rectangle this part can fit into, in scene
	coordinates. It is different from boundingRect() because it is not supposed
	to imply any margin, and it is different from shape because it is a regular
	rectangle, not a complex shape.
*/
QRectF PartTextField::sceneGeometricRect() const {
	return(sceneBoundingRect());
}

/**
	Start the user-induced transformation, provided this primitive is contained
	within the \a initial_selection_rect bounding rectangle.
*/
void PartTextField::startUserTransformation(const QRectF &initial_selection_rect) {
	Q_UNUSED(initial_selection_rect)
	saved_point_ = pos(); // scene coordinates, no need to mapFromScene()
	saved_font_size_ = real_font_size_;
}

/**
	Handle the user-induced transformation from \a initial_selection_rect to \a new_selection_rect
*/
void PartTextField::handleUserTransformation(const QRectF &initial_selection_rect, const QRectF &new_selection_rect) {
	// let's try the naive approach
	QPointF new_pos = mapPoints(initial_selection_rect, new_selection_rect, QList<QPointF>() << saved_point_).first();
	setPos(new_pos);
	
	// adjust the font size following the vertical scale factor
	qreal sy = new_selection_rect.height() / initial_selection_rect.height();
	qreal new_font_size = saved_font_size_ * sy;
	setProperty("real_size", qMax(1, qRound(new_font_size)));
}
/**
	Dessine le texte statique.
	@param painter QPainter a utiliser pour effectuer le rendu
	@param qsogi   Pptions de dessin
	@param widget  Widget sur lequel on dessine (facultatif)
*/
void PartTextField::paint(QPainter *painter, const QStyleOptionGraphicsItem *qsogi, QWidget *widget) {
	// According to the source code of QGraphicsTextItem::paint(), this should
	// avoid the drawing of the dashed rectangle around the text.
	QStyleOptionGraphicsItem our_qsogi(*qsogi);
	our_qsogi.state = QStyle::State_None;
	
	QGraphicsTextItem::paint(painter, &our_qsogi, widget);
#ifdef QET_DEBUG_EDITOR_TEXTS
	painter -> setPen(Qt::blue);
	painter -> drawRect(boundingRect());
	
	painter -> setPen(Qt::red);
	drawPoint(painter, QPointF(0, 0));
	
	painter -> setPen(QColor("#800000"));
	drawPoint(painter, mapFromScene(pos()));
#endif
}

/**
	@reimp CustomElementPart::setDecorator(ElementPrimitiveDecorator *)
	Install or remove a sceneEventFilter on the decorator and ensure it will
	adjust itself while the text is being edited.
*/
void PartTextField::setDecorator(ElementPrimitiveDecorator *decorator) {
	if (decorator) {
		decorator -> installSceneEventFilter(this);
		// ensure the decorator will adjust itself when the text area expands or shrinks
		connect(document(), SIGNAL(contentsChanged()), decorator, SLOT(adjust()));
	}
	else {
		decorator_ -> removeSceneEventFilter(this);
		endEdition();
	}
	decorator_ = decorator;
}

/**
	Accept the mouse \a event relayed by \a decorator if this text item has focus.
*/
bool PartTextField::singleItemPressEvent(ElementPrimitiveDecorator *decorator, QGraphicsSceneMouseEvent *event) {
	Q_UNUSED(decorator)
	Q_UNUSED(event)
	return(hasFocus());
}

/**
	Accept the mouse \a event relayed by \a decorator if this text item has focus.
*/
bool PartTextField::singleItemMoveEvent(ElementPrimitiveDecorator *decorator, QGraphicsSceneMouseEvent *event) {
	Q_UNUSED(decorator)
	Q_UNUSED(event)
	return(hasFocus());
}

/**
	Accept the mouse \a event relayed by \a decorator if this text item has focus.
*/
bool PartTextField::singleItemReleaseEvent(ElementPrimitiveDecorator *decorator, QGraphicsSceneMouseEvent *event) {
	Q_UNUSED(decorator)
	Q_UNUSED(event)
	return(hasFocus());
}

/**
	Accept the mouse \a event relayed by \a decorator if this text item has focus.
*/
bool PartTextField::singleItemDoubleClickEvent(ElementPrimitiveDecorator *decorator, QGraphicsSceneMouseEvent *event) {
	Q_UNUSED(decorator)
	// calling mouseDoubleClickEvent() will set this text item editable and grab keyboard focus
	if (event -> button() == Qt::LeftButton) {
		mouseDoubleClickEvent(event);
		return(true);
	}
	return(false);
}

/**
	Cette methode s'assure que la position du champ de texte est coherente
	en repositionnant son origine (c-a-d le milieu du bord gauche du champ de
	texte) a la position originale. Cela est notamment utile lorsque le champ
	de texte est agrandi ou retreci verticalement (ajout ou retrait de lignes).
	@param new_block_count Nombre de blocs dans le PartTextField
*/
void PartTextField::adjustItemPosition(int new_block_count) {
	Q_UNUSED(new_block_count);
	qreal origin_offset = boundingRect().bottom() / 2.0;
	
	QTransform base_translation;
	base_translation.translate(0.0, -origin_offset);
	setTransform(base_translation, false);
	setTransformOriginPoint(0.0, origin_offset);
}

/**
	@param editable Whether this text item should be interactively editable.
*/
void PartTextField::setEditable(bool editable) {
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
void PartTextField::startEdition() {
	// !previous_text.isNull() means the text is being edited
	previous_text = toPlainText();
}

/**
	End text edition, potentially generating a ChangePartCommand if the text
	has changed.
*/
void PartTextField::endEdition() {
	if (!previous_text.isNull()) {
		// the text was being edited
		QString new_text = toPlainText();
		if (previous_text != new_text) {
			// the text was changed
			ChangePartCommand *text_change = new ChangePartCommand(
				TextFieldEditor::tr("contenu") + " " + name(),
				this,
				"text",
				previous_text,
				new_text
			);
			previous_text = QString();
			undoStack().push(text_change);
		}
	}
	
	// deselectionne le texte
	QTextCursor qtc = textCursor();
	qtc.clearSelection();
	setTextCursor(qtc);
	
	setEditable(false);
	if (decorator_) {
		decorator_ -> setFocus();
	}
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
	painter -> drawLine(QLineF(px, py, px + size_1, py));
	painter -> drawLine(QLineF(px + size_1 - size_2, py - size_2, px + size_1, py));
	painter -> drawLine(QLineF(px + size_1 - size_2, py + size_2, px + size_1, py));
	painter -> drawLine(QLineF(px, py, px, py + size_1));
	painter -> drawLine(QLineF(px, py + size_1, px - size_2, py + size_1 - size_2));
	painter -> drawLine(QLineF(px, py + size_1, px + size_2, py + size_1 - size_2));
}
#endif
