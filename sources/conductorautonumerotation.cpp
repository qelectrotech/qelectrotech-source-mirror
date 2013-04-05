#include <QStringList>
#include "conductorautonumerotation.h"
#include "conductorautonumerotationwidget.h"
#include "diagram.h"
#include "qetdiagrameditor.h"
#include "QGraphicsView"

/**
 * Constructor
 */
ConductorAutoNumerotation::ConductorAutoNumerotation() :
conductor_ (0),
diagram_ (0),
strategy_(0)
{}

/**
 *Constructor
 * @param c le conducteur a appliquer une numerotation
 */
ConductorAutoNumerotation::ConductorAutoNumerotation(Conductor *c) :
	conductor_ (c),
	diagram_ (c -> diagram()),
	conductor_list(c -> relatedPotentialConductors()),
	strategy_(0)
{}

/**
 *destructor
 */
ConductorAutoNumerotation::~ConductorAutoNumerotation() {
	delete strategy_;
}

/**
 * @param c le conducteur a appliquer une numerotation
 */
void ConductorAutoNumerotation::setConductor(Conductor *c) {
	conductor_ = c;
	diagram_ = c -> diagram();
	strategy_ = 0;
	conductor_list = c -> relatedPotentialConductors();
}

/**
 * @brief ConductorAutoNumerotation::numerate
 *execute la numerotation automatique du conducteur
 */
void ConductorAutoNumerotation::numerate() {
	if (conductor_ == 0) return;
	//ce conducteur est sur un potentiel existant
	if (conductor_list.size() >= 1) {
		setNumStrategy(new SamePotential);
		strategy_ -> createNumerotation(conductor_, diagram_);
	}
	//ce conducteur est le premier d'un nouveau potentiel
	else if (conductor_list.size() == 0) {
	}
}

/**
 * @brief ConductorAutoNumerotation::setNumStrategy
 *applique la strategy adéquate à la situation
 * @param strategy la class de la strategy à appliquer
 */
void ConductorAutoNumerotation::setNumStrategy(NumStrategy *strategy) {
	if (strategy_ != 0)
		delete strategy_;
	strategy_ = strategy;
}

NumStrategy::NumStrategy () {}
NumStrategy::~NumStrategy() {}

/**
 * @brief SamePotential::createNumerotation
 *crée la numerotation pour le conducteur @c connecté sur un potentiel deja existant
 */
void SamePotential::createNumerotation(Conductor *c, Diagram *d) {
	QSet <Conductor *> cl;
	QStringList strl;

	cl = c -> relatedPotentialConductors();
	foreach (const Conductor *cc, cl) strl<<(cc->text());
	//tout les textes sont identique
	if (eachIsEqual(strl)) {
		ConductorProperties cp;
		cp.text = strl.at(0);
		c -> setProperties(cp);
		c -> setText(strl.at(0));
	}
	//les textes ne sont pas identique
	else {
		ConductorAutoNumerotationWidget canw (c, cl, c -> diagramEditor());
		canw.exec();
	}
}

bool eachIsEqual (const QStringList &qsl) {
	foreach (const QString t, qsl) {
		if (qsl.at(0) != t) return false;
	}
	return true;
}
