#include "elementscategorieswidget.h"
#include "elementscategorieslist.h"

ElementsCategoriesWidget::ElementsCategoriesWidget() {
	// initialise la liste des categories
	elementscategorieslist = new ElementsCategoriesList(this);
	
	// initialise la barre d'outils
	toolbar = new QToolBar(this);
	toolbar -> setMovable(false);
	toolbar -> addAction(QIcon(":/ico/reload.png"),     tr("Recharger les cat\351gories"), elementscategorieslist, SLOT(reload())        );
	toolbar -> addAction(QIcon(":/ico/new.png"),        tr("Nouvelle cat\351gorie"),       this,                   SLOT(newCategory())   );
	toolbar -> addAction(QIcon(":/ico/open.png"),       tr("\311diter la cat\351gorie"),   this,                   SLOT(editCategory())  );
	toolbar -> addAction(QIcon(":/ico/editdelete.png"), tr("Supprimer la cat\351gorie"),   this,                   SLOT(removeCategory()));
	
	// disposition verticale
	QVBoxLayout *vlayout = new QVBoxLayout(this);
	vlayout -> setMargin(0);
	vlayout -> setSpacing(0);
	vlayout -> addWidget(toolbar);
	vlayout -> addWidget(elementscategorieslist);
	vlayout -> setStretchFactor(elementscategorieslist, 75000);
	setLayout(vlayout);
}

ElementsCategoriesWidget::~ElementsCategoriesWidget() {
	
}

void ElementsCategoriesWidget::newCategory() {
	
}

void ElementsCategoriesWidget::editCategory() {
	
}

void ElementsCategoriesWidget::removeCategory() {
	qDebug() << elementscategorieslist -> selectedCategoryName();
	qDebug() << elementscategorieslist -> selectedCategoryPath();
}
