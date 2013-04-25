#include <QStringList>
#include "conductorautonumerotation.h"
#include "conductorautonumerotationwidget.h"
#include "qetdiagrameditor.h"
#include "QGraphicsView"
#include "diagramcommands.h"
#include "qetapp.h"

/**
 * Constructor
 */
ConductorAutoNumerotation::ConductorAutoNumerotation() :
conductor_ (0),
diagram_ (0),
strategy_ (0)
{}

/**
 *Constructor
 * @param c the conductor to apply automatic numerotation
 */
ConductorAutoNumerotation::ConductorAutoNumerotation(Conductor *c) :
	conductor_ (c),
	diagram_ (c -> diagram()),
	conductor_list(c -> relatedPotentialConductors()),
	strategy_ (0)
{}

/**
 * Constructor
 * @param d a diagram to apply automatic numerotation
 */
ConductorAutoNumerotation::ConductorAutoNumerotation(Diagram *d) :
	conductor_ (0),
	diagram_ (d),
	strategy_ (0)
{}

/**
 *destructor
 */
ConductorAutoNumerotation::~ConductorAutoNumerotation() {
	delete strategy_;
}

/**
 * @param c the conductor to apply automatic numerotation
 */
void ConductorAutoNumerotation::setConductor(Conductor *c) {
	conductor_ = c;
	diagram_ = c -> diagram();
	conductor_list = c -> relatedPotentialConductors();
	if (strategy_) delete strategy_;
}

/**
 * @brief ConductorAutoNumerotation::numerate
 * execute the automatic numerotation
 */
void ConductorAutoNumerotation::numerate() {
	if (!conductor_) return;
	//conductor is on an existing potential
	if (conductor_list.size() >= 1 ) {
		QStringList strl;
		foreach (const Conductor *cc, conductor_list) strl<<(cc->text());
		//the texts is identicals
		if (eachIsEqual(strl)) {
			ConductorProperties cp;
			cp.text = strl.at(0);
			conductor_ -> setProperties(cp);
			conductor_ -> setText(strl.at(0));
		}
		//the texts isn't identicals
		else {
			ConductorAutoNumerotationWidget *canw = new ConductorAutoNumerotationWidget(conductor_, conductor_list, conductor_ -> diagramEditor());
			connect(canw, SIGNAL(textIsSelected(QString)),
					this, SLOT(applyText(QString)));
			canw -> exec();
		}
	}
	//conductor create a new potential
	else {
	}
}

/**
 * @brief ConductorAutoNumerotation::setText
 * apply the text @t by the strategy
 */
void ConductorAutoNumerotation::applyText(QString t) {
	if (!conductor_) return;
	if (conductor_list.empty()) {
		//initialize the corresponding UndoCommand object
		ChangeConductorPropertiesCommand *ccpc = new ChangeConductorPropertiesCommand (conductor_);
		ccpc -> setOldSettings (conductor_ -> properties());
		ConductorProperties cp = conductor_ -> properties();
		cp.text = t;
		ccpc -> setNewSettings(cp);
		diagram_ -> undoStack().push(ccpc);
	}
	else {
		QSet <Conductor *> clist = conductor_list;
		clist << conductor_;
		QList <ConductorProperties> old_properties, new_properties;
		ConductorProperties cp;

		foreach (Conductor *c, clist) {
			old_properties << c -> properties();
			cp = c -> properties();
			cp.text = t;
			new_properties << cp;
		}
		//initialize the corresponding UndoCommand object
		ChangeSeveralConductorsPropertiesCommand *cscpc = new ChangeSeveralConductorsPropertiesCommand(clist);
		cscpc -> setOldSettings(old_properties);
		cscpc -> setNewSettings(new_properties);
		diagram_ -> undoStack().push(cscpc);
	}
}

/**
 * @brief ConductorAutoNumerotation::setNumStrategy
 * apply the good strategy relative to the conductor
 */
void ConductorAutoNumerotation::setNumStrategy() {}


/**
 * @brief Set the default text to all potentials of the diagram
 * @param dg the diagram
 */
void ConductorAutoNumerotation::removeNum_ofDiagram() {
	if (!diagram_) return;
	//Get all potentials presents in diagram
	QList <QSet <Conductor *> > potential_list = diagram_ -> potentials();
	//Browse all potentials and set the default text
	for (int i=0; i < potential_list.size(); i++) {
		setConductor (potential_list.at(i).toList().first());
		applyText (diagram_ -> defaultConductorProperties.text);
	}
}


/**
 * Constructor
 */
NumStrategy::NumStrategy (Conductor *c):
	conductor_ (c),
	c_list (c -> relatedPotentialConductors()),
	diagram_ (c -> diagram())
{}

NumStrategy::~NumStrategy() {}


/**
 * @return true if every text of qsl is identical, else false.
 */
bool eachIsEqual (const QStringList &qsl) {
	foreach (const QString t, qsl) {
		if (qsl.at(0) != t) return false;
	}
	return true;
}
