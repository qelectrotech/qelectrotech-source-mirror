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
	Indique au QGIManager de detruire les QGraphicsItem restants lors de sa
	destruction si ceux-ci n'appartiennent pas a la scene
*/
void QGIManager::setDestroyQGIOnDelete(bool b) {
	destroy_qgi_on_delete = b;
}
