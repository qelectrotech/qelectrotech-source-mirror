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
#ifndef ELEMENTVIEWGRID_H
#define ELEMENTVIEWGRID_H

#include <QtGlobal>

/**
	The symbol editor's grid at a given zoom: the step parts snap to, and
	how the grid is drawn. The step gets finer as the view zooms in, so a
	small detail can be placed precisely.

	Below 100 % the grid is not drawn (its dots would merge into a grey
	wash), but parts still snap every 10 units, as at 100 %. Snapping to 1
	unit there left everything dragged while zoomed out off the grid, with
	nothing on screen to show it (bugtracker #112).
*/
struct ElementViewGrid
{
	int step = 10;
	bool draw_grid = true;
	bool draw_cross = false;

	static ElementViewGrid forZoom(qreal zoom_factor)
	{
		ElementViewGrid grid;
		if (zoom_factor < 1.0) {
			grid.draw_grid = false;
		} else if (zoom_factor < 4.0) {
			grid.step = 10;
		} else if (zoom_factor < 8.0) {
			grid.step = 5;
			grid.draw_cross = true;
		} else if (zoom_factor < 10.0) {
			grid.step = 2;
			grid.draw_cross = true;
		} else {
			grid.step = 1;
			grid.draw_cross = true;
		}
		return grid;
	}
};

#endif // ELEMENTVIEWGRID_H
