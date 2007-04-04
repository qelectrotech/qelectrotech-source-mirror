#ifndef ELEMENTS_CATEGORIES_LIST_H
#define ELEMENTS_CATEGORIES_LIST_H
#include <QtGui>
/**
	Cette classe fournit une liste graphique des categories d'elements de
	l'utilisateur.
*/
class ElementsCategoriesList : public QTreeWidget {
	Q_OBJECT
	// Constructeur, destructeur
	public:
	ElementsCategoriesList(QWidget * = 0);
	~ElementsCategoriesList();
	
	// attributs
	
	
	// methodes
	private:
	void addDir(QTreeWidgetItem *, QString, QString = QString());
	QString categoryName(QDir &);
	
	public:
	QString selectedCategoryPath();
	QString selectedCategoryName();
	
	public slots:
	void reload();
};
#endif
