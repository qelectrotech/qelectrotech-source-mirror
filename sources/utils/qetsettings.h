/*
	Copyright 2006-2025 The QElectroTech Team
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
#ifndef QETSETTINGS_H
#define QETSETTINGS_H

#include <Qt>

/**
 *Interface to use QSettings in QElectroTech
 *Prefer use this namespace instead of directly QSettings
 *It's notably useful when some QSettings value are used in
 *different part of QElectroTech source code
 */
namespace QetSettings
{
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
	void setHdpiScaleFactorRoundingPolicy(const QString &policy_str);
	void setHdpiScaleFactorRoundingPolicy(Qt::HighDpiScaleFactorRoundingPolicy policy);
	Qt::HighDpiScaleFactorRoundingPolicy hdpiScaleFactorRoundingPolicy(
			Qt::HighDpiScaleFactorRoundingPolicy default_policy = Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);
#endif
}

#endif // QETSETTINGS_H
