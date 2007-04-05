#include "elementscategory.h"
#include <QtXml>

/**
	Constructeur
	@param path Chemin du dossier de la categorie
*/
ElementsCategory::ElementsCategory(const QString &path) : QDir(path) {
	if (path != QString()) loadNames();
}

/**
	Destructeur
*/
ElementsCategory::~ElementsCategory() {
	
}

/**
	Supprime un repertoire recursivement.
	@return true si la suppression a reussie, false sinon
*/
bool ElementsCategory::rmdir(const QString &path) const {
	QDir directory(path);
	
	// supprime les dossiers, recursivement
	foreach (QString file, directory.entryList(QDir::AllDirs | QDir::NoDotAndDotDot)) {
		if (!rmdir(directory.absolutePath() + "/" + file)) return(false);
	}
	
	// supprime les fichiers
	foreach (QString file, directory.entryList(QDir::Files | QDir::NoDotAndDotDot)) {
		if (!directory.remove(file)) return(false);
	}
	
	// supprime le dossier lui-meme
	return(directory.rmdir(path));
}

/**
	Charge la liste des noms possibles pour la categorie
*/
void ElementsCategory::loadNames() {
	// repere le chemin du fichier de configuration de la categorie
	QFile directory_conf(canonicalPath() + "/qet_directory");
	
	// verifie l'existence du fichier
	if (!directory_conf.exists()) return;
	
	// ouvre le fichier
	if (!directory_conf.open(QIODevice::ReadOnly | QIODevice::Text)) return;
	
	// lit le contenu du fichier dans un QDomDocument XML
	QDomDocument document;
	if (!document.setContent(&directory_conf)) return;
	
	// verifie la racine
	QDomElement root = document.documentElement();
	if (root.tagName() != "qet-directory") return;
	
	// parcourt les "names"
	for (QDomNode node = root.firstChild() ; !node.isNull() ; node = node.nextSibling()) {
		QDomElement names = node.toElement();
		if (names.isNull() || names.tagName() != "names") continue;
		// parcourt les "name"
		for (QDomNode n = names.firstChild() ; !n.isNull() ; n = n.nextSibling()) {
			QDomElement name = n.toElement();
			if (name.isNull() || name.tagName() != "name") continue;
			category_names.insert(name.attribute("lang"), name.text());
		}
	}
	
	// ferme le fichier
	directory_conf.close();
}

/**
	Methode permettant d'obtenir le nom affichable de cette categorie.
	Par ordre de preference, on prendra :
		- le nom dans la langue du systeme
		- le nom en anglais
		- le nom du dossier
	@return Le nom affichable de la categorie
*/
QString ElementsCategory::name() const {
	// recupere les deux premiers caracteres de la locale en cours du systeme
	QString system_language = QLocale::system().name().left(2);
	QString category_name;
	if (category_names[system_language] != QString()) {
		category_name = category_names[system_language];
	} else if (category_names["en"] != QString()) {
		category_name = category_names["en"];
	} else {
		category_name = dirName();
	}
	return(category_name);
}

/**
	@return La liste des differents noms possibles pour la categorie
*/
QHash<QString, QString> ElementsCategory::categoryNames() const {
	return(category_names);
}

void ElementsCategory::clearNames() {
	category_names.clear();
}

void ElementsCategory::addName(const QString &lang, const QString &value) {
	category_names.insert(lang, value);
}

bool ElementsCategory::write() const {
	
	// cree le dossier de la categorie
	if (!mkpath(path())) return(false);
	
	// prepare la structure XML
	QDomDocument document;
	QDomElement root = document.createElement("qet-directory");
	document.appendChild(root);
	QDomElement names = document.createElement("names");
	root.appendChild(names);
	foreach(QString lang, category_names.keys()) {
		QDomElement name = document.createElement("name");
		name.setAttribute("lang", lang);
		QDomText name_value = document.createTextNode(category_names[lang]);
		name.appendChild(name_value);
		names.appendChild(name);
	}
	
	// repere le chemin du fichier de configuration de la categorie
	QFile directory_conf(canonicalPath() + "/qet_directory");
	
	// ouvre le fichier
	if (!directory_conf.open(QIODevice::Text | QIODevice::WriteOnly)) return(false);
	
	// ecrit le fichier
	QTextStream out(&directory_conf);
	out.setCodec("UTF-8");
	out << document.toString(4);
	
	// ferme le fichier
	directory_conf.close();
	
	return(true);
}

/**
	Supprime la categorie
	@return true si la suppression a reussie, false sinon
*/
bool ElementsCategory::remove() const {
	return(rmdir(absolutePath()));
}

