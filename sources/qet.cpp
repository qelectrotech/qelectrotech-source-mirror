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
#include "qet.h"
#include <limits>
#include <QGraphicsSceneContextMenuEvent>

/**
	Permet de convertir une chaine de caracteres ("n", "s", "e" ou "w")
	en orientation. Si la chaine fait plusieurs caracteres, seul le
	premier est pris en compte. En cas d'incoherence, Qet::North est
	retourne.
	@param s Chaine de caractere cense representer une orientation
	@return l'orientation designee par la chaine de caractere
*/
Qet::Orientation Qet::orientationFromString(const QString &s) {
	QChar c = s[0];
	if (c == 'e') return(Qet::East);
	else if (c == 's') return(Qet::South);
	else if (c == 'w') return (Qet::West);
	else return(Qet::North);
}

/**
	@param o une orientation
	@return une chaine de caractere representant l'orientation
*/
QString Qet::orientationToString(Qet::Orientation o) {
	QString ret;
	switch(o) {
		case Qet::North: ret = "n"; break;
		case Qet::East : ret = "e"; break;
		case Qet::South: ret = "s"; break;
		case Qet::West : ret = "w"; break;
	}
	return(ret);
}

/**
	Indique si deux orientations de Borne sont sur le meme axe (Vertical / Horizontal).
	@param a La premiere orientation de Borne
	@param b La seconde orientation de Borne
	@return Un booleen a true si les deux orientations de bornes sont sur le meme axe
*/
bool Qet::surLeMemeAxe(Qet::Orientation a, Qet::Orientation b) {
	if ((a == Qet::North || a == Qet::South) && (b == Qet::North || b == Qet::South)) return(true);
	else if ((a == Qet::East || a == Qet::West) && (b == Qet::East || b == Qet::West)) return(true);
	else return(false);
}

/**
 * @brief Qet::isOpposed
 * @param a
 * @param b
 * @return true if a and b is opposed, else false;
 */
bool Qet::isOpposed(Qet::Orientation a, Qet::Orientation b)
{
	bool result = false;

	switch (a)
	{
		case Qet::North:
			if (b == Qet::South) result = true;
			break;
		case Qet::East:
			if (b == Qet::West) result = true;
			break;
		case Qet::South:
			if (b == Qet::North) result = true;
			break;
		case Qet::West:
			if (b == Qet::East) result = true;
			break;
		default:
			break;
	}

	return result;
}

/**
 * @brief Qet::isHorizontal
 * @param a
 * @return true if @a is horizontal, else false.
 */
bool Qet::isHorizontal(Qet::Orientation a) {
	return(a == Qet::East || a == Qet::West);
}

/**
 * @brief Qet::isVertical
 * @param a
 * @return true if @a is vertical, else false.
 */
bool Qet::isVertical(Qet::Orientation a) {
	return(a == Qet::North || a == Qet::South);
}

/**
	Permet de connaitre l'orientation suivante apres celle donnee en parametre.
	Les orientations sont generalement presentees dans l'ordre suivant : Nord,
	Est, Sud, Ouest.
	@param o une orientation
	@return l'orientation suivante
*/
Qet::Orientation Qet::nextOrientation(Qet::Orientation o) {
	if (o < 0 || o > 2) return(Qet::North);
	return((Qet::Orientation)(o + 1));
}

/**
	Permet de connaitre l'orientation precedant celle donnee en parametre.
	Les orientations sont generalement presentees dans l'ordre suivant : Nord,
	Est, Sud, Ouest.
	@param o une orientation
	@return l'orientation precedente
*/
Qet::Orientation Qet::previousOrientation(Qet::Orientation o) {
	if (o < 0 || o > 3) return(Qet::North);
	if (o == Qet::North) return(Qet::West);
	return((Qet::Orientation)(o - 1));
}

/**
	@param line Un segment de droite
	@param point Un point
	@return true si le point appartient au segment de droite, false sinon
*/
bool QET::lineContainsPoint(const QLineF &line, const QPointF &point) {
	if (point == line.p1()) return(true);
	QLineF point_line(line.p1(), point);
	if (point_line.unitVector() != line.unitVector()) return(false);
	return(point_line.length() <= line.length());
}

