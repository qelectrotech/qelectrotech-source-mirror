/*
	Copyright 2006-2020 The QElectroTech Team
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
