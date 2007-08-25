#ifndef TEXT_EDITOR_H
#define TEXT_EDITOR_H
#include <QtGui>
#include "elementitemeditor.h"
class PartText;
/**
	Cette classe represente un editeur de champ de texte non editable
	Elle permet d'editer a travers une interface graphique les
	proprietes d'un champ de texte non editable.
*/
class TextEditor : public ElementItemEditor {
	Q_OBJECT
	// Constructeurs, destructeur
	public:
	TextEditor(QETElementEditor *, PartText *, QWidget * = 0);
	virtual ~TextEditor();
	private:
	TextEditor(const TextEditor &);
	
	// attributs
	private:
	PartText *part;
	QLineEdit *qle_x, *qle_y, *qle_text;
	QSpinBox *font_size;
	
	// methodes
	public slots:
	void updateText();
	void updateTextX();
	void updateTextY();
	void updateTextT();
	void updateTextS();
	void updateForm();
	
	private:
	void activeConnections(bool);
};
#endif
