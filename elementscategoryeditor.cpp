#include "elementscategoryeditor.h"
#include "elementscategory.h"

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
		QHash<QString, QString> names = category -> categoryNames();
		foreach(QString lang, names.keys()) {
			QStringList qsl;
			qsl << lang << names[lang];
			QTreeWidgetItem *qtwi = new QTreeWidgetItem(qsl);
			qtwi -> setFlags(Qt::ItemIsEditable | Qt::ItemIsEnabled | Qt::ItemIsSelectable);
			category_names -> addTopLevelItem(qtwi);
		}
	} else {
		setWindowTitle(tr("Cr\351er une nouvelle cat\351gorie"));
		connect(buttons, SIGNAL(accepted()), this, SLOT(acceptCreation()));
		
		// nouvelle categorie = une ligne pre-machee
		QStringList qsl;
		qsl << QLocale::system().name().left(2) << tr("Nom de la nouvelle cat\351gorie");
		QTreeWidgetItem *qtwi = new QTreeWidgetItem(qsl);
		category_names -> addTopLevelItem(qtwi);
		category_names -> openPersistentEditor(qtwi, 1);
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
	
	category_names = new QTreeWidget();
	category_names -> setColumnCount(2);
	QStringList headers;
	headers << tr("Langue") << tr("Nom");
	category_names -> setHeaderLabels(headers);
	
	buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
	connect(buttons, SIGNAL(rejected()), this, SLOT(reject()));
	
	editor_layout -> addWidget(new QLabel(tr("Vous pouvez sp\351cifier un nom par langue pour la cat\351gorie.")));
	editor_layout -> addWidget(category_names);
	editor_layout -> addWidget(buttons);
}

/**
	Valide les donnees entrees par l'utilisateur lors d'une creation de
	categorie
*/
void ElementsCategoryEditor::acceptCreation() {
	// il doit y avoir au moins un nom
	int names_count = category_names -> topLevelItemCount();
	if (!names_count || (category_names -> topLevelItem(0) -> text(1)) == QString()) {
		QMessageBox::critical(
			this,
			tr("La cat\351gorie doit avoir au moins un nom."),
			tr("Vous devez entrer au moins un nom pour la cat\351gorie.")
		);
	}
	
	// chargement des noms
	category -> clearNames();
	for (int i = 0 ; i < names_count ; ++ i) {
		category -> addName(
			category_names -> topLevelItem(i) -> text(0),
			category_names -> topLevelItem(i) -> text(1)
		);
	}
	
	// cree un nom de dossier a partir du 1er nom de la categorie
	QString dirname = category_names -> topLevelItem(0) -> text(1).toLower().replace(" ", "_");
	category -> setPath(category -> path() + "/" + dirname);
	category -> write();
	
	QDialog::accept();
}

/**
	Valide les donnees entrees par l'utilisateur lors d'une modification de
	categorie
*/
void ElementsCategoryEditor::acceptUpdate() {
	qDebug() << "acceptUpdate";
}
