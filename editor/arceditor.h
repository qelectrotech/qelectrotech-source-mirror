#ifndef ARC_EDITOR_H
#define ARC_EDITOR_H
#include <QtGui>
class PartArc;
class ArcEditor : public QWidget {
	Q_OBJECT
	//constructeurs, destructeur
	public:
	ArcEditor(PartArc *, QWidget * = 0);
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
	void updateForm();
};
#endif
