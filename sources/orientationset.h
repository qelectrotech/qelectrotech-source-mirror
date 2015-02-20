/*
	Copyright 2006-2015 The QElectroTech Team
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
#ifndef ORIENTATION_SET_H
#define ORIENTATION_SET_H
#include "qet.h"
/**
	This class provides a container for element orientations, i.e. a data
	structure that stores, for a particular element:
	  - its default orientation,
	  - its current orientation,
	  - whether the North, East, South and West orientations are allowed.
*/
class OrientationSet {
	
	// constructors, destructor
	public:
	OrientationSet();
	virtual ~OrientationSet() {};
	
	// attributes
	private:
	bool north_ori;
	bool east_ori;
	bool south_ori;
	bool west_ori;
	Qet::Orientation default_ori;
	Qet::Orientation current_ori;
	
	// methods
	public:
	bool north() const;
	bool east() const;
	bool south() const;
	bool west() const;
	bool setNorth(bool);
	bool setEast(bool);
	bool setSouth(bool);
	bool setWest(bool);
	Qet::Orientation defaultOrientation() const;
	void setDefaultOrientation(const Qet::Orientation &);
	Qet::Orientation current() const;
	bool setCurrent(Qet::Orientation);
	Qet::Orientation next() const;
	Qet::Orientation previous() const;
	Qet::Orientation setNext();
	Qet::Orientation setPrevious();
	bool accept(Qet::Orientation) const;
	const OrientationSet operator++(int);
	const OrientationSet operator--(int);
	const OrientationSet operator++();
	const OrientationSet operator--();
	bool operator==(const OrientationSet &) const;
	bool operator!=(const OrientationSet &) const;
	bool fromString(const QString &);
	QString toString() const;
};

/**
	@return whether the Northern orientation is allowed
*/
inline bool OrientationSet::north() const {
	return(north_ori);
}

/**
	@return whether the Eastern orientation is allowed
*/
inline bool OrientationSet::east() const {
	return(east_ori);
}

/**
	@return whether the Southern orientation is allowed
*/
inline bool OrientationSet::south() const {
	return(south_ori);
}

/**
	@return whether the Western orientation is allowed
*/
inline bool OrientationSet::west() const {
	return(west_ori);
}

/**
	@param new_default_orientation The new default orientation
*/
inline void OrientationSet::setDefaultOrientation(const Qet::Orientation& new_default_orientation) {
	default_ori = new_default_orientation;
}

/**
	@return the default orientation
*/
inline Qet::Orientation OrientationSet::defaultOrientation() const {
	return(default_ori);
}

/**
	@return the current orientation
*/
inline Qet::Orientation OrientationSet::current() const {
	return(current_ori);
}

#endif
