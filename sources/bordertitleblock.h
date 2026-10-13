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
#ifndef BORDERTITLEBLOCK_H
#define BORDERTITLEBLOCK_H
#include "autoNum/numerotationcontext.h"
#include "borderproperties.h"
#include "diagramcontext.h"
#include "titleblockproperties.h"

#include <QDate>
#include <QObject>
#include <QRectF>
class QPainter;
class DiagramPosition;
class TitleBlockTemplate;
class TitleBlockTemplateRenderer;
class Diagram;
class projectDataBase;
/**
	@brief The BorderTitleBlock class
	This class represents the border and the titleblock which frame a
	particular electric diagram.
*/
class BorderTitleBlock : public QObject
{
		Q_OBJECT
	
	public:
		BorderTitleBlock(QObject * = nullptr);
		~BorderTitleBlock() override;
	
	private:
		BorderTitleBlock(const BorderTitleBlock &);
	
		//METHODS
	public:	
		void draw(QPainter *painter);
		void drawDxf(QString &, int);
	
		//METHODS TO GET DIMENSION
		//COLUMNS
		/// @return the number of columns
		int   columnsCount() const { return(columns_count_); }
		/// @return the columns width, in pixels
		qreal columnsWidth() const { return(columns_width_); }
		/// @return the total width of all columns, headers excluded
		qreal columnsTotalWidth() const {
			return(columns_count_ * columns_width_); }
		/// @return the column headers height, in pixels
		qreal columnsHeaderHeight() const {
			return(columns_header_height_); }
	
		//ROWS
		/// @return the number of rows
		int rowsCount() const { return(rows_count_); }
		/// @return the rows height, in pixels
		qreal rowsHeight() const { return(rows_height_); }
		/// @return the total height of all rows, headers excluded
		qreal rowsTotalHeight() const {
			return(rows_count_ * rows_height_); }
		/// @return la rows header width, in pixels
		qreal rowsHeaderWidth() const { return(rows_header_width_); }
		/// @return the edge where title block is docked
		Qt::Edge titleBlockEdge() const { return(m_edge); }
	
		// border - title block = diagram
		/**
			@brief diagramWidth
			@return the diagram width,
			i.e. the width of the border without title block
		*/
		qreal diagramWidth() const
{
			return(columnsTotalWidth() + rowsHeaderWidth()); }
		/**
			@brief diagramHeight
			@return the diagram height,
			i.e. the height of the border without title block
		*/
		qreal diagramHeight() const
{
			return(rowsTotalHeight() + columnsHeaderHeight()); }

		QRectF titleBlockRect () const;

		DiagramContext titleblockInformation() const;
	private:
		QRectF titleBlockRectForQPainter () const;

	public:
		QRectF borderAndTitleBlockRect () const;
		QRectF columnsRect () const;
		QRectF rowsRect () const;
		QRectF outsideBorderRect() const;
		QRectF insideBorderRect() const;
	
		// methods to get title block basic data. A folio placed in a
		// project reads them from the project's store
		// (DB-ACCESSORS-PLAN.md stage 4.4), any other from its own copy.
		/// @return the value of the title block "Author" field
		QString author() const { const TitleBlockProperties *s = stored(); return(s ? s->author : btb_author_); }
		/// @return the value of the title block "Date" field
		QDate date() const { const TitleBlockProperties *s = stored(); return(s ? s->date : btb_date_); }
		/// @return the value of the title block "Title" field
		QString title() const { const TitleBlockProperties *s = stored(); return(s ? s->title : btb_title_); }
		/// @return the value of the title block "Folio" field
		QString folio() const { const TitleBlockProperties *s = stored(); return(s ? s->folio : btb_folio_); }
		/// @return the value of the title block "Folio" field as displayed
		QString finalfolio() const { return(btb_final_folio_); }
		/// @return the value of the title block "Plant" field
		QString plant() const { const TitleBlockProperties *s = stored(); return(s ? s->plant : btb_plant_); }
		/// @return the value of the title block "Locmach" field
		QString locmach() const { const TitleBlockProperties *s = stored(); return(s ? s->locmach : btb_locmach_); }
		/// @return the value of the revision index block "Folio" field
		QString indexrev() const { const TitleBlockProperties *s = stored(); return(s ? s->indexrev : btb_indexrev_); }
		/// @return the value of the title block "File" field
		QString fileName() const { const TitleBlockProperties *s = stored(); return(s ? s->filename : btb_filename_); }
		/// @return the value of the title block Additional Fields
		QString version() const { const TitleBlockProperties *s = stored(); return(s ? s->version : btb_version_); }
		/// @return the value of the title block Additional Fields
		DiagramContext additionalFields() const {
			const TitleBlockProperties *s = stored(); return(s ? s->context : additional_fields_); }
		/// @return the value of the title block
		QString autoPageNum() const { const TitleBlockProperties *s = stored(); return(s ? s->auto_page_num : btb_auto_page_num_); }
			/// The title of the folio numbering this folio follows, nothing else changes
		void setAutoPageNum(const QString &title);
		/// @return the value of the total number of folios
		int folioTotal() const { return(folio_total_);}
	
