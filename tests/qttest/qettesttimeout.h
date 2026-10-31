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
#ifndef QETTESTTIMEOUT_H
#define QETTESTTIMEOUT_H

#include <QtGlobal>

namespace QetTest {

/**
	@brief timeout
	A time limit of a test, multiplied by QET_TEST_TIMEOUT_SCALE when it is
	set: CMake sets it for the builds that run QElectroTech much slower
	(MSVC's Debug libraries), so that a limit chosen on a GCC build still
	holds there. Unset, the limit is unchanged.
	@param msecs : the limit, in milliseconds
	@return the limit to use, in milliseconds
*/
inline int timeout(int msecs)
{
	bool ok = false;
	const int scale = qEnvironmentVariableIntValue("QET_TEST_TIMEOUT_SCALE", &ok);
	return ok && scale > 1 ? msecs * scale : msecs;
}

} // namespace QetTest

#endif // QETTESTTIMEOUT_H
