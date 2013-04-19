#ifndef CONDUCTORAUTONUMEROTATION_H
#define CONDUCTORAUTONUMEROTATION_H

#include <QObject>
#include "conductor.h"
#include "diagram.h"

class NumStrategy;

class ConductorAutoNumerotation: public QObject
{
	Q_OBJECT

	public:
	//constructors & destructor
	ConductorAutoNumerotation ();
	ConductorAutoNumerotation (Conductor *);
	ConductorAutoNumerotation (Diagram *);
	~ConductorAutoNumerotation();

	//methods
	void setConductor(Conductor *);
	void numerate();
	void removeNum_ofDiagram();

	public slots:
	void applyText(QString);

	
	protected:
	//methods
	void setNumStrategy ();

	//attributes
	Conductor *conductor_;
	Diagram *diagram_;
	QSet <Conductor *> conductor_list;
	NumStrategy *strategy_;
};


class NumStrategy: public QObject
{
	Q_OBJECT

	public:
	NumStrategy (Conductor *);
	virtual ~NumStrategy ();
	virtual void createNumerotation() = 0; //cree la numerotation en fonction de la strategie utilisé

	protected:
	Conductor *conductor_;
	QSet <Conductor *> c_list;
	Diagram *diagram_;

};

bool eachIsEqual (const QStringList &);

#endif // CONDUCTORAUTONUMEROTATION_H
