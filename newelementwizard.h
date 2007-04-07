#ifndef NEW_ELEMENT_WIZARD_H
#define NEW_ELEMENT_WIZARD_H
#include <QtGui>
/**
	Cette classe represente un dialogue qui permet a l'utilisateur de specifier
	les premiers parametres de l'element qu'il va construire.
	Ces parametres sont :
	  - la largeur
	  - la hauteur
	  - et euh... pleins de choses interessantes
*/
class ElementsCategoriesWidget;
class NamesList;
class Diagram;
class NewElementWizard : public QDialog {
	Q_OBJECT
	// Constructeur, destructeur
	public:
	NewElementWizard(QWidget * = 0, Qt::WindowFlags = 0);
	~NewElementWizard();
	
	// attributs
	private:
	enum WizardState { Category, Filename, Names, Dimensions, Orientations };
	QString str_next, str_previous, str_finish;
	QWidget *step1, *step2, *step3, *step4, *step5;
	ElementsCategoriesWidget* categories_list;
	QLineEdit *qle_filename;
	QSpinBox *sb_width;
	QSpinBox *sb_height;
	QSpinBox *sb_hotspot_x;
	QSpinBox *sb_hotspot_y;
	NamesList *element_names;
	QPushButton *button_previous;
	QPushButton *button_next;
	WizardState current_state;
	QGraphicsView *diagram_view;
	Diagram *diagram_scene;
	
	// methodes
	private:
	void buildStep1();
	void buildStep2();
	void buildStep3();
	void buildStep4();
	void buildStep5();
	bool validStep1();
	bool validStep2();
	bool validStep3();
	bool validStep4();
	bool validStep5();
	
	public slots:
	void previous();
	void next();
	void updateScene();
	void updateHotspotLimits();
};
#endif
