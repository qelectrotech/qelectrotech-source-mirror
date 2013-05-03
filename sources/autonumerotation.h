#ifndef AUTONUMEROTATION_H
#define AUTONUMEROTATION_H

#include "diagram.h"

class AutoNumerotation: public QObject
{
	Q_OBJECT

	public:
	AutoNumerotation(Diagram *);
	virtual void numerate() = 0;

	public slots:
	virtual void applyText(QString) = 0;

	protected:
	Diagram *diagram_;
	NumerotationContext num_context;
};

#endif // AUTONUMEROTATION_H
