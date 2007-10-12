#include "elementview.h"
/**
	Constructeur
	@param scene ElementScene visualisee par cette ElementView
	@param parent QWidget parent de cette ElementView
*/
ElementView::ElementView(ElementScene *scene, QWidget *parent) :
	QGraphicsView(scene, parent),
	scene_(scene)
{
	setInteractive(true);
	setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
	//setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
	setResizeAnchor(QGraphicsView::AnchorUnderMouse);
	//setSceneRect(QRectF(0.0, 0.0, 50.0, 200.0));
	scale(4.0, 4.0);
}

/// Destructeur
ElementView::~ElementView() {
}

/// @return l'ElementScene visualisee par cette ElementView
ElementScene *ElementView::scene() const {
	return(scene_);
}

/**
	Definit l'ElementScene visualisee par cette ElementView
	@param s l'ElementScene visualisee par cette ElementView
*/
void ElementView::setScene(ElementScene *s) {
	QGraphicsView::setScene(s);
	scene_ = s;
}

bool ElementView::event(QEvent *e) {
	if (e -> type() == QEvent::ShortcutOverride && scene_ -> focusItem()) {
		e -> accept();
		return(true);
	}
	return(QGraphicsView::event(e));
}
