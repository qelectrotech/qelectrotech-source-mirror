#ifndef ELLIPSE_EDITOR_H
#define ELLIPSE_EDITOR_H
#include <QtGui>
class PartEllipse;
class EllipseEditor : public QWidget {
	Q_OBJECT
	//constructeurs, destructeur
	public:
	EllipseEditor(PartEllipse *, QWidget * = 0);
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
	void updateForm();
};
#endif
