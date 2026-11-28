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
#ifndef TERMINALLABEL_H
#define TERMINALLABEL_H

#include <QColor>
#include <QFont>
#include <QPointF>
#include <QString>

#include <QtXml>

class TerminalData;

/**
	@brief The TerminalLabel class
	What a terminal shows: its name, whether the name is drawn, and how the
	name is drawn. The same values TerminalData holds, gathered so one
	placed copy of a symbol can give its terminals values of its own
	(@see Element::terminalLabels()) without the definition being touched.
*/
class TerminalLabel
{
	public:
		TerminalLabel() = default;

		static TerminalLabel fromData(const TerminalData &data);
		void applyTo(TerminalData &data) const;

		bool operator==(const TerminalLabel &other) const;
		bool operator!=(const TerminalLabel &other) const {return !(*this == other);}

		QDomElement toXml(QDomDocument &document) const;
		bool fromXml(const QDomElement &xml_element);

	/// Name of the terminal, as the drawing gives it
	QString m_name;
	/// Whether the name is drawn at all
	bool m_show_name = false;
	/// Position of the name, relative to the terminal
	QPointF m_label_pos;
	/// Font the name is drawn with
	QFont m_label_font;
	/// Rotation of the name, in degrees
	qreal m_label_rotation = 0.0;
	/// Horizontal alignment of the name
	Qt::Alignment m_label_halignment = Qt::AlignHCenter;
	/// Vertical alignment of the name
	Qt::Alignment m_label_valignment = Qt::AlignVCenter;
	/// Whether a frame is drawn around the name
	bool m_label_frame = false;
	/// Colour the name is drawn in
	QColor m_label_color = Qt::black;
};

#endif // TERMINALLABEL_H
