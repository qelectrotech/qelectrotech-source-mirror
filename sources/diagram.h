/*
	Copyright 2006-2021 The QElectroTech Team
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
#ifndef DIAGRAM_H
#define DIAGRAM_H
#include "autoNum/numerotationcontext.h"
#include "bordertitleblock.h"
#include "conductorproperties.h"
#include "elementsmover.h"
#include "elementtextsmover.h"
#include "exportproperties.h"
#include "properties/xrefproperties.h"
#include "qetproject.h"
#include "qgimanager.h"

#include <QHash>
#include <QUuid>
#include <QtWidgets>
#include <QtXml>

class Conductor;
class CustomElement;
class DiagramContent;
class DiagramPosition;
class DiagramTextItem;
class Element;
class ElementsLocation;
class QETProject;
class Terminal;
class DiagramImageItem;
class DiagramEventInterface;
class DiagramFolioList;
class QETProject;

/**
	@brief The Diagram class
	This class represents an electric diagram.
	It manages its various child elements,
	conductors and texts and handles their graphic rendering.
*/
class Diagram : public QGraphicsScene
{
	friend DiagramFolioList;
	friend QETProject;

	Q_OBJECT
	
		// constructors, destructor
	private:
		Diagram(QETProject *project);
		~Diagram() override;
		Diagram(const Diagram &diagram);
	
	// ATTRIBUTES
	public:
		/**
			@brief The BorderOptions enum
			Represents available options when rendering a particular diagram:
			EmptyBorder: display border only
			TitleBlock: display title block
			Columns: display columns
		*/
		enum BorderOptions { EmptyBorder, TitleBlock, Columns };
		/// Represents available option of Numerotation type.
		enum NumerotationType { Conductors };
		/// Default properties for new conductors
		ConductorProperties defaultConductorProperties;
		/// Diagram dimensions and title block
		BorderTitleBlock border_and_titleblock;
		/// abscissa grid step size
		static int xGrid;
		/// ordinate grid step size
		static int yGrid;
		/// Key grid x step size
		static int xKeyGrid;
		/// Key grid y step size
		static int yKeyGrid;
		/// Key grid fine x step size
		static int xKeyGridFine;
		/// Key grid fine y step size
		static int yKeyGridFine;
		/// margin around the diagram
		static const qreal margin;
		/// background color of diagram
		static QColor background_color;
		/// Hash containing max values for folio sequential autonums in this diagram
		QHash <QString, QStringList> m_elmt_unitfolio_max;
		QHash <QString, QStringList> m_elmt_tenfolio_max;
		QHash <QString, QStringList> m_elmt_hundredfolio_max;
		/// Hash containing max values for folio sequential autonums in this diagram
		QHash <QString, QStringList> m_cnd_unitfolio_max;
		QHash <QString, QStringList> m_cnd_tenfolio_max;
		QHash <QString, QStringList> m_cnd_hundredfolio_max;

	private:
		QGraphicsLineItem *conductor_setter_;
		ElementsMover     m_elements_mover;
		ElementTextsMover m_element_texts_mover;
		QGIManager        *qgi_manager_;
		QETProject        *m_project;

		QDomDocument xml_document_;

		qreal diagram_qet_version_;

		bool draw_grid_;
		bool use_border_;
		bool draw_terminals_;
		bool draw_colored_conductors_;

		QString m_conductors_autonum_name;
		DiagramEventInterface *m_event_interface;

		bool m_freeze_new_elements;
		bool m_freeze_new_conductors_;
		QUuid m_uuid = QUuid::createUuid();
	
	// METHODS
	protected:
		void drawBackground(QPainter *, const QRectF &) override;

		void mouseDoubleClickEvent (
				QGraphicsSceneMouseEvent *event) override;
		void mousePressEvent (QGraphicsSceneMouseEvent *event) override;
		void mouseMoveEvent (QGraphicsSceneMouseEvent *event) override;
		void mouseReleaseEvent (
				QGraphicsSceneMouseEvent *event) override;
		void wheelEvent (QGraphicsSceneWheelEvent *event) override;
		void keyPressEvent (QKeyEvent *event) override;
		void keyReleaseEvent (QKeyEvent *) override;
	
