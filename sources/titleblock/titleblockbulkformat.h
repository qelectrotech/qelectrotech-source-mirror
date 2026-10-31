/*
	Copyright 2006-2026 The QElectroTech Team
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
#ifndef TITLEBLOCK_BULK_FORMAT_H
#define TITLEBLOCK_BULK_FORMAT_H
#include <QColor>
#include <QFont>
#include <QList>
#include <QString>
#include <QUndoCommand>

class QWidget;
class TitleBlockCell;
class TitleBlockTemplateView;

/**
	@brief The TitleBlockBulkFormat class
	What to change in the text of several title block cells at once.
	Only the checked properties are written, every other property of
	each cell keeps its own value.
*/
class TitleBlockBulkFormat
{
	public:
	enum Property {
		Family    = 1 << 0,
		Size      = 1 << 1,
		Bold      = 1 << 2,
		Italic    = 1 << 3,
		Color     = 1 << 4,
		Opacity   = 1 << 5,
		Alignment = 1 << 6
	};
	enum Part {
		LabelAndValue,
		LabelOnly,
		ValueOnly
	};

	int properties = 0;                 ///< Properties to change, bitwise or of Property
	Part part = LabelAndValue;          ///< Which text of the cells to change
	bool all_cells = false;             ///< true to format every text cell, false for the selected cells
	QFont font;                         ///< Source of the family, size, bold and italic
	QColor color = Qt::black;           ///< Source of the colour
	int opacity = 100;                  ///< Source of the opacity, in percent
	int alignment = Qt::AlignLeft;      ///< Source of the horizontal alignment

	static bool getFromUser(TitleBlockBulkFormat &, bool, QWidget *);
	QString mergedFont(const QString &, const QFont &) const;
	QColor mergedColor(const QColor &) const;
	QUndoCommand *createCommand(const QList<TitleBlockCell *> &,
				    TitleBlockTemplateView * = nullptr) const;
};
#endif
