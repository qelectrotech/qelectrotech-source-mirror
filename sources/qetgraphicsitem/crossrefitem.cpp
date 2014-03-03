#include "crossrefitem.h"
#include "element.h"
#include "qetapp.h"
#include "diagramposition.h"

/**
 * @brief CrossRefItem::CrossRefItem
 * Default constructor
 * @param elmt element to dispaly the cross ref
 * @param parent parent QetGraphicsItem
 */
CrossRefItem::CrossRefItem(Element *elmt, QetGraphicsItem *parent) :
	QetGraphicsItem(parent),
	element_ (elmt)
{
	setFlags(QGraphicsItem::ItemIsSelectable|QGraphicsItem::ItemIsMovable);
	connect(elmt, SIGNAL(positionChange(QPointF)), this, SLOT(autoPos()));
	connect(diagram()->project(), SIGNAL(projectDiagramsOrderChanged(QETProject*,int,int)), this, SLOT(updateLabel()));
	updateLabel();
	autoPos();
}

/**
 * @brief CrossRefItem::~CrossRefItem
 * Default destructor
 */
CrossRefItem::~CrossRefItem() {
	disconnect(element_, SIGNAL(positionChange(QPointF)), this, SLOT(autoPos()));
	disconnect(diagram()->project(), SIGNAL(projectDiagramsOrderChanged(QETProject*,int,int)), this, SLOT(updateLabel()));
}

/**
 * @brief CrossRefItem::updateLabel
 * Update the content of the item
 */
void CrossRefItem::updateLabel() {
	//init the painter
	QPainter qp(&drawing_);
	QPen pen_;
	pen_.setWidth(1);
	qp.setPen(pen_);

	//draw the cross
	qp.drawLine(30,0, 30,50); //vertical line
	qp.drawLine(0,10, 60,10); //horizontal line

	//draw the header
	qp.setFont(QETApp::diagramTextsFont(7));
	QRectF header_rect (0,0,30,10);
	qp.drawText(header_rect, Qt::AlignCenter, "NO");
	header_rect.setRect(30, 0, 30, 10);
	qp.drawText(header_rect, Qt::AlignCenter, "NC");

	//and fill it
	fillCrossRef(&qp);
	update();
}

/**
 * @brief CrossRefItem::autoPos
 * Calculate and set position automaticaly.
 */
void CrossRefItem::autoPos() {
	if (isSelected() && element_->isSelected()) return;
	QRectF border= element_->diagram()->border();
	QPointF point;

	//if this item have parent calcule
	//te position by using mapped point.
	if(parentItem()) {
		point = element_->boundingRect().center();
		QPointF ypoint_ = mapToParent(mapFromScene(0, border.height() - element_->diagram()->border_and_titleblock.titleBlockHeight() - boundingRect().height()));
		point.setY(ypoint_.y());
	}
	else {
		point = element_->sceneBoundingRect().center();
		point.setY(border.height() - element_->diagram()->border_and_titleblock.titleBlockHeight() - boundingRect().height());
	}

	point.setX(point.x() - boundingRect().width()/2);
	setPos(point);
}

/**
 * @brief CrossRefItem::boundingRect
 * @return the bounding rect of this item
 */
QRectF CrossRefItem::boundingRect() const {
	return (QRectF(QPointF(0,0), QSizeF(60,50)));
}

/**
 * @brief CrossRefItem::paint
 * Paint this item
 * @param painter
 * @param option
 * @param widget
 */
void CrossRefItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) {
	Q_UNUSED(option);
	Q_UNUSED(widget);

	//draw the selection rect
	if (isSelected()) {
		painter->save();
		QPen t(Qt::black);
		t.setStyle(Qt::DashLine);
		painter -> setPen(t);
		painter->drawRect(boundingRect());
		painter->restore();
	}
	drawing_.play(painter);
}

/**
 * @brief CrossRefItem::mouseMoveEvent
 * handle mouse move event
 * @param e event
 */
void CrossRefItem::mouseMoveEvent(QGraphicsSceneMouseEvent *e) {
	element_->setHighlighted(true);
	QetGraphicsItem::mouseMoveEvent(e);
}

/**
 * @brief CrossRefItem::mouseReleaseEvent
 * handle mouse release event
 * @param e event
 */
void CrossRefItem::mouseReleaseEvent(QGraphicsSceneMouseEvent *e) {
	element_->setHighlighted(false);
	QetGraphicsItem::mouseReleaseEvent(e);
}

/**
 * @brief CrossRefItem::fillCrossRef
 * Fill the content of the cross ref
 * @param painter painter to use.
 */
void CrossRefItem::fillCrossRef(QPainter *painter) {
	if (element_->isFree()) return;

	QList <Element *> NO_list;
	QList <Element *> NC_list;

	//find each no and nc of connected element to element_
	foreach (Element *elmt, element_->linkedElements()) {
		QString state = elmt->kindInformations()["state"].toString();
		if (state == "NO")		NO_list << elmt;
		else if (state == "NC") NC_list << elmt;
	}

	painter->setFont(QETApp::diagramTextsFont(6));
	//fill the NO
	QString contact_str;
	foreach (Element *elmt, NO_list) {
		contact_str += QString::number(elmt->diagram()->folioIndex() + 1);
		contact_str += "-";
		contact_str += elmt->diagram()->convertPosition(elmt -> scenePos()).toString();
		contact_str += "\n";
	}
	QRectF rect_(0, 13, 30, 40);
	painter->drawText(rect_, Qt::AlignHCenter, contact_str);

	//fill the NC
	contact_str.clear();
	foreach (Element *elmt, NC_list) {
		contact_str += QString::number(elmt->diagram()->folioIndex() + 1);
		contact_str += "-";
		contact_str += elmt->diagram()->convertPosition(elmt -> scenePos()).toString();
		contact_str += "\n";
	}
	rect_.setRect(30, 13, 30, 40);
	painter->drawText(rect_, Qt::AlignHCenter, contact_str);
}
