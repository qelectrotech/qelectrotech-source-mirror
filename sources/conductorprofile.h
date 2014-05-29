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
#ifndef CONDUCTOR_PROFILE_H
#define CONDUCTOR_PROFILE_H
#include <QList>
#include "qet.h"
class Conductor;
class ConductorSegmentProfile;
/**
	This class represents the profile of a conductor, i.e. its primary
	characteristics.
*/
class ConductorProfile {
	public:
	// constructors, destructor
	ConductorProfile();
	ConductorProfile(Conductor *conductor);
	ConductorProfile(const ConductorProfile &);
	ConductorProfile &operator=(const ConductorProfile &);
	virtual ~ConductorProfile();
	
	// attributes
	public:
	/// Segments composing the conductor
	QList<ConductorSegmentProfile *> segments;
	/// Orientation of the start terminal
	Qet::Orientation beginOrientation;
	/// Orientation of the end terminal.
	Qet::Orientation endOrientation;
	
	// methods
	public:
	bool isNull() const;
	void setNull();
	qreal width() const;
	qreal height() const;
	uint segmentsCount(QET::ConductorSegmentType) const;
	QList<ConductorSegmentProfile *> horizontalSegments();
	QList<ConductorSegmentProfile *> verticalSegments();
	void fromConductor(Conductor *);
};
QDebug &operator<<(QDebug d, ConductorProfile &);
#endif
