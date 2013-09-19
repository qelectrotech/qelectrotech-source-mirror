/*
	Copyright 2006-2013 The QElectroTech Team
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
#include "diagramcontent.h"
#include <QGraphicsItem>
#include "element.h"
#include "independenttextitem.h"
#include "conductor.h"
#include "diagramimageitem.h"

/**
	Constructeur par defaut. Ne contient rien.
*/
DiagramContent::DiagramContent() {
}

/**
	Constructeur de copie.
*/
DiagramContent::DiagramContent(const DiagramContent &other) :
	elements(other.elements),
	textFields(other.textFields),
	images(other.images),
	conductorsToUpdate(other.conductorsToUpdate),
	conductorsToMove(other.conductorsToMove),
	otherConductors(other.otherConductors)
{
}

/**
	Constructeur
*/
DiagramContent::~DiagramContent() {
}

/**
	@param filter Types de conducteurs desires
	@return tous les conducteurs
*/
QList<Conductor *> DiagramContent::conductors(int filter) const {
	QSet<Conductor *> result;
	if (filter & ConductorsToMove)   result += conductorsToMove;
	if (filter & ConductorsToUpdate) result += conductorsToUpdate;
	if (filter & OtherConductors)    result += otherConductors;
	if (filter & SelectedOnly) {
		foreach(Conductor *conductor, result) {
			if (!conductor -> isSelected()) result.remove(conductor);
		}
	}
	return(result.toList());
}

/**
	Vide le conteneur
*/
void DiagramContent::clear() {
	elements.clear();
	textFields.clear();
	images.clear();
	conductorsToUpdate.clear();
	conductorsToMove.clear();
	otherConductors.clear();
}

/**
	@param filter Types desires
	@return la liste des items formant le contenu du schema
*/
QList<QGraphicsItem *> DiagramContent::items(int filter) const {
	QList<QGraphicsItem *> items_list;
	foreach(QGraphicsItem *qgi, conductors(filter)) items_list << qgi;
	if (filter & Elements)   foreach(QGraphicsItem *qgi, elements)   items_list << qgi;
	if (filter & TextFields) foreach(QGraphicsItem *qgi, textFields)  items_list << qgi;
	if (filter & Images) foreach(QGraphicsItem *qgi, images) items_list << qgi;
	if (filter & SelectedOnly) {
		foreach(QGraphicsItem *qgi, items_list) {
			if (!qgi -> isSelected()) items_list.removeOne(qgi);
		}
	}
	return(items_list);
}

/**
	@param filter Types desires
	@return le nombre d'items formant le contenu du schema
*/
int DiagramContent::count(int filter) const {
	int count = 0;
	if (filter == SelectedOnly) {
		if (filter == Elements)           foreach(Element *element,     elements)                  { if (element   -> isSelected()) ++ count; }
		if (filter == TextFields)         foreach(DiagramTextItem *dti, textFields)                { if (dti       -> isSelected()) ++ count; }
		if (filter == Images)			  foreach(DiagramImageItem *dii, images)				   { if (dii	   -> isSelected()) ++ count; }
		if (filter == ConductorsToMove)   foreach(Conductor *conductor, conductorsToMove)          { if (conductor -> isSelected()) ++ count; }
		if (filter == ConductorsToUpdate) foreach(Conductor *conductor, conductorsToUpdate)        { if (conductor -> isSelected()) ++ count; }
		if (filter == OtherConductors)    foreach(Conductor *conductor, otherConductors)           { if (conductor -> isSelected()) ++ count; }
	} else if (filter == All) {
		count += elements.count();
		count += textFields.count();
		count += images.count();
		count += conductorsToMove.count();
		count += conductorsToUpdate.count();
		count += otherConductors.count();
	} else {
		if (filter & Elements)           count += elements.count();
		if (filter & TextFields)         count += textFields.count();
		if (filter & Images)             count += images.count();
		if (filter & ConductorsToMove)   count += conductorsToMove.count();
		if (filter & ConductorsToUpdate) count += conductorsToUpdate.count();
		if (filter & OtherConductors)    count += otherConductors.count();
	}
	return(count);
}

/**
	Permet de composer rapidement la proposition "x elements, y conducteurs et
	z champs de texte".
	@param filter Types desires
	@return la proposition decrivant le contenu.
*/
QString DiagramContent::sentence(int filter) const {
	int elements_count   = (filter & Elements) ? elements.count() : 0;
	int conductors_count = conductors(filter).count();
	int textfields_count = (filter & TextFields) ? textFields.count() : 0;
	int images_count	 = (filter & Images) ? images.count() : 0;
	
	return(
		QET::ElementsAndConductorsSentence(
			elements_count,
			conductors_count,
			textfields_count,
			images_count
		)
	);
}

/**
	Permet de debugger un contenu de schema
	@param d Object QDebug a utiliser pour l'affichage des informations de debug
	@param content Contenu de schema a debugger
*/
QDebug &operator<<(QDebug d, DiagramContent &content) {
	Q_UNUSED(content);
	d << "DiagramContent {" << "\n";
	/*
	FIXME Le double-heritage QObject / QGraphicsItem a casse cet operateur
	d << "  elements :" << c.elements << "\n";
	d << "  conductorsToUpdate :" << c.conductorsToUpdate << "\n";
	d << "  conductorsToMove :" << c.conductorsToMove << "\n";
	d << "  otherConductors :" << c.otherConductors << "\n";
	*/
	d << "}";
	return(d.space());
}
