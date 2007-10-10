#ifndef ELEMENTS_CATEGORY_DELETER_H
#define ELEMENTS_CATEGORY_DELETER_H
#include "elementscategory.h"
#include <QtGui>
/**
	Cette ckasse represente une couche d'abstraction pour supprimer
	une categorie d'elements et les elements qu'elle contient.
	Elle demande notamment confirmation a l'utilisateur par deux fois.
*/
class ElementsCategoryDeleter : public QWidget {
	Q_OBJECT
	// constructeurs, destructeur
	public:
	ElementsCategoryDeleter(const QString &, QWidget * = 0);
	virtual ~ElementsCategoryDeleter();
	private:
	ElementsCategoryDeleter(const ElementsCategory &);
	
	// methodes
	public slots:
	void exec();
	
	// attributs
	private:
	ElementsCategory cat;
	bool empty_category_path;
};
#endif
