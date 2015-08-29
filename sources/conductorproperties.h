/*
	Copyright 2006-2015 The QElectroTech Team
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
#ifndef CONDUCTOR_PROPERTIES_H
#define CONDUCTOR_PROPERTIES_H

#include "qet.h"
#include <QColor>

class QPainter;

/**
	This class represents the properties of a singleline conductor.
*/
class SingleLineProperties {
	public:
	SingleLineProperties();
	virtual ~SingleLineProperties();
	
	void setPhasesCount(int);
	unsigned short int phasesCount();
	bool isPen() const;
	void draw(QPainter *, QET::ConductorSegmentType, const QRectF &);
	void toXml(QDomElement &) const;
	void fromXml(QDomElement &);
	void toSettings(QSettings &, const QString & = QString()) const;
	void fromSettings(QSettings &, const QString & = QString());
	
	/// Whether the singleline conductor should display the ground symbol
	bool hasGround;
	/// Whether the singleline conductor should display the neutral symbol
	bool hasNeutral;
	/// Protective Earth Neutral: visually merge neutral and ground
	bool is_pen;
	
	int operator==(const SingleLineProperties &) const;
	int operator!=(const SingleLineProperties &) const;
	
	private:
	unsigned short int phases;
	void drawGround (QPainter *, QET::ConductorSegmentType, QPointF, qreal);
	void drawNeutral(QPainter *, QET::ConductorSegmentType, QPointF, qreal);
	void drawPen(QPainter *, QET::ConductorSegmentType, QPointF, qreal);
};

/**
	This class represents the functional properties of a particular conductor,
	i.e. properties other than path and terminals.
*/
class ConductorProperties
{
	public:
		ConductorProperties();
		virtual ~ConductorProperties();
	
			/**
			 * @brief The ConductorType enum Represents the kind of a particular conductor:
			 * Single: singleline symbols, no text input
			 * Multi: text input, no symbol
			 */
		enum ConductorType { Single, Multi };


			//Attributes
		ConductorType type;
		QColor color;
		QString text,
				m_function,
				m_tension_protocol;
		int text_size;
		double verti_rotate_text;
		double horiz_rotate_text;
		bool m_show_text;
		bool m_one_text_per_folio;
		Qt::PenStyle style;
		SingleLineProperties singleLineProperties;
	
			// methods
		void toXml(QDomElement &) const;
		void fromXml(QDomElement &);
		void toSettings(QSettings &, const QString & = QString()) const;
		void fromSettings(QSettings &, const QString & = QString());
		static QString typeToString(ConductorType);

		static ConductorProperties defaultProperties();
	
			// operators
		bool operator==(const ConductorProperties &) const;
		bool operator!=(const ConductorProperties &) const;
	
	private:
		void readStyle(const QString &);
		QString writeStyle() const;
};

Q_DECLARE_METATYPE(ConductorProperties)

#endif
