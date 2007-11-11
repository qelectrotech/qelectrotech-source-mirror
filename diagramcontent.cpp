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
	QList<Conductor *> result;
	if (filter & ConductorsToMove)   result += conductorsToMove;
	if (filter & ConductorsToUpdate) result += conductorsToUpdate.keys();
	if (filter & OtherConductors)    result += otherConductors;
	return(result);
}

/**
	Vide le conteneur
*/
void DiagramContent::clear() {
	elements.clear();
	textFields.clear();
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
	return(items_list);
}

/**
	@param filter Types desires
	@return le nombre d'items formant le contenu du schema
*/
int DiagramContent::count(int filter) const {
	int count = 0;
	if (filter & Elements)           count += elements.count();
	if (filter & TextFields)         count += textFields.count();
	if (filter & ConductorsToMove)   count += conductorsToMove.count();
	if (filter & ConductorsToUpdate) count += conductorsToUpdate.count();
	if (filter & OtherConductors)    count += otherConductors.count();
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
	
	return(
		QET::ElementsAndConductorsSentence(
			elements_count,
			conductors_count,
			textfields_count
		)
	);
}

/**
	Permet de debugger un contenu de schema
	@param d Object QDebug a utiliser pour l'affichage des informations de debug
	@param c Contenu de schema a debugger
*/
QDebug &operator<<(QDebug d, DiagramContent &c) {
	d << "DiagramContent {" << "\n";
	d << "  elements :" << c.elements << "\n";
	d << "  conductorsToUpdate :" << c.conductorsToUpdate.keys() << "\n";
	d << "  conductorsToMove :" << c.conductorsToMove << "\n";
	d << "  otherConductors :" << c.otherConductors << "\n";
	d << "}";
	return(d.space());
}
