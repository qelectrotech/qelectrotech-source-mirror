#ifndef TEXTFIELD_EDITOR_H
#define TEXTFIELD_EDITOR_H
#include <QtGui>
#include "elementitemeditor.h"
class PartTextField;
/**
	Cette classe represente un editeur de champ de texte
	Elle permet d'editer a travers une interface graphique les
	proprietes d'un champ de texte : taille de la police, texte par
	defaut et position.
*/
class TextFieldEditor : public ElementItemEditor {
	Q_OBJECT
	// Constructeurs, destructeur
	public:
	TextFieldEditor(QETElementEditor *, PartTextField *, QWidget * = 0);
	virtual ~TextFieldEditor();
	private:
	TextFieldEditor(const TextFieldEditor &);
	
	// attributs
	private:
	PartTextField *part;
	QLineEdit *qle_x, *qle_y, *qle_text;
	QSpinBox *font_size;
	QCheckBox *rotate;
	
	// methodes
	public slots:
	void updateTextField();
	void updateTextFieldX();
	void updateTextFieldY();
	void updateTextFieldT();
	void updateTextFieldS();
	void updateTextFieldR();
	void updateForm();
	
	private:
	void activeConnections(bool);
};
#endif
