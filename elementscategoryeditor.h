#ifndef ELEMENTS_CATEGORY_EDITOR_H
#define ELEMENTS_CATEGORY_EDITOR_H
#include <QtGui>
class ElementsCategory;
class ElementsCategoryEditor : public QDialog {
	Q_OBJECT
	//Constructeurs, destructeur
	public:
	ElementsCategoryEditor(QWidget * = 0);
	ElementsCategoryEditor(const QString &, bool = true, QWidget * = 0);
	~ElementsCategoryEditor();
	
	// attributs
	private:
	QTreeWidget *category_names;
	ElementsCategory *category;
	QDialogButtonBox *buttons;
	bool mode_edit;
	// methodes
	private:
	void buildDialog();
	
	public slots:
	void acceptCreation();
	void acceptUpdate();
};
#endif
