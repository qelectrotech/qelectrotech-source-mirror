#ifndef SCHEMAVUE_H
	#define SCHEMAVUE_H
	#include <QtGui>
	class Schema;
	#include "element.h"
	#include "conducteur.h"
	#define TAILLE_GRILLE 10
	/**
		Classe representant un SchemaVue electrique
		@todo creer une structure capable de retenir les differents composants du SchemaVue : elements, fils, indications eventuelles => revoir les SchemaVues
	*/
	class SchemaVue : public QGraphicsView {
		Q_OBJECT
		public:
		// constructeurs
		SchemaVue();
		SchemaVue(QWidget * = 0);
		
		// nouveaux attributs
		Schema *scene;
		
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
		bool antialiasing; // booleen indiquant s'il faut effectuer un antialiasing sur le rendu graphique du SchemaVue
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
