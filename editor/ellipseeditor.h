#ifndef ELLIPSE_EDITOR_H
#define ELLIPSE_EDITOR_H
#include <QtGui>
#include "elementitemeditor.h"
class PartEllipse;
class EllipseEditor : public ElementItemEditor {
	Q_OBJECT
	//constructeurs, destructeur
	public:
	EllipseEditor(QETElementEditor *, PartEllipse *, QWidget * = 0);
	~EllipseEditor();
	private:
	EllipseEditor(const EllipseEditor &);
	
	// attributs
	private:
	PartEllipse *part;
	QLineEdit *x, *y, *h, *v;
	
	// methodes
	public slots:
	void updateEllipse();
	void updateEllipseX();
	void updateEllipseY();
	void updateEllipseH();
	void updateEllipseV();
	void updateForm();
	
	private:
	void activeConnections(bool);
};
#endif
