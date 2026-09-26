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
#ifndef TITLEBLOCK_TEMPLATE_H
#define TITLEBLOCK_TEMPLATE_H
#include "diagramcontext.h"
#include "qet.h"
#include "titleblock/dimension.h"
#include "titleblockcell.h"

#include <QtSvg>
#include <QtXml>
#include <optional>

/**
	@brief The TitleBlockTemplate class
	This class represents an title block template for an electric diagram.
	It can read from an XML document the layout of the table
	that graphically represents the title block,
	and can produce a graphical rendering of it from a diagram context
	(object embedding the informations of the diagram we want to
	represent the title block.
*/
class TitleBlockTemplate : public QObject {
	Q_OBJECT

	public:
		/**
		@brief The TitleBlockTemplate::WidthConstraintCase enum Distinguishes 
		the possible outcomes of minimumWidth() or maximumWidth() for a 
		template's column layout: either a genuine finite pixel width, or one 
		of the cases below where no single finite width applies -- some of which are
		perfectly ordinary (Unconstrained), and some of which indicate the 
		template's columns cannot be laid out at any width.

		Both functions return one of the non-Bounded* cases below,
		cast to int, in place of a genuine width whenever no finite
		width applies. (*minimumWidth()/maximumWidth() never return
		a literal "Bounded" value -- when a finite width exists,
		they return that width directly. Bounded only appears as a
		concept in classifyWidthConstraint()'s std::nullopt return.)
		The int-based encoding exists because this project
		currently targets C++17.

		@todo Once this project's minimum supported C++ standard
		reaches C++23, migrate minimumWidth() and maximumWidth() to
		return std::expected<int, WidthConstraintCase> instead of
		encoding these cases as negative int sentinels. That removes
		the possibility of a caller silently misinterpreting a
		sentinel as a real pixel width -- something the current
		int-based encoding cannot prevent at compile time.
		*/
		enum class WidthConstraintCase : int {
			/**
				There is no finite width constraint in this direction:
				any width is valid. This is a perfectly ordinary case,
				not a problem with the template -- for minimumWidth(),
				it happens when the relative-to-total-length (RTT)
				columns account for exactly 100% of the total width and
				there are no absolute-width (ABS) columns competing for
				space -- every term in the minimum-width inequality
				vanishes (0 >= 0), which holds for any width. For
				maximumWidth(), it happens whenever at least one column
				scales with the total width, so nothing caps how wide
				the template may grow -- the common case for most
				templates.
			*/
			Unconstrained = -1,

			/**
				The RTT columns alone already exceed 100% of the total
				width (sum(RTT) > 100), independently of whether any ABS
				columns exist. The minimum-width inequality then only
				holds for a non-positive width, which cannot represent a
				real template: this indicates a genuinely misconfigured
				template that cannot be laid out at any width.
			*/
			RelativeWidthExceeds100Percent = -2,

			/**
				The RTT columns account for exactly 100% of the total
				width, but at least one ABS column also needs a nonzero,
				fixed amount of space on top of that. The minimum-width
				inequality reduces to "0 >= (a positive number)", which
				never holds: this also indicates a genuinely
				misconfigured template that cannot be laid out at any
				width.
			*/
			AbsoluteColumnsExceedRemainingWidth = -3
		};
	
	// constructors, destructor
	TitleBlockTemplate(QObject * = nullptr);
	~TitleBlockTemplate() override;
	private:
	TitleBlockTemplate(const TitleBlockTemplate &);
	
	// methods
	public:
	TitleBlockCell *createCell(const TitleBlockCell * = nullptr);
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
	bool insertColumn(const TitleBlockDimension &,
			  const QList<TitleBlockCell *> &, int = -1);
	QList<TitleBlockCell *> takeColumn(int);
	QList<TitleBlockCell *> createColumn();
	
	TitleBlockCell *cell(int, int) const;
	QSet<TitleBlockCell *> spannedCells(
			const TitleBlockCell *, bool = false) const;
	QHash<TitleBlockCell *, QPair<int, int> > getAllSpans() const;
	void setAllSpans(const QHash<TitleBlockCell *, QPair<int, int> > &);
	bool addLogo(
			const QString &,
			QByteArray *,
			const QString & = "svg",
			const QString & = "xml");
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
	void renderDxf(
			QRectF &,
			const DiagramContext &,
			int,
			QString &,
			int) const;
	void renderCell(QPainter &,
			const TitleBlockCell &,
			const DiagramContext &,
			const QRect &) const;
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
	bool checkCell(const QDomElement &, TitleBlockCell ** = nullptr);
	void flushCells();
	void initCells();
	/**
	@brief TitleBlockTemplate::classifyWidthConstraint Classifies this template's absolute-width (ABS) and
	relative-to-total-length (RTT) columns, independently of whether a minimum or a maximum width is being computed -- see
	minimumWidth() for the derivation this is based on.
	@param[out] abs_total set to columnTypeTotal(QET::Absolute).
	@param[out] remaining_width_fraction set to the fraction of the	total template width left over once the RTT columns have taken
	their share: (100.0 - sum(RTT)) / 100.0. Zero means the RTT columns claim the entire width, leaving nothing for ABS
	columns; negative means they claim more than the entire width, which is unsatisfiable regardless of any ABS columns. Only
	meaningful when this function returns std::nullopt.
	@return WidthConstraintCase::RelativeWidthExceeds100Percent, WidthConstraintCase::AbsoluteColumnsExceedRemainingWidth, or
	WidthConstraintCase::Unconstrained if no finite width exists for this template's columns, or std::nullopt if a finite width
	does exist (computable as qRound(abs_total / remaining_width_fraction)).
	*/
	std::optional<WidthConstraintCase> classifyWidthConstraint(int &abs_total, qreal &remaining_width_fraction);
	int lengthRange(int, int, const QList<int> &) const;
	QString finalTextForCell(
			const TitleBlockCell &,
			const DiagramContext &) const;
	QString interpreteVariables(
			const QString &,
			const DiagramContext &) const;
	void renderTextCell(
			QPainter &,
			const QString &,
			const TitleBlockCell &,
			const QRectF &) const;
	void renderTextCellDxf(
			QString &,
			const QString &,
			const TitleBlockCell &,
			qreal,
			qreal,
			qreal,
			qreal,
			int) const;
	
	// attributes
	private:
	/**
		@brief name_ : name identifying
		the Title Block Template within its parent collection
	*/
	QString name_;
	QString information_;
	/**
		@brief data_logos_ : Logos raw data
	*/
	QHash<QString, QByteArray >    data_logos_;
	/**
		@brief storage_logos_ :
		Logos applied storage type (e.g. "xml" or "base64")
	*/
	QHash<QString, QString>        storage_logos_;
	/**
		@brief type_logos_ :
		Logos types (e.g. "png", "jpeg", "svg")
	*/
	QHash<QString, QString>        type_logos_;
	/**
		@brief vector_logos_ : Rendered objects for vector logos
	*/
	QHash<QString, QSvgRenderer *> vector_logos_;
	/**
		@brief bitmap_logos_ : Pixmaps for bitmap logos
	*/
	QHash<QString, QPixmap>        bitmap_logos_;
	
	QList<int> rows_heights_; ///< rows heights -- simple integers
	/**
		@brief columns_width_ :
		columns widths -- @see TitleBlockColDimension
	*/
	QList<TitleBlockDimension> columns_width_;
	/**
		@brief registered_cells_ : Cells objects created rattached
		to this template, but not mandatorily used
	*/
	QList<TitleBlockCell *> registered_cells_;
	QList< QList<TitleBlockCell *> > cells_;         ///< Cells grid
};
#endif
