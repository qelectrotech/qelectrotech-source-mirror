/*
	Copyright 2006-2015 The QElectroTech Team
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
#include "moveelementsdescription.h"

/**
	Constructeur - construit une description ne comprenant aucun nom internem,
	aucune destination, aucun handler. Par defaut, la recursivite est activee.
	@param parent QObject parent
*/
MoveElementsDescription::MoveElementsDescription(QObject *parent) :
	QObject(parent),
	recursive_(true),
	handler_(0),
	destination_(0),
	dest_internal_name_orig_(""),
	dest_internal_name_final_(""),
	created_item_(0),
	src_deleted_(false),
	abort_(false)
{
}

/**
	Destructeur
*/
MoveElementsDescription::~MoveElementsDescription() {
}

/**
	@return true si le mouvement decrit est recursif (c'est-a-dire que la copie
	d'une categorie entrainera la copie de ses sous-categories et de ses
	elements)
*/
bool MoveElementsDescription::isRecursive() const {
	return(recursive_);
}

/**
	@param r true pour activer la recursivite, false sinon
	@see isRecursive()
*/
void MoveElementsDescription::setRecursive(bool r) {
	recursive_ = r;
}

/**
	@return le MoveElementsHandler utilise pour gerer les erreurs lors de la
	realisation de ce mouvement. Si aucun handler n'est specifie, cette methode
	retourne 0.
	@see MoveElementsHandler
*/
MoveElementsHandler *MoveElementsDescription::handler() const {
	return(handler_);
}

/**
	@param handler Le MoveElementHandler a utiliser pour gerer les erreurs lors
	de la realisation de ce mouvement. Indiquer 0 pour enlever le
	MoveElementsHandler.
*/
void MoveElementsDescription::setHandler(MoveElementsHandler *handler) {
	handler_ = handler;
}

/**
	@return la categorie de destination qui accueillera l'element cree par le
	mouvement.
*/
ElementsCategory *MoveElementsDescription::destinationParentCategory() const {
	return(destination_);
}

/**
	@param destination la categorie de destination qui accueillera l'element
	cree par le mouvement
*/
void MoveElementsDescription::setDestinationParentCategory(ElementsCategory *destination) {
	destination_ = destination;
}

/**
	@return Le nom interne souhaite pour l'item a creer.
	Typiquement, il s'agit du meme nom que l'item d'origine. Il faut toutefois
	le specifier explicitement.
*/
QString MoveElementsDescription::originalDestinationInternalName() const {
	return(dest_internal_name_orig_);
}

/**
	@param name Le nom interne souhaite pour l'item a creer.
	Typiquement, il s'agit du meme nom que l'item d'origine. Il faut toutefois
	le specifier explicitement.
*/
void MoveElementsDescription::setOriginalDestinationInternalName(const QString &name) {
	dest_internal_name_orig_ = name;
}

/**
	@return Le nom interne finalement retenu pour creer l'item.
	Si le nom interne est deja pris dans la categorie de destination, il est
	courant de changer le nom interne de destination (cette decision revient
	typiquement au MoveElementsHandler).
*/
QString MoveElementsDescription::finalDestinationInternalName() const {
	return(dest_internal_name_final_);
}

/**
	@param name Le nom interne finalement retenu pour creer l'item.
	Si le nom interne est deja pris dans la categorie de destination, il est
	courant de changer le nom interne de destination (cette decision revient
	typiquement au MoveElementsHandler).
*/
void MoveElementsDescription::setFinalDestinationInternalName(const QString &name) {
	dest_internal_name_final_ = name;
}

/**
	@return l'item cree par le mouvement, ou 0 si celui-ci n'as pas encore ete
	cree ou ne sera pas cree.
*/
ElementsCollectionItem *MoveElementsDescription::createdItem() const {
	return(created_item_);
}

/**
	@param item l'item cree par le mouvement. Indiquer 0 si celui-ci n'as pas
	encore ete cree ou ne sera pas cree.
*/
void MoveElementsDescription::setCreatedItem(ElementsCollectionItem *item) {
	created_item_ = item;
}

/**
	@return true si, dans le cadre normal du mouvement, l'item source a ete
	supprime (exemple : deplacement) avec succes.
*/
bool MoveElementsDescription::sourceItemWasDeleted() const {
	return(src_deleted_);
}

/**
	@param deleted Definit si oui ou non l'item source a ete supprime avec
	succes, et ce dans le cadre normal du mouvement (exemple : deplacement).
*/
void MoveElementsDescription::setSourceItemDeleted(bool deleted) {
	src_deleted_ = deleted;
}

/**
	@return true si le mouvement, ainsi que les mouvements qui suivent, doivent
	etre annules.
*/
bool MoveElementsDescription::mustAbort() const {
	return(abort_);
}

/**
	Definit ce mouvement ainsi que les mouvements qui suivent comme etant a
	annuler.
*/
void MoveElementsDescription::abort() {
	abort_ = true;
}
