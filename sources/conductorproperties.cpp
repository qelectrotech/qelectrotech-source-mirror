/*
	Copyright 2006-2012 Xavier Guerrin
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
#include "conductorproperties.h"

/**
	Constructeur par defaut
*/
SingleLineProperties::SingleLineProperties() :
	hasGround(true),
	hasNeutral(true),
	phases(1)
{
}

/// Destructeur
SingleLineProperties::~SingleLineProperties() {
}

/**
	Definit le nombre de phases (0, 1, 2, ou 3)
	@param n Nombre de phases
*/
void SingleLineProperties::setPhasesCount(int n) {
	phases = qBound(0, n, 3);
}

/// @return le nombre de phases (0, 1, 2, ou 3)
unsigned short int SingleLineProperties::phasesCount() {
	return(phases);
}

/**
	Dessine les symboles propres a un conducteur unifilaire
	@param painter QPainter a utiliser pour dessiner les symboles
	@param direction direction du segment sur lequel les symboles apparaitront
	@param rect rectangle englobant le dessin ; utilise pour specifier a la fois la position et la taille du dessin
*/
void SingleLineProperties::draw(QPainter *painter, QET::ConductorSegmentType direction, const QRectF &rect) {
	// s'il n'y a rien a dessiner, on retourne immediatement
	if (!hasNeutral && !hasGround && !phases) return;
	
	// prepare le QPainter
	painter -> save();
	QPen pen(painter -> pen());
	pen.setCapStyle(Qt::FlatCap);
	pen.setJoinStyle(Qt::MiterJoin);
	painter -> setPen(pen);
	painter -> setRenderHint(QPainter::Antialiasing, true);
	
	uint symbols_count = (hasNeutral ? 1 : 0) + (hasGround ? 1 : 0) + phases;
	qreal interleave;
	qreal symbol_width;
	if (direction == QET::Horizontal) {
		interleave = rect.width() / (symbols_count + 1);
		symbol_width = rect.width() / 12;
		for (uint i = 1 ; i <= symbols_count ; ++ i) {
			// dessine le tronc du symbole
			QPointF symbol_p1(rect.x() + (i * interleave) + symbol_width, rect.y() + rect.height() * 0.75);
			QPointF symbol_p2(rect.x() + (i * interleave) - symbol_width, rect.y() + rect.height() * 0.25);
			painter -> drawLine(QLineF(symbol_p1, symbol_p2));
			
			// dessine le reste des symboles terre et neutre
			if (hasGround && i == 1) {
				drawGround(painter, direction, symbol_p2, symbol_width * 2.0);
			} else if (hasNeutral && ((i == 1 && !hasGround) || (i == 2 && hasGround))) {
				drawNeutral(painter, direction, symbol_p2, symbol_width * 1.35);
			}
		}
	} else {
		interleave = rect.height() / (symbols_count + 1);
		symbol_width = rect.height() / 12;
		for (uint i = 1 ; i <= symbols_count ; ++ i) {
			// dessine le tronc du symbole
			QPointF symbol_p2(rect.x() + rect.width() * 0.75, rect.y() + (i * interleave) - symbol_width);
			QPointF symbol_p1(rect.x() + rect.width() * 0.25, rect.y() + (i * interleave) + symbol_width);
			painter -> drawLine(QLineF(symbol_p1, symbol_p2));
			
			// dessine le reste des symboles terre et neutre
			if (hasGround && i == 1) {
				drawGround(painter, direction, symbol_p2, symbol_width * 2.0);
			} else if (hasNeutral && ((i == 1 && !hasGround) || (i == 2 && hasGround))) {
				drawNeutral(painter, direction, symbol_p2, symbol_width * 1.5);
			}
		}
	}
	painter -> restore();
}

/**
	Dessine le segment correspondant au symbole de la terre sur un conducteur unifilaire
	@param painter QPainter a utiliser pour dessiner le segment
	@param direction direction du segment sur lequel le symbole apparaitra
	@param center centre du segment
	@param size taille du segment
*/
void SingleLineProperties::drawGround(QPainter *painter, QET::ConductorSegmentType direction, QPointF center, qreal size) {
	painter -> save();
	
	// prepare le QPainter
	painter -> setRenderHint(QPainter::Antialiasing, false);
	QPen pen2(painter -> pen());
	pen2.setCapStyle(Qt::SquareCap);
	painter -> setPen(pen2);
	
	// dessine le segment representant la terre
	qreal half_size = size / 2.0;
	QPointF offset_point(
		(direction == QET::Horizontal) ? half_size : 0.0,
		(direction == QET::Horizontal) ? 0.0 : half_size
	);
	painter -> drawLine(
		QLineF(
			center + offset_point,
			center - offset_point
		)
	);
	
	painter -> restore();
}

