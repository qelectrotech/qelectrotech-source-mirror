/*
	Copyright 2006-2014 The QElectroTech Team
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
#ifndef DIAGRAM_CONTENT_H
#define DIAGRAM_CONTENT_H
#include <QtGui>
#include "qetgraphicsitem/qetshapeitem.h"
class Conductor;
class Element;
class IndependentTextItem;
class DiagramImageItem;

/**
	This class provides a container that makes the transmission of diagram content
	to other functions/methods easier. The different kind of items are made
	available through a handful of filter-aware methods. Considering selected
	elements are to be moved, the filter notably distinguishes conductors to be
	moved from those to be updated.
	Please note this container does not systematically contains a whole
	diagram: it may describe only a part of it, e.g. selected items.
*/
class DiagramContent {
	public:
	DiagramContent();
	DiagramContent(const DiagramContent &);
	~DiagramContent();
	
	/// Used to filter the different items carried by this container.
	enum Filter {
		Elements = 1,
		TextFields = 2,
		Images = 4,
		ConductorsToMove = 8,
		ConductorsToUpdate = 16,
		OtherConductors = 32,
		AnyConductor = 56,
		Shapes = 64,
		All = 127,
		SelectedOnly = 128
	};
	
	/// Hold electrical elements
	QSet<Element *> elements;
	/// Hold independent text items
	QSet<IndependentTextItem *> textFields;
	/// Hold image
	QSet<DiagramImageItem *> images;
	/// Hold shape
	QSet<QetShapeItem *> shapes;
	/// Hold conductors that would get updated considering electrical elements are moved
	QSet<Conductor *> conductorsToUpdate;
	/// Hold conductors that would be moved as is considering electrical elements are moved
	QSet<Conductor *> conductorsToMove;
	/// Hold conductors that would be left untouched considering electrical elements are moved
	QSet<Conductor *> otherConductors;
	
	QList<Conductor *> conductors(int = AnyConductor) const;
	QList<QGraphicsItem *> items(int = All) const;
	QString sentence(int = All) const;
	int count(int = All) const;
	void clear();
};
QDebug &operator<<(QDebug, DiagramContent &);
#endif