/**
	@param point Un point donne
	@param line Un segment de droite donnee
	@param intersection si ce pointeur est different de 0, le QPointF ainsi
	designe contiendra les coordonnees du projete orthogonal, meme si celui-ci
	n'appartient pas au segment de droite
	@return true si le projete orthogonal du point sur la droite appartient au
	segment de droite.
*/
bool QET::orthogonalProjection(const QPointF &point, const QLineF &line, QPointF *intersection) {
	// recupere le vecteur normal de `line'
	QLineF line_normal_vector(line.normalVector());
	QPointF normal_vector(line_normal_vector.dx(), line_normal_vector.dy());
	
	// cree une droite perpendiculaire a `line' passant par `point'
	QLineF perpendicular_line(point, point + normal_vector);
	
	// determine le point d'intersection des deux droites = le projete orthogonal
	QPointF intersection_point;
	QLineF::IntersectType it = line.intersect(perpendicular_line, &intersection_point);
	
	// ne devrait pas arriver (mais bon...)
	if (it == QLineF::NoIntersection) return(false);
	
	// fournit le point d'intersection a l'appelant si necessaire
	if (intersection) {
		*intersection = intersection_point;
	}
	
	// determine si le point d'intersection appartient au segment de droite
	if (QET::lineContainsPoint(line, intersection_point)) {
		return(true);
	}
	return(false);
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
bool QET::attributeIsAReal(const QDomElement &e, QString nom_attribut, qreal *reel) {
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
	ou encore "x elements, y conducteurs et z champs de texte".
	@param elements_count nombre d'elements
	@param conductors_count nombre de conducteurs
	@param texts_count nombre de champs de texte
	@param images_count nombre d'images
	@return la proposition decrivant le nombre d'elements, de conducteurs et de
	textes
*/
QString QET::ElementsAndConductorsSentence(int elements_count, int conductors_count, int texts_count, int images_count, int shapes_count) {
	QString text;
	if (elements_count) {
		text += QObject::tr(
			"%n \351l\351ment(s)",
			"part of a sentence listing the content of a diagram",
			elements_count
		);
		if ((conductors_count && texts_count) ||
			(conductors_count && images_count) ||
			(texts_count && images_count)) {
			text += QObject::tr(
				", ",
				"separator between elements and conductors in a sentence "
				"listing the content of a diagram"
			);
		} else if (conductors_count || texts_count || images_count) {
			text += QObject::tr(
				" et ",
				"separator between elements and conductors (or texts) in a "
				"sentence listing the content of a diagram"
			);
		}
	}
	
	if (conductors_count) {
		text += QObject::tr(
			"%n conducteur(s)",
			"part of a sentence listing the content of a diagram",
			conductors_count
		);
		if (texts_count && images_count) {
			text += QObject::tr(
				", ",
				"separator between elements and conductors in a sentence "
				"listing the content of a diagram"
			);
		}
		else if (texts_count || images_count) {
			text += QObject::tr(
				" et ",
				"separator between conductors and texts in a sentence listing "
				"the content of a diagram"
			);
		}
	}
	
	if (texts_count) {
		text += QObject::tr(
			"%n champ(s) de texte",
			"part of a sentence listing the content of a diagram",
			texts_count
		);
		if (images_count) {
			text += QObject::tr(
				" et ",
				"separator between conductors and texts in a sentence listing "
				"the content of a diagram"
			);
		}
	}

	if (images_count) {
		text += QObject::tr(
			"%n image(s)",
			"part of a sentence listing the content of a diagram",
			images_count
		);
	}

	if (shapes_count) {
		text += QObject::tr(
			"%n forme(s)",
			"part of a sentence listing the content of a diagram",
			shapes_count
		);
	}
	return(text);
}

/**
	@return the list of \a tag_name elements directly under the \a e XML element.
*/
QList<QDomElement> QET::findInDomElement(const QDomElement &e, const QString &tag_name) {
	QList<QDomElement> return_list;
	for (QDomNode node = e.firstChild() ; !node.isNull() ; node = node.nextSibling()) {
		if (!node.isElement()) continue;
		QDomElement element = node.toElement();
		if (element.isNull() || element.tagName() != tag_name) continue;
		return_list << element;
	}
	return(return_list);
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
	@return une chaine listant les caracteres interdits dans les noms de fichiers sous
	Windows
	@param escape true pour remplacer les caracteres < et > par leurs entites HTML
*/
QString QET::forbiddenCharactersString(bool escape) {
	QString result;
	foreach(QChar c, QET::forbiddenCharacters()) {
		if (escape) {
			if (c == '<')      result += "&lt;";
			else if (c == '>') result += "&gt;";
			else               result += QString(c);
		} else {
			result += QString(c);
		}
		result += " ";
	}
	return(result);
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
	Cette fonction transforme une chaine de caracteres (typiquement : un nom de
	schema, de projet, d'element) en un nom de fichier potable.
	Par nom de fichier potable, on entend un nom :
	  * ne comprenant pas de caracteres interdits sous Windows
	  * ne comprenant pas d'espace
	@param name Chaine de caractere a transformer en nom de fichier potable
	@todo virer les caracteres accentues ?
*/
QString QET::stringToFileName(const QString &name) {
	QString file_name(name.toLower());
	
	// remplace les caracteres interdits par des tirets
	foreach(QChar c, QET::forbiddenCharacters()) {
		file_name.replace(c, '-');
	}
	
	// remplace les espaces par des underscores
	file_name.replace(' ', '_');
	
	return(file_name);
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

/**
	@param end_type un type d'extremite
	@return une chaine representant le type d'extremite
*/
QString Qet::endTypeToString(const Qet::EndType &end_type) {
	switch(end_type) {
		case Qet::Simple:   return("simple");
		case Qet::Triangle: return("triangle");
		case Qet::Circle:   return("circle");
		case Qet::Diamond:  return("diamond");
		case Qet::None:
		default:
			return("none");
	}
}

/**
	@param string une chaine representant un type d'extremite
	@return le type d'extremite correspondant ; si la chaine est invalide,
	QET::None est retourne.
*/
Qet::EndType Qet::endTypeFromString(const QString &string) {
	if (string == "simple")        return(Qet::Simple);
	else if (string == "triangle") return(Qet::Triangle);
	else if (string == "circle")   return(Qet::Circle);
	else if (string == "diamond")  return(Qet::Diamond);
	else return(Qet::None);
}

/**
	@param diagram_area un type de zone de schema
	@return une chaine representant le type de zone de schema
*/
QString QET::diagramAreaToString(const QET::DiagramArea &diagram_area) {
	if (diagram_area == ElementsArea) return("elements");
	else return("border");
}

/**
	@param string une chaine representant un type de zone de schema
	@return le type de zone de schema correspondant ; si la chaine est invalide,
	QET::ElementsArea est retourne.
*/
QET::DiagramArea QET::diagramAreaFromString(const QString &string) {
	if (!string.compare("border", Qt::CaseInsensitive)) return(QET::BorderArea);
	else return(QET::ElementsArea);
}

/**
	@param ptr pointeur quelconque
	@return une representation hexadecimale de l'adresse du pointeur
*/
QString QET::pointerString(void *ptr) {
	static int hexa_digits = -1;
	
	if (hexa_digits == -1) {
		// determine le nombre de bits dans un unsigned long int
		hexa_digits = std::numeric_limits<unsigned long int>::digits / 4;
	}
	
	return(
		QString("0x%1").arg(
			reinterpret_cast<unsigned long int>(ptr),
			hexa_digits,
			16,
			QChar('0')
		)
	);
}

/**
	Round \a x to the nearest multiple of the invert of \a epsilon.
	For instance, epsilon = 10 will round to 1/10 = 0.1
*/
qreal QET::round(qreal x, qreal epsilon) {
	return(int(x * epsilon) / epsilon);
}

/**
	Round the coordinates of \a p to the nearest multiple of \a epsilon.
*/
QPointF QET::roundPoint(const QPointF &p, qreal epsilon) {
	return(QPointF(QET::round(p.x(), epsilon), QET::round(p.y(), epsilon)));
}

/**
	@param angle Un angle quelconque
	@return l'angle passe en parametre, mais ramene entre -360.0 + 360.0 degres
*/
qreal QET::correctAngle(const qreal &angle) {
	// ramene l'angle demande entre -360.0 et +360.0 degres
	qreal corrected_angle = angle;
	while (corrected_angle <= -360.0) corrected_angle += 360.0;
	while (corrected_angle >=  360.0) corrected_angle -= 360.0;
	return(corrected_angle);
}

/**
	@param first  Un premier chemin vers un fichier
	@param second Un second chemin vers un fichier
	@return true si les deux chemins existent existent et sont identiques
	lorsqu'ils sont exprimes sous forme canonique
*/
bool QET::compareCanonicalFilePaths(const QString &first, const QString &second) {
	QString first_canonical_path = QFileInfo(first).canonicalFilePath();
	if (first_canonical_path.isEmpty()) return(false);

	QString second_canonical_path = QFileInfo(second).canonicalFilePath();
	if (second_canonical_path.isEmpty()) return(false);
	
#ifdef Q_WS_WIN
	// sous Windows, on ramene les chemins en minuscules
	first_canonical_path  = first_canonical_path.toLower();
	second_canonical_path = second_canonical_path.toLower();
#endif
	
	return(first_canonical_path == second_canonical_path);
}

/**
	@param icl an TitleBlockColumnLength object
	@see TitleBlockColumnLength
	@return a string describing the type of this TitleBlockColumnLength object
*/
QString QET::titleBlockColumnLengthToString(const TitleBlockColumnLength  &icl) {
	QString type_str;
	if (icl== Absolute) type_str = "absolute";
	else if (icl == RelativeToTotalLength) type_str = "relative to total";
	else if (icl == RelativeToRemainingLength) type_str = "relative to remaining";
	return(type_str);
}

/**
	Export an XML document to an UTF-8 text file indented with 4 spaces, with LF
	end of lines and no BOM.
	@param xml_doc An XML document to be exported
	@param filepath Path to the file to be written
	@param error_message If non-zero, will contain an error message explaining
	what happened when this function returns false.
	@return false if an error occured, true otherwise
*/
bool QET::writeXmlFile(QDomDocument &xml_doc, const QString &filepath, QString *error_message) {
	QFile file(filepath);
	
	// Note: we do not set QIODevice::Text to avoid generating CRLF end of lines
	bool file_opening = file.open(QIODevice::WriteOnly);
	if (!file_opening) {
		if (error_message) {
			*error_message = QString(
				QObject::tr(
					"Impossible d'ouvrir le fichier %1 en \351criture, erreur %2 rencontr\351e.",
					"error message when attempting to write an XML file"
				)
			).arg(filepath).arg(file.error());
		}
		return(false);
	}
	
	QTextStream out(&file);
	out.setCodec("UTF-8");
	out.setGenerateByteOrderMark(false);
	out << xml_doc.toString(4);
	file.close();
	
	return(true);
}

/**
	@return the scene position where \a event occurred, provided it is
	QGraphicsScene-related event; otherwise, this function returns a null
	QPointF.
*/
QPointF QET::graphicsSceneEventPos(QEvent *event) {
	QPointF event_scene_pos;
	if (event -> type() < QEvent::GraphicsSceneContextMenu) return(event_scene_pos);
	if (event -> type() > QEvent::GraphicsSceneWheel) return(event_scene_pos);
	
	switch (event -> type()) {
		case QEvent::GraphicsSceneContextMenu: {
			QGraphicsSceneContextMenuEvent *qgs_event = static_cast<QGraphicsSceneContextMenuEvent *>(event);
			event_scene_pos = qgs_event -> scenePos();
			break;
		}
		case QEvent::GraphicsSceneDragEnter:
		case QEvent::GraphicsSceneDragLeave:
		case QEvent::GraphicsSceneDragMove:
		case QEvent::GraphicsSceneDrop: {
			QGraphicsSceneDragDropEvent *qgs_event = static_cast<QGraphicsSceneDragDropEvent *>(event);
			event_scene_pos = qgs_event -> scenePos();
			break;
		}
		case QEvent::GraphicsSceneHelp: {
			QGraphicsSceneHelpEvent *qgs_event = static_cast<QGraphicsSceneHelpEvent *>(event);
			event_scene_pos = qgs_event -> scenePos();
			break;
		}
		
		case QEvent::GraphicsSceneHoverEnter:
		case QEvent::GraphicsSceneHoverLeave:
		case QEvent::GraphicsSceneHoverMove: {
			QGraphicsSceneHoverEvent *qgs_event = static_cast<QGraphicsSceneHoverEvent *>(event);
			event_scene_pos = qgs_event -> scenePos();
			break;
		}
		case QEvent::GraphicsSceneMouseDoubleClick:
		case QEvent::GraphicsSceneMouseMove:
		case QEvent::GraphicsSceneMousePress:
		case QEvent::GraphicsSceneMouseRelease: {
			QGraphicsSceneMouseEvent *qgs_event = static_cast<QGraphicsSceneMouseEvent *>(event);
			event_scene_pos = qgs_event -> scenePos();
			break;
		}
		case QEvent::GraphicsSceneWheel: {
			QGraphicsSceneWheelEvent *qgs_event = static_cast<QGraphicsSceneWheelEvent *>(event);
			event_scene_pos = qgs_event -> scenePos();
			break;
		}
		default:
			break;
	}
	return(event_scene_pos);
}

/**
 * @brief QET::eachStrIsEqual
 * @param qsl list of string to compare
 * @return true if every string is identical, else false;
 * The list must not be empty
 * If the list can be empty, call isEmpty() before calling this function
 */
bool QET::eachStrIsEqual(const QStringList &qsl) {
	if (qsl.size() == 1) return true;
	foreach (const QString t, qsl) {
		if (qsl.at(0) != t) return false;
	}
	return true;
}
