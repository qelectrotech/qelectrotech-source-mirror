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

#include "qetsettings.h"
#include <QSettings>
#include <QVariant>

namespace QetSettings
{
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
	/**
	* @brief setHdpiScaleFactorRoundingPolicy
	* Write the value of HdpiScaleFactorRoundingPolicy in
	* QElectroTech settings
	* the value is in form of a string.
	* @a policy can be : Round, Ceil, Floor, RoundPreferFloor, PassThrough
	* In case of wrong policy, PassThrough is use as default value.
	* The value is stored with key : hdpi_scale_factor_rounding_policy
	* @sa Qt::HighDpiScaleFactorRoundingPolicy
	* @param policy
	*/
	void setHdpiScaleFactorRoundingPolicy(const QString &policy_str)
	{
		auto policy = QString("PassThrough");
		if (policy_str == QLatin1String("Round")) {
			policy = "Round";
		} else if (policy_str == QLatin1String("Ceil")) {
			policy = "Ceil";
		} else if (policy_str == QLatin1String("Floor")) {
			policy = "Floor";
		} else if (policy_str == QLatin1String("RoundPreferFloor")) {
			policy = "RoundPreferFloor";
		}
		QSettings settings;
		settings.setValue("hdpi_scale_factor_rounding_policy", policy);
	}

	/**
	* @brief setHdpiScaleFactorRoundingPolicy
	* Overload function
	* @sa void setHdpiScaleFactorRoundingPolicy(const QString &policy_str)
	* @param policy
	*/
	void setHdpiScaleFactorRoundingPolicy(Qt::HighDpiScaleFactorRoundingPolicy policy)
	{
		QString policy_str;
		switch (policy) {
			case Qt::HighDpiScaleFactorRoundingPolicy::Round:
				policy_str = "Round";
				break;
			case Qt::HighDpiScaleFactorRoundingPolicy::Ceil:
				policy_str = "Ceil";
				break;
			case Qt::HighDpiScaleFactorRoundingPolicy::Floor:
				policy_str = "Floor";
				break;
			case Qt::HighDpiScaleFactorRoundingPolicy::RoundPreferFloor:
				policy_str = "RoundPreferFloor";
				break;
			default:
				policy_str = "PassThrough";
				break;
		}

		QSettings settings;
		settings.setValue("hdpi_scale_factor_rounding_policy", policy_str);
	}

	/**
	* @brief hdpiScaleFactorRoundingPolicy
	* @param default_policy
	* @return the hdpiScaleFactorRoundingPolicy value stored in current settings.
	* @sa setHdpiScaleFactorRoundingPolicy
	*/
	Qt::HighDpiScaleFactorRoundingPolicy hdpiScaleFactorRoundingPolicy(Qt::HighDpiScaleFactorRoundingPolicy default_policy)
	{
		QSettings settings;
		auto value_ = settings.value("hdpi_scale_factor_rounding_policy");

		if (value_ == QLatin1String("Round")) {
			return Qt::HighDpiScaleFactorRoundingPolicy::Round;
		} else if (value_ == QLatin1String("Ceil")) {
			return Qt::HighDpiScaleFactorRoundingPolicy::Ceil;
		} else if (value_ == QLatin1String("Floor")) {
			return Qt::HighDpiScaleFactorRoundingPolicy::Floor;
		} else if (value_ == QLatin1String("RoundPreferFloor")) {
			return Qt::HighDpiScaleFactorRoundingPolicy::RoundPreferFloor;
		} else if (value_ == QLatin1String("PassThrough")) {
			return Qt::HighDpiScaleFactorRoundingPolicy::PassThrough;
		} else {
			return default_policy;
		}
	}
#endif
}
