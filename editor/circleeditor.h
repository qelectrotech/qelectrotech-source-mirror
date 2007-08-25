#ifndef CIRCLE_EDITOR_H
#define CIRCLE_EDITOR_H
#include <QtGui>
#include "elementitemeditor.h"
class PartCircle;
/**
	Cette classe represente un editeur de cercle.
	Elle permet d'editer a travers une interface graphique les
	proprietes d'une cercle composant le dessin d'un element.
*/
class CircleEditor : public ElementItemEditor {
	Q_OBJECT
	// Constructeurs, destructeur
	public:
	CircleEditor(QETElementEditor *, PartCircle *, QWidget * = 0);
	virtual ~CircleEditor();
	private:
	CircleEditor(const CircleEditor &);
	
	// attributs
	private:
	PartCircle *part;
	QLineEdit *x, *y, *r;
	
	// methodes
	public slots:
	void updateCircle();
	void updateCircleX();
	void updateCircleY();
	void updateCircleD();
	void updateForm();
	
	private:
	void activeConnections(bool);
};
#endif
