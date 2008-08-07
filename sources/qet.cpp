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
#include "qet.h"

/**
	Permet de convertir une chaine de caracteres ("n", "s", "e" ou "w")
	en orientation. Si la chaine fait plusieurs caracteres, seul le
	premier est pris en compte. En cas d'incoherence, QET::North est
	retourne.
	@param s Chaine de caractere cense representer une orientation
	@return l'orientation designee par la chaine de caractere
*/
QET::Orientation QET::orientationFromString(const QString &s) {
	QChar c = s[0];
	if (c == 'e') return(QET::East);
	else if (c == 's') return(QET::South);
	else if (c == 'w') return (QET::West);
	else return(QET::North);
}

/**
	@param o une orientation
	@return une chaine de caractere representant l'orientation
*/
QString QET::orientationToString(QET::Orientation o) {
	QString ret;
	switch(o) {
		case QET::North: ret = "n"; break;
		case QET::East : ret = "e"; break;
		case QET::South: ret = "s"; break;
		case QET::West : ret = "w"; break;
	}
	return(ret);
}

/**
	Indique si deux orientations de Borne sont sur le meme axe (Vertical / Horizontal).
	@param a La premiere orientation de Borne
	@param b La seconde orientation de Borne
	@return Un booleen a true si les deux orientations de bornes sont sur le meme axe
*/
bool QET::surLeMemeAxe(QET::Orientation a, QET::Orientation b) {
	if ((a == QET::North || a == QET::South) && (b == QET::North || b == QET::South)) return(true);
	else if ((a == QET::East || a == QET::West) && (b == QET::East || b == QET::West)) return(true);
	else return(false);
}

/**
	Indique si une orientation de borne est horizontale (Est / Ouest).
	@param a L'orientation de borne
	@return True si l'orientation de borne est horizontale, false sinon
*/
bool QET::estHorizontale(QET::Orientation a) {
	return(a == QET::East || a == QET::West);
}

/**
	Indique si une orientation de borne est verticale (Nord / Sud).
	@param a L'orientation de borne
	@return True si l'orientation de borne est verticale, false sinon
*/
bool QET::estVerticale(QET::Orientation a) {
	return(a == QET::North || a == QET::South);
}

/**
	Permet de connaitre l'orientation suivante apres celle donnee en parametre.
	Les orientations sont generalement presentees dans l'ordre suivant : Nord,
	Est, Sud, Ouest.
	@param o une orientation
	@return l'orientation suivante
*/
QET::Orientation QET::nextOrientation(QET::Orientation o) {
	if (o < 0 || o > 2) return(QET::North);
	return((QET::Orientation)(o + 1));
}

/**
	Permet de connaitre l'orientation precedant celle donnee en parametre.
	Les orientations sont generalement presentees dans l'ordre suivant : Nord,
	Est, Sud, Ouest.
	@param o une orientation
	@return l'orientation precedente
*/
QET::Orientation QET::previousOrientation(QET::Orientation o) {
	if (o < 0 || o > 3) return(QET::North);
	if (o == QET::North) return(QET::West);
	return((QET::Orientation)(o - 1));
}

/**
	Permet de savoir si l'attribut nom_attribut d'un element XML e est bien un
	entier. Si oui, sa valeur est copiee dans entier.
	@param e Element XML
	@param nom_attribut Nom de l'attribut a analyser
	@param entier Pointeur facultatif vers un entier
	@return true si l'attribut est bien un entier, false sinon
*/
bool QET::attributeIsAnInteger(const QDomElement &e, QString nom_attribut, int *entier) {
	// verifie la presence de l'attribut
	if (!e.hasAttribute(nom_attribut)) return(false);
	// verifie la validite de l'attribut
	bool ok;
	int tmp = e.attribute(nom_attribut).toInt(&ok);
	if (!ok) return(false);
	if (entier != NULL) *entier = tmp;
	return(true);
}

/**
	Permet de savoir si l'attribut nom_attribut d'un element XML e est bien un
	reel. Si oui, sa valeur est copiee dans reel.
	@param e Element XML
	@param nom_attribut Nom de l'attribut a analyser
	@param reel Pointeur facultatif vers un double
	@return true si l'attribut est bien un reel, false sinon
*/
bool QET::attributeIsAReal(const QDomElement &e, QString nom_attribut, double *reel) {
	// verifie la presence de l'attribut
	if (!e.hasAttribute(nom_attribut)) return(false);
	// verifie la validite de l'attribut
	bool ok;
	qreal tmp = e.attribute(nom_attribut).toDouble(&ok);
	if (!ok) return(false);
	if (reel != NULL) *reel = tmp;
	return(true);
}

/**
	Permet de composer rapidement la proposition "x elements et y conducteurs"
	@param elements_count nombre d'elements
	@param conductors_count nombre de conducteurs
	@param texts_count nombre de champs de texte
	@return la proposition decrivant le nombre d'elements et de conducteurs
*/
QString QET::ElementsAndConductorsSentence(int elements_count, int conductors_count, int texts_count) {
	QString text;
	if (elements_count) {
		text += QString::number(elements_count) + " ";
		text += elements_count > 1 ? QObject::tr("\351l\351ments") : QObject::tr("\351l\351ment");
		if (conductors_count && texts_count) text += QObject::tr(", ");
		else if (conductors_count || texts_count) text += QObject::tr(" et ");
	}
	if (conductors_count) {
		text += QString::number(conductors_count) + " ";
		text += conductors_count > 1 ? QObject::tr("conducteurs") : QObject::tr("conducteur");
		if (texts_count) text += QObject::tr(" et ");
	}
	if (texts_count) {
		text += QString::number(texts_count) + " ";
		text += texts_count > 1 ? QObject::tr("champs de texte") : QObject::tr("champ de texte");
	}
	return(text);
}

