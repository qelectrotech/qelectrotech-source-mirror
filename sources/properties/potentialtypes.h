// SPDX-License-Identifier: GPL-2.0-or-later
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
#ifndef POTENTIALTYPES_H
#define POTENTIALTYPES_H

#include <QColor>
#include <QDomElement>
#include <QList>
#include <QString>
#include <Qt>

/**
	@brief The PotentialType struct
	One kind of potential (L, N, PE, 24V...) and how the conductors on it
	are drawn.
*/
struct PotentialType
{
	QString name;
	QColor color = Qt::black;
	Qt::PenStyle style = Qt::SolidLine;
	qreal width = 1.0;

	bool operator==(const PotentialType &other) const;
	bool operator!=(const PotentialType &other) const {return !(*this == other);}
};

/**
	@brief The PotentialTypes class
	The potential types of a project, and whether the project draws its
	conductors from them. Off by default: a project saved before this
	existed, or one that never turns it on, is drawn from the style stored
	on each conductor as always.
	A conductor names its type in ConductorProperties::m_potential_type;
	this class says what that name looks like.
*/
class PotentialTypes
{
	public:
		bool isEnabled() const {return m_enabled;}
		void setEnabled(bool enabled) {m_enabled = enabled;}

		const QList<PotentialType> &types() const {return m_types;}
		void setTypes(const QList<PotentialType> &types) {m_types = types;}

		bool find(const QString &name, PotentialType *found = nullptr) const;

		static QList<PotentialType> defaultTypes();

		QDomElement toXml(QDomDocument &document) const;
		static PotentialTypes fromXml(const QDomElement &element);

		bool isDefault() const;
		bool operator==(const PotentialTypes &other) const;
		bool operator!=(const PotentialTypes &other) const {return !(*this == other);}

	private:
		bool m_enabled = false;
		QList<PotentialType> m_types = defaultTypes();
};

#endif // POTENTIALTYPES_H
