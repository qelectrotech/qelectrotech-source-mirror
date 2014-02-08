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
#include "diagramfoliolist.h"
#include <QPainter>

int DiagramFolioList::folioList_quantity = 0;
qreal DiagramFolioList::colWidths[4] = {0.1, 0.55, 0.2, 0.15};

DiagramFolioList::DiagramFolioList(QObject *parent) : Diagram(parent)
{
	list_lines_.clear();
	list_rectangles_.clear();

	id = folioList_quantity;
	folioList_quantity++;

	qreal width  = border_and_titleblock.columnsTotalWidth();
	qreal height = border_and_titleblock.rowsTotalHeight();

	//top left corner of drawable area
	qreal x0 = border_and_titleblock.rowsHeaderWidth();
	qreal y0 = border_and_titleblock.columnsHeaderHeight();
	QRectF row_RectF(x0, y0, width, height);

	buildGrid(row_RectF,30,2,colWidths);
}

DiagramFolioList::~DiagramFolioList()
{
	if (folioList_quantity > 0)
		folioList_quantity--;
}

void DiagramFolioList::drawBackground(QPainter *p, const QRectF &r)
{
	p -> save();

	// desactive tout antialiasing, sauf pour le texte
	p -> setRenderHint(QPainter::Antialiasing, false);
	p -> setRenderHint(QPainter::TextAntialiasing, true);
	p -> setRenderHint(QPainter::SmoothPixmapTransform, false);

	// dessine un fond blanc
	p -> setPen(Qt::NoPen);
	p -> setBrush(Diagram::background_color);
	p -> drawRect(r);
	p -> setPen(Qt::black);

	QString authorTranslatable = tr("Auteur");
	QString titleTranslatable = tr("Titre");
	QString folioTranslatable = tr("Folio");
	QString dateTranslatable = tr("Date");

	qreal x0 = list_rectangles_[0] -> topLeft().x();
	qreal y0 = list_rectangles_[0] -> topLeft().y();
	qreal rowHeight = (list_rectangles_[0] -> height())/30;
	QRectF row_RectF(x0, y0, list_rectangles_[0] -> width(), rowHeight);

	fillRow(p, row_RectF, authorTranslatable, titleTranslatable, folioTranslatable, dateTranslatable);
	QList<Diagram *> diagram_list = project() -> diagrams();

	int startDiagram = id * 58;

	for (int i = startDiagram; i < startDiagram+29 && i < diagram_list.size(); ++i) {
		y0 += rowHeight;
		QRectF row_rect(x0, y0, list_rectangles_[0] -> width(), rowHeight);
		fillRow(p, row_rect, diagram_list[i] -> border_and_titleblock.author(),
				diagram_list[i] -> border_and_titleblock.title(),
				 diagram_list[i] -> border_and_titleblock.folio(),
				diagram_list[i] -> border_and_titleblock.date().toString("dd/MM/yyyy"));
	}

	x0 = list_rectangles_[1] -> topLeft().x();
	y0 = list_rectangles_[1] -> topLeft().y();
	rowHeight = (list_rectangles_[1] -> height())/30;
	QRectF row_RectF2(x0, y0, list_rectangles_[1] -> width(), rowHeight);
	fillRow(p, row_RectF2, authorTranslatable, titleTranslatable, folioTranslatable, dateTranslatable);

	startDiagram += 29;

	for (int i = startDiagram; i < startDiagram+29 && i < diagram_list.size(); ++i) {
		y0 += rowHeight;
		QRectF row_rect(x0, y0, list_rectangles_[1] -> width(), rowHeight);
		fillRow(p, row_rect, diagram_list[i] -> border_and_titleblock.author(),
				diagram_list[i] -> border_and_titleblock.title(),
				 diagram_list[i] -> border_and_titleblock.folio(),
				diagram_list[i] -> border_and_titleblock.date().toString("dd/MM/yyyy"));
	}

	border_and_titleblock.draw(p, margin, margin);
	p -> restore();
}

void DiagramFolioList::fillRow(QPainter *qp, const QRectF &row_rect, QString author, QString title,
							   QString folio, QString date)
{
	qreal x = row_rect.topLeft().x();
	qreal y = row_rect.topLeft().y();

	qp -> drawText(QRectF(x, y, colWidths[0]*row_rect.width(), row_rect.height()), Qt::AlignCenter, folio);
	x += colWidths[0]*row_rect.width();

	qp -> drawText(QRectF(x, y, colWidths[1]*row_rect.width(), row_rect.height()), Qt::AlignCenter, title);
	x += colWidths[1]*row_rect.width();

	qp -> drawText(QRectF(x, y, colWidths[2]*row_rect.width(), row_rect.height()), Qt::AlignCenter, author);
	x += colWidths[2]*row_rect.width();

	qp -> drawText(QRectF(x, y, colWidths[3]*row_rect.width(), row_rect.height()), Qt::AlignCenter, date);
}

void DiagramFolioList::buildGrid(const QRectF &rect, int rows, int tables, qreal colWidths[])
{
	qreal sum = 0;
	for (int i = 0; i < 4; i++ )
		sum += colWidths[i];
	if ( sum < 0.99 || sum > 1.01 ) {
		qDebug() << "Invalid input: Column widths do not sum to 1";
		return;
	}

	qreal tablesSpacing = rect.height() * 0.02;
	qreal tableWidth = (rect.width() - tablesSpacing*(tables+1) ) / tables;
	qreal rowHeight = (rect.height() - 2*tablesSpacing) / rows;
	int cols = 4;//colWidths.size();

	qreal x0 = tablesSpacing + rect.topLeft().x();
	qreal y0 = tablesSpacing + rect.topLeft().y();

	for (int i = 0; i < tables; ++i) {
		QRectF *tableRect = new QRectF(x0, y0, tableWidth, rect.height() - 2*tablesSpacing);
		addRect(*tableRect);
		list_rectangles_.push_back(tableRect);
		for (int j = 1; j < rows; ++j) {
			QLineF *line = new QLineF(x0, y0 + j*rowHeight, x0 + tableWidth,y0 + j*rowHeight);
			addLine(*line);
			list_lines_.push_back(line);
		}
		for (int j = 0; j < cols-1; ++j) {
			QLineF *line = new QLineF(x0 + colWidths[j]*tableWidth, y0, x0 + colWidths[j]*tableWidth,y0 + rows*rowHeight);
			addLine(*line);
			list_lines_.push_back(line);
			x0 += colWidths[j]*tableWidth;
		}
		x0 += colWidths[cols-1]*tableWidth + tablesSpacing;
	}
}
