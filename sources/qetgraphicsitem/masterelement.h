#ifndef MASTERELEMENT_H
#define MASTERELEMENT_H

#include "customelement.h"

class MasterElement : public CustomElement
{
	Q_OBJECT
	
	public:
	explicit MasterElement(const ElementsLocation &, QGraphicsItem * = 0, Diagram * = 0, int * = 0);
	~MasterElement();
	virtual void linkToElement(Element *elmt);
	virtual void unlinkAllElements();
	virtual void unlinkElement(Element *elmt);
	
	signals:

	public slots:

};

#endif // MASTERELEMENT_H
