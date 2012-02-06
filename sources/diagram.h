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
#ifndef DIAGRAM_H
#define DIAGRAM_H
#include <QtGui>
#include <QtXml>
#include "bordertitleblock.h"
#include "conductorproperties.h"
#include "exportproperties.h"
#include "qgimanager.h"
class Conductor;
class CustomElement;
class DiagramContent;
class DiagramPosition;
class DiagramTextItem;
class Element;
class ElementsLocation;
class ElementsMover;
class ElementTextItem;
class ElementTextsMover;
class IndependentTextItem;
class QETProject;
class Terminal;
/**
	Cette classe represente un schema electrique.
	Elle gere les differents elements et conducteurs qui le composent
	et en effectue le rendu graphique.
*/
class Diagram : public QGraphicsScene {
	Q_OBJECT
	
	// constructeurs, destructeur
	public:
	Diagram(QObject * = 0);
	virtual ~Diagram();
	
	private:
	Diagram(const Diagram &diagram);
	
	// attributs
	public:
	/**
		Represente les options possibles pour l'affichage du schema :
		 * EmptyBorder : N'afficher que la bordure
		 * TitleBlock : Afficher le cartouche
		 * Columns : Afficher les colonnes
	*/
	enum BorderOptions { EmptyBorder, TitleBlock, Columns };
	/// Proprietes par defaut des nouveaux conducteurs
	ConductorProperties defaultConductorProperties;
	/// Dimensions et cartouches du schema
	BorderTitleBlock border_and_titleblock;
	/// taille de la grille en abscisse
	static const int xGrid;
	/// taille de la grille en ordonnee
	static const int yGrid;
	/// marge autour du schema
	static const qreal margin;
	
	private:
	QGraphicsLineItem *conductor_setter;
	ElementsMover *elements_mover_;
	ElementTextsMover *element_texts_mover_;
	bool draw_grid;
	bool use_border;
	QGIManager *qgi_manager;
	QUndoStack *undo_stack;
	bool draw_terminals;
	bool draw_colored_conductors_;
	QDomDocument xml_document;
	QETProject *project_;
	bool read_only_;
	qreal diagram_qet_version_;
	
	// methodes
	protected:
	virtual void drawBackground(QPainter *, const QRectF &);
	virtual void keyPressEvent(QKeyEvent *);
	virtual void keyReleaseEvent(QKeyEvent *);
	
	public:
	static bool clipboardMayContainDiagram();
	
	// fonctions relatives au projet parent
	QETProject *project() const;
	void setProject(QETProject *);
	int folioIndex() const;
	qreal declaredQElectroTechVersion(bool = true) const;
	
	// fonctions relatives a la lecture seule
	bool isReadOnly() const;
	void setReadOnly(bool);
	
	// fonctions relatives a la pose de conducteurs
	void setConductor(bool);
	void setConductorStart (QPointF);
	void setConductorStop(QPointF);
	
	// fonctions relatives a l'import / export XML
	QDomDocument toXml(bool = true);
	bool initFromXml(QDomElement &, QPointF = QPointF(), bool = true, DiagramContent * = 0);
	bool fromXml(QDomDocument &, QPointF = QPointF(), bool = true, DiagramContent * = 0);
	bool fromXml(QDomElement &, QPointF = QPointF(), bool = true, DiagramContent * = 0);
	void write();
	void write(const QDomElement &);
	bool wasWritten() const;
	QDomElement writeXml(QDomDocument &) const;
	
	// fonctions relative a l'ajout et a l'enlevement d'elements graphiques sur le schema
	void addElement(Element *);
	void addConductor(Conductor *);
	void addIndependentTextItem(IndependentTextItem *);
	
	void removeElement(Element *);
	void removeConductor(Conductor *);
	void removeIndependentTextItem(IndependentTextItem *);
	
	// fonctions relatives aux options graphiques
	ExportProperties applyProperties(const ExportProperties &);
	void setDisplayGrid(bool);
	bool displayGrid();
	void setUseBorder(bool);
	bool useBorder();
	void setBorderOptions(BorderOptions);
	BorderOptions borderOptions();
	DiagramPosition convertPosition(const QPointF &);
	
	bool drawTerminals() const;
	void setDrawTerminals(bool);
	bool drawColoredConductors() const;
	void setDrawColoredConductors(bool);
	
	QRectF border() const;
	QString title() const;
	bool toPaintDevice(QPaintDevice &, int = -1, int = -1, Qt::AspectRatioMode = Qt::KeepAspectRatio);
	QSize imageSize() const;
	
	bool isEmpty() const;
	
