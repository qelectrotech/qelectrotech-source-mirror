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
#include "cellruler.h"

#include "bordercelllabels.h"
#include "diagram.h"
#include "diagramview.h"

#include <QPainter>
#include <QSettings>

/**
	@brief CellRuler::CellRuler
	@param orientation : Qt::Horizontal for the column numbers along the
	top, Qt::Vertical for the row letters along the left
	@param view : the view this ruler is placed on
*/
CellRuler::CellRuler(Qt::Orientation orientation, DiagramView *view) :
	QWidget(view),
	m_orientation(orientation),
	m_view(view)
{
	setAttribute(Qt::WA_OpaquePaintEvent);
	hide();
}

/**
	@brief CellRuler::thickness
	@return the height of the top ruler, which is also the width of the
	side ruler, so the corner they share is square. Constant whatever the
	zoom.
*/
int CellRuler::thickness() const
{
	const QFontMetrics metrics = fontMetrics();
	return qMax(metrics.height(), metrics.horizontalAdvance(QStringLiteral("WW"))) + 6;
}

/**
	@brief CellRuler::setLeadingSpace
	@param space : pixels left empty before the first pixel of the
	viewport, along this ruler
*/
void CellRuler::setLeadingSpace(int space)
{
	m_leading_space = space;
	update();
}

/**
	@brief CellRuler::paintEvent
	Draw a cell for each column (or row) of the folio border, at the
	position the view shows it. When the cells get too small for their
	labels, only one label every 2, 5, 10... cells is written.
*/
void CellRuler::paintEvent(QPaintEvent *event)
{
	Q_UNUSED(event)

	QPainter painter(this);
	painter.fillRect(rect(), palette().color(QPalette::Button));

	const bool horizontal = m_orientation == Qt::Horizontal;
	const int length = horizontal ? width() : height();
	const int depth = horizontal ? height() : width();

		//Separate the ruler from the drawing
	painter.setPen(palette().color(QPalette::Dark));
	if (horizontal) {
		painter.drawLine(0, depth - 1, length, depth - 1);
	} else {
		painter.drawLine(depth - 1, 0, depth - 1, length);
	}

	Diagram *diagram = m_view->diagram();
	if (!diagram) {
		return;
	}
	const BorderTitleBlock &border = diagram->border_and_titleblock;
	const QRectF inside = border.insideBorderRect();
	const QTransform transform = m_view->viewportTransform();

	const int count = horizontal ? border.columnsCount() : border.rowsCount();
	const qreal cell_size = horizontal ? border.columnsWidth() : border.rowsHeight();
	const qreal first = horizontal ? inside.left() : inside.top();
	const qreal scale = horizontal ? transform.m11() : transform.m22();
	const qreal offset = (horizontal ? transform.dx() : transform.dy()) + m_leading_space;
	const qreal cell_pixels = cell_size * scale;
	if (count < 1 || cell_pixels <= 0) {
		return;
	}

	const bool columns_start_at_zero =
			QSettings().value("border-columns_0", true).toBool();
	auto label = [&](int index) {
		return horizontal ? BorderCellLabels::columnLabel(index, columns_start_at_zero)
				  : BorderCellLabels::rowLabel(index);
	};

		//Room one label needs along the ruler, and the smallest step
		//between written labels that gives it that room
	const QFontMetrics metrics = fontMetrics();
	const int label_room = horizontal
			? metrics.horizontalAdvance(label(count)) + 6
			: metrics.height() + 2;
	int step = 1;
	for (int candidate : {1, 2, 5, 10, 20, 50, 100, 200, 500}) {
		step = candidate;
		if (candidate * cell_pixels >= label_room) {
			break;
		}
	}

	for (int i = 1 ; i <= count ; ++i) {
		const qreal start = offset + (first + (i - 1) * cell_size) * scale;
		const qreal end = start + cell_pixels;
		if (end < m_leading_space || start > length) {
			continue;
		}

			//Cell edges, drawn only when the cells are wide enough
			//for them to read as cells rather than as a hatching
		painter.setPen(palette().color(QPalette::Dark));
		if (cell_pixels >= 4) {
			if (horizontal) {
				painter.drawLine(QPointF(start, 0), QPointF(start, depth - 1));
				if (i == count) painter.drawLine(QPointF(end, 0), QPointF(end, depth - 1));
			} else {
				painter.drawLine(QPointF(0, start), QPointF(depth - 1, start));
				if (i == count) painter.drawLine(QPointF(0, end), QPointF(depth - 1, end));
			}
		}

			//Written labels are the ones a multiple of step: 0, 5, 10...
			//for the columns, A, F, K... for the rows
		const int position = (horizontal && !columns_start_at_zero) ? i : i - 1;
		if (position % step != 0) {
			continue;
		}

		painter.setPen(palette().color(QPalette::ButtonText));
		const qreal centre = (start + end) / 2;
		const QRectF text_rect = horizontal
				? QRectF(centre - label_room / 2.0, 0, label_room, depth - 1)
				: QRectF(0, centre - label_room / 2.0, depth - 1, label_room);
		painter.drawText(text_rect, Qt::AlignCenter | Qt::TextDontClip, label(i));
	}

		//Keep the corner empty: the other ruler's labels do not belong there
	if (m_leading_space > 0) {
		painter.fillRect(horizontal ? QRect(0, 0, m_leading_space, depth - 1)
					    : QRect(0, 0, depth - 1, m_leading_space),
				 palette().color(QPalette::Button));
	}
}
