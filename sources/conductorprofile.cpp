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
#include "conductorprofile.h"
#include "qetgraphicsitem/conductor.h"
#include "conductorsegmentprofile.h"
#include "terminal.h"

/// Constructeur
ConductorProfile::ConductorProfile() {
}

/**
	Constructeur
	@param conductor conducteur dont il faut extraire le profil
*/
ConductorProfile::ConductorProfile(Conductor *conductor) {
	fromConductor(conductor);
}

/**
	Constructeur de copie
	@param c autre conducteur
*/
ConductorProfile::ConductorProfile(const ConductorProfile &c) {
	beginOrientation = c.beginOrientation;
	endOrientation   = c.endOrientation;
	foreach(ConductorSegmentProfile *csp, c.segments) {
		segments << new ConductorSegmentProfile(*csp);
	}
}

/**
	Operateur =
	@param c autre conducteur
*/
ConductorProfile &ConductorProfile::operator=(const ConductorProfile &c) {
	if (&c == this) return(*this);
	
	// supprime ses informations
	setNull();
	
	// copie les informations de l'autre profil de conducteur
	beginOrientation = c.beginOrientation;
	endOrientation   = c.endOrientation;
	foreach(ConductorSegmentProfile *csp, c.segments) {
		segments << new ConductorSegmentProfile(*csp);
	}
	return(*this);
}

/// destructeur
ConductorProfile::~ConductorProfile() {
	setNull();
}

/// @return true si le profil est nul
bool ConductorProfile::isNull() const {
	return(segments.isEmpty());
}

/// supprime les segments du profil de conducteur
void ConductorProfile::setNull() {
	foreach(ConductorSegmentProfile *csp, segments) delete csp;
	segments.clear();
}

/// @return la largeur occupee par le conducteur
qreal ConductorProfile::width() const {
	qreal width = 0.0;
	foreach(ConductorSegmentProfile *csp, segments) {
		if (csp -> isHorizontal) width += csp -> length;
	}
	return(width);
}

/// @return la hauteur occupee par le conducteur
qreal ConductorProfile::height() const{
	qreal height = 0.0;
	foreach(ConductorSegmentProfile *csp, segments) {
		if (!csp -> isHorizontal) height += csp -> length;
	}
	return(height);
}

/**
	@param type Type de Segments
	@return Le nombre de segments composant le conducteur.
*/
uint ConductorProfile::segmentsCount(QET::ConductorSegmentType type) const {
	if (type == QET::Both) return(segments.count());
	uint nb_seg = 0;
	foreach(ConductorSegmentProfile *csp, segments) {
		if (type == QET::Horizontal && csp -> isHorizontal) ++ nb_seg;
		else if (type == QET::Vertical && !csp -> isHorizontal) ++ nb_seg;
	}
	return(nb_seg);
}

/// @return les segments horizontaux de ce profil
QList<ConductorSegmentProfile *> ConductorProfile::horizontalSegments() {
	QList<ConductorSegmentProfile *> segments_list;
	foreach(ConductorSegmentProfile *csp, segments) {
		if (csp -> isHorizontal) segments_list << csp;
	}
	return(segments_list);
}

/// @return les segments verticaux de ce profil
QList<ConductorSegmentProfile *> ConductorProfile::verticalSegments() {
	QList<ConductorSegmentProfile *> segments_list;
	foreach(ConductorSegmentProfile *csp, segments) {
		if (!csp -> isHorizontal) segments_list << csp;
	}
	return(segments_list);
}

/**
	Reconstruit le profil a partir d'un conducteur existant
*/
void ConductorProfile::fromConductor(Conductor *conductor) {
	// supprime les segments precedents
	setNull();
	
	foreach(ConductorSegment *conductor_segment, conductor -> segmentsList()) {
		segments << new ConductorSegmentProfile(conductor_segment);
	}
	beginOrientation = conductor -> terminal1 -> orientation();
	endOrientation   = conductor -> terminal2 -> orientation();
}

/**
	Permet de debugger un profil de conducteur
	@param d Object QDebug a utiliser pour l'affichage des informations de debug
	@param t Profil de conducteur a debugger
*/
QDebug &operator<<(QDebug d, ConductorProfile &t) {
	d << "ConductorProfile {";
	foreach(ConductorSegmentProfile *csp, t.segments) {
		d << "CSP" << (csp -> isHorizontal ? "horizontal" : "vertical") << ":" << csp -> length << ",";
	}
	d << "}";
	return(d.space());
}
