#ifndef DIAGRAMVIEW_H
	#define DIAGRAMVIEW_H
	#include <QtGui>
	class Diagram;
	#include "element.h"
	#include "conducer.h"
	#define TAILLE_GRILLE 10
	/**
		Classe representant graphiquement un schema electrique
	*/
	class DiagramView : public QGraphicsView {
		Q_OBJECT
		public:
		// constructeurs
		DiagramView();
		DiagramView(QWidget * = 0);
		
		// nouveaux attributs
		Diagram *scene;
		
		// methodes publiques
		bool antialiased() const;
		void setAntialiasing(bool);
		bool ouvrir(QString, int * = NULL);
		void closeEvent(QCloseEvent *);
		QString nom_fichier;
		bool enregistrer();
		bool enregistrer_sous();
		
		private:
		bool private_enregistrer(QString &);
		void initialise();
		bool antialiasing; // booleen indiquant s'il faut effectuer un antialiasing sur le rendu graphique du DiagramView
		QList<QGraphicsItem *> garbage;
		
		void throwToGarbage(QGraphicsItem *);
		void mousePressEvent(QMouseEvent *);
		void dragEnterEvent(QDragEnterEvent *);
		void dragLeaveEvent(QDragLeaveEvent *);
		void dragMoveEvent(QDragMoveEvent *);
		void dropEvent(QDropEvent *);
		
		signals:
		void selectionChanged();
		void antialiasingChanged();
		void modeChanged();
		
		public slots:
		void selectNothing();
		void selectAll();
		void selectInvert();
		void supprimer();
		void pivoter();
		void setVisualisationMode();
		void setSelectionMode();
		void zoomPlus();
		void zoomMoins();
		void zoomFit();
		void zoomReset();
		void couper();
		void copier();
		void coller();
		
		private slots:
		void flushGarbage();
		void slot_selectionChanged();
	};
#endif
