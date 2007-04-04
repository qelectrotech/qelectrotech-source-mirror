#include <QtXml>
#include "elementscategorieslist.h"
#include "qetapp.h"

/**
	Constructeur
	@param parent QWidget parent de ce widget
*/
ElementsCategoriesList::ElementsCategoriesList(QWidget *parent) : QTreeWidget(parent) {
	// selection unique
	setSelectionMode(QAbstractItemView::SingleSelection);
	setColumnCount(1);
	header() -> hide();
	
	// charge les categories
	reload();
}

/**
	Destructeur
*/
ElementsCategoriesList::~ElementsCategoriesList() {
	
}

/**
	Recharge l'arbre des categories
*/
void ElementsCategoriesList::reload() {
	// vide l'arbre
	while (takeTopLevelItem(0));
	
	// chargement des elements de la collection utilisateur
	addDir(invisibleRootItem(), QETApp::customElementsDir(), tr("Collection utilisateur"));
}

/**
	Methode privee permettant d'ajouter un dossier au panel d'appareils
	@param qtwi_parent QTreeWidgetItem parent sous lequel sera insere l'element
	@param dossier Chemin absolu du dossier a inserer
	@param nom Parametre facultatif permettant de forcer le nom du dossier.
	S'il n'est pas precise, la fonction ouvre le fichier qet_directory situe
	dans le dossier et y lit le nom du dossier ; si ce fichier n'existe pas ou
	est invalide, la fonction utilise le nom du dossier.
*/
void ElementsCategoriesList::addDir(QTreeWidgetItem *qtwi_parent, QString adr_dossier, QString nom) {
	QDir dossier(adr_dossier);
	if (!dossier.exists()) return;
	adr_dossier = dossier.canonicalPath() + "/";
	
	// recupere le nom de la categorie
	QString nom_categorie = (nom != QString()) ? nom : categoryName(dossier);
	
	// creation du QTreeWidgetItem representant le dossier
	QTreeWidgetItem *qtwi_dossier = new QTreeWidgetItem(qtwi_parent, QStringList(nom_categorie));
	qtwi_dossier -> setData(0, Qt::UserRole, dossier.absolutePath());
	qtwi_dossier -> setExpanded(true);
	
	// ajout des sous-categories / sous-dossiers
	QStringList dossiers = dossier.entryList(QStringList(), QDir::AllDirs | QDir::NoSymLinks | QDir::NoDotAndDotDot, QDir::Name);
	foreach(QString dossier, dossiers) addDir(qtwi_dossier, adr_dossier + dossier);
}

/**
	Methode permettant d'obtenir le nom affichable d'une categorie etant donne
	son chemin (dossier).
	@param directory le chemin du dossier representant la categorie
	@return Le nom affichable de la categorie
*/
QString ElementsCategoriesList::categoryName(QDir &directory) {
	// en cas d'echec de la lecture du fichier de configuration
	// "qet_directory", le nom du dossier est retourne
	QString category_name = directory.dirName();
	// repere le chemin du fichier de configuration de la categorie
	QFile directory_conf(directory.canonicalPath() + "/qet_directory");
	// verifie l'existence du fichier
	if (directory_conf.exists()) {
		// ouvre le fichier
		if (directory_conf.open(QIODevice::ReadOnly | QIODevice::Text)) {
			// recupere les deux premiers caracteres de la locale en cours du systeme
			QString system_language = QLocale::system().name().left(2);
			// lit le contenu du fichier dans un QDomDocument XML
			QDomDocument document;
			if (document.setContent(&directory_conf)) {
				/* parcourt le document XML a la recherche d'un nom
				par ordre de preference, on prendra :
					- le nom dans la langue du systeme
					- le nom en anglais
					- le nom du dossier
				*/
				QDomElement root = document.documentElement();
				if (root.tagName() == "qet-directory") {
					bool name_found = false;
					// parcourt les "names"
					for (QDomNode node = root.firstChild() ; !node.isNull() ; node = node.nextSibling()) {
						QDomElement names = node.toElement();
						if (names.isNull() || names.tagName() != "names") continue;
						// parcourt les "name"
						for (QDomNode n = names.firstChild() ; !n.isNull() ; n = n.nextSibling()) {
							QDomElement name = n.toElement();
							if (name.isNull() || name.tagName() != "name") continue;
							if (name.attribute("lang") == system_language) {
								category_name = name.text();
								name_found = true;
								break;
							} else if (name.attribute("lang") == "en") {
								category_name = name.text();
							}
						}
						if (name_found) break;
					}
				}
			}
			directory_conf.close();
		}
	}
	return(category_name);
}

/**
	@return Le dossier de la categorie selectionnee
*/
QString ElementsCategoriesList::selectedCategoryPath() {
	QTreeWidgetItem *qtwi = currentItem();
	if (qtwi) return(qtwi -> data(0, Qt::UserRole).toString());
	else return(NULL);
}

/**
	@return Le nom de la categorie selectionnee
*/
QString ElementsCategoriesList::selectedCategoryName() {
	QTreeWidgetItem *qtwi = currentItem();
	if (qtwi) return(qtwi -> data(0, Qt::DisplayRole).toString());
	else return(NULL);
}
