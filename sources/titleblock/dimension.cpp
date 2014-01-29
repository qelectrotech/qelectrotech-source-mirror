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
#include "dimension.h"

/**
	Constructor
	@param v Numeric value for this dimension
	@param t Kind of length, determining how to interpret the numeric value
*/
TitleBlockDimension::TitleBlockDimension(int v, QET::TitleBlockColumnLength t) :
	type(t),
	value(v)
{
}

/**
	@return a string describing this dimension in a human-readable format.
*/
QString TitleBlockDimension::toString() const {
	QString dim_str;
	if (type == QET::Absolute) {
		dim_str = QObject::tr("%1px", "titleblock: absolute width");
	} else if (type == QET::RelativeToTotalLength) {
		dim_str = QObject::tr("%1%", "titleblock: width relative to total length");
	} else if (type == QET::RelativeToRemainingLength) {
		dim_str = QObject::tr("%1% du restant", "titleblock: width relative to remaining length");
	}
	return(dim_str.arg(value));
}

/**
	@return a string describing this dimension in a short format.
*/
QString TitleBlockDimension::toShortString() const {
	QString short_string;
	if (type == QET::RelativeToTotalLength) {
		short_string = "t";
	} else if (type == QET::RelativeToRemainingLength) {
		short_string = "r";
	}
	short_string += QString("%1%2;").arg(value).arg(type == QET::Absolute ? "px" : "%");
	return(short_string);
}
