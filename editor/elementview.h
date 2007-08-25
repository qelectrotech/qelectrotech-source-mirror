#ifndef ELEMENT_VIEW_H
#define ELEMENT_VIEW_H
#include <QGraphicsView>
#include "elementscene.h"
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
	
	//attributs
	private:
	ElementScene *_scene;
};
#endif
