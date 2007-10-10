#ifndef ELEMENT_DELETER_H
#define ELEMENT_DELETER_H
#include "elementscategory.h"
#include <QtGui>
/**
	Cette classe represente une couche d'abstraction pour supprimer
	un element de la collection d'elements.
	Elle demande notamment confirmation a l'utilisateur
*/
class ElementDeleter : public QWidget {
	Q_OBJECT
	// constructeurs, destructeur
	public:
	ElementDeleter(const QString &, QWidget * = 0);
	virtual ~ElementDeleter();
	private:
	ElementDeleter(const ElementsCategory &);
	
	// methodes
	public slots:
	void exec();
	
	// attributs
	private:
	QString element_path;
};
#endif
