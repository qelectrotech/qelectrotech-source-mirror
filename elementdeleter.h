#ifndef ELEMENT_DELETER_H
#define ELEMENT_DELETER_H
#include "elementscategory.h"
#include <QtGui>
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
