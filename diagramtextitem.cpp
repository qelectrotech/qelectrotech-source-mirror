#include "diagramtextitem.h"
#include "diagramcommands.h"

/**
	Constructeur
	@param parent Le QGraphicsItem parent du champ de texte
	@param scene La scene a laquelle appartient le champ de texte
*/
DiagramTextItem::DiagramTextItem(QGraphicsItem *parent, QGraphicsScene *scene) :
	QGraphicsTextItem(parent, scene)
{
	setTextInteractionFlags(Qt::TextEditorInteraction);
}

/**
	Constructeur
	@param parent Le QGraphicsItem parent du champ de texte
	@param scene La scene a laquelle appartient le champ de texte
	@param text Le texte affiche par le champ de texte
*/
DiagramTextItem::DiagramTextItem(const QString &text, QGraphicsItem *parent, QGraphicsScene *scene) :
	QGraphicsTextItem(text, parent, scene),
	previous_text(text)
{
	setTextInteractionFlags(Qt::TextEditorInteraction);
}

/// Destructeur
DiagramTextItem::~DiagramTextItem() {
}

/// @return le Diagram auquel ce texte appartient, ou 0 si ce texte est independant
Diagram *DiagramTextItem::diagram() const {
	return(qobject_cast<Diagram *>(scene()));
}

/**
	gere la perte de focus du champ de texte
*/
void DiagramTextItem::focusOutEvent(QFocusEvent *e) {
	QGraphicsTextItem::focusOutEvent(e);
	// si le texte a ete modifie
	if (toPlainText() != previous_text) {
		if (Diagram *dia = diagram()) {
			dia -> undoStack().push(new ChangeDiagramTextCommand(this, previous_text, toPlainText()));
			previous_text = toPlainText();
		}
	}
}

/**
	Gere le relachement des touches du clavier.
	Cette methode a ete reimplementee pour gerer la touche Suppr/Del
*/
void DiagramTextItem::keyReleaseEvent(QKeyEvent *e) {
	if (e -> key() == Qt::Key_Delete) {
		QTextCursor text_cursor = textCursor();
		text_cursor.deleteChar();
		setTextCursor(text_cursor);
	}
	QGraphicsTextItem::keyReleaseEvent(e);
}