		// methods to get display options
		/// @return true si le cartouche est affiche, false sinon
		bool titleBlockIsDisplayed() const {
			return(display_titleblock_); }
		/// @return true si les entetes des colonnes sont affiches,
		/// false sinon
		bool columnsAreDisplayed() const { return(display_columns_); }
		/// @return true si les entetes des lignes sont affiches,
		/// false sinon
		bool rowsAreDisplayed() const { return(display_rows_); }
		/// @return true si la bordure est affichee, false sinon
		bool borderIsDisplayed() const { return(display_border_); }
		
		// methods to set dimensions
		void setColumnsCount(int);
		void setRowsCount(int);
		void setColumnsWidth(const qreal &);
		void setRowsHeight(const qreal &);
		void setColumnsHeaderHeight(const qreal &);
		void setRowsHeaderWidth(const qreal &);
		void setDiagramHeight(const qreal &);
		
		DiagramPosition convertPosition(const QPointF &);
		QRectF cellRect(const QString &cell) const;
		
		// methods to set title block basic data
		void setFolio(const QString &folio);
		void setFolioData(int, int, const QString& = nullptr,
				  const DiagramContext & = DiagramContext());
		void setPreviousFolioNum(const QString &previous);
		void setNextFolioNum(const QString &next);
		
		void titleBlockToXml(QDomElement &);
		void titleBlockFromXml(const QDomElement &);
		void borderToXml(QDomElement &);
		void borderFromXml(const QDomElement &);
		
		TitleBlockProperties exportTitleBlock() const;
		TitleBlockProperties localTitleBlock() const;
		void setOwnerFolio(Diagram *folio);
		void importTitleBlock(const TitleBlockProperties &);
		BorderProperties exportBorder();
		void importBorder(const BorderProperties &);
		
		const TitleBlockTemplate *titleBlockTemplate();
		void setTitleBlockTemplate(const TitleBlockTemplate *);
		QString titleBlockTemplateName() const;
		
	public slots:
		void titleBlockTemplateChanged(const QString &);
		void titleBlockTemplateRemoved(
				const QString &,
				const TitleBlockTemplate * = nullptr);
		
		// methods to set display options
		void displayTitleBlock(bool);
		void displayColumns(bool);
		void displayRows(bool);
		void displayBorder(bool);
	
	private:
		void updateRectangles();
		void updateDiagramContextForTitleBlock(
				const DiagramContext & = DiagramContext());
		QString incrementLetters(const QString &);
	
		signals:
		/**
			@brief borderChanged
			Signal emitted after the border has changed
			@param old_border Former border
			@param new_border New border
		*/
		void borderChanged(QRectF old_border, QRectF new_border);
		/**
			@brief displayChanged
			Signal emitted after display options have changed
		*/
		void displayChanged();
		/**
			@brief titleBlockFolioChanged
			Signal emitted after Folio has changed
		*/
		void titleBlockFolioChanged(const QString &);
		/**
			@brief needFolioData
			Signal emitted when the title block
			requires its data to be updated
			in order to generate the folio field.
		*/
		void needFolioData();
		/**
			@brief needTitleBlockTemplate
			Signal emitted when this object needs to
			set a specific title block template.
			This object cannot handle the job
			since it does not know of its parent project.
		*/
		void needTitleBlockTemplate(const QString &);

		/**
		 * @brief infoChanged
		 * Signal emitted when the informations changed.
		 */
		void informationChanged();
	
	// attributes
	private:
		// titleblock basic data
		QString btb_author_;        ///< titleblock author
		QDate   btb_date_;          ///< titleblock date
		QString btb_title_;         ///< titleblock title
		QString btb_folio_;         ///< titleblock folio
		QString btb_plant_;         ///< titleblock plant
		QString btb_locmach_;       ///< titleblock locmach
		QString btb_indexrev_;      ///< titleblock index rev
		QString btb_final_folio_;   ///< titleblock final folio
		QString btb_auto_page_num_; ///< titleblock auto page num
		int folio_index_ = 0;       ///< titleblock index
		int folio_total_ = 0;       ///< titleblock total
		QString btb_filename_;      ///< titleblock filename
		QString btb_version_;       ///< titleblock version
		/// titleblock additional fields
		DiagramContext additional_fields_;
		Qt::Edge m_edge = Qt::BottomEdge; ///< titleblock edge
		QString m_next_folio_num;     ///< titleblock next folio num
		QString m_previous_folio_num; ///< titleblock previous folio num
	
		// border dimensions (rows and columns)
		// columns: number and dimensions
		int columns_count_ = 0;           ///< columns count
		qreal columns_width_ = 0;         ///< columns width
		qreal columns_header_height_ = 0; ///< columns header height
	
		// rows: number and dimensions
		int rows_count_ = 0;          ///< rows count
		qreal rows_height_ = 0;       ///< rows height
		qreal rows_header_width_ = 0; ///< rows header width
	
		// title block dimensions
		qreal titleblock_height_ = 0;
	
		// rectangles used for drawing operations
		QRectF diagram_rect_;
	
		// display options. Initialised here because the constructor sets
		// them through importBorder() and importTitleBlock(), whose setters
		// compare the new value with the old one before storing it.
		bool display_titleblock_ = false;
		bool display_columns_ = false;
		bool display_rows_ = false;
		bool display_border_ = false;
		TitleBlockTemplateRenderer *m_titleblock_template_renderer;
		Diagram *m_owner_folio = nullptr;
		TitleBlockProperties current() const;
		const TitleBlockProperties *stored() const;
		projectDataBase *placedStore() const;
		void storeTitleBlock();
};
#endif
