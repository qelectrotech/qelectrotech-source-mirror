#ifndef NEW_ELEMENT_WIZARD_H
#define NEW_ELEMENT_WIZARD_H
#include <QtGui>
/**
	Cette classe represente un dialogue qui permet a l'utilisateur de specifier
	les premiers parametres de l'element qu'il va construire.
	Ces parametres sont :
	  - la categorie dans laquelle il faut enregistrer l'element
	  - le nom du fichier dans lequel il faut enregistrer l'element
	  - les noms de cet element
	  - les dimensions
	  - la position du hotspot
	  - les orientations possibles
*/
class ElementsCategoriesWidget;
class NamesListWidget;
class OrientationSetWidget;
class HotspotEditor;
class NewElementWizard : public QDialog {
	Q_OBJECT
	
	// constructeurs, destructeur
	public:
	NewElementWizard(QWidget * = 0, Qt::WindowFlags = 0);
	virtual ~NewElementWizard();
	
	private:
	NewElementWizard(const NewElementWizard &);
	
	// attributs
	private:
	enum WizardState { Category, Filename, Names, Dimensions, Orientations };
	QString str_next, str_previous, str_finish;
	QWidget *step1, *step2, *step3, *step4, *step5;
	ElementsCategoriesWidget* categories_list;
	QLineEdit *qle_filename;
	NamesListWidget *element_names;
	OrientationSetWidget *orientation_set;
	HotspotEditor *hotspot_editor;
	QPushButton *button_previous;
	QPushButton *button_next;
	WizardState current_state;
	QString chosen_file;
	
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
	void createNewElement();
	
	public slots:
	void previous();
	void next();
};
#endif
