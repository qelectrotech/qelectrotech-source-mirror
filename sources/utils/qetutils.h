/*
	Copyright 2006-2025 The QElectroTech Team
	This file is part of QElectroTech.

	QElectroTech is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 2 of the License, or
	(at your option) any later version.

	QElectroTech is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with QElectroTech. If not, see <http://www.gnu.org/licenses/>.
*/
#ifndef QETUTILS_H
#define QETUTILS_H

#include <QMargins>
#include <QWeakPointer>

class QGraphicsItem;

/**
	Provide some small utils function
*/
namespace QETUtils
{
	QString marginsToString(const QMargins &margins);
	QMargins marginsFromString(const QString &string);
	qreal graphicsHandlerSize(QGraphicsItem *item);

	bool sortBeginIntString(const QString &str_a, const QString &str_b);

	template <typename T>
	QVector<QWeakPointer<T>> sharedVectorToWeak(const QVector<QSharedPointer<T>> &vector)
	{
		QVector<QWeakPointer<T>> return_vector;
		for (const auto shared : vector) {
			return_vector.append(shared.toWeakRef());
		}
		return return_vector;
	}


	template <typename T>
	QVector<QSharedPointer<T>> weakVectorToShared(const QVector<QWeakPointer<T>> &vector)
	{
		QVector<QSharedPointer<T>> return_vector;
		for (const auto weak : vector) {
			return_vector.append(weak.toStrongRef());
		}
		return return_vector;
	}
}

#endif // QETUTILS_H
