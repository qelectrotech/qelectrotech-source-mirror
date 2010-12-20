/*
	Copyright 2006-2010 Xavier Guerrin
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
#ifndef TITLEBLOCK_TEMPLATE_H
#define TITLEBLOCK_TEMPLATE_H
#include <QtXml>
#include <QtSvg>
#include "diagramcontext.h"
#include "titleblockcell.h"
#include "qet.h"

/**
	This struct is a simple container associating a length with its type.
	@see TitleBlockColumnLength 
*/
struct TitleBlockColDimension {
	TitleBlockColDimension(int v, QET::TitleBlockColumnLength t = QET::Absolute) {
		value = v;
		type = t;
	}
	QET::TitleBlockColumnLength type;
	int value;
};

/**
	This class represents an titleblock templ)ate for an electric diagram.
	It can read from an XML document the layout of the table that graphically
	represents the titleblock, and can produce a graphical rendering of it from a
	diagram context (object embedding the informations of the diagram we want to
	represent the titleblock.
*/
class TitleBlockTemplate : public QObject {
	Q_OBJECT
	
	// constructeurs, destructeur
	public:
	TitleBlockTemplate(QObject * = 0);
	virtual ~TitleBlockTemplate();
	private:
	TitleBlockTemplate(const TitleBlockTemplate &);
	
	// methodes
	public:
	bool loadFromXmlFile(const QString &);
	bool loadFromXmlElement(const QDomElement &);
	void setContext(const DiagramContext &);
	
	QList<int> columnsWidth(int) const;
	int height() const;
	
	void render(QPainter &, const DiagramContext &, int) const;
	QString toString() const;
	
	protected:
	bool loadLogos(const QDomElement &, bool = false);
	bool loadLogo(const QDomElement &);
	bool loadGrid(const QDomElement &);
	bool loadCells(const QDomElement &);
	
	private:
	void parseRows(const QString &);
	void parseColumns(const QString &);
	bool checkCell(const QDomElement &, TitleBlockCell ** = 0);
	void flushCells();
	void initCells();
	int lengthRange(int, int, const QList<int> &) const;
	QString finalTextForCell(const TitleBlockCell &, const DiagramContext &) const;
	
	// attributs
	private:
	QDomDocument xml_description_;
	QHash<QString, QSvgRenderer *> vector_logos_;
	QHash<QString, QPixmap *>      bitmap_logos_;
	QList<int> rows_heights_;
	QList<TitleBlockColDimension> columns_width_;
	QVector< QVector<TitleBlockCell> > cells_;
};
#endif
