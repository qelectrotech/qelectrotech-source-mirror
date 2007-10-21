#ifndef STYLE_EDITOR_H
#define STYLE_EDITOR_H
#include <QtGui>
#include "elementitemeditor.h"
class CustomElementGraphicPart;
/**
	Cette classe represente un widget d'edition des styles que peut comporter
	une partie d'elements (couleur, epaisseur et style du trait, remplissage,
	antialiasing). Elle peut accueillir un widget sous cette interface grace a
	la methode appendWidget.
*/
class StyleEditor : public ElementItemEditor {
	Q_OBJECT
	// constructeurs, destructeur
	public:
	StyleEditor(QETElementEditor *, CustomElementGraphicPart *, QWidget * = 0);
	virtual ~StyleEditor();
	
	private:
	StyleEditor(const StyleEditor &);
	
	// attributs
	private:
	CustomElementGraphicPart *part;
	QVBoxLayout *main_layout;
	QButtonGroup *color, *style, *weight, *filling;
	QRadioButton *black_color, *white_color, *normal_style, *dashed_style;
	QRadioButton *none_weight, *thin_weight, *normal_weight, *no_filling;
	QRadioButton *black_filling, *white_filling;
	QCheckBox *antialiasing;
	
	//methodes
	public:
	void appendWidget(QWidget *w);
	
	public slots:
	void updatePart();
	void updateForm();
	void updatePartAntialiasing();
	void updatePartColor();
	void updatePartLineStyle();
	void updatePartLineWeight();
	void updatePartFilling();
	
	private:
	void activeConnections(bool);
};
#endif
