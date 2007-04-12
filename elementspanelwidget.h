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
	
	// constructeurs, destructeur
	public:
	ElementsPanelWidget(QWidget * = 0);
	virtual ~ElementsPanelWidget();
	
	private:
	ElementsPanelWidget(const ElementsPanelWidget &);
	
	// attributs
	private:
	ElementsPanel *elements_panel;
	QToolBar *toolbar;
	
	// methodes
	public:
	inline ElementsPanel &elementsPanel() const;
	
	public slots:
	void newElement();
};

/**
	@return Le panel d'elements imbrique dans ce widget
*/
inline ElementsPanel &ElementsPanelWidget::elementsPanel() const {
	return(*elements_panel);
}

#endif
