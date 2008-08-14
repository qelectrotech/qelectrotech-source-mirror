/*
	Copyright 2006-2008 Xavier Guerrin
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
#include "borderproperties.h"

/**
	Constructeur
*/
BorderProperties::BorderProperties() {
}

/**
	Destructeur
*/
BorderProperties::~BorderProperties() {
}

/**
	@param bp autre conteneur BorderProperties
	@return true si ip et ce conteneur sont identiques, false sinon
*/
bool BorderProperties::operator==(const BorderProperties &bp) {
	return(
		bp.columns_count == columns_count &&\
		bp.columns_width == columns_width &&\
		bp.columns_header_height == columns_header_height &&\
		bp.display_columns == display_columns &&\
		bp.rows_count == rows_count &&\
		bp.rows_height == rows_height &&\
		bp.rows_header_width == rows_header_width &&\
		bp.display_rows == display_rows
	);
}

/**
	@param bp autre conteneur BorderProperties
	@return false si bp et ce conteneur sont identiques, true sinon
*/
bool BorderProperties::operator!=(const BorderProperties &bp) {
	return(!(*this == bp));
}
