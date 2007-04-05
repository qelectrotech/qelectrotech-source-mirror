#include "elementscategorieswidget.h"
#include "elementscategorieslist.h"
#include "elementscategoryeditor.h"
#include "elementscategory.h"

ElementsCategoriesWidget::ElementsCategoriesWidget(QWidget * parent) : QWidget(parent) {
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
	QString s_c_path = elementscategorieslist -> selectedCategoryPath();
	if (s_c_path.isNull()) return;
	(new ElementsCategoryEditor(s_c_path, false, this)) -> exec();
	elementscategorieslist -> reload();
}

void ElementsCategoriesWidget::editCategory() {
	QString s_c_path = elementscategorieslist -> selectedCategoryPath();
	if (s_c_path.isNull()) return;
	(new ElementsCategoryEditor(s_c_path, true, this)) -> exec();
	elementscategorieslist -> reload();
}

void ElementsCategoriesWidget::removeCategory() {
	// recupere le nom et le chemin de la categorie
	QString s_c_name = elementscategorieslist -> selectedCategoryName();
	QString s_c_path = elementscategorieslist -> selectedCategoryPath();
	
	if (s_c_path.isNull()) return;
	
	// confirmation #1
	QMessageBox::StandardButton answer_1 = QMessageBox::question(
		this,
		tr("Supprimer la cat\351gorie ?"),
		tr("\312tes-vous s\373r de vouloir supprimer cette cat\351gorie ?\n"
		"Tous les \351l\351ments et les cat\351gories contenus dans cette "
		"cat\351gorie seront supprim\351s"),
		QMessageBox::Yes|QMessageBox::No|QMessageBox::Cancel
	);
	if (answer_1 != QMessageBox::Yes) return;
	
	// confirmation #2
	QMessageBox::StandardButton answer_2 = QMessageBox::question(
		this,
		tr("Supprimer la cat\351gorie ?"),
		tr("\312tes-vous vraiment s\373r de vouloir supprimer cette "
		"cat\351gorie ?\nLes changements seront d\351finitifs."),
		QMessageBox::Yes|QMessageBox::No|QMessageBox::Cancel
	);
	if (answer_2 != QMessageBox::Yes) return;
	
	// verifie l'existence de la categorie
	ElementsCategory category(s_c_path);
	if (!category.exists()) return;
	
	// supprime la categorie
	if (!category.remove()) {
		QMessageBox::warning(
			this,
			tr("Suppression de la cat\351gorie"),
			tr("La suppression de la cat\351gorie a \351chou\351.\n"
			"V\351rifiez vos droits sur le dossier ") + s_c_path + tr(".")
		);
	}
	
	// recharge la liste des categories
	elementscategorieslist -> reload();
}
