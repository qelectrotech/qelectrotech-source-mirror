#ifndef CONDUCTORAUTONUMEROTATION_H
#define CONDUCTORAUTONUMEROTATION_H

#include <conductor.h>

class NumStrategy;

class ConductorAutoNumerotation
{
	public:
	//constructors & destructor
	ConductorAutoNumerotation ();
	ConductorAutoNumerotation(Conductor *);
	~ConductorAutoNumerotation();

	//methods
	void setConductor(Conductor *);
	void numerate();

	protected:
	//methods
	void setNumStrategy (NumStrategy *);

	//attributes
	Conductor *conductor_;
	Diagram *diagram_;
	QSet <Conductor *> conductor_list;
	NumStrategy *strategy_;
};


class NumStrategy
{
	public:
	NumStrategy ();
	virtual ~NumStrategy ();
	virtual void createNumerotation(Conductor *, Diagram *) = 0; //cree la numerotation en fonction de la strategie utilisé

};


class SamePotential: public NumStrategy
{
	virtual void createNumerotation(Conductor *, Diagram *);
};

bool eachIsEqual (const QStringList &);

#endif // CONDUCTORAUTONUMEROTATION_H
