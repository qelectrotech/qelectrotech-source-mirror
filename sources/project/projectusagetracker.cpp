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
#include "projectusagetracker.h"

#include <QDomElement>

/**
	@brief ProjectUsageTracker::secondsSpent
	@return the total accumulated time, in seconds, including the time
	elapsed since the tracker was last made active (if it currently is).
*/
qint64 ProjectUsageTracker::secondsSpent() const
{
	qint64 total = m_seconds_spent;
	if (m_elapsed_timer.isValid()) {
		total += m_elapsed_timer.elapsed() / 1000;
	}
	return total;
}

/**
	@brief ProjectUsageTracker::resetSecondsSpent
	Zero the accumulated time. If the tracker is currently timing, it keeps
	timing from this point on.
*/
void ProjectUsageTracker::resetSecondsSpent()
{
	m_seconds_spent = 0;
	if (m_elapsed_timer.isValid()) {
		m_elapsed_timer.restart();
	}
}

/**
	@brief ProjectUsageTracker::setEnabled
	Enable or disable time tracking for this project. Disabling while
	currently timing flushes the elapsed time first; re-enabling while
	the project is the active tab resumes timing immediately.
	@param enabled
*/
void ProjectUsageTracker::setEnabled(bool enabled)
{
	if (m_enabled == enabled) {
		return;
	}

	if (!enabled && m_elapsed_timer.isValid()) {
		m_seconds_spent += m_elapsed_timer.elapsed() / 1000;
		m_elapsed_timer.invalidate();
	} else if (enabled && m_active) {
		m_elapsed_timer.start();
	}
	m_enabled = enabled;
}

/**
	@brief ProjectUsageTracker::setActive
	Tell the tracker whether this project's tab is currently the active
	one. No-op if tracking is disabled or the state doesn't change.
	@param active
*/
void ProjectUsageTracker::setActive(bool active)
{
	if (m_active == active) {
		return;
	}

	if (!active && m_elapsed_timer.isValid()) {
		m_seconds_spent += m_elapsed_timer.elapsed() / 1000;
		m_elapsed_timer.invalidate();
	} else if (active && m_enabled) {
		m_elapsed_timer.start();
	}
	m_active = active;
}

/**
	@brief ProjectUsageTracker::toXml
	Append a <usage> child element to @p parent_element with the current
	accumulated time and enabled state.
	@param parent_element
*/
void ProjectUsageTracker::toXml(QDomElement &parent_element) const
{
	QDomElement usage_element =
			parent_element.ownerDocument().createElement(QStringLiteral("usage"));
	usage_element.setAttribute(QStringLiteral("time_spent"), QString::number(secondsSpent()));
	usage_element.setAttribute(QStringLiteral("enabled"), m_enabled ? QStringLiteral("true") : QStringLiteral("false"));
	parent_element.appendChild(usage_element);
}

/**
	@brief ProjectUsageTracker::fromXml
	Read the <usage> child element of @p parent_element, if any.
	@param parent_element
*/
void ProjectUsageTracker::fromXml(const QDomElement &parent_element)
{
	const QDomElement usage_element = parent_element.firstChildElement(QStringLiteral("usage"));
	if (usage_element.isNull()) {
		return;
	}

	m_seconds_spent = usage_element.attribute(QStringLiteral("time_spent"), QStringLiteral("0")).toLongLong();
	m_enabled = usage_element.attribute(QStringLiteral("enabled"), QStringLiteral("true")) != QStringLiteral("false");
}