/**
	Dessine le cercle correspondant au symbole du neutre sur un conducteur unifilaire
	@param painter QPainter a utiliser pour dessiner le segment
	@param direction direction du segment sur lequel le symbole apparaitra
	@param center centre du cercle
	@param size diametre du cercle
*/
void SingleLineProperties::drawNeutral(QPainter *painter, QET::ConductorSegmentType direction, QPointF center, qreal size) {
	Q_UNUSED(direction);
	painter -> save();
	
	// prepare le QPainter
	if (painter -> brush() == Qt::NoBrush) painter -> setBrush(Qt::black);
	painter -> setPen(Qt::NoPen);
	
	// desine le cercle representant le neutre
	painter -> drawEllipse(
		QRectF(
			center - QPointF(size / 2.0, size / 2.0),
			QSizeF(size, size)
		)
	);
	
	painter -> restore();
}

/**
	Exporte les parametres du conducteur unifilaire sous formes d'attributs XML
	ajoutes a l'element e.
	@param e Element XML auquel seront ajoutes des attributs
*/
void SingleLineProperties::toXml(QDomElement &e) const {
	e.setAttribute("ground",  hasGround  ? "true" : "false");
	e.setAttribute("neutral", hasNeutral ? "true" : "false");
	e.setAttribute("phase",   phases);
}

/**
	Importe les parametres du conducteur unifilaire a partir des attributs XML
	de l'element e
	@param e Element XML dont les attributs seront lus
*/
void SingleLineProperties::fromXml(QDomElement &e) {
	hasGround  = e.attribute("ground")  == "true";
	hasNeutral = e.attribute("neutral") == "true";
	setPhasesCount(e.attribute("phase").toInt());
}

/**
	Constructeur : par defaut, les proprietes font un conducteur
	multifilaire noir dont le texte est "_"
*/
ConductorProperties::ConductorProperties() :
	type(Multi),
	color(Qt::black),
	text("_"),
	style(Qt::SolidLine)
{
}

/**
	Destructeur
*/
ConductorProperties::~ConductorProperties() {
}

/**
	Exporte les parametres du conducteur sous formes d'attributs XML
	ajoutes a l'element e.
	@param e Element XML auquel seront ajoutes des attributs
*/
void ConductorProperties::toXml(QDomElement &e) const {
	e.setAttribute("type", typeToString(type));
	
	if (color != QColor(Qt::black)) {
		e.setAttribute("color", color.name());
	}
	
	if (type == Single) {
		singleLineProperties.toXml(e);
	} else if (type == Multi) {
		e.setAttribute("num", text);
	}
	
	QString conductor_style = writeStyle();
	if (!conductor_style.isEmpty()) {
		e.setAttribute("style", conductor_style);
	}
}

/**
	Importe les parametres du conducteur unifilaire a partir des attributs XML
	de l'element e
	@param e Element XML dont les attributs seront lus
*/
void ConductorProperties::fromXml(QDomElement &e) {
	// recupere la couleur du conducteur
	QColor xml_color= QColor(e.attribute("color"));
	if (xml_color.isValid()) {
		color = xml_color;
	} else {
		color = QColor(Qt::black);
	}
	
	// lit le style du conducteur
	readStyle(e.attribute("style"));
	
	if (e.attribute("type") == typeToString(Single)) {
		// recupere les parametres specifiques a un conducteur unifilaire
		singleLineProperties.fromXml(e);
		type = Single;
	} else if (e.attribute("type") == typeToString(Simple)) {
		type = Simple;
	} else {
		// recupere le champ de texte
		text = e.attribute("num");
		type = Multi;
	}
}

/**
	@param settings Parametres a ecrire
	@param prefix prefixe a ajouter devant les noms des parametres
*/
void ConductorProperties::toSettings(QSettings &settings, const QString &prefix) const {
	settings.setValue(prefix + "color", color.name());
	settings.setValue(prefix + "style", writeStyle());
	settings.setValue(prefix + "type", typeToString(type));
	settings.setValue(prefix + "text", text);
	singleLineProperties.toSettings(settings, prefix);
}

