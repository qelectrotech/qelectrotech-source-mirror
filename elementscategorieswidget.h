#ifndef ELEMENTS_CATEGORIES_WIDGET_H
#define ELEMENTS_CATEGORIES_WIDGET_H
#include <QtGui>
/**
	Cette classe represente un widget integrant la liste des categories
	de l'utilisteur surplombee de boutons permettant d'ajouter 
*/
class ElementsCategoriesList;
class ElementsCategoriesWidget : public QWidget {
	Q_OBJECT
	// Constructeur, destructeur
	public:
	ElementsCategoriesWidget();
	~ElementsCategoriesWidget();
	
	// attributs
	private:
	ElementsCategoriesList *elementscategorieslist;
	QToolBar *toolbar;
	
	// methodes
	public:
	inline ElementsCategoriesList &elementsCategoriesList() const { return(*elementscategorieslist); }
	
	public slots:
	void newCategory();
	void editCategory();
	void removeCategory();
};
#endif
