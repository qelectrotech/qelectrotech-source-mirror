/*
	Copyright 2006-2017 The QElectroTech Team
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
#include "nameslist.h"

/**
	This class is a container for the various parameters of a titleblock cell
	@see TitleBlockColumnLength 
*/
class TitleBlockCell {
	public:
	enum TemplateCellType {
		EmptyCell,
		TextCell,
		LogoCell
	};
	enum TemplateCellSpanState {
		Disabled,     ///< the cell span parameters should not applied at all
		Enabled,      ///< the cell span parameters should be applied without restriction
		Restricted    ///< the cell span parameters should be applied with some restrictions
	};
	
	// Constructor, destructor
	public:
	TitleBlockCell();
	virtual ~TitleBlockCell();
	
	// methods
	public:
	TemplateCellType type() const;
	int horizontalAlign() const;
	int verticalAlign() const;
	void setAttribute(const QString &, const QVariant &);
	QVariant attribute(const QString &);
	static QString attributeName(const QString &);
	bool spans() const;
	void loadContentFromCell(const TitleBlockCell &);
	void loadContentFromXml(const QDomElement &);
	void saveContentToXml(QDomElement &);
	
	
	// attributes
	public:
	TemplateCellType cell_type;        ///< Cell type: empty, text, logo?
	int num_row;                       ///< y coordinate of the cell within its parent title block template grid
	int num_col;                       ///< x coordinate of the cell within its parent title block template grid
	int row_span;                      ///< number of extra rows spanned by this cell
	int col_span;                      ///< number of extra columns spanned by this cell
	int span_state;                    ///< how should row_span and col_span be applied given other cells in the parent template
	int applied_row_span;              ///< Actually applied row span
	int applied_col_span;              ///< Actually applied column span
	TitleBlockCell *spanner_cell;      ///< Cell spanning this cell, if any
	QString value_name;                ///< name of the cell; not displayed when the title block template is rendered
	NamesList value;                   ///< Text displayed by the cell
	NamesList label;                   ///< Label displayed by the cell
	bool display_label;                ///< Whether to display the label or not
	int alignment;                     ///< Where the label+text should be displayed within the visual cell
	int font_size;                     ///< Font size the text should be rendered with
	bool hadjust;                      ///< Whether to reduce the font size if the text does not fit in the cell
	QString logo_reference;            ///< Logo displayed by this cell, it it is a logo cell
};
#endif
