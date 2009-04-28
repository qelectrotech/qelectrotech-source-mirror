/*
	Copyright 2006-2009 Xavier Guerrin
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
#include "elementscategorieslist.h"
#include "qetapp.h"
#include "customelement.h"
#include "elementscollection.h"
#include "elementscategory.h"
#include "elementdefinition.h"

/**
	Constructeur
	@param display_elements true pour afficher les elements, false sinon
	@param selectables Types selectionnables
	@see QET::ItemType
	@param parent QWidget parent de ce widget
*/
ElementsCategoriesList::ElementsCategoriesList(bool display_elements, uint selectables, QWidget *parent) :
	QTreeWidget(parent),
	display_elements_(display_elements),
	selectables_(selectables),
	first_load(true)
{
	// selection unique
	setSelectionMode(QAbstractItemView::SingleSelection);
	setColumnCount(1);
	header() -> hide();
	
	// charge les categories
	reload();
	
	connect(this, SIGNAL(currentItemChanged(QTreeWidgetItem *, QTreeWidgetItem *)), this, SLOT(selectionChanged(QTreeWidgetItem *, QTreeWidgetItem *)));
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
	clear();
	
	foreach(ElementsCollection *collection, QETApp::availableCollections()) {
		if (collection == QETApp::commonElementsCollection()) continue;
		if (collection == QETApp::customElementsCollection()) continue;
		addCollection(invisibleRootItem(), collection, tr("Collection projet"));
	}
	
	// chargement des elements de la collection commune si droits d'ecriture
	if (QETApp::commonElementsCollection() -> isWritable()) {
		if (!first_load) QETApp::commonElementsCollection() -> reload();
		addCollection(invisibleRootItem(), QETApp::commonElementsCollection(), tr("Collection QET"), QIcon(":/ico/qet-16.png"));
	}
	
	// chargement des elements de la collection utilisateur
	if (!first_load) QETApp::customElementsCollection() -> reload();
	addCollection(invisibleRootItem(), QETApp::customElementsCollection(), tr("Collection utilisateur"), QIcon(":/ico/go-home.png"));
	
	if (first_load) first_load = false;
}

/**
	Methode privee permettant d'ajouter une collection d'elements
	@param qtwi_parent QTreeWidgetItem parent sous lequel sera insere la collection d'elements
	@param collection Collection a inserer dans le panel d'elements
	@param coll_name Nom a utiliser pour la collection
	@param icon Icone a utiliser pour l'affichage de la collection
	@return Le QTreeWidgetItem insere le plus haut
*/
QTreeWidgetItem *ElementsCategoriesList::addCollection(QTreeWidgetItem *qtwi_parent, ElementsCollection *collection, const QString &coll_name, const QIcon &icon) {
	QTreeWidgetItem *qtwi_coll = addCategory(qtwi_parent, collection -> rootCategory(), coll_name, icon);
	qtwi_coll -> setExpanded(true);
	Qt::ItemFlags flags_coll = Qt::ItemIsEnabled;
	if (selectables_ & QET::Collection) flags_coll |= Qt::ItemIsSelectable;
	qtwi_coll -> setFlags(flags_coll);
	return(qtwi_coll);
}

