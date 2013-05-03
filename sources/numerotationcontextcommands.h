#ifndef NUMEROTATIONCONTEXTCOMMANDS_H
#define NUMEROTATIONCONTEXTCOMMANDS_H

#include "numerotationcontext.h"
#include "diagram.h"

class NumStrategy;

/**
 * this class provide methods to handle content of NumerotationContext.
 */
class NumerotationContextCommands
{
	public:
	NumerotationContextCommands (Diagram *, const NumerotationContext &);
	~NumerotationContextCommands ();
	NumerotationContext next ();
	QString toRepresentedString ();

	private:
	void setNumStrategy (const QString &);

	Diagram *diagram_;
	NumerotationContext context_;
	NumStrategy *strategy_;
};

class NumStrategy
{
	public:
	NumStrategy (Diagram *);
	virtual ~NumStrategy ();
	virtual QString toRepresentedString (const QString) const = 0;
	virtual NumerotationContext next (const NumerotationContext &, const int) const = 0;

	protected:
	NumerotationContext nextString (const NumerotationContext &, const int) const;
	NumerotationContext nextNumber (const NumerotationContext &, const int) const;

	Diagram *diagram_;
};

class UnitNum: public NumStrategy
{
	public:
	UnitNum (Diagram *);
	QString toRepresentedString(const QString) const;
	NumerotationContext next (const NumerotationContext &, const int) const;
};

class TenNum: public NumStrategy
{
	public:
	TenNum (Diagram *);
	QString toRepresentedString(const QString) const;
	NumerotationContext next (const NumerotationContext &, const int) const;
};

class HundredNum: public NumStrategy
{
	public:
	HundredNum (Diagram *);
	QString toRepresentedString(const QString) const;
	NumerotationContext next (const NumerotationContext &, const int) const;
};

class StringNum: public NumStrategy
{
	public:
	StringNum (Diagram *);
	QString toRepresentedString(const QString) const;
	NumerotationContext next (const NumerotationContext &, const int) const;
};

class FolioNum: public NumStrategy
{
	public:
	FolioNum (Diagram *);
	QString toRepresentedString(const QString) const;
	NumerotationContext next (const NumerotationContext &, const int) const;
};

#endif // NUMEROTATIONCONTEXTCOMMANDS_H
