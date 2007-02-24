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
		inline void poseConducer(bool pf) {
			if (pf) {
				if (!poseur_de_conducer -> scene()) addItem(poseur_de_conducer);
			} else {
				if (poseur_de_conducer -> scene()) removeItem(poseur_de_conducer);
			}
		}
		inline void setDepart (QPointF d) { poseur_de_conducer -> setLine(QLineF(d, poseur_de_conducer -> line().p2())); }
		inline void setArrivee(QPointF a) { poseur_de_conducer -> setLine(QLineF(poseur_de_conducer -> line().p1(), a)); }
		
		// fonctions relatives a l'import / export XML
		QDomDocument toXml(bool = true);
		bool fromXml(QDomDocument &, QPointF = QPointF(), bool = true);
		
		// fonctions relatives aux options graphiques
		inline void setAffichageGrille(bool dg) { draw_grid = dg; }
		inline bool displayGrid() { return(draw_grid); }
		inline void setUseBorder(bool ub) { use_border = ub; }
		inline bool useBorder() { return(use_border); }
		inline void setBorderOptions(BorderOptions bo) {
			border_and_inset.displayBorder(!(bo & EmptyBorder));
			border_and_inset.displayColumns(bo & Columns);
			border_and_inset.displayInset(bo & Inset);
		}
		inline BorderOptions borderOptions() {
			BorderOptions retour = EmptyBorder;
			if (border_and_inset.insetIsDisplayed()) retour = (BorderOptions)(retour|Inset);
			if (border_and_inset.columnsAreDisplayed()) retour = (BorderOptions)(retour|Columns);
			return(retour);
		}
		
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
		
		signals:
		void selectionChanged();
	};
#endif
