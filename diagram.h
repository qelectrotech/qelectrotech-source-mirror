#ifndef SCHEMA_H
#define SCHEMA_H
#include <QtGui>
#include <QtXml>
#include "qetdiagrameditor.h"
#include "borderinset.h"
#include "qgimanager.h"
#include "conductorproperties.h"
class Element;
class Terminal;
class Conductor;
class DiagramTextItem;
class DiagramContent;
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
	QSet<DiagramTextItem *> texts_to_move;
	QGIManager qgi_manager;
	QUndoStack undo_stack;
	bool draw_terminals;
	
	// methodes
	protected:
	virtual void drawBackground(QPainter *, const QRectF &);
	virtual void keyPressEvent(QKeyEvent *);
	virtual void keyReleaseEvent(QKeyEvent *);
	
	public:
	static bool clipboardMayContainDiagram();
	
	// fonctions relatives a la pose de conducteurs
	void setConductor(bool);
	void setConductorStart (QPointF);
	void setConductorStop(QPointF);
	
	// fonctions relatives a l'import / export XML
	QDomDocument toXml(bool = true);
	bool fromXml(QDomDocument &, QPointF = QPointF(), bool = true, DiagramContent * = NULL);
	
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
	bool toPaintDevice(QPaintDevice &, int = -1, int = -1, Qt::AspectRatioMode = Qt::KeepAspectRatio);
	QSize imageSize() const;
	
	void invalidateMovedElements();
	void fetchMovedElements();
	const QSet<Element *> &elementsToMove();
	const QSet<Conductor *> &conductorsToMove();
	const QHash<Conductor *, Terminal *> &conductorsToUpdate();
	const QSet<DiagramTextItem *> &textsToMove();
	QSet<Conductor *> selectedConductors() const;
	DiagramContent content() const;
	DiagramContent selectedContent();
	void moveElements(const QPointF &, QGraphicsItem * = NULL);
	
	QUndoStack &undoStack();
	QGIManager &qgiManager();
	
	private slots:
	void slot_checkSelectionEmptinessChange();
	
	signals:
	/**
		Ce signal est emis lorsque la selection passe de l'etat rempli (par un
		nombre quelconque d'elements et conducteurs) a l'etat vide et
		vice-versa.
	*/
	void selectionEmptinessChanged();
};

/**
	Permet d'ajouter ou enlever le « poseur de conducteur », c'est-a-dire la
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
inline const QSet<DiagramTextItem *> &Diagram::textsToMove() {
	if (!moved_elements_fetched) fetchMovedElements();
	return(texts_to_move);
}

/// @return la pile d'annulations de ce schema
inline QUndoStack &Diagram::undoStack() {
	return(undo_stack);
}

/// @return le egstionnaire de QGraphicsItem de ce schema
inline QGIManager &Diagram::qgiManager() {
	return(qgi_manager);
}

/// @return true si les bornes sont affichees, false sinon
inline bool Diagram::drawTerminals() const {
	return(draw_terminals);
}

#endif
