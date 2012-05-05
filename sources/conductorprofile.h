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
#ifndef CONDUCTOR_PROFILE_H
#define CONDUCTOR_PROFILE_H
#include <QList>
#include "qet.h"
class Conductor;
class ConductorSegmentProfile;
/**
	Cette classe contient le profil (= les caracteristiques essentielles) d'un
	conducteur.
*/
class ConductorProfile {
	public:
	// constructeurs, destructeur
	ConductorProfile();
	ConductorProfile(Conductor *conductor);
	ConductorProfile(const ConductorProfile &);
	ConductorProfile &operator=(const ConductorProfile &);
	virtual ~ConductorProfile();
	
	// attributs
	public:
	/// Segment composant le profil du conducteur
	QList<ConductorSegmentProfile *> segments;
	/// Orientation de la borne de depart du profil
	QET::Orientation beginOrientation;
	/// Orientation de la borne d'arrivee du profil
	QET::Orientation endOrientation;
	
	// methodes
	public:
	bool isNull() const;
	void setNull();
	qreal width() const;
	qreal height() const;
	uint nbSegments(QET::ConductorSegmentType) const;
	QList<ConductorSegmentProfile *> horizontalSegments();
	QList<ConductorSegmentProfile *> verticalSegments();
	void fromConductor(Conductor *);
};
QDebug &operator<<(QDebug d, ConductorProfile &);
#endif
