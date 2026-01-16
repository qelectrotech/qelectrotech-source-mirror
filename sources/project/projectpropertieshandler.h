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
#ifndef PROJECTPROPERTIESHANDLER_H
#define PROJECTPROPERTIESHANDLER_H

#include <QPointer>

#include "../TerminalStrip/GraphicsItem/properties/terminalstriplayoutshandler.h"

class QETProject;

/**
 * @brief The ProjectPropertiesHandler class
 * A central class that handles, keeps and provides all utilities
 * to easily manage all kind of properties used in a project.
 *
 * This is a new class since QElectroTech 0.9
 * by consequence it is small and you can find a lot of properties (made before qet 0.9)
 * everywhere in the code.
 * All new properties should be managed by this class
 * (of course if it makes sense to be managed by this class).
 * Older properties which are not managed by this class but should be,
 * will be managed in future.
 */
class ProjectPropertiesHandler
{
	public:
		ProjectPropertiesHandler(QETProject *project);

		TerminalStripLayoutsHandler& terminalStripLayoutHandler();

	private:
		QPointer<QETProject> m_project;

		TerminalStripLayoutsHandler m_terminal_strip_layout_handler;
};

#endif // PROJECTPROPERTIESHANDLER_H
