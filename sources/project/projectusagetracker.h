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
#ifndef PROJECTUSAGETRACKER_H
#define PROJECTUSAGETRACKER_H

#include <QElapsedTimer>

class QDomElement;

/**
	@brief The ProjectUsageTracker class
	Accumulates how long a project has been the active tab, purely locally:
	the value lives only in the project's own file (a <usage> element next
	to <properties>) and is never transmitted anywhere.

	The owning code (QETDiagramEditor) is expected to call setActive(true)
	when this project's tab becomes the current one, and setActive(false)
	when it stops being current (tab switch, project closed, ...).
*/
class ProjectUsageTracker
{
	public:
		ProjectUsageTracker() = default;

		qint64 secondsSpent() const;
		void resetSecondsSpent();

		bool isEnabled() const {return m_enabled;}
		void setEnabled(bool enabled);

		void setActive(bool active);

		void toXml(QDomElement &parent_element) const;
		void fromXml(const QDomElement &parent_element);

	private:
		qint64 m_seconds_spent = 0;
		bool m_enabled = true;
		bool m_active = false;
		QElapsedTimer m_elapsed_timer;
};

#endif // PROJECTUSAGETRACKER_H
