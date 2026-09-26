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
#ifndef CELLRULER_H
#define CELLRULER_H

#include <QWidget>

class DiagramView;

/**
	@brief The CellRuler class
	A bar along the top or the left edge of a DiagramView that repeats the
	column numbers or the row letters of the folio border, aligned with the
	cells at any zoom, so they stay in sight however far the view is
	scrolled. It sits in the view's margins, outside of the scene: printing
	and exporting are unaffected.
*/
class CellRuler : public QWidget
{
		Q_OBJECT

	public:
		CellRuler(Qt::Orientation orientation, DiagramView *view);

		int thickness() const;
		void setLeadingSpace(int space);

	protected:
		void paintEvent(QPaintEvent *event) override;

	private:
		QColor background() const;

		Qt::Orientation m_orientation;
		DiagramView *m_view;
		/// Pixels before the viewport starts, left empty: the corner the
		/// side ruler fills when both rulers are shown.
		int m_leading_space = 0;
};

#endif // CELLRULER_H
