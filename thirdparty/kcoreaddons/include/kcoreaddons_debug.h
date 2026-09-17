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

/**
 * @file kcoreaddons_debug.h
 * @brief Minimal Qt logging category for kcoreaddons.
 * Defines "kf.kcoreaddons" category controllable via QT_LOGGING_RULES.
 */

#include <QLoggingCategory>

QLoggingCategory kf_kcoreaddons_category("kf.kcoreaddons");

#define KCOREADDONS_DEBUG kf_kcoreaddons_category
