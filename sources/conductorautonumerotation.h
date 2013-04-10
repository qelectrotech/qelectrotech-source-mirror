#ifndef CONDUCTORAUTONUMEROTATION_H
#define CONDUCTORAUTONUMEROTATION_H

#include <QObject>
#include "conductor.h"
#include "diagram.h"

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
	void setText(QString);

	protected:
	//methods
	void setNumStrategy ();

	//attributes
	Conductor *conductor_;
	Diagram *diagram_;
	QSet <Conductor *> conductor_list;
	NumStrategy *strategy_;

	private:
	bool strategy_is_set;
};


class NumStrategy: public QObject
{
	Q_OBJECT

	public:
	NumStrategy (Conductor *);
	virtual ~NumStrategy ();
	virtual void createNumerotation() = 0; //cree la numerotation en fonction de la strategie utilisé

	public slots:
	void applyText(QString);

	protected:
	Conductor *conductor_;
	QSet <Conductor *> c_list;
	Diagram *diagram_;

};


class SamePotential: public NumStrategy
{
	public:
	SamePotential (Conductor *);
	virtual void createNumerotation();
};

bool eachIsEqual (const QStringList &);

#endif // CONDUCTORAUTONUMEROTATION_H
