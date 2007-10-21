#ifndef ELEMENTS_CATEGORY_EDITOR_H
#define ELEMENTS_CATEGORY_EDITOR_H
#include <QtGui>
class ElementsCategory;
class NamesListWidget;
/**
	Cette classe permet d'editer une categorie existante ou de creer une
	categorie.
*/
class ElementsCategoryEditor : public QDialog {
	Q_OBJECT
	
	// constructeurs, destructeur
	public:
	ElementsCategoryEditor(QWidget * = 0);
	ElementsCategoryEditor(const QString &, bool = true, QWidget * = 0);
	virtual ~ElementsCategoryEditor();
	
	private:
	ElementsCategoryEditor(const ElementsCategoryEditor &);
	
	// attributs
	private:
	ElementsCategory *category;
	QDialogButtonBox *buttons;
	NamesListWidget *names_list;
	bool mode_edit;
	
	// methodes
	private:
	void buildDialog();
	
	public slots:
	void acceptCreation();
	void acceptUpdate();
};
#endif