/**
	Etant donne un element XML e, renvoie la liste de tous les elements
	children imbriques dans les elements parent, eux-memes enfants de l'elememt e
	@param e Element XML a explorer
	@param parent tag XML intermediaire
	@param children tag XML a rechercher
	@return La liste des elements XML children
*/
QList<QDomElement> QET::findInDomElement(const QDomElement &e, const QString &parent, const QString &children) {
	QList<QDomElement> return_list;
	
	// parcours des elements parents
	for (QDomNode enfant = e.firstChild() ; !enfant.isNull() ; enfant = enfant.nextSibling()) {
		// on s'interesse a l'element XML "parent"
		QDomElement parents = enfant.toElement();
		if (parents.isNull() || parents.tagName() != parent) continue;
		// parcours des enfants de l'element XML "parent"
		for (QDomNode node_children = parents.firstChild() ; !node_children.isNull() ; node_children = node_children.nextSibling()) {
			// on s'interesse a l'element XML "children"
			QDomElement n_children = node_children.toElement();
			if (!n_children.isNull() && n_children.tagName() == children) return_list.append(n_children);
		}
	}
	return(return_list);
}

/// @return le texte de la licence de QElectroTech (GNU/GPL)
QString QET::license() {
	// Recuperation du texte de la GNU/GPL dans un fichier integre a l'application
	QFile *file_license = new QFile(":/LICENSE");
	QString txt_license;
	// verifie que le fichier existe
	if (!file_license -> exists()) {
		txt_license = QString(QObject::tr("Le fichier texte contenant la licence GNU/GPL est introuvable - bon bah de toute fa\347on, vous la connaissez par coeur non ?"));
	} else {
		// ouvre le fichier en mode texte et en lecture seule
		if (!file_license -> open(QIODevice::ReadOnly | QIODevice::Text)) {
			txt_license = QString(QObject::tr("Le fichier texte contenant la licence GNU/GPL existe mais n'a pas pu \352tre ouvert - bon bah de toute fa\347on, vous la connaissez par coeur non ?"));
		} else {
			// charge le contenu du fichier dans une QString
			QTextStream in(file_license);
			txt_license = QString("");
			while (!in.atEnd()) txt_license += in.readLine()+"\n";
			// ferme le fichier
			file_license -> close();
		}
	}
	return(txt_license);
};


/**
	@return la liste des caracteres interdits dans les noms de fichiers sous
	Windows
*/
QList<QChar> QET::forbiddenCharacters() {
	return(QList<QChar>() << '\\' << '/' << ':' << '*' << '?' << '"' << '<' << '>' << '|');
}

/**
	@param string une chaine de caracteres
	@return true si string contient un caractere interdit dans les noms de
	fichiers sous Windows
*/
bool QET::containsForbiddenCharacters(const QString &string) {
	foreach(QChar c, QET::forbiddenCharacters()) {
		if (string.contains(c)) return(true);
	}
	return(false);
}

/**
	@param string une chaine de caracteres
	@return la meme chaine de caracteres, mais avec les espaces et backslashes
	echappes
*/
QString QET::escapeSpaces(const QString &string) {
	return(QString(string).replace('\\', "\\\\").replace(' ', "\\ "));
}

/**
	@param string une chaine de caracteres
	@return la meme chaine de caracteres, mais avec les espaces et backslashes
	non echappes
*/
QString QET::unescapeSpaces(const QString &string) {
	return(QString(string).replace("\\\\", "\\").replace("\\ ", " "));
}

/**
	Assemble une liste de chaines en une seule. Un espace separe chaque chaine.
	Les espaces et backslashes des chaines sont echappes.
	@param string_list une liste de chaine
	@return l'assemblage des chaines
*/
QString QET::joinWithSpaces(const QStringList &string_list) {
	QString returned_string;
	
	for (int i = 0 ; i < string_list.count() ; ++ i) {
		returned_string += QET::escapeSpaces(string_list.at(i));
		if (i != string_list.count() - 1) returned_string += " ";
	}
	
	return(returned_string);
}

/**
	@param string Une chaine de caracteres contenant des sous-chaines a
	extraire separees par des espaces non echappes. Les espaces des sous-chaines
	sont echappes.
	@return La liste des sous-chaines, sans echappement.
*/
QStringList QET::splitWithSpaces(const QString &string) {
	// les chaines sont separees par des espaces non echappes = avec un nombre nul ou pair de backslashes devant
	QStringList escaped_strings = string.split(QRegExp("[^\\]?(?:\\\\)* "), QString::SkipEmptyParts);
	
	QStringList returned_list;
	foreach(QString escaped_string, escaped_strings) {
		returned_list << QET::unescapeSpaces(escaped_string);
	}
	return(returned_list);
}
