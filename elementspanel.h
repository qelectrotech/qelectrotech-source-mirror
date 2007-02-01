#ifndef PANELAPPAREILS_H
	#define PANELAPPAREILS_H
	#include <QtGui>
	#include "qetapp.h"
	/**
		Cette classe represente le panel d'appareils (en tant qu'element
		graphique) dans lequel l'utilisateur choisit les composants de
		son choix et les depose sur le schema par drag'n drop.
	*/
	class ElementsPanel : public QTreeWidget {
		Q_OBJECT
		public:
		ElementsPanel(QWidget * = 0);
		static QString categoryName(QDir &);
		public slots:
		void dragMoveEvent(QDragMoveEvent *);
		void dropEvent(QDropEvent *);
		void startDrag(Qt::DropActions);
		private:
		void addFile(QTreeWidgetItem *, QString);
		void addDir(QTreeWidgetItem *, QString, QString = QString());
	};
#endif
