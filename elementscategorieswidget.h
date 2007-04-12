#ifndef ELEMENTS_CATEGORIES_WIDGET_H
#define ELEMENTS_CATEGORIES_WIDGET_H
#include <QtGui>
/**
	Cette classe represente un widget integrant la liste des categories
	de l'utilisteur surplombee de boutons permettant d'ajouter, de modifier
	ou de supprimer des categories
*/
class ElementsCategoriesList;
class ElementsCategoriesWidget : public QWidget {
	Q_OBJECT
	
	// Constructeurs, destructeur
	public:
	ElementsCategoriesWidget(QWidget * = 0);
	virtual ~ElementsCategoriesWidget();
	
	private:
	ElementsCategoriesWidget(const ElementsCategoriesWidget &);
	
	// attributs
	private:
	ElementsCategoriesList *elementscategorieslist;
	QToolBar *toolbar;
	QAction *action_reload;
	QAction *action_new;
	QAction *action_open;
	QAction *action_delete;
	
	// methodes
	public:
	ElementsCategoriesList &elementsCategoriesList() const;
	
	public slots:
	void newCategory();
	void editCategory();
	void removeCategory();
	void updateButtons();
};

/**
	@return La liste des categories d'elements du widget
*/
inline ElementsCategoriesList &ElementsCategoriesWidget::elementsCategoriesList() const {
	return(*elementscategorieslist);
}

#endif
