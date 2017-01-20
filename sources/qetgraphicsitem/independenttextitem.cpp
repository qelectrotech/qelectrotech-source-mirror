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
#include "independenttextitem.h"
#include <QDomElement>

/**
	Constructeur
	@param parent_diagram Le schema auquel est rattache le champ de texte
*/
IndependentTextItem::IndependentTextItem() :
	DiagramTextItem(0)
{}

/**
	Constructeur
	@param text Le texte affiche par le champ de texte
	@param parent_diagram Le schema auquel est rattache le champ de texte
*/
IndependentTextItem::IndependentTextItem(const QString &text) :
	DiagramTextItem(text, 0)
{}

/// Destructeur
IndependentTextItem::~IndependentTextItem() {
}

/**
	Permet de lire le texte a mettre dans le champ a partir d'un element XML.
	Cette methode se base sur la position du champ pour assigner ou non la
	valeur a ce champ.
	@param e L'element XML representant le champ de texte
*/
void IndependentTextItem::fromXml(const QDomElement &e) {
	setPos(e.attribute("x").toDouble(), e.attribute("y").toDouble());
	setHtml(e.attribute("text"));
	setRotationAngle(e.attribute("rotation").toDouble());
}

/**
	@param document Le document XML a utiliser
	@return L'element XML representant ce champ de texte
*/
QDomElement IndependentTextItem::toXml(QDomDocument &document) const {
	QDomElement result = document.createElement("input");
	result.setAttribute("x", QString("%1").arg(pos().x()));
	result.setAttribute("y", QString("%1").arg(pos().y()));
	result.setAttribute("text", toHtml());
	if (rotationAngle()) {
		result.setAttribute("rotation", QString("%1").arg(rotationAngle()));
	}
	return(result);
}
