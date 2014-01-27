/*
	Copyright 2006-2013 The QElectroTech Team
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
#include "dimension.h"
#include "qet.h"

/**
	This class represents an title block template for an electric diagram.
	It can read from an XML document the layout of the table that graphically
	represents the title block, and can produce a graphical rendering of it from a
	diagram context (object embedding the informations of the diagram we want to
	represent the title block.
*/
class TitleBlockTemplate : public QObject {
	Q_OBJECT
	
	// constructors, destructor
	public:
	TitleBlockTemplate(QObject * = 0);
	virtual ~TitleBlockTemplate();
	private:
	TitleBlockTemplate(const TitleBlockTemplate &);
	
	// methods
	public:
	TitleBlockCell *createCell(const TitleBlockCell * = 0);
	static QFont fontForCell(const TitleBlockCell &);
	bool loadFromXmlFile(const QString &);
	bool loadFromXmlElement(const QDomElement &);
	bool saveToXmlFile(const QString &);
	bool saveToXmlElement(QDomElement &) const;
	void exportCellToXml(TitleBlockCell *,QDomElement &) const;
	TitleBlockTemplate *clone() const;
	QString name() const;
	QString information() const;
	void setInformation(const QString &);
	int rowDimension(int);
	void setRowDimension(int, const TitleBlockDimension &);
	TitleBlockDimension columnDimension(int);
	void setColumnDimension(int, const TitleBlockDimension &);
	int columnsCount() const;
	int rowsCount() const;
	QList<int> columnsWidth(int) const;
	QList<int> rowsHeights() const;
	int columnTypeCount(QET::TitleBlockColumnLength);
	int columnTypeTotal(QET::TitleBlockColumnLength);
	int minimumWidth();
	int maximumWidth();
	int width(int);
	int height() const;
	
	bool moveRow(int, int);
	void addRow(int = -1);
	bool insertRow(int, const QList<TitleBlockCell *> &, int = -1);
	QList<TitleBlockCell *> takeRow(int);
	QList<TitleBlockCell *> createRow();
	
	bool moveColumn(int, int);
	void addColumn(int = -1);
	bool insertColumn(const TitleBlockDimension &, const QList<TitleBlockCell *> &, int = -1);
	QList<TitleBlockCell *> takeColumn(int);
	QList<TitleBlockCell *> createColumn();
	
	TitleBlockCell *cell(int, int) const;
	QSet<TitleBlockCell *> spannedCells(const TitleBlockCell *, bool = false) const;
	QHash<TitleBlockCell *, QPair<int, int> > getAllSpans() const;
	void setAllSpans(const QHash<TitleBlockCell *, QPair<int, int> > &);
	bool addLogo(const QString &, QByteArray *, const QString & = "svg", const QString & = "xml");
	bool addLogoFromFile(const QString &, const QString & = QString());
	bool saveLogoToFile(const QString &, const QString &);
	bool removeLogo(const QString &);
	bool renameLogo(const QString &, const QString &);
	void setLogoStorage(const QString &, const QString &);
	QList<QString> logos() const;
	QString logoType(const QString &) const;
	QSvgRenderer *vectorLogo(const QString &) const;
	QPixmap bitmapLogo(const QString &) const;
	
	void render(QPainter &, const DiagramContext &, int) const;
	void renderDxf(QRectF &, const DiagramContext &, int, QString &, int) const;
	void renderCell(QPainter &, const TitleBlockCell &, const DiagramContext &, const QRect &) const;
	QString toString() const;
	void applyCellSpans();
	void forgetSpanning();
	void forgetSpanning(TitleBlockCell *, bool = true);
	bool checkCellSpan(TitleBlockCell *);
	void applyCellSpan(TitleBlockCell *);
	void applyRowColNums();
	void rowColsChanged();
	QStringList listOfVariables();
	
	protected:
	void loadInformation(const QDomElement &);
	bool loadLogos(const QDomElement &, bool = false);
	bool loadLogo(const QDomElement &);
	bool loadGrid(const QDomElement &);
	bool loadCells(const QDomElement &);
	void loadCell(const QDomElement &);
	void saveInformation(QDomElement &) const;
	void saveLogos(QDomElement &) const;
	void saveLogo(const QString &, QDomElement &) const;
	void saveGrid(QDomElement &) const;
	void saveCells(QDomElement &) const;
	void saveCell(TitleBlockCell *, QDomElement &, bool = false) const;
	QList<TitleBlockCell *> createCellsList(int);
	
	private:
	void parseRows(const QString &);
	void parseColumns(const QString &);
	bool checkCell(const QDomElement &, TitleBlockCell ** = 0);
	void flushCells();
	void initCells();
	int lengthRange(int, int, const QList<int> &) const;
	QString finalTextForCell(const TitleBlockCell &, const DiagramContext &) const;
	QString interpreteVariables(const QString &, const DiagramContext &) const;
	void renderTextCell(QPainter &, const QString &, const TitleBlockCell &, const QRectF &) const;
	void renderTextCellDxf(QString &, const QString &, const TitleBlockCell &, qreal, qreal, qreal, qreal, int) const;
	
	// attributes
	private:
	QString name_;                                   ///< name identifying the Title Block Template within its parent collection
	QString information_;
	
	QHash<QString, QByteArray >    data_logos_;      ///< Logos raw data
	QHash<QString, QString>        storage_logos_;   ///< Logos applied storage type (e.g. "xml" or "base64")
	QHash<QString, QString>        type_logos_;      ///< Logos types (e.g. "png", "jpeg", "svg")
	QHash<QString, QSvgRenderer *> vector_logos_;    ///< Rendered objects for vector logos
	QHash<QString, QPixmap>        bitmap_logos_;    ///< Pixmaps for bitmap logos
	
	QList<int> rows_heights_;                        ///< rows heights -- simple integers
	QList<TitleBlockDimension> columns_width_;       ///< columns widths -- @see TitleBlockColDimension
	QList<TitleBlockCell *> registered_cells_;       ///< Cells objects created rattached to this template, but not mandatorily used
	QList< QList<TitleBlockCell *> > cells_;         ///< Cells grid
};
#endif
