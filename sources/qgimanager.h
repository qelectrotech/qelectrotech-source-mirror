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
#ifndef QGI_MANAGER_H
#define QGI_MANAGER_H
#include <QtCore>
#include <QGraphicsScene>
#include <QGraphicsItem>
/**
	This class provides a QGraphicsItem manager, which can delete QGraphicsItem
	as soon as there is no reference to them anymore.
*/
class QGIManager {
	// constructors, destructors
	public:
	QGIManager(QGraphicsScene *);
	virtual ~QGIManager();
	private:
	QGIManager(const QGIManager &);
	
	// attributes
	private:
	QGraphicsScene *scene;
	QHash<QGraphicsItem *, int> qgi_manager;
	bool destroy_qgi_on_delete;
	
	// methods
	public:
	void manage(QGraphicsItem *);
	void release(QGraphicsItem *);
	void manage(const QList<QGraphicsItem *> &);
	void release(const QList<QGraphicsItem *> &);
	void setDestroyQGIOnDelete(bool);
	bool manages(QGraphicsItem *) const;
};
#endif
