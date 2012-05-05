/*
	Copyright 2006-2012 Xavier Guerrin
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
#ifndef QGI_MANAGER_H
#define QGI_MANAGER_H
#include <QtCore>
#include <QGraphicsScene>
#include <QGraphicsItem>
/**
	Cette classe represente un gestionnaire de QGraphicsItem.
	Elle permet de supprimer des QGraphicsItem lorsqu'il n'y a plus aucune
	reference vers eux et qu'ils ne sont plus sur la scene.
*/
class QGIManager {
	// constructeurs, destructeurs
	public:
	QGIManager(QGraphicsScene *);
	virtual ~QGIManager();
	private:
	QGIManager(const QGIManager &);
	
	// attributs
	private:
	QGraphicsScene *scene;
	QHash<QGraphicsItem *, int> qgi_manager;
	bool destroy_qgi_on_delete;
	
	//methodes
	public:
	void manage(QGraphicsItem *);
	void release(QGraphicsItem *);
	void manage(const QList<QGraphicsItem *> &);
	void release(const QList<QGraphicsItem *> &);
	void setDestroyQGIOnDelete(bool);
	bool manages(QGraphicsItem *) const;
};
#endif
