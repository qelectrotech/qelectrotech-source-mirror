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
#ifndef DIAGRAM_POSITION_H
#define DIAGRAM_POSITION_H
#include <QPointF>
#include <QString>
#include <QRegExp>
/**
	Cette classe represente la position d'un element sur le schema. Il ne
	s'agit pas de ses coordonnees (bien que celles-ci puissent etre embarquees
	par commodite), mais du secteur du schema dans lequel il se situe, par
	exemple B2 ou C4.
*/
class DiagramPosition {
	// constructeurs, destructeur
	public:
	DiagramPosition(const QString & = "", unsigned int = 0);
	virtual ~DiagramPosition();
	
	// methodes
	public:
	QPointF position() const;
	void setPosition(const QPointF &);
	QString toString();
	bool isOutOfBounds() const;
	
	// attributs
	private:
	QString letter_;
	unsigned int number_;
	QPointF position_;
};
#endif
