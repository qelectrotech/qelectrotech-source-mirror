#include "elementtextitem.h"

ElementTextItem::ElementTextItem(QGraphicsItem *parent, QGraphicsScene *scene) : QGraphicsTextItem(parent, scene) {
	follow_parent_rotations = false;
	setTextInteractionFlags(Qt::TextEditorInteraction);
}

ElementTextItem::ElementTextItem(const QString &text, QGraphicsItem *parent, QGraphicsScene *scene) : QGraphicsTextItem(text, parent, scene) {
	follow_parent_rotations = false;
	setTextInteractionFlags(Qt::TextEditorInteraction);
}

void ElementTextItem::setPos(const QPointF &pos) {
	QPointF actual_pos = pos;
	actual_pos -= QPointF(0.0, boundingRect().height() / 2.0);
	QGraphicsItem::setPos(actual_pos);
}

void ElementTextItem::setPos(qreal x, qreal y) {
	setPos(QPointF(x, y));
}

QPointF ElementTextItem::pos() const {
	QPointF actual_pos = pos();
	actual_pos += QPointF(0.0, boundingRect().height() / 2.0);
	return(actual_pos);
}
