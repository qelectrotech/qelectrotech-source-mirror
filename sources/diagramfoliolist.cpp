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
#include "diagramfoliolist.h"
#include <QPainter>
#include "qetapp.h"

qreal DiagramFolioList::colWidths[7] = {0.05, 0.50, 0.10, 0.10, 0.05, 0.10, 0.10};

/**
 * @brief DiagramFolioList::DiagramFolioList
 * Constructor
 * @param project : The project of this diagram and also parent QObject
 */
DiagramFolioList::DiagramFolioList( QETProject *project) :
	Diagram(project)
{
	id = project -> getFolioSheetsQuantity();
}

/**
 * @brief DiagramFolioList::~DiagramFolioList
 * Destructor
 */
DiagramFolioList::~DiagramFolioList()
{
	qDeleteAll (list_lines_);
	qDeleteAll (list_rectangles_);
	int folioSheetQty = project() -> getFolioSheetsQuantity();
	if (folioSheetQty > 0)
		project() -> setFolioSheetsQuantity(folioSheetQty-1);
}

/**
 * @brief DiagramFolioList::drawBackground
 * Draw background, and call method to draw the folio list (grid)
 * @param p painter to use
 * @param r rectangle where we paint
 */
void DiagramFolioList::drawBackground(QPainter *p, const QRectF &r)
{
	p -> save();
	QSettings settings;
	// disable all antialiasing, except for the texts
	p -> setRenderHint(QPainter::Antialiasing, false);
	p -> setRenderHint(QPainter::TextAntialiasing, true);
	p -> setRenderHint(QPainter::SmoothPixmapTransform, false);

	// draw white background
	p -> setPen(Qt::NoPen);
	p -> setBrush(Diagram::background_color);
	p -> drawRect(r);
	p -> setPen(Qt::black);	

	qreal width  = border_and_titleblock.columnsTotalWidth();
	qreal height = border_and_titleblock.rowsTotalHeight();

	//top left corner of drawable area
	qreal x0 = border_and_titleblock.rowsHeaderWidth();
	qreal y0 = border_and_titleblock.columnsHeaderHeight();
	QRectF row_RectBorder(x0, y0, width, height);

	buildGrid(p, row_RectBorder,30,1,colWidths);

	x0 = list_rectangles_[0] -> topLeft().x();
	y0 = list_rectangles_[0] -> topLeft().y();

	qreal rowHeight = (list_rectangles_[0] -> height())/30;
	QRectF row_RectF(x0, y0, list_rectangles_[0] -> width(), rowHeight);

	fillHeader(p, row_RectF);
	QList<Diagram *> diagram_list = project() -> diagrams();

	int startDiagram = id * 29;
	for (int i = startDiagram; i < startDiagram+29 && i < diagram_list.size(); ++i) {
		y0 += rowHeight;
		QRectF row_rect(x0, y0, list_rectangles_[0] -> width(), rowHeight);
		if (settings.value("genericpanel/folio", true).toBool()){
		fillRow(p, row_rect, diagram_list[i] -> border_and_titleblock.author(),
				diagram_list[i] -> title(),
				diagram_list[i] -> border_and_titleblock.finalfolio(),
				diagram_list[i] -> border_and_titleblock.machine(),
				diagram_list[i] -> border_and_titleblock.locmach(),
				diagram_list[i] -> border_and_titleblock.indexrev(),
				diagram_list[i] -> border_and_titleblock.date().toString(Qt::SystemLocaleShortDate));
	}else{
			fillRow(p, row_rect, diagram_list[i] -> border_and_titleblock.author(),
				diagram_list[i] -> title(),
				QString::number(diagram_list[i] ->folioIndex()+1),
				diagram_list[i] -> border_and_titleblock.machine(),
				diagram_list[i] -> border_and_titleblock.locmach(),
				diagram_list[i] -> border_and_titleblock.indexrev(),
				diagram_list[i] -> border_and_titleblock.date().toString(Qt::SystemLocaleShortDate));
	}

	}

	border_and_titleblock.draw(p);
	p -> restore();
}

