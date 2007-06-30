#ifndef STYLE_EDITOR_H
#define STYLE_EDITOR_H
#include <QtGui>
class CustomElementGraphicPart;
class StyleEditor : public QWidget {
	Q_OBJECT
	// constructeurs, destructeur
	public:
	StyleEditor(CustomElementGraphicPart *, QWidget * = 0);
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
};
#endif
