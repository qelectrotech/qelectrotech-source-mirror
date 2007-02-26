#include "elementspanelwidget.h"

ElementsPanelWidget::ElementsPanelWidget(QWidget *parent) : QWidget(parent) {
	// initalise le panel d'elements
	elements_panel = new ElementsPanel(this);
	
	// initialise la barre d'outils
	toolbar = new QToolBar(this);
	toolbar -> setMovable(false);
	toolbar -> addAction(QIcon(":/ico/reload.png"), tr("Recharger les collections"), elements_panel, SLOT(reload()));
	
	// disposition verticale
	QVBoxLayout *vlayout = new QVBoxLayout(this);
	vlayout -> setMargin(0);
	vlayout -> setSpacing(0);
	vlayout -> addWidget(toolbar);
	vlayout -> addWidget(elements_panel);
	vlayout -> setStretchFactor(elements_panel, 75000);
	setLayout(vlayout);
}
