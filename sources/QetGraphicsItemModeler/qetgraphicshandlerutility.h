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
#ifndef QETGRAPHICSHANDLERUTILITY_H
#define QETGRAPHICSHANDLERUTILITY_H

#include <QPixmap>

/**
 * @brief The QetGraphicsHandlerUtility class
 * This class provide some static methods to create and use handler for
 * modify graphics shape like line rectangle etc...
 */
class QetGraphicsHandlerUtility
{
	public:
		static QPixmap pixmapHandler();
		static QPointF posForHandler(const QPointF &point);
		static bool pointIsInHandler (const QPointF &point, const QPointF &key_point);
		static int pointIsHoverHandler (const QPointF &point, const QVector<QPointF> &vector);
		static QVector<QRectF> handlerRect (const QVector<QPointF> &vector);
};

#endif // QETGRAPHICSHANDLERUTILITY_H
