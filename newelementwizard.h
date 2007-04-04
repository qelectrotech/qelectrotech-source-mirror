/**
	Cette classe represente un dialogue qui permet a l'utilisateur de specifier
	les premiers parametres de l'element qu'il va construire.
	Ces parametres sont :
	  - la largeur
	  - la hauteur
	  - et euh... pleins de choses interessantes
*/

#include <QtGui>
class ElementsCategoriesWidget;
class NewElementWizard : public QDialog {
	Q_OBJECT
	// Constructeur, destructeur
	public:
	NewElementWizard(QWidget * = 0, Qt::WindowFlags = 0);
	~NewElementWizard();
	
	// attributs
	private:
	enum WizardState { Dimensions, Enregistrement };
	QSpinBox *sb_largeur;
	QSpinBox *sb_hauteur;
	QWidget *etape1;
	QWidget *etape2;
	QLineEdit *qle_nom_element;
	QPushButton *button_previous;
	QPushButton *button_next;
	ElementsCategoriesWidget* categories_list;
	WizardState current_state;
	
	// methodes
	public:
	
	// slots
	public slots:
	void previous();
	void next();
	void valid();
};
