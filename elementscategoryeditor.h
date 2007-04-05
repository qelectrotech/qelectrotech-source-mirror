#ifndef ELEMENTS_CATEGORY_EDITOR_H
#define ELEMENTS_CATEGORY_EDITOR_H
#include <QtGui>
/**
	Cette classe permet d'editer une categorie existante ou de creer une
	categorie.
*/
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
	QPushButton *button_add_line;
	QHash<QString, QString> hash_names;
	
	bool mode_edit;
	// methodes
	private:
	void buildDialog();
	void clean();
	bool checkOneName();
	void updateHash();
	
	public slots:
	void acceptCreation();
	void acceptUpdate();
	void addLine();
};
#endif
