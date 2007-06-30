#ifndef TEXT_EDITOR_H
#define TEXT_EDITOR_H
#include <QtGui>
class PartText;
/**
	Cette classe represente un editeur de champ de texte non editable
	Elle permet d'editer a travers une interface graphique les
	proprietes d'un champ de texte non editable.
*/
class TextEditor : public QWidget {
	Q_OBJECT
	// Constructeurs, destructeur
	public:
	TextEditor(PartText *, QWidget * = 0);
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
	void updateForm();
};
#endif
