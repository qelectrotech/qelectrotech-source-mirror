#ifndef ELEMENTS_CATEGORY_DELETER_H
#define ELEMENTS_CATEGORY_DELETER_H
#include "elementscategory.h"
#include <QtGui>
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
