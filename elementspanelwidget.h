#ifndef ELEMENTS_PANEL_WIDGET_H
	#define ELEMENTS_PANEL_WIDGET_H
	#include <QtGui>
	#include "elementspanel.h"
	class ElementsPanelWidget : public QWidget {
		Q_OBJECT
		
		// constructeurs
		public:
		ElementsPanelWidget(QWidget * = 0);
		
		// attributs
		private:
		ElementsPanel *elements_panel;
		QToolBar *toolbar;
		
		// methodes
		public:
		inline ElementsPanel &elementsPanel() const { return(*elements_panel); }
	};
#endif
