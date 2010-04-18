/*
	Copyright 2006-2010 Xavier Guerrin
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
class Conductor;
class Element;
class Terminal;
class IndependentTextItem;
/**
	Cette classe est un conteneur pour passer facilement le contenu d'un schema
	a une fonction. Il permet d'acceder rapidement aux differents types de
	composants que l'on peut trouver sur un schema, comme les elements, les
	champs de texte, les conducteurs (a deplacer ou a mettre a jour, en cas de
	deplacements), etc.
	A noter que ce container ne contient pas systematiquement l'integralite
	d'un schema. Il peut n'en contenir qu'une partie, typiquement les
	composants selectionnes.
*/
class DiagramContent {
	public:
	DiagramContent();
	DiagramContent(const DiagramContent &);
	~DiagramContent();
	
	/// Permet de filtrer facilement les differentes parties d'un schema
	enum Filter {
		Elements = 1,
		TextFields = 2,
		ConductorsToMove = 4,
		ConductorsToUpdate = 8,
		OtherConductors = 16,
		AnyConductor = 28,
		All = 31,
		SelectedOnly = 32
	};
	
	/// Elements de texte du schema
	QList<Element *> elements;
	/// Champs de texte independants du schema
	QList<IndependentTextItem *> textFields;
	/// Conducteurs a mettre a jour du schema
	QHash<Conductor *, Terminal *> conductorsToUpdate;
	/// Conducteurs a deplacer du schema
	QList<Conductor *> conductorsToMove;
	/// Conducteurs isoles (ni a deplacer, ni a mettre a jour)
	QList<Conductor *> otherConductors;
	
	QList<Conductor *> conductors(int = AnyConductor) const;
	QList<QGraphicsItem *> items(int = All) const;
	QString sentence(int = All) const;
	int count(int = All) const;
	void clear();
};
QDebug &operator<<(QDebug, DiagramContent &);
#endif