/**
 * @brief DiagramFolioList::fillRow
 * Add new row and fill it with the given information.
 * @param qp Qpainter to use
 * @param row_rect rectangle where we must draw the new row
 */
void DiagramFolioList::fillRow(QPainter *qp, const QRectF &row_rect, QString author, QString title,
							   QString folio, QString machine, QString loc, QString indexrev, QString date)
{
	qreal x = row_rect.topLeft().x();
	qreal y = row_rect.topLeft().y();
	QSettings settings;


	QFontMetrics origFontMetrics(QETApp::diagramTextsFont());
	qreal origFontSize = QETApp::diagramTextsFont().pointSizeF();
	QFont workingFont(QETApp::diagramTextsFont());

	if (settings.value("genericpanel/folio", true).toBool()){
	// reduce the font size if the text entry is long
	if (origFontMetrics.width(folio) > 0.95*colWidths[0]*row_rect.width())
		workingFont.setPointSizeF(origFontSize * 0.95*colWidths[0]*row_rect.width() / origFontMetrics.width(folio));
	else
		workingFont.setPointSizeF(origFontSize);
	qp -> setFont(workingFont);
	qp -> drawText(QRectF(x, y, colWidths[0]*row_rect.width(), row_rect.height()), Qt::AlignCenter, folio);
	
	x += colWidths[0]*row_rect.width();
	}else{
	if (origFontMetrics.width(folio) > 0.95*colWidths[0]*row_rect.width())
		workingFont.setPointSizeF(origFontSize * 0.95*colWidths[0]*row_rect.width() / origFontMetrics.width(folio));
	else
		workingFont.setPointSizeF(origFontSize);
	qp -> setFont(workingFont);
	qp -> drawText(QRectF(x, y, colWidths[0]*row_rect.width(), row_rect.height()), Qt::AlignCenter, folio);
	x += colWidths[0]*row_rect.width();
	}


	if (origFontMetrics.width(title) > 0.95*colWidths[1]*row_rect.width())
		workingFont.setPointSizeF(origFontSize * 0.95*colWidths[1]*row_rect.width() / origFontMetrics.width(title));
	else
		workingFont.setPointSizeF(origFontSize);
	qp -> setFont(workingFont);
	QString heading = tr("Titre");
	if (title == heading)
		qp -> drawText(QRectF(x, y, colWidths[1]*row_rect.width(),row_rect.height()), Qt::AlignCenter, title);
	else
		qp -> drawText(QRectF(x+0.01*row_rect.width(), y, colWidths[1]*row_rect.width()*10.2,
					   row_rect.height()), Qt::AlignLeft | Qt::AlignVCenter, title);
	x += colWidths[1]*row_rect.width();
	
	
	if (origFontMetrics.width(machine) > 0.95*colWidths[2]*row_rect.width())
		workingFont.setPointSizeF(origFontSize * 0.95*colWidths[2]*row_rect.width() / origFontMetrics.width(machine));
	else
		workingFont.setPointSizeF(origFontSize);
	qp -> setFont(workingFont);
	qp -> drawText(QRectF(x, y, colWidths[3]*row_rect.width(), row_rect.height()), Qt::AlignCenter, machine);
	x += colWidths[2]*row_rect.width();
	
	
	if (origFontMetrics.width(loc) > 0.95*colWidths[3]*row_rect.width())
		workingFont.setPointSizeF(origFontSize * 0.95*colWidths[3]*row_rect.width() / origFontMetrics.width(loc));
	else
		workingFont.setPointSizeF(origFontSize);
	qp -> setFont(workingFont);
	qp -> drawText(QRectF(x, y, colWidths[3]*row_rect.width(), row_rect.height()), Qt::AlignCenter, loc);
	x += colWidths[3]*row_rect.width();
	
	
	if (origFontMetrics.width(indexrev) > 0.95*colWidths[4]*row_rect.width())
		workingFont.setPointSizeF(origFontSize * 0.95*colWidths[4]*row_rect.width() / origFontMetrics.width(indexrev));
	else
		workingFont.setPointSizeF(origFontSize);
	qp -> setFont(workingFont);
	qp -> drawText(QRectF(x, y, colWidths[4]*row_rect.width(), row_rect.height()), Qt::AlignCenter, indexrev);
	x += colWidths[4]*row_rect.width();
	
	
	

	if (origFontMetrics.width(author) > 0.95*colWidths[5]*row_rect.width())
		workingFont.setPointSizeF(origFontSize * 0.95*colWidths[5]*row_rect.width() / origFontMetrics.width(author));
	else
		workingFont.setPointSizeF(origFontSize);
	qp -> setFont(workingFont);
	qp -> drawText(QRectF(x, y, colWidths[6]*row_rect.width(), row_rect.height()), Qt::AlignCenter, author);
	x += colWidths[5]*row_rect.width();
	

	if (origFontMetrics.width(date) > 0.95*colWidths[6]*row_rect.width())
		workingFont.setPointSizeF(origFontSize * 0.95*colWidths[6]*row_rect.width() / origFontMetrics.width(date));
	else
		workingFont.setPointSizeF(origFontSize);
	qp -> setFont(workingFont);
	qp -> drawText(QRectF(x, y, colWidths[6]*row_rect.width(), row_rect.height()), Qt::AlignCenter, date);
}

