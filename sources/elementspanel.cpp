/*
	Copyright 2006-2008 Xavier Guerrin
	This file is part of QElectroTech.
	
	QElectroTech is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 2 of the License, or
	(at your option) any later version.
	
	QElectroTech is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.
	
	You should have received a copy of the GNU General Public License
	along with QElectroTech.  If not, see <http://www.gnu.org/licenses/>.
*/
#include "elementspanel.h"
#include "qetapp.h"
#include "elementscategory.h"
#include "elementscategoryeditor.h"
#include "elementscategorydeleter.h"
#include "elementdeleter.h"
#include "customelement.h"
#include "qetelementeditor.h"

/**
	Constructeur
	@param parent Le QWidget parent du panel d'appareils
*/
ElementsPanel::ElementsPanel(QWidget *parent) : QTreeWidget(parent) {
	
	// selection unique
	setSelectionMode(QAbstractItemView::SingleSelection);
	setColumnCount(1);
	header() -> hide();
	
	// drag'n drop autorise
	setDragEnabled(true);
	setAcceptDrops(false);
	setDropIndicatorShown(false);
	
	// taille des elements
	setIconSize(QSize(50, 50));
	
	// charge les collections
	reload();
	
	// la premiere fois, etend le premier niveau des collections
	QList<QTreeWidgetItem *> items = findItems("*", Qt::MatchWildcard);
	if (items.count() == 2) {
		items[0] -> setExpanded(true);
		items[1] -> setExpanded(true);
	}
	
	// force du noir sur une alternance de blanc (comme le schema) et de gris
	// clair, avec du blanc sur bleu pas trop fonce pour la selection
	QPalette qp = palette();
	qp.setColor(QPalette::Text,            Qt::black);
	qp.setColor(QPalette::Base,            Qt::white);
	qp.setColor(QPalette::AlternateBase,   QColor("#e8e8e8"));
	qp.setColor(QPalette::Highlight,       QColor("#678db2"));
	qp.setColor(QPalette::HighlightedText, Qt::white);
	setPalette(qp);
	
	// double-cliquer sur un element permet de l'editer
	connect(this, SIGNAL(itemDoubleClicked(QTreeWidgetItem *, int)), this, SLOT(slot_doubleClick(QTreeWidgetItem *, int)));
	
	// emet un signal au lieu de gerer son menu contextuel
	setContextMenuPolicy(Qt::CustomContextMenu);
}

/**
	Destructeur
*/
ElementsPanel::~ElementsPanel() {
}

/// @return true si un element est selectionne, false sinon
bool ElementsPanel::selectedItemIsAnElement() const {
	QFileInfo infos_file = selectedFile();
	if (!infos_file.exists()) return(false);
	return(infos_file.isFile());
}

/// @return true si une categorie est selectionnee, false sinon
bool ElementsPanel::selectedItemIsACategory() const {
	QFileInfo infos_file = selectedFile();
	if (!infos_file.exists()) return(false);
	return(infos_file.isDir());
}

/**
	Gere le mouvement lors d'un drag'n drop
*/
void ElementsPanel::dragMoveEvent(QDragMoveEvent */*e*/) {
}

/**
	Gere le depot lors d'un drag'n drop
*/
void ElementsPanel::dropEvent(QDropEvent */*e*/) {
}

/**
	Gere le debut des drag'n drop
	@param supportedActions Les actions supportees
 */
