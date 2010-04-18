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
#ifndef DIAGRAM_H
#define DIAGRAM_H
#include <QtGui>
#include <QtXml>
#include "borderinset.h"
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
		 * Inset : Afficher le cartouche
		 * Columns : Afficher les colonnes
	*/
	enum BorderOptions { EmptyBorder, Inset, Columns };
	/// Proprietes par defaut des nouveaux conducteurs
	ConductorProperties defaultConductorProperties;
	/// Dimensions et cartouches du schema
	BorderInset border_and_inset;
	/// Mouvement en cours lors d'un deplacement d'elements et conducteurs
	QPointF current_movement;
	/// taille de la grille en abscisse
	static const int xGrid;
	/// taille de la grille en ordonnee
	static const int yGrid;
	/// marge autour du schema
	static const qreal margin;
	
	private:
	QGraphicsLineItem *conductor_setter;
	bool draw_grid;
	bool use_border;
	bool moved_elements_fetched;
	QSet<Element *> elements_to_move;
	QSet<Conductor *> conductors_to_move;
	QHash<Conductor *, Terminal *> conductors_to_update;
	QSet<IndependentTextItem *> texts_to_move;
	QGIManager *qgi_manager;
	QUndoStack *undo_stack;
	bool draw_terminals;
	bool draw_colored_conductors_;
	QDomDocument xml_document;
	QETProject *project_;
	bool read_only_;
	
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
	void invalidateMovedElements();
	void fetchMovedElements();
	const QSet<Element *> &elementsToMove();
	const QSet<Conductor *> &conductorsToMove();
	const QHash<Conductor *, Terminal *> &conductorsToUpdate();
	const QSet<IndependentTextItem *> &independentTextsToMove();
	QSet<DiagramTextItem *> selectedTexts() const;
	QSet<Conductor *> selectedConductors() const;
	DiagramContent content() const;
	DiagramContent selectedContent();
	bool canRotateSelection() const;
	void moveElements(const QPointF &, QGraphicsItem * = 0);
	bool usesElement(const ElementsLocation &);
	
	QUndoStack &undoStack();
	QGIManager &qgiManager();
	
	public slots:
	void diagramTextChanged(DiagramTextItem *, const QString &, const QString &);
	
	// fonctions relative a la selection sur le schema
	void selectAll();
	void deselectAll();
	void invertSelection();
	
	signals:
	void written();
	void readOnlyChanged(bool);
};

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
	border_and_inset.displayBorder(!(bo & EmptyBorder));
	border_and_inset.displayColumns(bo & Columns);
	border_and_inset.displayInset(bo & Inset);
}

/**
	Permet de savoir les options du cadre, des colonnes et du cartouche.
	@return Un OU binaire entre les options possibles
	@see BorderOptions
*/
inline Diagram::BorderOptions Diagram::borderOptions() {
	BorderOptions retour = EmptyBorder;
	if (border_and_inset.insetIsDisplayed()) retour = (BorderOptions)(retour|Inset);
	if (border_and_inset.columnsAreDisplayed()) retour = (BorderOptions)(retour|Columns);
	return(retour);
}

/// @return la liste des elements a deplacer
inline const QSet<Element *> &Diagram::elementsToMove() {
	if (!moved_elements_fetched) fetchMovedElements();
	return(elements_to_move);
}

/// @return la liste des conducteurs a deplacer
inline const QSet<Conductor *> &Diagram::conductorsToMove() {
	if (!moved_elements_fetched) fetchMovedElements();
	return(conductors_to_move);
}

/// @return la liste des conducteurs a modifier (typiquement les conducteurs dont seul un element est deplace)
inline const QHash<Conductor *, Terminal *> &Diagram::conductorsToUpdate() {
	if (!moved_elements_fetched) fetchMovedElements();
	return(conductors_to_update);
}

/// @return la liste des textes a deplacer
inline const QSet<IndependentTextItem *> &Diagram::independentTextsToMove() {
	if (!moved_elements_fetched) fetchMovedElements();
	return(texts_to_move);
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
