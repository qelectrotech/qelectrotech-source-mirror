/*
	Copyright 2006-2012 Xavier Guerrin
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
#include <QtGui>
#include <QtXml>
/**
	Cette classe represente les proprietes specifiques a un conducteur unifilaire
*/
class SingleLineProperties {
	public:
	SingleLineProperties();
	virtual ~SingleLineProperties();
	
	void setPhasesCount(int);
	unsigned short int phasesCount();
	void draw(QPainter *, QET::ConductorSegmentType, const QRectF &);
	void toXml(QDomElement &) const;
	void fromXml(QDomElement &);
	void toSettings(QSettings &, const QString & = QString()) const;
	void fromSettings(QSettings &, const QString & = QString());
	
	/// indique si le conducteur unifilaire doit afficher le symbole terre
	bool hasGround;
	/// indique si le conducteur unifilaire doit afficher le symbole neutre
	bool hasNeutral;
	
	int operator==(const SingleLineProperties &) const;
	int operator!=(const SingleLineProperties &) const;
	
	private:
	unsigned short int phases;
	void drawGround (QPainter *, QET::ConductorSegmentType, QPointF, qreal);
	void drawNeutral(QPainter *, QET::ConductorSegmentType, QPointF, qreal);
};

/**
	Cette classe represente les proprietes specifiques a un conducteur,
	en dehors de ses bornes et de son trajet.
*/
class ConductorProperties {
	// constructeurs, destructeur
	public:
	ConductorProperties();
	virtual ~ConductorProperties();
	
	/**
		Represente le type d'un conducteur :
		 * Simple : ni symbole ni champ de texte
		 * Single : symboles unifilaires, pas de champ de texte
		 * Multi : champ de texte, pas de symbole
	*/
	enum ConductorType { Simple, Single, Multi };
	
	// attributs
	/// type du conducteur
	ConductorType type;
	/// couleur du conducteur
	QColor color;
	/// texte affiche si le conducteur est multifilaire
	QString text;
	/// style du conducteur (Qt::SolidLine ou Qt::DashLine)
	Qt::PenStyle style;
	
	/// proprietes si le conducteur est unifilaire
	SingleLineProperties singleLineProperties;
	
	// methodes
	void toXml(QDomElement &) const;
	void fromXml(QDomElement &);
	void toSettings(QSettings &, const QString & = QString()) const;
	void fromSettings(QSettings &, const QString & = QString());
	static QString typeToString(ConductorType);
	
	// operateurs
	int operator==(const ConductorProperties &);
	int operator!=(const ConductorProperties &);
	
	private:
	void readStyle(const QString &);
	QString writeStyle() const;
};
#endif
