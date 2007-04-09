#ifndef ELEMENTS_PANEL_WIDGET_H
#define ELEMENTS_PANEL_WIDGET_H
#include <QtGui>
#include "elementspanel.h"

/**
	Cette classe est un widget qui contient le panel d'elements surplombe d'une
	barre d'outils avec differentes actions pour gerer les elements.
*/
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
	
	public slots:
	void newElement();
};
#endif
