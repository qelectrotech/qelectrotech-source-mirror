#ifndef ELEMENT_VIEW_H
#define ELEMENT_VIEW_H
#include <QGraphicsView>
#include "elementscene.h"
/**
	Cette classe represente un widget permettant de visualiser une
	ElementScene, c'est-a-dire la classe d'edition des elements.
*/
class ElementView : public QGraphicsView {
	Q_OBJECT
	// constructeurs, destructeur
	public:
	ElementView(ElementScene *, QWidget * = 0);
	virtual ~ElementView();
	
	private:
	ElementView(const ElementView &);
	
	// methodes
	public:
	ElementScene *scene() const;
	void setScene(ElementScene *);
	protected:
	bool event(QEvent *);
	
	//attributs
	private:
	ElementScene *scene_;
};
#endif
