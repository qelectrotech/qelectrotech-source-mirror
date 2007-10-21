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
void SingleLineProperties::drawNeutral(QPainter *painter, QET::ConductorSegmentType, QPointF center, qreal size) {
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
	exporte les parametres du conducteur unifilaire sous formes d'attributs XML
	ajoutes a l'element e.
	@param d Document XML ; utilise pour ajouter (potentiellement) des elements XML
	@param e Element XML auquel seront ajoutes des attributs
*/
void SingleLineProperties::toXml(QDomDocument &, QDomElement &e) const {
	e.setAttribute("ground",  hasGround  ? "true" : "false");
	e.setAttribute("neutral", hasNeutral ? "true" : "false");
	e.setAttribute("phase",   phases);
}

/**
	importe les parametres du conducteur unifilaire a partir des attributs XML
	de l'element e
	@param e Element XML dont les attributs seront lus
*/
void SingleLineProperties::fromXml(QDomElement &e) {
	hasGround  = e.attribute("ground")  == "true";
	hasNeutral = e.attribute("neutral") == "true";
	setPhasesCount(e.attribute("phase").toInt());
}

/**
	exporte les parametres du conducteur sous formes d'attributs XML
	ajoutes a l'element e.
	@param d Document XML ; utilise pour ajouter (potentiellement) des elements XML
	@param e Element XML auquel seront ajoutes des attributs
*/
void ConductorProperties::toXml(QDomDocument &d, QDomElement &e) const {
	e.setAttribute("type", typeToString(type));
	if (type == Single) {
		singleLineProperties.toXml(d, e);
	} else if (type == Multi) {
		e.setAttribute("num", text);
	}
}

/**
	importe les parametres du conducteur unifilaire a partir des attributs XML
	de l'element e
	@param e Element XML dont les attributs seront lus
*/
void ConductorProperties::fromXml(QDomElement &e) {
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
		other.text != text ||\
		other.singleLineProperties != singleLineProperties
	);
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
