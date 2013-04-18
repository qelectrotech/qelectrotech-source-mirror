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
strategy_ (0),
strategy_is_set (false)
{}

/**
 *Constructor
 * @param c the conductor to apply automatic numerotation
 */
ConductorAutoNumerotation::ConductorAutoNumerotation(Conductor *c) :
	conductor_ (c),
	diagram_ (c -> diagram()),
	conductor_list(c -> relatedPotentialConductors()),
	strategy_ (0),
	strategy_is_set (false)
{
	setNumStrategy();
}

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
	setNumStrategy();
}

/**
 * @brief ConductorAutoNumerotation::numerate
 * execute the automatic numerotation
 */
void ConductorAutoNumerotation::numerate() {
	if (strategy_is_set)
		strategy_ -> createNumerotation();
}

/**
 * @brief ConductorAutoNumerotation::setText
 * apply the text @t by the strategy
 */
void ConductorAutoNumerotation::setText(QString t) {
	if (strategy_is_set)
		strategy_ -> applyText(t);
}

/**
 * @brief ConductorAutoNumerotation::setNumStrategy
 * apply the good strategy relative to the conductor
 */
void ConductorAutoNumerotation::setNumStrategy() {
	if (strategy_ != 0)
		delete strategy_;

	if (conductor_list.size() >= 1) {
		strategy_ = new SamePotential (conductor_);
		strategy_is_set = true;
	}
	else if (conductor_list.size() == 0) {
		strategy_is_set = false;
	}
}


/**
 * @brief Set the default text to all conductors of the diagram
 * @param dg the diagram
 */
void ConductorAutoNumerotation::removeNum_ofDiagram(Diagram *dg) {
	// Get all conductors presents in diagram
	QList<Conductor *> Conductors = dg -> content().conductors();
	// Browse all conductors and set the default value
	for (int i=0; i<Conductors.count(); i++) {
		Conductors.at(i) -> setText( dg ->defaultConductorProperties.text );
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
 * @brief ConductorAutoNumerotationWidget::applyText
 *apply the text @t on every conductors of @c_list and @conductor_
 */
void NumStrategy::applyText(QString t) {
	if (!c_list.empty()) {
		QSet <Conductor *> conductorslist = c_list;
		conductorslist << conductor_;
		QList <ConductorProperties> old_properties, new_properties;
		ConductorProperties cp;

		foreach (Conductor *c, conductorslist) {
			old_properties << c -> properties();
			cp = c -> properties();
			cp.text = t;
			c -> setProperties(cp);
			new_properties << c -> properties();
			c -> setText(t);
		}
		//initialize the corresponding UndoCommand object
		ChangeSeveralConductorsPropertiesCommand *cscpc = new ChangeSeveralConductorsPropertiesCommand(conductorslist);
		cscpc -> setOldSettings(old_properties);
		cscpc -> setNewSettings(new_properties);
		diagram_ -> undoStack().push(cscpc);
	}
	else {
		//initialize the corresponding UndoCommand object
		ChangeConductorPropertiesCommand *ccpc = new ChangeConductorPropertiesCommand (conductor_);
		ConductorProperties cp;
		cp = conductor_ ->properties();
		ccpc -> setOldSettings(cp);
		cp.text = t;
		ccpc -> setNewSettings(cp);
		diagram_ -> undoStack().push(ccpc);
		conductor_ -> setProperties(cp);
		conductor_ -> setText(t);
	}
}


/**
 * Constructor
 */
SamePotential::SamePotential(Conductor *c):
	NumStrategy(c)
{}

/**
 * @brief SamePotential::createNumerotation
 *create the numerotation for the conductor @c connected on an existing potential
 */
void SamePotential::createNumerotation() {
	QStringList strl;

	foreach (const Conductor *cc, c_list) strl<<(cc->text());
	//the texts is identicals
	if (eachIsEqual(strl)) {
		ConductorProperties cp;
		cp.text = strl.at(0);
		conductor_ -> setProperties(cp);
		conductor_ -> setText(strl.at(0));
	}
	//the texts isn't identicals
	else {
		ConductorAutoNumerotationWidget *canw = new ConductorAutoNumerotationWidget(conductor_, c_list, conductor_ -> diagramEditor());
		connect(canw, SIGNAL(textIsSelected(QString)),
				this, SLOT(applyText(QString)));
		canw -> exec();
	}
}

/**
 * @return true if every text of qsl is identical, else false.
 */
bool eachIsEqual (const QStringList &qsl) {
	foreach (const QString t, qsl) {
		if (qsl.at(0) != t) return false;
	}
	return true;
}