	QList<CustomElement *> customElements() const;
	QSet<DiagramTextItem *> selectedTexts() const;
	QSet<Conductor *> selectedConductors() const;
	DiagramContent content() const;
	DiagramContent selectedContent();
	bool canRotateSelection() const;
	int  beginMoveElements(QGraphicsItem * = 0);
	void continueMoveElements(const QPointF &);
	void endMoveElements();
	int  beginMoveElementTexts(QGraphicsItem * = 0);
	void continueMoveElementTexts(const QPointF &);
	void endMoveElementTexts();
	bool usesElement(const ElementsLocation &);
	bool usesTitleBlockTemplate(const QString &);
	
	QUndoStack &undoStack();
	QGIManager &qgiManager();
	
	public slots:
	void titleChanged(const QString &);
	void diagramTextChanged(DiagramTextItem *, const QString &, const QString &);
	void titleBlockTemplateChanged(const QString &);
	void titleBlockTemplateRemoved(const QString &, const QString & = QString());
	void setTitleBlockTemplate(const QString &);
	
	// fonctions relative a la selection sur le schema
	void selectAll();
	void deselectAll();
	void invertSelection();
	
	signals:
	void written();
	void readOnlyChanged(bool);
	void usedTitleBlockTemplateChanged(const QString &);
	void diagramTitleChanged(Diagram *, const QString &);
};
Q_DECLARE_METATYPE(Diagram *)

/**
	Permet d'ajouter ou enlever le "poseur de conducteur", c'est-a-dire la
	droite en pointilles qui apparait lorsqu'on pose un conducteur entre deux
	bornes.
	@param pf true pour ajouter le poseur de conducteur, false pour l'enlever
*/
inline void Diagram::setConductor(bool pf) {
	if (pf) {
		if (!conductor_setter -> scene()) addItem(conductor_setter);
	} else {
		if (conductor_setter -> scene()) removeItem(conductor_setter);
	}
}

/**
	Specifie les coordonnees du point de depart du poseur de conducteur
	@param d Le nouveau point de depart du poseur de conducteur
*/
inline void Diagram::setConductorStart(QPointF d) {
	conductor_setter -> setLine(QLineF(d, conductor_setter -> line().p2()));
}

/**
	Specifie les coordonnees du point d'arrivee du poseur de conducteur
	@param a Le nouveau point d'arrivee du poseur de conducteur
*/
inline void Diagram::setConductorStop(QPointF a) {
	conductor_setter -> setLine(QLineF(conductor_setter -> line().p1(), a));
}

/**
	Permet de specifier si la grille du schema doit etre dessinee ou non
	@param dg true pour afficher la grille, false pour ne pas l'afficher
*/
inline void Diagram::setDisplayGrid(bool dg) {
	draw_grid = dg;
}

/**
	Permet de savoir si la grille du schema est dessinee ou non
	@return true si la grille est affichee , false sinon
*/
inline bool Diagram::displayGrid() {
	return(draw_grid);
}

/**
	Permet de specifier si le cadre du schema doit etre pris en compte pour
	determiner le contour du schema.
	@param ub true pour prendre le schema en compte, false sinon
*/
inline void Diagram::setUseBorder(bool ub) {
	use_border = ub;
}

/**
	Permet de savoir si le cadre du schema est pris en compte pour
	determiner le contour du schema.
*/
inline bool Diagram::useBorder() {
	return(use_border);
}

/**
	Permet de definir les options du cadre, des colonnes et du cartouche.
	@param bo Un OU binaire entre les options possibles
	@see BorderOptions
*/
inline void Diagram::setBorderOptions(Diagram::BorderOptions bo) {
	border_and_titleblock.displayBorder(!(bo & EmptyBorder));
	border_and_titleblock.displayColumns(bo & Columns);
	border_and_titleblock.displayTitleBlock(bo & TitleBlock);
}

/**
	Permet de savoir les options du cadre, des colonnes et du cartouche.
	@return Un OU binaire entre les options possibles
	@see BorderOptions
*/
inline Diagram::BorderOptions Diagram::borderOptions() {
	BorderOptions retour = EmptyBorder;
	if (border_and_titleblock.titleBlockIsDisplayed()) retour = (BorderOptions)(retour|TitleBlock);
	if (border_and_titleblock.columnsAreDisplayed()) retour = (BorderOptions)(retour|Columns);
	return(retour);
}

/// @return la pile d'annulations de ce schema
inline QUndoStack &Diagram::undoStack() {
	return(*undo_stack);
}

/// @return le egstionnaire de QGraphicsItem de ce schema
inline QGIManager &Diagram::qgiManager() {
	return(*qgi_manager);
}

/// @return true si les bornes sont affichees, false sinon
inline bool Diagram::drawTerminals() const {
	return(draw_terminals);
}

/// @return true si les couleurs des conducteurs sont respectees, false sinon
inline bool Diagram::drawColoredConductors() const {
	return(draw_colored_conductors_);
}

#endif
