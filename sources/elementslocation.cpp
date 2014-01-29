/*
	Copyright 2006-2014 The QElectroTech Team
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
#include "elementslocation.h"
#include "qetapp.h"

// make this class usable with QVariant
int ElementsLocation::MetaTypeId = qRegisterMetaType<ElementsLocation>("ElementsLocation");

/**
	Constructeur par defaut
*/
ElementsLocation::ElementsLocation() : project_(0) {
}

/**
	Constructeur
	@param p Chemin de l'emplacement de l'element
	@param pr Projet de l'emplacement de l'element
*/
ElementsLocation::ElementsLocation(const QString &p, QETProject *pr) :
	project_(pr)
{
	setPath(p);
}

/**
	Destructeur
*/
ElementsLocation::~ElementsLocation() {
}

/**
	Constructeur de copie
	@param other Autre emplacement d'element a copier
*/
ElementsLocation::ElementsLocation(const ElementsLocation &other) :
	path_(other.path_),
	project_(other.project_)
{
}

/**
	Operateur d'affectation
	@param other Autre emplacement d'element a affecter
*/
ElementsLocation &ElementsLocation::operator=(const ElementsLocation &other) {
	path_ = other.path_;
	project_ = other.project_;
	return(*this);
}

/**
	Operateur de comparaison
	@param other Autre emplacement d'element a comparer
	@return true si other et cet ElementsLocation sont identiques, false sinon
*/
bool ElementsLocation::operator==(const ElementsLocation &other) const {
	return(
		path_ == other.path_ &&\
		project_ == other.project_
	);
}

/**
	Operateur de comparaison
	@param other Autre emplacement d'element a comparer
	@return true si other et cet ElementsLocation sont differents, false sinon
*/
bool ElementsLocation::operator!=(const ElementsLocation &other) const {
	return(
		path_ != other.path_ ||\
		project_ != other.project_
	);
}

/**
	@return le nom de base de l'element
*/
QString ElementsLocation::baseName() const {
	QRegExp regexp("^.*([^/]+)\\.elmt$");
	if (regexp.exactMatch(path_)) {
		return(regexp.capturedTexts().at(1));
	}
	return(QString());
}

/**
	@return Le chemin virtuel de cet emplacement
*/
QString ElementsLocation::path() const {
	return(path_);
}

/**
	Change le chemin virtuel de cet emplacement
	@param p Nouveau chemin virtuel
*/
void ElementsLocation::setPath(const QString &p) {
#ifdef Q_OS_WIN32
	// sous Windows : on convertit les backslashs en slashs
	path_ = QDir::fromNativeSeparators(p);
#else
	// ailleurs : si on detecte des backslashs, on tente d'etre "compatible"
	path_ = p;
	path_.replace("\\", "/");
#endif
}

/**
	Ajoute une chaine au chemin
	@param string Chaine a ajouter
	@return true si l'operation a reussi, false si l'operation n'a pas de sens.
	Par exemple, il n'y a pas de sens a vouloir ajouter quelque chose apres le
	chemin d'un element.
*/
bool ElementsLocation::addToPath(const QString &string) {
	if (path_.endsWith(".elmt", Qt::CaseInsensitive)) return(false);
	if (!path_.endsWith("/") && !string.startsWith("/")) path_ += "/";
	path_ += string;
	return(true);
}

/**
	@return the location of the parent category, or a copy of this location
	when it represents a root category.
*/
ElementsLocation ElementsLocation::parent() const {
	ElementsLocation copy(*this);
	QRegExp re1("^([a-z]+://)(.*)/*$");
	if (re1.exactMatch(path_)) {
		QString path_proto = re1.capturedTexts().at(1);
		QString path_path = re1.capturedTexts().at(2);
		QString parent_path = path_path.remove(QRegExp("/*[^/]+$"));
		copy.setPath(path_proto + parent_path);
	}
	return(copy);
}

/**
	@return le projet de cet emplacement ou 0 si celui-ci n'est pas lie a
	un projet.
*/
QETProject *ElementsLocation::project() const {
	return(project_);
}

/**
	@param project le nouveau projet pointe par cet emplacement
	Indiquer 0 pour que cet emplacement ne soit plus lie a un projet.
*/
void ElementsLocation::setProject(QETProject *project) {
	project_ = project;
}

/**
	@return true si l'emplacement semble utilisable (chemin virtuel non vide).
*/
bool ElementsLocation::isNull() const {
	return(path_.isEmpty());
}

/**
	@return Une chaine de caracteres representant l'emplacement
*/
QString ElementsLocation::toString() const {
	QString result;
	if (project_) {
		int project_id = QETApp::projectId(project_);
		if (project_id != -1) {
			result += "project" + QString().setNum(project_id) + "+";
		}
	}
	result += path_;
	return(result);
}

/**
	Charge l'emplacemant a partir d'une chaine de caractere du type
	project42+embed://foo/bar/thing.elmt
	@param string Une chaine de caracteres representant l'emplacement
*/
void ElementsLocation::fromString(const QString &string) {
	QRegExp embedded("^project([0-9]+)\\+(embed:\\/\\/.*)$", Qt::CaseInsensitive);
	if (embedded.exactMatch(string)) {
		bool conv_ok = false;
		uint project_id = embedded.capturedTexts().at(1).toUInt(&conv_ok);
		if (conv_ok) {
			QETProject *the_project = QETApp::project(project_id);
			if (the_project) {
				path_ = embedded.capturedTexts().at(2);
				project_ = the_project;
				return;
			}
		}
	}
	
	// fallback : le chemin devient la chaine complete et aucun projet n'est utilise
	path_ = string;
	project_ = 0;
}

/**
	@param string Une chaine de caracteres representant l'emplacement
	@return un emplacemant a partir d'une chaine de caractere du type
	project42+embed://foo/bar/thing.elmt
*/
ElementsLocation ElementsLocation::locationFromString(const QString &string) {
	ElementsLocation location;
	location.fromString(string);
	return(location);
}

/**
	@param location A standard element location
	@return a hash identifying this location
*/
uint qHash(const ElementsLocation &location) {
	return(qHash(location.toString()));
}
