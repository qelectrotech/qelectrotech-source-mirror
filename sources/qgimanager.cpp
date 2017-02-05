/*
	Copyright 2006-2017 The QElectroTech Team
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
#include "qgimanager.h"

/**
	Constructeur
	@param sc QGraphicsScene a utiliser pour gerer au mieux les QGraphicsItem
*/
QGIManager::QGIManager(QGraphicsScene *sc) :
	scene(sc),
	destroy_qgi_on_delete(true)
{
}

/**
	Destructeur
	Lors de sa destruction, le QGI Manager detruit les QGraphicsItem restants
	si ceux-ci n'appartiennent pas a la scene ; ce comportement peut etre
	change avec la methode setDestroyQGIOnDelete
	@see setDestroyQGIOnDelete
*/
QGIManager::~QGIManager(){
	if (!destroy_qgi_on_delete) return;
	foreach(QGraphicsItem *qgi, qgi_manager.keys()) {
		if (!scene -> items().contains(qgi)) delete qgi;
	}
}

/**
	Demande au QGIManager de gerer un QGI
	@param qgi QGraphicsItem a gerer
*/
void QGIManager::manage(QGraphicsItem *qgi) {
	if (qgi -> parentItem()) return;
	if (qgi_manager.contains(qgi)) ++ qgi_manager[qgi];
	else qgi_manager.insert(qgi, 1);
}

/**
	Indique au QGIManager qu'une reference vers un QGI a ete detruite
	S'il n'y a plus de references vers ce QGI et que celui-ci n'est pas present
	sur la scene de ce QGIManager, alors il sera detruit.
	@param qgi QGraphicsItem a ne plus gerer
*/
void QGIManager::release(QGraphicsItem *qgi) {
	if (!qgi_manager.contains(qgi)) return;
	-- qgi_manager[qgi];
	if (qgi_manager[qgi] <= 0 && !(scene -> items().contains(qgi))) {
		delete qgi;
		qgi_manager.remove(qgi);
	}
}

/**
	Demande au QGIManager de gerer plusieurs QGI
	@param qgis QGraphicsItems a gerer
*/
void QGIManager::manage(const QList<QGraphicsItem *> &qgis) {
	foreach(QGraphicsItem *qgi, qgis) manage(qgi);
}

/**
	Indique au QGIManager que pour chaque QGI fourni, une reference vers celui-ci
	a ete detruite.
	S'il n'y a plus de references vers un QGI et que celui-ci n'est pas present
	sur la scene de ce QGIManager, alors il sera detruit.
	@param qgis QGraphicsItems a ne plus gerer
*/
void QGIManager::release(const QList<QGraphicsItem *> &qgis) {
	foreach(QGraphicsItem *qgi, qgis) release(qgi);
}

/**
	Indique au QGIManager de detruire les QGraphicsItem restants lors de sa
	destruction si ceux-ci n'appartiennent pas a la scene
*/
void QGIManager::setDestroyQGIOnDelete(bool b) {
	destroy_qgi_on_delete = b;
}

/**
	Permet de savoir si ce QGIManager gere ou non un item donne
	@param qgi QGraphicsItem dont il faut verifier la presence
	@return true si l'item est gere, false sinon
*/
bool QGIManager::manages(QGraphicsItem *qgi) const {
	return(qgi_manager.contains(qgi));
}
