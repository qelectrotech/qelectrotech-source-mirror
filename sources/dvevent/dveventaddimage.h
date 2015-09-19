/*
	Copyright 2006-2015 The QElectroTech Team
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
#ifndef DVEVENTADDIMAGE_H
#define DVEVENTADDIMAGE_H

#include "dveventinterface.h"

class DiagramImageItem;

/**
 * @brief The DVEventAddImage class
 * This dv event, open an image and add it to diagram view.
 */
class DVEventAddImage : public DVEventInterface
{
	Q_OBJECT

	public:
		DVEventAddImage(DiagramView *dv);
		virtual ~DVEventAddImage();

		virtual bool mousePressEvent       (QMouseEvent *event);
		virtual bool mouseMoveEvent        (QMouseEvent *event);
		virtual bool mouseDoubleClickEvent (QMouseEvent *event);
		virtual bool wheelEvent            (QWheelEvent *event);

		bool isNull () const;

	private:
		void openDialog();

		DiagramImageItem *m_image;
		bool m_is_added;

};

#endif // DVEVENTADDIMAGE_H
