#ifndef ARC_EDITOR_H
#define ARC_EDITOR_H
#include <QtGui>
#include "elementitemeditor.h"
class PartArc;
class ArcEditor : public ElementItemEditor {
	Q_OBJECT
	//constructeurs, destructeur
	public:
	ArcEditor(QETElementEditor *, PartArc *, QWidget * = 0);
	~ArcEditor();
	private:
	ArcEditor(const ArcEditor &);
	
	// attributs
	private:
	PartArc *part;
	QLineEdit *x, *y, *h, *v;
	QSpinBox *angle, *start_angle;
	
	// methodes
	public slots:
	void updateArc();
	void updateArcX();
	void updateArcY();
	void updateArcH();
	void updateArcV();
	void updateArcS();
	void updateArcA();
	void updateForm();
	
	private:
	void activeConnections(bool);
};
#endif
