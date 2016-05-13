/*
	Copyright 2006-2016 The QElectroTech Team
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
#ifndef CONDUCTOR_SEGMENT_PROFILE_H
#define CONDUCTOR_SEGMENT_PROFILE_H
#include <QtCore>
#include "conductorsegment.h"
/**
	This class embeds the profile (i.e. main characteristics) of a conductor
	segment.
*/
class ConductorSegmentProfile {
	// constructors, destructor
	public:
	/**
		Constructor
		@param l segment length
		@param ori true if the segment is horizontal, false if it is vertical
	*/
	ConductorSegmentProfile(qreal l, bool ori = true) :
		length(l),
		isHorizontal(ori)
	{
	}
	
	/**
		Constructor
		@param segment Segment the profile should be copied from.
	*/
	ConductorSegmentProfile(ConductorSegment *segment) :
		length(segment -> length()),
		isHorizontal(segment -> isHorizontal())
	{
	}
	
	/// Destructor
	virtual ~ConductorSegmentProfile() {
	}
	
	// attributes
	public:
	/// segment length
	qreal length;
	/// segment orientation
	bool isHorizontal;
};
#endif