/**
	@param settings Parametres a lire
	@param prefix prefixe a ajouter devant les noms des parametres
*/
void ConductorProperties::fromSettings(QSettings &settings, const QString &prefix) {
	// recupere la couleur dans les parametres
	QColor settings_color = QColor(settings.value(prefix + "color").toString());
	if (settings_color.isValid()) {
		color = settings_color;
	} else {
		color = QColor(Qt::black);
	}
	
	QString setting_type = settings.value(prefix + "type", typeToString(Multi)).toString();
	if (setting_type == typeToString(Single)) {
		type = Single;
	} else if (setting_type == typeToString(Simple)) {
		type = Simple;
	} else {
		type = Multi;
	}
	singleLineProperties.fromSettings(settings, prefix);
	text = settings.value(prefix + "text", "_").toString();
	
	// lit le style du conducteur
	readStyle(settings.value(prefix + "style").toString());
}

/**
	@param t type du conducteur
*/
QString ConductorProperties::typeToString(ConductorType t) {
	switch(t) {
		case Simple: return("simple");
		case Single: return("single");
		case Multi:  return("multi");
		default: return(QString());
	}
}

/**
	@param other l'autre ensemble de proprietes avec lequel il faut effectuer la comparaison
	@return true si les deux ensembles de proprietes sont identiques, false sinon
*/
int ConductorProperties::operator==(const ConductorProperties &other) {
	return(
		other.type == type &&\
		other.color == color &&\
		other.style == style &&\
		other.text == text &&\
		other.singleLineProperties == singleLineProperties
	);
}

/**
	@param other l'autre ensemble de proprietes avec lequel il faut effectuer la comparaison
	@return true si les deux ensembles de proprietes sont differents, false sinon
*/
int ConductorProperties::operator!=(const ConductorProperties &other) {
	return(
		other.type != type ||\
		other.color != color ||\
		other.style != style ||\
		other.text != text ||\
		other.singleLineProperties != singleLineProperties
	);
}

/**
	Applique les styles passes en parametre dans cet objet
	@param style_string Chaine decrivant le style du conducteur
*/
void ConductorProperties::readStyle(const QString &style_string) {
	style = Qt::SolidLine; // style par defaut
	
	if (style_string.isEmpty()) return;
	
	// recupere la liste des couples style / valeur
	QStringList styles = style_string.split(";", QString::SkipEmptyParts);
	
	QRegExp rx("^\\s*([a-z-]+)\\s*:\\s*([a-z-]+)\\s*$");
	foreach (QString style_str, styles) {
		if (rx.exactMatch(style_str)) {
			QString style_name = rx.cap(1);
			QString style_value = rx.cap(2);
			if (style_name == "line-style") {
				if (style_value == "dashed") style = Qt::DashLine;
				else if (style_value == "normal") style = Qt::SolidLine;
			}
		}
	}
}

/**
	Exporte le style du conducteur sous forme d'une chaine de caracteres
	@return une chaine de caracteres decrivant le style du conducteur
*/
QString ConductorProperties::writeStyle() const {
	if (style == Qt::DashLine) {
		return("line-style: dashed;");
	} else {
		return(QString());
	}
}

/**
	@param other l'autre ensemble de proprietes avec lequel il faut effectuer la comparaison
	@return true si les deux ensembles de proprietes sont identiques, false sinon
*/
int SingleLineProperties::operator==(const SingleLineProperties &other) const {
	return(
		other.hasGround == hasGround &&\
		other.hasNeutral == hasNeutral &&\
		other.phases == phases
	);
}

/**
	@param other l'autre ensemble de proprietes avec lequel il faut effectuer la comparaison
	@return true si les deux ensembles de proprietes sont differents, false sinon
*/
int SingleLineProperties::operator!=(const SingleLineProperties &other) const {
	return(!(other == (*this)));
}

/**
	@param settings Parametres a ecrire
	@param prefix prefix a ajouter devant les noms des parametres
*/
void SingleLineProperties::toSettings(QSettings &settings, const QString &prefix) const {
	settings.setValue(prefix + "hasGround",  hasGround);
	settings.setValue(prefix + "hasNeutral", hasNeutral);
	settings.setValue(prefix + "phases",     phases);
}

/**
	@param settings Parametres a lire
	@param prefix prefix a ajouter devant les noms des parametres
*/
void SingleLineProperties::fromSettings(QSettings &settings, const QString &prefix) {
	hasGround  = settings.value(prefix + "hasGround",  true).toBool();
	hasNeutral = settings.value(prefix + "hasNeutral", true).toBool();
	phases     = settings.value(prefix + "phases",     1).toInt();
}
