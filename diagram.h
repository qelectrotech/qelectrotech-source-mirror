#ifndef SCHEMA_H
#define SCHEMA_H
#define GRILLE_X 10
#define GRILLE_Y 10
#define MARGIN   5.0
#include <QtGui>
#include <QtXml>
#include "qetdiagrameditor.h"
#include "borderinset.h"
#include "qgimanager.h"
class Element;
class Terminal;
class Conducer;
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
	enum BorderOptions { EmptyBorder, Inset, Columns };
	BorderInset border_and_inset;
	QPointF current_movement;
	
	private:
	QGraphicsLineItem *conducer_setter;
	bool draw_grid;
	bool use_border;
	bool moved_elements_fetched;
	QSet<Element *> elements_to_move;
	QSet<Conducer *> conducers_to_move;
	QHash<Conducer *, Terminal *> conducers_to_update;
	QGIManager qgi_manager;
	QUndoStack undo_stack;
	bool draw_terminals;
	
	// methodes
	public:
	void drawBackground(QPainter *, const QRectF &);
	void keyPressEvent(QKeyEvent *);
	void keyReleaseEvent(QKeyEvent *);
	
	// fonctions relatives a la pose de conducteurs
	void setConducer(bool);
	void setConducerStart (QPointF);
	void setConducerStop(QPointF);
	
	// fonctions relatives a l'import / export XML
	QDomDocument toXml(bool = true);
	bool fromXml(QDomDocument &, QPointF = QPointF(), bool = true, QList<Element *> * = NULL, QList<Conducer *> * = NULL);
	
	// fonctions relatives aux options graphiques
	void setDisplayGrid(bool);
	bool displayGrid();
	void setUseBorder(bool);
	bool useBorder();
	void setBorderOptions(BorderOptions);
	BorderOptions borderOptions();
	
	bool drawTerminals() const;
	void setDrawTerminals(bool);
	
	QRectF border() const;
	QImage toImage(int = -1, int = -1, Qt::AspectRatioMode = Qt::KeepAspectRatio);
	QSize imageSize() const;
	
	void invalidateMovedElements();
	void fetchMovedElements();
	const QSet<Element *> &elementsToMove();
	const QSet<Conducer *> &conducersToMove();
	const QHash<Conducer *, Terminal *> &conducersToUpdate();
	QSet<Conducer *> selectedConducers() const;
	
	QUndoStack &undoStack();
	QGIManager &qgiManager();
	
	private slots:
	void slot_checkSelectionEmptinessChange();
	
	signals:
	void selectionEmptinessChanged();
};

/**
	Permet d'ajouter ou enlever le « poseur de conducteur », c'est-a-dire la
	droite en pointilles qui apparait lorsqu'on pose un conducteur entre deux
	bornes.
	@param true pour ajouter le poseur de conducteur, false pour l'enlever
*/
inline void Diagram::setConducer(bool pf) {
	if (pf) {
		if (!conducer_setter -> scene()) addItem(conducer_setter);
	} else {
		if (conducer_setter -> scene()) removeItem(conducer_setter);
	}
}

/**
	Specifie les coordonnees du point de depart du poseur de conducteur
	@param d Le nouveau point de depart du poseur de conducteur
*/
inline void Diagram::setConducerStart(QPointF d) {
	conducer_setter -> setLine(QLineF(d, conducer_setter -> line().p2()));
}

/**
	Specifie les coordonnees du point d'arrivee du poseur de conducteur
	@param d Le nouveau point d'arrivee du poseur de conducteur
*/
inline void Diagram::setConducerStop(QPointF a) {
	conducer_setter -> setLine(QLineF(conducer_setter -> line().p1(), a));
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
	@param ub true le cadre est pris en compte, false sinon
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
inline const QSet<Conducer *> &Diagram::conducersToMove() {
	if (!moved_elements_fetched) fetchMovedElements();
	return(conducers_to_move);
}

/// @return la liste des conducteurs a modifier (typiquement les conducteurs dont seul un element est deplace)
inline const QHash<Conducer *, Terminal *> &Diagram::conducersToUpdate() {
	if (!moved_elements_fetched) fetchMovedElements();
	return(conducers_to_update);
}

/// @return la pile d'annulations de ce schema
inline QUndoStack &Diagram::undoStack() {
	return(undo_stack);
}

/// @return le egstionnaire de QGraphicsItem de ce schema
inline QGIManager &Diagram::qgiManager() {
	return(qgi_manager);
}

inline bool Diagram::drawTerminals() const {
	return(draw_terminals);
}

#endif
