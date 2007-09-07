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
class NewElementWizard : public QWizard {
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
	ElementsCategoriesWidget* categories_list;
	QLineEdit *qle_filename;
	NamesListWidget *element_names;
	OrientationSetWidget *orientation_set;
	HotspotEditor *hotspot_editor;
	WizardState current_state;
	QString chosen_file;
	
	// methodes
	private:
	QWizardPage *buildStep1();
	QWizardPage *buildStep2();
	QWizardPage *buildStep3();
	QWizardPage *buildStep4();
	QWizardPage *buildStep5();
	bool validStep1();
	bool validStep2();
	bool validateCurrentPage();
	void createNewElement();
};
#endif
