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
	setResizeAnchor(QGraphicsView::AnchorUnderMouse);
	zoomReset();
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

/**
	Gere les evenements envoyes a la vue.
	Methode reimplentee pour gerer le conflit de raccourcis avec Suppr
	(supprimer une partie ou supprimer le caractere suivant)
	@param e evenement a gerer
*/
bool ElementView::event(QEvent *e) {
	if (e -> type() == QEvent::ShortcutOverride && scene_ -> focusItem()) {
		e -> accept();
		return(true);
	}
	return(QGraphicsView::event(e));
}


/**
	Agrandit le schema (+33% = inverse des -25 % de zoomMoins())
*/
void ElementView::zoomIn() {
	scale(4.0/3.0, 4.0/3.0);
}

/**
	Retrecit le schema (-25% = inverse des +33 % de zoomPlus())
*/
void ElementView::zoomOut() {
	scale(0.75, 0.75);
}

/**
	Agrandit ou rectrecit le schema de facon a ce que tous les elements du
	schema soient visibles a l'ecran. S'il n'y a aucun element sur le schema,
	le zoom est reinitialise
*/
void ElementView::zoomFit() {
	adjustSceneRect();
	fitInView(sceneRect(), Qt::KeepAspectRatio);
}

/**
	Reinitialise le zoom
*/
void ElementView::zoomReset() {
	resetMatrix();
	scale(4.0, 4.0);
}

/**
	Ajuste le sceneRect (zone du schema visualisee par l'ElementView) afin que
	celui inclut a la fois les parties dans et en dehors du cadre et le cadre
	lui-meme.
*/
void ElementView::adjustSceneRect() {
	QRectF old_scene_rect = scene_ -> sceneRect();
	QRectF new_scene_rect = scene_ -> sceneContent();
	setSceneRect(new_scene_rect);
	
	// met a jour la scene
	scene_ -> update(old_scene_rect.united(new_scene_rect));
}

/**
	Gere les actions liees a la rollette de la souris
	@param e QWheelEvent decrivant l'evenement rollette
*/
void ElementView::wheelEvent(QWheelEvent *e) {
	// si la touche Ctrl est enfoncee, on zoome / dezoome
	if (e -> modifiers() & Qt::ControlModifier) {
		if (e -> delta() > 0) {
			zoomIn();
		} else {
			zoomOut();
		}
	} else {
		QAbstractScrollArea::wheelEvent(e);
	}
}
