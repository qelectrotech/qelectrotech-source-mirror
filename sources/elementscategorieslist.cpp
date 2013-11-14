/*
	Copyright 2006-2013 The QElectroTech Team
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
#include "qetgraphicsitem/customelement.h"
#include "elementscollection.h"
#include "elementscategory.h"
#include "elementdefinition.h"
#include "qeticons.h"

/**
	Constructeur
	@param display_elements true pour afficher les elements, false sinon
	@param selectables Types selectionnables
	@see QET::ItemType
	@param parent QWidget parent de ce widget
*/
ElementsCategoriesList::ElementsCategoriesList(bool display_elements, uint selectables, QWidget *parent) :
	GenericPanel(parent),
	display_elements_(display_elements),
	selectables_(selectables),
	first_load(true)
{
	// selection unique
	setSelectionMode(QAbstractItemView::SingleSelection);
	setColumnCount(1);
	
	// charge les categories
	setElementsCache(QETApp::collectionCache());
	reload();
	
	connect(
		this, SIGNAL(currentItemChanged(QTreeWidgetItem *, QTreeWidgetItem *)),
		this, SLOT(selectionChanged(QTreeWidgetItem *, QTreeWidgetItem *))
	);
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
	GenericPanel::PanelOptions  options = display_elements_ ? GenericPanel::AddAllChildElements : GenericPanel::AddChildElementsContainers;
	options |= GenericPanel::DisplayElementsPreview;
	
	foreach(ElementsCollection *collection, QETApp::availableCollections()) {
		if (collection == QETApp::commonElementsCollection()) continue;
		if (collection == QETApp::customElementsCollection()) continue;
		addElementsCollection(collection, invisibleRootItem(), options) -> setExpanded(true);
	}
	
	// chargement des elements de la collection commune si droits d'ecriture
	if (QETApp::commonElementsCollection() -> isWritable()) {
		addElementsCollection(
			QETApp::commonElementsCollection(),
			invisibleRootItem(),
			options
		) -> setExpanded(true);
	}
	
	// chargement des elements de la collection utilisateur
	addElementsCollection(
		QETApp::customElementsCollection(),
		invisibleRootItem(),
		options
	) -> setExpanded(true);
	
	if (first_load) first_load = false;
}

/**
	Create a QTreeWidgetItem
	@param type Item type (e.g QET::Diagram, QET::Project, ...)
	@param parent Parent for the created item
	@param label Label for the created item
	@param icon Icon for the created item
	@return the create QTreeWidgetItem
*/
QTreeWidgetItem *ElementsCategoriesList::makeItem(QET::ItemType type, QTreeWidgetItem *parent, const QString &label, const QIcon &icon) {
	QTreeWidgetItem *item = GenericPanel::makeItem(type, parent, label, icon);
	Qt::ItemFlags flags = Qt::ItemIsEnabled;
	if (selectables_ & item -> type()) flags |= Qt::ItemIsSelectable;
	item -> setFlags(flags);
	return(item);
}

/**
	@return l'emplacement correspondant au QTreeWidgetItem selectionne
*/
ElementsLocation ElementsCategoriesList::selectedLocation() const {
	QTreeWidgetItem *current_qtwi = currentItem();
	if (!current_qtwi) return(ElementsLocation());
	return(valueForItem<ElementsLocation>(current_qtwi));
}

/**
	Selectionne un element dans la liste a partir de son emplacement
	@see ElementsLocation
	@param location Emplacement a selectionner
	@return true si la selection a pu etre effectuee, false sinon
*/
bool ElementsCategoriesList::selectLocation(const ElementsLocation &location) {
	QTreeWidgetItem *qtwi = itemForElementsLocation(location);
	if (qtwi) setCurrentItem(qtwi);
	return(qtwi);
}

/**
	Recupere le chemin virtuel de l'element selectionne et emet le signal
	virtualPathChanged.
	@param current  QTreeWidgetItem selectionne
	@param previous QTreeWidgetItem precedemment selectionne
*/
void ElementsCategoriesList::selectionChanged(QTreeWidgetItem *current, QTreeWidgetItem *previous) {
	Q_UNUSED(previous);
	ElementsLocation emited_location;
	if (current) {
		emited_location = valueForItem<ElementsLocation>(current);
	}
	emit(locationChanged(emited_location));
}
