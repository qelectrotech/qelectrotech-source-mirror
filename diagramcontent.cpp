#include "diagramcontent.h"
#include <QGraphicsItem>
#include "element.h"
#include "diagramtextitem.h"
#include "conductor.h"

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
	conductorsToUpdate(other.conductorsToUpdate),
	conductorsToMove(other.conductorsToMove)
{
}

/**
	Constructeur
*/
DiagramContent::~DiagramContent() {
}

/**
	@return tous les conducteurs
*/
QList<Conductor *> DiagramContent::conductors() const {
	return(conductorsToMove + conductorsToUpdate.keys());
}

/**
	Vide le conteneur
*/
void DiagramContent::clear() {
	elements.clear();
	textFields.clear();
	conductorsToUpdate.clear();
	conductorsToMove.clear();
}

/**
	@return la liste des items formant le contenu du schema
*/
QList<QGraphicsItem *> DiagramContent::items() const {
	QList<QGraphicsItem *> items_list;
	foreach(QGraphicsItem *qgi, conductors()) items_list << qgi;
	foreach(QGraphicsItem *qgi, elements)     items_list << qgi;
	foreach(QGraphicsItem *qgi, textFields)   items_list << qgi;
	return(items_list);
}

/**
	@param include_updated_conductors true pour compter les conducteurs mis a jour, false sinon
	@return le nombre d'items formant le contenu du schema
*/
int DiagramContent::count(bool include_updated_conductors) const {
	int conductors_count = conductorsToMove.count();
	if (include_updated_conductors) conductors_count += conductorsToUpdate.count();
	
	return(
		elements.count()
		+ textFields.count()
		+ conductors_count
		+ conductorsToUpdate.count()
	);
}

/**
	Permet de composer rapidement la proposition "x elements, y conducteurs et
	z champs de texte".
	@param include_updated_conductors true pour compter les conducteurs mis a jour, false sinon
	@return la proposition decrivant le contenu.
*/
QString DiagramContent::sentence(bool include_updated_conductors) const {
	int conductors_count = conductorsToMove.count();
	if (include_updated_conductors) conductors_count += conductorsToUpdate.count();
	
	return(
		QET::ElementsAndConductorsSentence(
			elements.count(),
			conductors_count,
			textFields.count()
		)
	);
}