/**
	Methode privee permettant d'ajouter une categorie
	@param qtwi_parent QTreeWidgetItem parent sous lequel sera insere la categorie
	@param category Categorie d'elements a inserer
	@param name Parametre facultatif permettant de forcer le nom affiche
	S'il n'est pas precise, la methode utilise le nom declare par la categorie.
	@param icon Icone a utiliser pour l'affichage de la categorie
	Si elle n'est pas precisee, une icone par defaut est utilisee
	@return Le QTreeWidgetItem insere le plus haut
*/
QTreeWidgetItem *ElementsCategoriesList::addCategory(QTreeWidgetItem *qtwi_parent, ElementsCategory *category, const QString &cat_name, const QIcon &icon) {
	// recupere le nom de la categorie
	QString final_name(cat_name.isEmpty() ? category -> name() : cat_name);
	QIcon final_icon(icon.isNull() ? QIcon(":/ico/folder.png") : icon);
	
	// creation du QTreeWidgetItem representant le dossier
	QTreeWidgetItem *qtwi_category = new QTreeWidgetItem(qtwi_parent, QStringList(final_name));
	qtwi_category -> setIcon(0, final_icon);
	locations_.insert(qtwi_category, category -> location());
	Qt::ItemFlags flags_category = Qt::ItemIsEnabled;
	if (selectables_ & QET::Category) flags_category |= Qt::ItemIsSelectable;
	qtwi_category -> setFlags(flags_category);
	
	// ajout des sous-categories
	foreach(ElementsCategory *sub_cat, category -> categories()) addCategory(qtwi_category, sub_cat);
	
	if (display_elements_) {
		foreach(ElementDefinition *elmt, category -> elements()) addElement(qtwi_category, elmt);
	}
	
	return(qtwi_category);
}

/**
	Methode privee permettant d'ajouter un element
	@param qtwi_parent QTreeWidgetItem parent sous lequel sera insere l'element
	@param element Element a inserer
	@param name Parametre facultatif permettant de forcer le nom affiche
	S'il n'est pas precise, la methode utilise le nom declare par la categorie.
	Une icone sera generee a partir de l'element.
	@return Le QTreeWidgetItem insere
*/
QTreeWidgetItem *ElementsCategoriesList::addElement(QTreeWidgetItem *qtwi_parent, ElementDefinition *element, const QString &elmt_name, const QIcon &icon) {
	int state;
	CustomElement custom_elmt(element -> xml(), 0, 0, &state);
	if (state) {
		qDebug() << "ElementsCategoriesList::addElement() : Le chargement du composant" << qPrintable(element -> location().toString()) << "a echoue avec le code d'erreur" << state;
		return(0);
	}
	QString final_name(elmt_name.isEmpty() ? custom_elmt.name() : elmt_name);
	QTreeWidgetItem *qtwi = new QTreeWidgetItem(qtwi_parent, QStringList(final_name));
	qtwi -> setToolTip(0, custom_elmt.name());
	Qt::ItemFlags flags_element = Qt::ItemIsEnabled;
	if (selectables_ & QET::Element) flags_element |= Qt::ItemIsSelectable;
	qtwi -> setFlags(flags_element);
	qtwi -> setIcon(0, icon);
	locations_.insert(qtwi, element -> location());
	
	return(qtwi);
}

/**
	@return Le nom de la categorie selectionnee
*/
QString ElementsCategoriesList::selectedCategoryName() const {
	QTreeWidgetItem *qtwi = currentItem();
	if (qtwi) return(qtwi -> data(0, Qt::DisplayRole).toString());
	else return(QString());
}

/**
	@return l'emplacement correspondant au QTreeWidgetItem selectionne
*/
ElementsLocation ElementsCategoriesList::selectedLocation() const {
	if (QTreeWidgetItem *current_qtwi = currentItem()) {
		return(locations_[current_qtwi]);
	} else {
		return(ElementsLocation());
	}
}

/**
	Selectionne un element dans la liste a partir de son emplacement
	@see ElementsLocation
	@param location Emplacement a selectionner
	@return true si la selection a pu etre effectuee, false sinon
*/
bool ElementsCategoriesList::selectLocation(const ElementsLocation &location) {
	if (QTreeWidgetItem *qtwi = locations_.key(location)) {
		setCurrentItem(qtwi);
		return(true);
	}
	return(false);
}

/**
	Recupere le chemin virtuel de l'element selectionne et emet le signal
	virtualPathChanged.
	@param current  QTreeWidgetItem selectionne
	@param previous QTreeWidgetItem precedemment selectionne
*/
void ElementsCategoriesList::selectionChanged(QTreeWidgetItem *current, QTreeWidgetItem */*previous*/) {
	ElementsLocation emited_location;
	if (current) {
		emited_location = locations_[current];
	}
	emit(locationChanged(emited_location));
}
