/*
	Copyright 2006-2011 Xavier Guerrin
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
#ifndef TITLEBLOCK_CELL_H
#define TITLEBLOCK_CELL_H
/**
	This class is a container for the various parameters of an titleblock cell
	@see TitleBlockColumnLength 
*/
class TitleBlockCell {
	public:
	TitleBlockCell();
	QString toString() const;
	bool is_null;
	int num_row;
	int num_col;
	int row_span;
	int col_span;
	TitleBlockCell *spanner_cell;
	QString value_name;
	QString value;
	QString label;
	bool display_label;
	int alignment;
	int font_size;
	bool hadjust;
	QString logo_reference;
};
#endif
