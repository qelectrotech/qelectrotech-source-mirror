#ifndef LINE_EDITOR_H
#define LINE_EDITOR_H
#include <QtGui>
#include "elementitemeditor.h"
class PartLine;
/**
	Cette classe represente le widget d'edition d'une ligne dans l'editeur
	d'element.
*/
class LineEditor : public ElementItemEditor {
	Q_OBJECT
	//constructeurs, destructeur
	public:
	LineEditor(QETElementEditor *, PartLine *, QWidget * = 0);
	~LineEditor();
	private:
	LineEditor(const LineEditor &);
	
	// attributs
	private:
	PartLine *part;
	QLineEdit *x1, *y1, *x2, *y2;
	
	// methodes
	public slots:
	void updateLine();
	void updateLineX1();
	void updateLineY1();
	void updateLineX2();
	void updateLineY2();
	void updateForm();
	
	private:
	void activeConnections(bool);
};
#endif
