#include "diagramfoliolist.h"
#include <QPainter>


void DiagramFolioList::drawBackground(QPainter *p, const QRectF &r)
{
	p -> save();

	// desactive tout antialiasing, sauf pour le texte
	p -> setRenderHint(QPainter::Antialiasing, false);
	p -> setRenderHint(QPainter::TextAntialiasing, true);
	p -> setRenderHint(QPainter::SmoothPixmapTransform, false);

	// dessine un fond blanc
	p -> setPen(Qt::NoPen);
	//set brush color to present background color.
	p -> setBrush(Diagram::background_color);
	p -> drawRect(r);

	p -> setPen(Qt::black);
	qreal width  = border_and_titleblock.columnsTotalWidth();
	qreal height = border_and_titleblock.rowsTotalHeight();

	QList<Diagram *> diagram_list = project() -> diagrams();

	//top left corner of drawable area
	qreal x0 = border_and_titleblock.rowsHeaderWidth();
	qreal y0 = border_and_titleblock.columnsHeaderHeight();
	qreal drawings_quantity = diagram_list.size();
	qreal rowHeight = height / (drawings_quantity+1) * 0.8;
	rowHeight = (rowHeight > height*0.05) ? height*0.05 : rowHeight;
	QRectF row_RectF(x0 + width*.1, y0 + height*.05, width*0.8, rowHeight);

	fillRow(p, row_RectF, "Author", "Title", "Folio", "Date");
	foreach (Diagram *diagram, diagram_list) {
		y0 += rowHeight;
		QRectF row_rect(x0 + width*.1, y0 + height*.05, width*0.8, rowHeight);
		fillRow(p, row_rect, diagram -> border_and_titleblock.author(), diagram -> border_and_titleblock.title(),
				 diagram -> border_and_titleblock.folio(), diagram -> border_and_titleblock.date().toString());
	}
	p -> setPen(Qt::NoPen);
	border_and_titleblock.draw(p, margin, margin);
	p -> restore();
}

void DiagramFolioList::fillRow(QPainter *qp, const QRectF &row_rect, QString author, QString title,
							   QString folio, QString date)
{
	qp -> drawRect(row_rect);
	qreal x = row_rect.topLeft().x();
	qreal y = row_rect.topLeft().y();
	qreal column_width = row_rect.width() / 4;

	qp -> drawText(QRectF(x, y, column_width, row_rect.height()), Qt::AlignCenter, folio);
	qp -> drawText(QRectF(x + column_width, y, column_width, row_rect.height()), Qt::AlignCenter, title);
	qp -> drawText(QRectF(x + 2*column_width, y, column_width, row_rect.height()), Qt::AlignCenter, author);
	qp -> drawText(QRectF(x + 3*column_width, y, column_width, row_rect.height()), Qt::AlignCenter, date);

	for (int i = 1; i <= 3; i++ )
		qp -> drawLine(x + i*column_width, y, x + i*column_width, y + row_rect.height());
}
