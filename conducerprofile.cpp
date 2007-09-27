#include "conducerprofile.h"
#include "conducer.h"
#include "conducersegmentprofile.h"

/// Constructeur
ConducerProfile::ConducerProfile() {
}

/**
	Constructeur
	@param Conducer conducteur dont il faut extraire le profil
*/
ConducerProfile::ConducerProfile(Conducer *conducer) {
	fromConducer(conducer);
}

/**
	Constructeur de copie
	@param c autre conducteur
*/
ConducerProfile::ConducerProfile(const ConducerProfile &c) {
	beginOrientation = c.beginOrientation;
	endOrientation   = c.endOrientation;
	foreach(ConducerSegmentProfile *csp, c.segments) {
		segments << new ConducerSegmentProfile(*csp);
	}
}

/**
	Operateur =
	@param c autre conducteur
*/
ConducerProfile &ConducerProfile::operator=(const ConducerProfile &c) {
	if (&c == this) return(*this);
	
	// supprime ses informations
	setNull();
	
	// copie les informations de l'autre profil de conducteur
	beginOrientation = c.beginOrientation;
	endOrientation   = c.endOrientation;
	foreach(ConducerSegmentProfile *csp, c.segments) {
		segments << new ConducerSegmentProfile(*csp);
	}
	return(*this);
}

/// destructeur
ConducerProfile::~ConducerProfile() {
	setNull();
}

/// @return true si le profil est nul
bool ConducerProfile::isNull() const {
	return(segments.isEmpty());
}

/// supprime les segments du profil de conducteur
void ConducerProfile::setNull() {
	foreach(ConducerSegmentProfile *csp, segments) delete csp;
	segments.clear();
}

/// @return la largeur occupee par le conducteur
qreal ConducerProfile::width() const {
	qreal width = 0.0;
	foreach(ConducerSegmentProfile *csp, segments) {
		if (csp -> isHorizontal) width += csp -> length;
	}
	return(width);
}

/// @return la hauteur occupee par le conducteur
qreal ConducerProfile::height() const{
	qreal height = 0.0;
	foreach(ConducerSegmentProfile *csp, segments) {
		if (!csp -> isHorizontal) height += csp -> length;
	}
	return(height);
}

/**
	@param type Type de Segments
	@return Le nombre de segments composant le conducteur.
*/
uint ConducerProfile::nbSegments(QET::ConducerSegmentType type) const {
	if (type == QET::Both) return(segments.count());
	uint nb_seg = 0;
	foreach(ConducerSegmentProfile *csp, segments) {
		if (type == QET::Horizontal && csp -> isHorizontal) ++ nb_seg;
		else if (type == QET::Vertical && !csp -> isHorizontal) ++ nb_seg;
	}
	return(nb_seg);
}

/// @return les segments horizontaux de ce profil
QList<ConducerSegmentProfile *> ConducerProfile::horizontalSegments() {
	QList<ConducerSegmentProfile *> segments_list;
	foreach(ConducerSegmentProfile *csp, segments) {
		if (csp -> isHorizontal) segments_list << csp;
	}
	return(segments_list);
}

/// @return les segments verticaux de ce profil
QList<ConducerSegmentProfile *> ConducerProfile::verticalSegments() {
	QList<ConducerSegmentProfile *> segments_list;
	foreach(ConducerSegmentProfile *csp, segments) {
		if (!csp -> isHorizontal) segments_list << csp;
	}
	return(segments_list);
}

void ConducerProfile::fromConducer(Conducer *conducer) {
	// supprime les segments precedents
	setNull();
	
	foreach(ConducerSegment *conducer_segment, conducer -> segmentsList()) {
		segments << new ConducerSegmentProfile(conducer_segment);
	}
	beginOrientation = conducer -> terminal1 -> orientation();
	endOrientation   = conducer -> terminal2 -> orientation();
}

/**
	Permet de debugger un profil de conducteur
	@param d Object QDebug a utiliser pour l'affichage des informations de debug
	@param t Profil de conducteur a debugger
*/
QDebug &operator<<(QDebug d, ConducerProfile &t) {
	d << "ConducerProfile {";
	foreach(ConducerSegmentProfile *csp, t.segments) {
		d << "CSP" << (csp -> isHorizontal ? "horizontal" : "vertical") << ":" << csp -> length << ",";
	}
	d << "}";
	return(d.space());
}
