/*
	Copyright 2006-2007 Xavier Guerrin
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
