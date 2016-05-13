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
#ifndef TITLEBLOCK_SLASH_TEMPLATE_CELLS_SET_H
#define TITLEBLOCK_SLASH_TEMPLATE_CELLS_SET_H
#include <QtWidgets>
class TitleBlockCell;
class TitleBlockTemplateVisualCell;
class TitleBlockTemplateView;
/**
	This class represents a set of cells (selected or not) when editing a
	title block template.
*/
class TitleBlockTemplateCellsSet : public QList<TitleBlockTemplateVisualCell *> {
	// Constructors, destructor
	public:
	TitleBlockTemplateCellsSet(const TitleBlockTemplateView *);
	TitleBlockTemplateCellsSet(const TitleBlockTemplateCellsSet &);
	virtual ~TitleBlockTemplateCellsSet();
	
	// methods
	public:
	QPainterPath painterPath() const;
	bool isRectangle() const;
	bool allCellsAreSelected() const;
	bool hasExternalSpan() const;
	TitleBlockTemplateVisualCell *topLeftCell() const;
	TitleBlockTemplateVisualCell *bottomRightCell() const;
	QRectF mergeAreaRect() const;
	TitleBlockTemplateCellsSet mergeArea(const QRectF & = QRectF()) const;
	QSet<TitleBlockCell *> cells(bool = true) const;
	
	// attributes
	public:
	const TitleBlockTemplateView *parent_view_; ///< the view displaying the cells
};
#endif
