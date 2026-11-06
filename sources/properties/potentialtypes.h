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
			/// How a conductor gets the type it is drawn from. Off: never.
			/// PerWire: the type chosen on the conductor, spread over its
			/// potential. AtSource: the type set where the potential starts,
			/// inherited through the devices; needs the internal connections
			/// of #1500, so until then it draws as PerWire and nothing offers
			/// it. A mode this QET does not know reads as Off.
		enum class Mode {Off, PerWire, AtSource};
		Mode mode() const {return m_mode;}
			/// Choosing a mode drops a mode name kept from a newer QET
		void setMode(Mode mode) {m_mode = mode; m_unknown_mode.clear();}
		static QString modeName(Mode mode);
		static Mode modeFromName(const QString &name, bool *known = nullptr);

		bool isEnabled() const {return m_mode != Mode::Off;}
		void setEnabled(bool enabled) {m_mode = enabled ? Mode::PerWire : Mode::Off;}

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
		Mode m_mode = Mode::Off;
			/// The mode name of a file that names one this QET does not
			/// know: read as Off, written back as it was
		QString m_unknown_mode;
		QList<PotentialType> m_types = defaultTypes();
};

#endif // POTENTIALTYPES_H
