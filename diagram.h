#ifndef SCHEMA_H
#define SCHEMA_H
#define GRILLE_X 10
#define GRILLE_Y 10
#define MARGIN   5.0
#include <QtGui>
#include <QtXml>
#include "qetapp.h"
#include "borderinset.h"
class Element;
class Terminal;
class Diagram : public QGraphicsScene {
	Q_OBJECT
	enum BorderOptions { EmptyBorder, Inset, Columns };
	public:
	Diagram(QObject * = 0);
	void drawBackground(QPainter *, const QRectF &);
	
	// fonctions relatives a la pose de conducteurs
	void poseConducer(bool);
	
	void setDepart (QPointF);
	void setArrivee(QPointF);
	
	// fonctions relatives a l'import / export XML
	QDomDocument toXml(bool = true);
	bool fromXml(QDomDocument &, QPointF = QPointF(), bool = true);
	
	// fonctions relatives aux options graphiques
	void setAffichageGrille(bool);
	bool displayGrid();
	void setUseBorder(bool);
	bool useBorder();
	void setBorderOptions(BorderOptions);
	BorderOptions borderOptions();
	BorderInset border_and_inset;
	QRectF border() const;
	QImage toImage(int = -1, int = -1, Qt::AspectRatioMode = Qt::KeepAspectRatio);
	QSize imageSize() const;
	
	private:
	QGraphicsLineItem *poseur_de_conducer;
	bool draw_grid;
	bool use_border;
	
	private slots:
	void slot_checkSelectionChange();
	void slot_checkSelectionEmptinessChange();
	
	signals:
	void selectionChanged();
	void selectionEmptinessChanged();
};

/**
	Permet d'ajouter ou enlever le « poseur de conducteur », c'est-a-dire la
	droite en pointilles qui apparait lorsqu'on pose un conducteur entre deux
	bornes.
	@param true pour ajouter le poseur de conducteur, false pour l'enlever
*/
inline void Diagram::poseConducer(bool pf) {
	if (pf) {
		if (!poseur_de_conducer -> scene()) addItem(poseur_de_conducer);
	} else {
		if (poseur_de_conducer -> scene()) removeItem(poseur_de_conducer);
	}
}

/**
	Specifie les coordonnees du point de depart du poseur de conducteur
	@param d Le nouveau point de depart du poseur de conducteur
*/
inline void Diagram::setDepart(QPointF d) {
	poseur_de_conducer -> setLine(QLineF(d, poseur_de_conducer -> line().p2()));
}

/**
	Specifie les coordonnees du point d'arrivee du poseur de conducteur
	@param d Le nouveau point d'arrivee du poseur de conducteur
*/
inline void Diagram::setArrivee(QPointF a) {
	poseur_de_conducer -> setLine(QLineF(poseur_de_conducer -> line().p1(), a));
}

/**
	Permet de specifier si la grille du schema doit etre dessinee ou non
	@param dg true pour afficher la grille, false pour ne pas l'afficher
*/
inline void Diagram::setAffichageGrille(bool dg) {
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

#endif
