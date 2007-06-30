#ifndef LINE_EDITOR_H
#define LINE_EDITOR_H
#include <QtGui>
class PartLine;
class LineEditor : public QWidget {
	Q_OBJECT
	//constructeurs, destructeur
	public:
	LineEditor(PartLine *, QWidget * = 0);
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
	void updateForm();
};
#endif
