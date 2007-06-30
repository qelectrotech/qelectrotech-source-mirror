#ifndef CUSTOM_ELEMENT_PART_H
#define CUSTOM_ELEMENT_PART_H
#include <QtGui>
#include <QtXml>
#include <QImage>
class CustomElement;
/**
	Cette classe abstraite represente une partie de la representation graphique
	d'un element de schema electrique. Les attributs et methodes qu'elle
	encapsule ne sont pas integres directement dans la classe CustomElement
	afin de ne pas alourdir celle-ci. Il est en effet inutile pour cette classe
	de retenir sa conception graphique autrement que sous la forme d'une
	QImage.
*/
class CustomElementPart {
	// constructeurs, destructeur
	public:
	CustomElementPart() {};
	virtual ~CustomElementPart() {
		qDebug() << "~CustomElementPart()";
	};
	private:
	CustomElementPart(const CustomElementPart &);
	
	// attributs
	private:
	
	// methodes
	public:
	virtual void fromXml(const QDomElement &) = 0;
	virtual const QDomElement toXml(QDomDocument &) const = 0;
	virtual QWidget *elementInformations() = 0;
	//virtual void renderToCustomElement(CustomElement &) const = 0;
	//virtual void toEditorPart(const EditorPart &);
	//virtual void fromEditorPart(const EditorPart &) = 0;
	protected:
	QPicture *getCustomElementQPicture(CustomElement &ce) const;
};
#endif
