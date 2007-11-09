#ifndef DIAGRAM_CONTENT_H
#define DIAGRAM_CONTENT_H
#include <QtGui>
class Conductor;
class Element;
class Terminal;
class DiagramTextItem;
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
	
	/// Elements de texte du schema
	QList<Element *> elements;
	/// Champs de texte du schema
	QList<DiagramTextItem *> textFields;
	/// Conducteurs a mettre a jour du schema
	QHash<Conductor *, Terminal *> conductorsToUpdate;
	/// Conducteurs a deplacer du schema
	QList<Conductor *> conductorsToMove;
	
	QList<Conductor *> conductors() const;
	QList<QGraphicsItem *> items() const;
	QString sentence(bool = false) const;
	int count(bool = false) const;
	void clear();
};
#endif
