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
#ifndef TEXTGRID_H
#define TEXTGRID_H

#include <QList>
#include <QPointF>
#include <QString>
#include <QtMath>

/**
	The text grid: the step texts snap to when dragged with the mouse,
	a fraction of the folio grid. A divisor of 1 is the folio grid itself,
	0 means no grid. Because every step divides the folio grid, a text
	snapped to it still lines up with every element and with the texts
	of other elements.
*/
namespace TextGrid
{
		/// The choices offered in the menu and the preferences, 0 first.
	inline const QList<qreal> divisors{0, 1, 2, 5, 10};

		/// QSettings key holding the divisor.
	inline const QString settings_key{QStringLiteral("diagrameditor/text_grid_divisor")};

		/// "1:5" for 5. Not meaningful for 0.
	inline QString ratioLabel(qreal divisor) {
		return QStringLiteral("1:") + QString::number(divisor);
	}

	/**
		@return p snapped to a grid of x_grid / divisor by y_grid / divisor,
		or rounded to the nearest pixel when divisor is 0 or less.
	*/
	inline QPointF snap(const QPointF &p, int x_grid, int y_grid, qreal divisor)
	{
		if (divisor <= 0 || x_grid <= 0 || y_grid <= 0)
			return QPointF(qRound(p.x()), qRound(p.y()));

		const qreal x_step = x_grid / divisor;
		const qreal y_step = y_grid / divisor;
		return QPointF(qRound(p.x() / x_step) * x_step,
					   qRound(p.y() / y_step) * y_step);
	}
}

#endif // TEXTGRID_H
