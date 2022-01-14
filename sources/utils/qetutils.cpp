/*
	Copyright 2006-2021 The QElectroTech Team
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
#include "qetutils.h"
#include <QString>
#include <QStringList>
#include <QGraphicsView>
#include "../qetapp.h"
#include "../qetdiagrameditor.h"

/**
	@brief QETUtils::marginsToString
	@param margins
	@return QString str "left;top;right;bottom"
*/
QString QETUtils::marginsToString(const QMargins &margins)
{
	QString str;
	str += QString::number(margins.left());
	str += ";";
	str += QString::number(margins.top());
	str += ";";
	str += QString::number(margins.right());
	str += ";";
	str += QString::number(margins.bottom());

	return str;
}

/**
	@brief QETUtils::marginsFromString
	@param string
	@return QMargins margins
*/
QMargins QETUtils::marginsFromString(const QString &string)
{
	QMargins margins;
	auto split = string.split(";");
	if (split.size() != 4)
		return  margins;

	margins.setLeft  (split.at(0).toInt());
	margins.setTop   (split.at(1).toInt());
	margins.setRight (split.at(2).toInt());
	margins.setBottom(split.at(3).toInt());

	return  margins;
}

/**
 * @brief QETUtils::graphicsHandlerSize
 * @param item
 * @return Return the handler size to use in the QGraphicsScene of @a item.
 * If size can't be found, return 10 by default.
 */
qreal QETUtils::graphicsHandlerSize(QGraphicsItem *item)
{
	if (const auto scene_ = item->scene())
	{
		if (!scene_->views().isEmpty())
		{
			if (const auto editor_ = QETApp::instance()->diagramEditorAncestorOf(scene_->views().at(0)))
			{
				const auto variant_ = editor_->property("graphics_handler_size");
					//If variant_ can't be converted to qreal, the returned qreal is 0.0
					//it's sufficient for us to check if value is set or not.
				if (const auto value_ =  variant_.toReal())
					return value_;
			}
		}
	}

		//Default value
	return 10;
}