void ElementsPanel::startDrag(Qt::DropActions) {
	// recupere le nom du fichier decrivant l'element
	QString nom_fichier = currentItem() -> data(0, 42).toString();
	if (nom_fichier.isEmpty()) return;
	
	// objet QDrag pour realiser le drag'n drop
	QDrag *drag = new QDrag(this);
	
	// donnees qui seront transmises par le drag'n drop
	QMimeData *mimeData = new QMimeData();
	
	// appareil temporaire pour fournir un apercu
	int etat;
	Element *appar = new CustomElement(nom_fichier, 0, 0, &etat);
	if (etat != 0) {
		delete appar;
		return;
	}
	
	mimeData -> setText(nom_fichier);
	drag -> setMimeData(mimeData);
	
	// accrochage d'une pixmap representant l'appareil au pointeur
	drag -> setPixmap(appar -> pixmap());
	drag -> setHotSpot(appar -> hotspot());
	
	// realisation du drag'n drop
	drag -> start(Qt::CopyAction);
	
	// suppression de l'appareil temporaire
	delete appar;
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
void ElementsPanel::addDir(QTreeWidgetItem *qtwi_parent, QString adr_dossier, QString nom) {
	ElementsCategory category(adr_dossier);
	if (!category.exists()) return;
	
	// recupere le nom de la categorie
	QString nom_categorie = (nom != QString()) ? nom : category.name();
	
	// creation du QTreeWidgetItem representant le dossier
	QTreeWidgetItem *qtwi_dossier = new QTreeWidgetItem(qtwi_parent, QStringList(nom_categorie));
	qtwi_dossier -> setIcon(0, QIcon(":/ico/folder.png"));
	QLinearGradient t(0, 0, 200, 0);
	t.setColorAt(0, QColor("#e8e8e8"));
	t.setColorAt(1, QColor("#ffffff"));
	qtwi_dossier -> setBackground(0, QBrush(t));
	qtwi_dossier -> setData(0, 42, adr_dossier);
	
	// reduit le dossier si besoin
	qtwi_dossier -> setExpanded(expanded_directories.contains(adr_dossier));
	
	// ajout des sous-categories / sous-dossiers
	QStringList dossiers = category.entryList(QStringList(), QDir::AllDirs | QDir::NoSymLinks | QDir::NoDotAndDotDot, QDir::Name);
	foreach(QString dossier, dossiers) addDir(qtwi_dossier, adr_dossier + dossier + "/");
	
	// ajout des elements / fichiers
	QStringList fichiers = category.entryList(QStringList("*.elmt"), QDir::Files, QDir::Name);
	foreach(QString fichier, fichiers) addFile(qtwi_dossier, adr_dossier + fichier);
}

/**
	Methode privee permettant d'ajouter un element au panel d'appareils
	@param qtwi_parent QTreeWidgetItem parent sous lequel sera insere l'element
	@param fichier Chemin absolu du fichier XML decrivant l'element a inserer
*/
void ElementsPanel::addFile(QTreeWidgetItem *qtwi_parent, QString fichier) {
	QString whats_this = tr("Ceci est un \351l\351ment que vous pouvez ins\351rer dans votre sch\351ma par cliquer-d\351placer");
	QString tool_tip = tr("Cliquer-d\351posez cet \351l\351ment sur le sch\351ma pour ins\351rer un \351l\351ment ");
	int etat;
	CustomElement elmt_perso(fichier, 0, 0, &etat);
	if (etat != 0) {
		qDebug() << "Le chargement du composant" << fichier << "a echoue avec le code d'erreur" << etat;
		return;
	}
	QTreeWidgetItem *qtwi = new QTreeWidgetItem(qtwi_parent, QStringList(elmt_perso.nom()));
	qtwi -> setStatusTip(0, tool_tip + "\253 " + elmt_perso.nom() + " \273");
	qtwi -> setToolTip(0, elmt_perso.nom());
	qtwi -> setWhatsThis(0, whats_this);
	qtwi -> setFlags(Qt::ItemIsSelectable | Qt::ItemIsDragEnabled | Qt::ItemIsEnabled);
	qtwi -> setIcon(0, QIcon(elmt_perso.pixmap()));
	qtwi -> setData(0, 42, fichier);
}

/**
	Recharge l'arbre des elements
*/
void ElementsPanel::reload() {
	// sauvegarde la liste des repertoires reduits
	saveExpandedCategories();
	
	// vide l'arbre
	clear();
	
	// chargement des elements de la collection QET
	addDir(invisibleRootItem(), QETApp::commonElementsDir(), tr("Collection QET"));
	
	// chargement des elements de la collection utilisateur
	addDir(invisibleRootItem(), QETApp::customElementsDir(), tr("Collection utilisateur"));
	
	// icones
	QList<QTreeWidgetItem *> items = findItems("*", Qt::MatchWildcard);
	if (items.count() == 2) {
		items[0] -> setIcon(0, QIcon(":/ico/qet-16.png"));
		items[1] -> setIcon(0, QIcon(":/ico/folder_home.png"));
	}
	
	// reselectionne le dernier element selectionne
	if (!last_selected_item.isNull()) {
		QTreeWidgetItem *qtwi = findFile(last_selected_item);
		if (qtwi) setCurrentItem(qtwi);
	}
}

/**
	Edite la categorie selectionnee
*/
void ElementsPanel::editCategory() {
	QFileInfo infos_file = selectedFile();
	if (!infos_file.exists() || !infos_file.isDir()) return;
	launchCategoryEditor(infos_file.absoluteFilePath());
}

/**
	Edite l'element selectionne
*/
void ElementsPanel::editElement() {
	QFileInfo infos_file = selectedFile();
	if (!infos_file.exists() || !infos_file.isFile()) return;
	launchElementEditor(infos_file.absoluteFilePath());
}

/**
	Supprime la categorie selectionnee
*/
void ElementsPanel::deleteCategory() {
	QFileInfo infos_file = selectedFile();
	if (!infos_file.exists() || !infos_file.isDir()) return;
	
	// supprime la categorie
	ElementsCategoryDeleter cat_deleter(infos_file.absoluteFilePath(), this);
	cat_deleter.exec();
	
	// recharge la liste des categories
	reload();
}

/**
	Supprime l'element selectionne
*/
void ElementsPanel::deleteElement() {
	QFileInfo infos_file = selectedFile();
	if (!infos_file.exists() || !infos_file.isFile()) return;
	
	// supprime l'element
	ElementDeleter elmt_deleter(infos_file.absoluteFilePath(), this);
	elmt_deleter.exec();
	
	// recharge la liste des categories
	reload();
}

/**
	Gere le double-clic sur un element. Permet de lancer l'editeur de
	categorie ou d'element.
*/
void ElementsPanel::slot_doubleClick(QTreeWidgetItem *, int) {
	// le fichier doit exister
	QFileInfo infos_file = selectedFile();
	if (!infos_file.exists()) return;
	
	if (infos_file.isFile()) {
		// il s'agit d'un element
		launchElementEditor(infos_file.absoluteFilePath());
	} else if (infos_file.isDir()) {
		// il s'agit d'une categorie
		launchCategoryEditor(infos_file.absoluteFilePath());
	}
}

/// @return un QFileInfo decrivant le fichier ou le dossier correspondant au QTreeWidgetItem selectionne
QFileInfo ElementsPanel::selectedFile() const {
	QTreeWidgetItem *current_qtwi = currentItem();
	if(!current_qtwi) return(QFileInfo());
	return(QFileInfo(currentItem() -> data(0, 42).toString()));
}

/**
	Lance l'editeur d'element pour l'element filename
	@param filename Chemin du fichier representant l'element
*/
void ElementsPanel::launchElementEditor(const QString &filename) {
	QETElementEditor *editor = new QETElementEditor();
	editor -> fromFile(filename);
	editor -> show();
}

/**
	Lance l'editeur de categorie pour la categorie filename
	@param filename Chemin du dossier representant la categorie
*/
void ElementsPanel::launchCategoryEditor(const QString &filename) {
	ElementsCategoryEditor ece(filename, true, this);
	if (ece.exec() == QDialog::Accepted) reload();
}

/**
	Enregistre la liste des categories repliees ainsi que le dernier element
	selectionne
*/
void ElementsPanel::saveExpandedCategories() {
	expanded_directories.clear();
	QList<QTreeWidgetItem *> items = findItems("*", Qt::MatchRecursive|Qt::MatchWildcard);
	foreach(QTreeWidgetItem *item, items) {
		QString file = item -> data(0, 42).toString();
		if (!file.endsWith(".elmt") && item -> isExpanded()) {
			expanded_directories << file;
		}
	}
	
	// sauvegarde egalement le dernier element selectionne
	QTreeWidgetItem *current_item = currentItem();
	if (current_item) last_selected_item = current_item -> data(0, 42).toString();
}

/**
	@param file fichier ou dossier a retrouver dans l'arborescence
	@return le QTreeWidgetItem correspondant au fichier file ou 0 si celui-ci n'est pas trouve
*/
QTreeWidgetItem *ElementsPanel::findFile(const QString &file) const {
	QList<QTreeWidgetItem *> items = findItems("*", Qt::MatchRecursive|Qt::MatchWildcard);
	foreach(QTreeWidgetItem *item, items) {
		if (item -> data(0, 42).toString() == file) return(item);
	}
	return(0);
}

/**
	N'affiche que les elements contenant une chaine donnee
	@param m Chaine a filtrer
*/
void ElementsPanel::filter(const QString &m) {
	QList<QTreeWidgetItem *> items = findItems("*", Qt::MatchRecursive | Qt::MatchWildcard);
	if (m.isEmpty()) {
		foreach(QTreeWidgetItem *item, items) item -> setHidden(false);
	} else {
		foreach(QTreeWidgetItem *item, items) {
			QString file = item -> data(0, 42).toString();
			bool item_matches = item -> text(0).contains(m, Qt::CaseInsensitive);
			item -> setHidden(!item_matches);
			if (item_matches) {
				// remonte l'arborescence pour afficher les categories contenant l'element
				QTreeWidgetItem *parent_qtwi = item -> parent();
				while(parent_qtwi && (parent_qtwi -> isHidden() || !parent_qtwi -> isExpanded())) {
					parent_qtwi -> setHidden(false);
					parent_qtwi -> setExpanded(true);
					parent_qtwi = parent_qtwi -> parent();
				}
			}
		}
	}
}