	public:
		QUuid uuid();
		void setEventInterface (DiagramEventInterface *event_interface);
		void clearEventInterface();

		//methods related to autonum
		QString conductorsAutonumName() const;
		void setConductorsAutonumName(const QString &name);

		static bool clipboardMayContainDiagram();
	
		// methods related to parent project
		QETProject *project() const;
		int         folioIndex() const;
		void        showMe() {emit showDiagram(this);}
		bool        isReadOnly() const;
	
		// methods related to conductor creation
		void setConductor(bool);
		void setConductorStart (QPointF);
		void setConductorStop(QPointF);
		QList < QSet <Conductor *> > potentials();
	
		// methods related to XML import/export
		QDomDocument toXml(bool = true);
		bool initFromXml(QDomElement &,
				 QPointF = QPointF(),
				 bool = true,
				 DiagramContent * = nullptr);
		bool fromXml(QDomDocument &,
			     QPointF = QPointF(),
			     bool = true,
			     DiagramContent * = nullptr);
		bool fromXml(QDomElement &,
			     QPointF = QPointF(),
			     bool = true,
			     DiagramContent * = nullptr);
		void folioSequentialsToXml(QHash<QString,
					   QStringList>*,
					   QDomElement *,
					   const QString&,
					   const QString&,
					   QDomDocument *);
		void folioSequentialsFromXml(const QDomElement&,
					     QHash<QString,
					     QStringList>*,
					     const QString&,
					     const QString&,
					     const QString&,
					     const QString&);
	
		void refreshContents();
	
		// methods related to graphics items addition/removal on the diagram
		virtual void addItem    (QGraphicsItem *item);
		virtual void removeItem (QGraphicsItem *item);
	
		// methods related to graphics options
		ExportProperties applyProperties(const ExportProperties &);
		void setDisplayGrid(bool);
		bool displayGrid();
		void setUseBorder(bool);
		bool useBorder();
		void setBorderOptions(BorderOptions);
		BorderOptions borderOptions();
		DiagramPosition convertPosition(const QPointF &);
		static QPointF snapToGrid(const QPointF &p);
	
		bool drawTerminals() const;
		void setDrawTerminals(bool);
		bool drawColoredConductors() const;
		void setDrawColoredConductors(bool);
	
		QString title() const;
		bool toPaintDevice(QPaintDevice &, int = -1, int = -1,
				   Qt::AspectRatioMode = Qt::KeepAspectRatio);
		QSize imageSize() const;
		
		bool isEmpty() const;
	
		QList<Element *> elements() const;
		QList<Conductor *> conductors() const;
		QSet<Conductor *> selectedConductors() const;
		DiagramContent content() const;
		bool canRotateSelection() const;
		ElementsMover &elementsMover();
		ElementTextsMover &elementTextsMover();
		bool usesElement(const ElementsLocation &);
		bool usesTitleBlockTemplate(const QString &);
		
		QUndoStack &undoStack();
		QGIManager &qgiManager();
	
		//methods related to element label Update Policy
		void freezeElements(bool freeze);
		void unfreezeElements();
		void setFreezeNewElements(bool);
		bool freezeNewElements();
	
		//methods related to conductor label Update Policy
		void freezeConductors(bool freeze);
		void setFreezeNewConductors(bool);
		bool freezeNewConductors();
	
		//methods related to insertion and loading of folio sequential
		void insertFolioSeqHash (QHash<QString, QStringList> *hash,
					 const QString& title,
					 const QString& seq,
					 NumerotationContext *nc);
		void loadFolioSeqHash (QHash<QString, QStringList> *hash,
				       const QString& title, const QString& seq,
				       NumerotationContext *nc);
		void changeZValue(QET::DepthOption option);

	public slots:
		void adjustSceneRect ();
		void titleChanged(const QString &);
		void titleBlockTemplateChanged(const QString &);
		void titleBlockTemplateRemoved(const QString &,
					       const QString & = QString());
		void setTitleBlockTemplate(const QString &);
		void updateLabels();
		void loadElmtFolioSeq();
		void loadCndFolioSeq();
	
		// methods related to graphics items selection
		void selectAll();
		void deselectAll();
		void invertSelection();

	signals:
		void showDiagram (Diagram *);
		void usedTitleBlockTemplateChanged(const QString &);
		void diagramTitleChanged(Diagram *, const QString &);

