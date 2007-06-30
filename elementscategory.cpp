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
	
	category_names.fromXml(root);
	
	// ferme le fichier
	directory_conf.close();
}

/**
	Methode permettant d'obtenir le nom affichable de cette categorie.
	@return Le nom affichable de la categorie
*/
QString ElementsCategory::name() const {
	return(category_names.name(dirName()));
}

/**
	@return La liste des differents noms possibles pour la categorie
*/
NamesList ElementsCategory::categoryNames() const {
	return(category_names);
}

/**
	Vide la liste des noms de la categorie
*/
void ElementsCategory::clearNames() {
	category_names.clearNames();
}

/**
	Ajoute un nom a la categorie.
	Si la langue existe deja, le nom pour cette langue est remplace.
	@param lang La langue pour laquelle le nom est utilisable
	@param value Le nom
*/
void ElementsCategory::addName(const QString &lang, const QString &value) {
	category_names.addName(lang, value);
}

/**
	Cree la categorie
	@return true si la creation a reussi, false sinon
*/
bool ElementsCategory::write() const {
	
	// cree le dossier de la categorie
	if (!mkpath(path())) return(false);
	
	// prepare la structure XML
	QDomDocument document;
	QDomElement root = document.createElement("qet-directory");
	document.appendChild(root);
	root.appendChild(category_names.toXml(document));
	
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

/**
	@return true s'il est possible d'ecrire le fichier qet_directory dans la
	categorie
*/
bool ElementsCategory::isWritable() const {
	// informations sur le dossier de la categorie
	QFileInfo category(canonicalPath());
	QFileInfo qet_directory(canonicalPath() + "/.qet_directory");
	/*
	soit .qet_directory n'existe pas et le dossier est accessible en ecriture,
	soit .qet_directory existe et est accessible en ecriture
	*/
	return(
		(!qet_directory.exists() && category.isWritable()) ||\
		(qet_directory.exists()  && qet_directory.isWritable())
	);
}
