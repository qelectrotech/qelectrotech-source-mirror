#include "elementscategoryeditor.h"
#include "elementscategory.h"
#include "nameslist.h"

/**
	Constructeur fournissant un dialogue d'edition de categorie.
	@param category_path Chemin de la categorie a editer ou de la categorie parente en cas de creation
	@param edit booleen a true pour le mode edition, a false pour le mode creation
	@param parent QWidget parent du dialogue
*/
ElementsCategoryEditor::ElementsCategoryEditor(const QString &category_path, bool edit, QWidget *parent) : QDialog(parent) {
	mode_edit = edit;
	// dialogue basique
	buildDialog();
	category = new ElementsCategory(category_path);
	if (mode_edit) {
		setWindowTitle(tr("\311diter une cat\351gorie"));
		connect(buttons, SIGNAL(accepted()), this, SLOT(acceptUpdate()));
		
		// edition de categorie = affichage des noms deja existants
		QHash<QString, QString> cat_names = category -> categoryNames();
		names_list -> setNames(cat_names);
	} else {
		setWindowTitle(tr("Cr\351er une nouvelle cat\351gorie"));
		connect(buttons, SIGNAL(accepted()), this, SLOT(acceptCreation()));
		
		// nouvelle categorie = une ligne pre-machee
		QHash<QString, QString> cat_names;
		cat_names.insert(QLocale::system().name().left(2), tr("Nom de la nouvelle cat\351gorie"));
		names_list -> setNames(cat_names);
		//names_list -> openPersistentEditor(qtwi, 1);
	}
}

/**
	Destructeur
*/
ElementsCategoryEditor::~ElementsCategoryEditor() {
	delete category;
}

/**
	Bases du dialogue de creation / edition
*/
void ElementsCategoryEditor::buildDialog() {
	QVBoxLayout *editor_layout = new QVBoxLayout();
	setLayout(editor_layout);
	
	names_list = new NamesList();
	
	buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
	connect(buttons, SIGNAL(rejected()), this, SLOT(reject()));
	
	editor_layout -> addWidget(new QLabel(tr("Vous pouvez sp\351cifier un nom par langue pour la cat\351gorie.")));
	editor_layout -> addWidget(names_list);
	editor_layout -> addWidget(buttons);
}

/**
	Valide les donnees entrees par l'utilisateur lors d'une creation de
	categorie
*/
void ElementsCategoryEditor::acceptCreation() {
	// il doit y avoir au moins un nom
	if (!names_list -> checkOneName()) return;
	
	// chargement des noms
	category -> clearNames();
	QHash<QString, QString> names = names_list -> names();
	foreach(QString lang, names.keys()) {
		category -> addName(lang, names[lang]);
	}
	
	// cree un nom de dossier a partir du 1er nom de la categorie
	QString dirname = names[names.keys().first()].toLower().replace(" ", "_");
	category -> setPath(category -> path() + "/" + dirname);
	category -> write();
	
	QDialog::accept();
}

/**
	Valide les donnees entrees par l'utilisateur lors d'une modification de
	categorie
*/
void ElementsCategoryEditor::acceptUpdate() {
	// il doit y avoir au moins un nom
	if (!names_list -> checkOneName()) return;
	
	// chargement des noms
	category -> clearNames();
	QHash<QString, QString> names = names_list -> names();
	foreach(QString lang, names.keys()) {
		category -> addName(lang, names[lang]);
	}
	
	category -> write();
	
	QDialog::accept();
}
