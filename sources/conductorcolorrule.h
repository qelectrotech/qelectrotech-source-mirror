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
#ifndef CONDUCTORCOLORRULE_H
#define CONDUCTORCOLORRULE_H

#include <QColor>
#include <QDomElement>
#include <QHash>
#include <QString>

/**
	@brief The ConductorColorRule class
	Implements discussion #606: the color a conductor should be drawn in
	when its auto-numbering-resolved label text exactly matches a
	configured entry -- "L1" gets colored brown, "PE" gets colored
	green/yellow, etc. One instance is one entry of the project-level
	QHash<QString, ConductorColorRule> that mapping lives in (see
	QETProject::conductorColorRules()); the QString key is the resolved
	label text this rule applies to, not stored on the rule itself.

	Deliberately just color + a second stripe color + whether the second
	color is used at all -- the three ConductorProperties fields
	(color/m_color_2/m_bicolor) that already exist for exactly this
	purpose (conductor.cpp:545 draws the m_color_2 stripe only when
	m_bicolor is set). Nothing new to teach the drawing code.
*/
class ConductorColorRule
{
	public:
		ConductorColorRule() = default;
		ConductorColorRule(const QColor &c, bool bicolor, const QColor &c2 = QColor(Qt::black)) :
			color(c), color_2(c2), m_bicolor(bicolor) {}

		QColor color   = QColor(Qt::black);
		QColor color_2 = QColor(Qt::black);
			/// Whether color_2 is drawn as a second stripe alongside color.
			/// Mirrors ConductorProperties::m_bicolor exactly -- a
			/// single-color rule (e.g. "L1") leaves this false and color_2
			/// unused; a dual-color rule (e.g. "PE") sets it true.
		bool m_bicolor = false;

		QDomElement toXml(QDomDocument &xml_document) const;
		void fromXml(const QDomElement &xml_element);

		bool operator==(const ConductorColorRule &other) const;
		bool operator!=(const ConductorColorRule &other) const;

			/// One common phase/neutral/earth wiring color convention
			/// (L1 brown, L2 black, L3 grey, N blue, PE green/yellow) as a
			/// starting point for a new project -- not a claim of
			/// regulatory correctness for any specific country or
			/// standard, and every entry is user-editable and removable
			/// once loaded into a project's rule set.
		static QHash<QString, ConductorColorRule> defaultRules();
};

#endif // CONDUCTORCOLORRULE_H