void DiagramFolioList::buildGrid(QPainter *qp, const QRectF &rect, int rows, int tables, qreal colWidths[])
{
	qDeleteAll (list_lines_);
	list_lines_.clear();
	qDeleteAll (list_rectangles_);
	list_rectangles_.clear();

    qreal sum = 0;
    for (int i = 0; i < 7; i++ )
        sum += colWidths[i];
    if ( sum < 0.99 || sum > 1.01 ) {
        qDebug() << "Invalid input: Column widths do not sum to 1";
        return;
    }

    qreal tablesSpacing = rect.height() * 0.02;
	qreal tableWidth = (rect.width() - tablesSpacing*(tables+1) ) / tables;
	qreal rowHeight = (rect.height() - 2*tablesSpacing) / rows;
	int cols = 7;//colWidths.size();

	qreal x0 = tablesSpacing + rect.topLeft().x();
	qreal y0 = tablesSpacing + rect.topLeft().y();

	for (int i = 0; i < tables; ++i) {
		QRectF *tableRect = new QRectF(x0, y0, tableWidth, rect.height() - 2*tablesSpacing);
		qp->drawRect(*tableRect);
		list_rectangles_.push_back(tableRect);
		for (int j = 1; j < rows; ++j) {
			QLineF *line = new QLineF(x0, y0 + j*rowHeight, x0 + tableWidth,y0 + j*rowHeight);
			qp->drawLine(*line);
			list_lines_.push_back(line);
		}
		for (int j = 0; j < cols-1; ++j) {
			QLineF *line = new QLineF(x0 + colWidths[j]*tableWidth, y0, x0 + colWidths[j]*tableWidth,y0 + rows*rowHeight);
			qp->drawLine(*line);
			list_lines_.push_back(line);
			x0 += colWidths[j]*tableWidth;
		}
		x0 += colWidths[cols-1]*tableWidth + tablesSpacing;
	}
}

/**
 * @brief DiagramFolioList::fillHeader
 * Fill the header with bigger font
 * @param qp the painter to use
 * @param row_RectF rectangle of header
 */
void DiagramFolioList::fillHeader(QPainter *qp, const QRectF &row_RectF) {
	QString authorTranslatable(QObject::tr("Auteur"));
	QString titleTranslatable(QObject::tr("Titre"));
	QString folioTranslatable(QObject::tr("Folio"));
	QString machineTranslatable(QObject::tr("Installation"));
	QString locTranslatable(QObject::tr("Localisation"));
	QString indexrevTranslatable(QObject::tr("Rev"));
	QString dateTranslatable(QObject::tr("Date"));

	qp->save();
	qp->setFont(QETApp::diagramTextsFont(13));
	fillRow(qp, row_RectF, authorTranslatable, titleTranslatable, folioTranslatable, machineTranslatable, locTranslatable, indexrevTranslatable, dateTranslatable);
	qp->restore();
}