		/// Signal emitted when users wish to locate an element
		/// from the diagram within elements collection
		void findElementRequired(const ElementsLocation &);

		void diagramActivated();
		void diagramInformationChanged();
};
Q_DECLARE_METATYPE(Diagram *)

/**
	@brief Diagram::setConductor
	Display or hide the conductor setter,
	i.e. a dashed conductor stub which appears
	when creating a conductor between two terminals.
	@param adding true add conductor ,false remove conductor
*/
inline void Diagram::setConductor(bool adding) {
	if (adding) {
		if (!conductor_setter_ -> scene()) addItem(conductor_setter_);
	} else {
		if (conductor_setter_ -> scene()) removeItem(conductor_setter_);
	}
}

/**
	@brief Diagram::setConductorStart
	Set the start point of the conductor setter.
	@param start the point (in scene coordinates) which the newly created
	conductor should start from.
*/
inline void Diagram::setConductorStart(QPointF start) {
	conductor_setter_ -> setLine(QLineF(start, conductor_setter_ -> line().p2()));
}

/**
	@brief Diagram::setConductorStop
	Set the end point of the conductor setter.
	@param end the point (in scene coordinates) upon to which the newly created
	conductor should be drawn.
*/
inline void Diagram::setConductorStop(QPointF end) {
	conductor_setter_ -> setLine(QLineF(conductor_setter_ -> line().p1(), end));
}

/**
	@brief Diagram::setDisplayGrid
	Set whether the diagram grid should be drawn.
	@param dg true to render the grid, false otherwise.
*/
inline void Diagram::setDisplayGrid(bool dg) {
	draw_grid_ = dg;
}

/**
	@brief Diagram::displayGrid
	@return draw_grid_ true if the grid is drawn, false otherwise.
*/
inline bool Diagram::displayGrid() {
	return(draw_grid_);
}

/**
	@brief Diagram::setUseBorder
	Set whether the diagram border (including rows/columns headers and the title
	block) should be rendered along with the diagram. When set to false, the size
	of the smallest rectangle containing all items is considered as the diagram
	size.
	@param ub true to take the border into account, false otherwise
*/
inline void Diagram::setUseBorder(bool ub) {
	use_border_ = ub;
}

/**
	@brief Diagram::useBorder
	@return use_border_ true if the border is rendered and take into account,
	false otherwise.
*/
inline bool Diagram::useBorder() {
	return(use_border_);
}

/**
	@brief Diagram::setBorderOptions
	Set the rendering options for the diagram border (including rows/columns
	headers and the title block)
	@param bo Enabled options ORed together
	@see BorderOptions
*/
inline void Diagram::setBorderOptions(Diagram::BorderOptions bo) {
	border_and_titleblock.displayBorder(!(bo & EmptyBorder));
	border_and_titleblock.displayColumns(bo & Columns);
	border_and_titleblock.displayTitleBlock(bo & TitleBlock);
}

/**
	@brief Diagram::borderOptions
	@return The rendering options for the diagram border
	@see setBorderOptions
*/
inline Diagram::BorderOptions Diagram::borderOptions() {
	BorderOptions options = EmptyBorder;
	if (border_and_titleblock.titleBlockIsDisplayed())
		options = (BorderOptions)(options|TitleBlock);
	if (border_and_titleblock.columnsAreDisplayed())
		options = (BorderOptions)(options|Columns);
	return(options);
}

/**
	@brief Diagram::undoStack
	@return the diagram undo stack
*/
inline QUndoStack &Diagram::undoStack() {
	return *(project()->undoStack());
}

/**
	@brief Diagram::qgiManager
	@return the diagram graphics item manager
*/
inline QGIManager &Diagram::qgiManager() {
	return(*qgi_manager_);
}

/**
	@brief Diagram::drawTerminals
	@return true if terminals are rendered, false otherwise
*/
inline bool Diagram::drawTerminals() const
{
	return(draw_terminals_);
}

/**
	@brief Diagram::drawColoredConductors
	@return true if conductors colors are rendered, false otherwise.
*/
inline bool Diagram::drawColoredConductors() const
{
	return(draw_colored_conductors_);
}

#endif
