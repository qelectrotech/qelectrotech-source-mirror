/*
	Copyright 2006-2026 The QElectroTech Team
	This file is part of QElectroTech.
	
	QElectroTech is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 2 of the License, or
	(at your option) any later version.
	
	QElectroTech is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.
	
	You should have received a copy of the GNU General Public License
	along with QElectroTech.  If not, see <http://www.gnu.org/licenses/>.
*/
#include "numerotationcontextcommands.h"

#include "../diagram.h"

/**
	@brief Constructor
*/
NumerotationContextCommands::NumerotationContextCommands(const NumerotationContext &nc, Diagram *d):
	diagram_ (d),
	context_ (nc),
	strategy_ (nullptr)
{}

/**
	@brief Destructor
*/
NumerotationContextCommands::~NumerotationContextCommands()
{
	if (strategy_) delete strategy_;
}

/**
	@brief NumerotationContextCommands::next
	@return the next numerotation context
*/
NumerotationContext NumerotationContextCommands::next()
{
	NumerotationContext contextnum;

	for (int i=0; i<context_.size(); ++i) {
		QStringList str = context_.itemAt(i);
		setNumStrategy(str.at(0));
		contextnum << strategy_ -> next(context_, i);

			//Wrap-and-carry: str still holds the pre-increment value, so
			//this checks the same condition WrapNum::next() used to decide
			//whether to wrap its own value back to 0.
		if (str.at(0) == "wrap" && str.size() > 4) {
			int modulus = str.at(4).toInt();
			if (modulus > 0 && (str.at(1).toInt() + str.at(2).toInt()) >= modulus)
				carry(contextnum, i - 1);
		}
	}
	return contextnum;
}

/**
	@brief NumerotationContextCommands::previous
	@return the previous numerotation context
*/
NumerotationContext NumerotationContextCommands::previous()
{
	NumerotationContext contextnum;

	for (int i=0; i<context_.size(); ++i) {
		QStringList str = context_.itemAt(i);
		setNumStrategy(str.at(0));
		contextnum << strategy_ -> previous(context_, i);

		if (str.at(0) == "wrap" && str.size() > 4) {
			int modulus = str.at(4).toInt();
			if (modulus > 0 && (str.at(1).toInt() - str.at(2).toInt()) < 0)
				borrow(contextnum, i - 1);
		}
	}
	return contextnum;
}

/**
	@brief NumerotationContextCommands::carry
	Add one unit to the nearest numeric part at or before from_index in
	contextnum, skipping non-numeric parts (e.g. a "." string separator)
	along the way. If that part is itself a wrap part and this pushes it
	to (or past) its own modulus, it wraps back to 0 and the carry
	cascades further back -- so wrap parts can be chained (e.g. seconds
	wrapping into minutes wrapping into hours).
	@param contextnum the context being built by next(); already contains
	entries for every index <= from_index
	@param from_index index to start looking from, going backwards
*/
void NumerotationContextCommands::carry(NumerotationContext &contextnum, int from_index)
{
	for (int j = from_index; j >= 0; --j) {
		QStringList strl = contextnum.itemAt(j);
		if (!contextnum.keyIsNumber(strl.at(0)))
			continue;

		int value = strl.at(1).toInt() + 1;
		if (strl.at(0) == "wrap" && strl.size() > 4) {
			int modulus = strl.at(4).toInt();
			if (modulus > 0 && value >= modulus) {
				contextnum.replaceValue(j, QString::number(value - modulus));
				carry(contextnum, j - 1);
				return;
			}
		}
		contextnum.replaceValue(j, QString::number(value));
		return;
	}
		//No preceding numeric part: the carry has nowhere to go and is dropped,
		//same as any other counter in this engine has no overflow tracking
		//beyond the parts the user actually configured.
}

/**
	@brief NumerotationContextCommands::borrow
	Inverse of carry(): subtract one unit from the nearest numeric part at
	or before from_index. If that part is itself a wrap part and this
	takes it below 0, it wraps to (modulus - 1) and the borrow cascades
	further back.
*/
void NumerotationContextCommands::borrow(NumerotationContext &contextnum, int from_index)
{
	for (int j = from_index; j >= 0; --j) {
		QStringList strl = contextnum.itemAt(j);
		if (!contextnum.keyIsNumber(strl.at(0)))
			continue;

		int value = strl.at(1).toInt() - 1;
		if (strl.at(0) == "wrap" && strl.size() > 4) {
			int modulus = strl.at(4).toInt();
			if (modulus > 0 && value < 0) {
				contextnum.replaceValue(j, QString::number(value + modulus));
				borrow(contextnum, j - 1);
				return;
			}
		}
		contextnum.replaceValue(j, QString::number(value));
		return;
	}
}

/**
	@brief NumerotationContextCommands::toFinalString
	@return the string represented by the numerotation context
*/
QString NumerotationContextCommands::toRepresentedString()
{
	QString num;
	if (context_.size()) {
		for (int i=0; i<context_.size(); i++) {
			QStringList  str = context_.itemAt(i);
			setNumStrategy(str.at(0));
			num += strategy_ -> toRepresentedString(str.at(1));
		}
		return num;
	}
	if (diagram_) return (diagram_ -> defaultConductorProperties.text);
	return QString();
}

/**
	@brief NumerotationContextCommands::setNumStrategy
	apply the good strategy relative to str
*/
void NumerotationContextCommands::setNumStrategy(const QString &str) {
	if (strategy_) delete strategy_;
	if (str == "unit") {
		strategy_ = new UnitNum(diagram_);
		return;
	}
	else if (str == "unitfolio") {
		strategy_ = new UnitFNum (diagram_);
		return;
	}
	else if (str == "ten") {
		strategy_ = new TenNum (diagram_);
		return;
	}
	else if (str == "tenfolio") {
		strategy_ = new TenFNum (diagram_);
		return;
	}
	else if (str == "hundred") {
		strategy_ = new HundredNum (diagram_);
		return;
	}
	else if (str == "hundredfolio") {
		strategy_ = new HundredFNum (diagram_);
		return;
	}
	else if (str == "wrap") {
		strategy_ = new WrapNum (diagram_);
		return;
	}
	else if (str == "string") {
		strategy_ = new StringNum (diagram_);
		return;
	}
	else if (str == "alpha") {
		strategy_ = new AlphaNum (diagram_);
		return;
	}
	else if (str == "idfolio") {
		strategy_ = new IdFolioNum (diagram_);
		return;
	}
	else if (str=="folio"){
		strategy_ = new FolioNum (diagram_);
		return;
	}
	else if (str=="plant"){
		strategy_ = new PlantNum (diagram_);
		return;
	}
	else if (str=="locmach"){
		strategy_ = new LocmachNum (diagram_);
		return;
	}
	else if (str=="elementline"){
		strategy_ = new ElementLineNum (diagram_);
		return;
	}
	else if (str=="elementcolumn"){
		strategy_ = new ElementColumnNum (diagram_);
		return;
	}
	else if (str=="elementprefix"){
		strategy_ = new ElementPrefixNum (diagram_);
		return;
	}
}



/**
	Constructor
*/
NumStrategy::NumStrategy (Diagram *d):
	diagram_ (d)
{}

NumStrategy::~NumStrategy()
{}

/**
	@brief NumStrategy::nextString
	@return the next value of nc at position i
*/
NumerotationContext NumStrategy::nextString (const NumerotationContext &nc,
					     const int i) const
{
	QStringList strl = nc.itemAt(i);
	NumerotationContext newnc;
	newnc.addValue(strl.at(0), strl.at(1), strl.at(2).toInt(), 0, 0, NumerotationContext::formatOf(strl));
	return (newnc);
}

/**
	@brief NumStrategy::nextNumber
	@return the next value of nc at position i
*/
NumerotationContext NumStrategy::nextNumber (const NumerotationContext &nc,
					     const int i) const
{
	QStringList strl = nc.itemAt(i);
	NumerotationContext newnc;
	QString value = QString::number( (strl.at(1).toInt()) + (strl.at(2).toInt()) );
	newnc.addValue(strl.at(0), value, strl.at(2).toInt(), strl.at(3).toInt(), 0, NumerotationContext::formatOf(strl));
	return (newnc);
}

/**
	@brief NumStrategy::previousNumber
	@return  the previous value of nc at position i
*/
NumerotationContext NumStrategy::previousNumber(const NumerotationContext &nc,
						const int i) const
{
	QStringList strl = nc.itemAt(i);
	NumerotationContext newnc;
	QString value = QString::number( (strl.at(1).toInt()) - (strl.at(2).toInt()) );
	newnc.addValue(strl.at(0), value, strl.at(2).toInt(), strl.at(3).toInt(), 0, NumerotationContext::formatOf(strl));
	return (newnc);
}

/**
	Constructor
*/
UnitNum::UnitNum(Diagram *d):
	NumStrategy(d)
{}

/**
	@brief UnitNum::toRepresentedString
	@return the represented string of num
*/
QString UnitNum::toRepresentedString(const QString num) const
{
	return (num);
}

/**
	@brief UnitNum::next
	@return the next NumerotationContext nc at position i
*/
NumerotationContext UnitNum::next (const NumerotationContext &nc, const int i) const
{
	return (nextNumber(nc, i));
}

/**
	@brief UnitNum::previous
	@return the previous NumerotationContext nc at posiiton i
*/
NumerotationContext UnitNum::previous(const NumerotationContext &nc, const int i) const
{
	return (previousNumber(nc, i));
}

/**
	Constructor
*/
UnitFNum::UnitFNum(Diagram *d):
	NumStrategy(d)
{}

/**
	@brief UnitFNum::toRepresentedString
	@return the represented string of num
*/
QString UnitFNum::toRepresentedString(const QString num) const
{
	return (num);
}

/**
	@brief UnitFNum::next
	@return the next NumerotationContext nc at position i
*/
NumerotationContext UnitFNum::next (const NumerotationContext &nc, const int i) const
{
	return (nextNumber(nc, i));
}

/**
	@brief UnitFNum::previous
	@return the previous NumerotationContext nc at posiiton i
*/
NumerotationContext UnitFNum::previous(const NumerotationContext &nc, const int i) const
{
	return (previousNumber(nc, i));
}

/**
	Constructor
*/
TenNum::TenNum (Diagram *d):
	NumStrategy (d)
{}

/**
	@brief TenNum::toRepresentedString
	@return the represented string of num
*/
QString TenNum::toRepresentedString(const QString num) const
{
	int numint = num.toInt();
	QString numstr = num;
	if (numint<10) numstr.prepend("0");
	return (numstr);
}

/**
	@brief TenNum::next
	@return the next NumerotationContext nc at position i
*/
NumerotationContext TenNum::next (const NumerotationContext &nc, const int i) const
{
	return (nextNumber(nc, i));
}

/**
	@brief TenNum::previous
	@return the previous NumerotationContext nc at posiiton i
*/
NumerotationContext TenNum::previous(const NumerotationContext &nc, const int i) const
{
	return (previousNumber(nc, i));
}

/**
	Constructor
*/
TenFNum::TenFNum (Diagram *d):
	NumStrategy (d)
{}

/**
	@brief TenFNum::toRepresentedString
	@return the represented string of num
*/
QString TenFNum::toRepresentedString(const QString num) const
{
	int numint = num.toInt();
	QString numstr = num;
	if (numint<10) numstr.prepend("0");
	return (numstr);
}

/**
	@brief TenFNum::next
	@return the next NumerotationContext nc at position i
*/
NumerotationContext TenFNum::next (const NumerotationContext &nc, const int i) const
{
	return (nextNumber(nc, i));
}

/**
	@brief TenFNum::previous
	@return the previous NumerotationContext nc at posiiton i
*/
NumerotationContext TenFNum::previous(const NumerotationContext &nc, const int i) const
{
	return (previousNumber(nc, i));
}


/**
	Constructor
*/
HundredNum::HundredNum (Diagram *d):
	NumStrategy (d)
{}

/**
	@brief HundredNum::toRepresentedString
	@return the represented string of num
*/
QString HundredNum::toRepresentedString(const QString num) const
{
	int numint = num.toInt();
	QString numstr = num;
	if (numint<100) {
		if (numint<10) {
			numstr.prepend("00");
		}
		else numstr.prepend("0");
	}
	return (numstr);
}

/**
	@brief HundredNum::next
	@return the next NumerotationContext nc at position i
*/
NumerotationContext HundredNum::next (const NumerotationContext &nc, const int i) const
{
	return (nextNumber(nc, i));
}

/**
	@brief HundredNum::previous
	@return the previous NumerotationContext nc at posiiton i
*/
NumerotationContext HundredNum::previous(const NumerotationContext &nc, const int i) const
{
	return (previousNumber(nc, i));
}

/**
	Constructor
*/
HundredFNum::HundredFNum (Diagram *d):
	NumStrategy (d)
{}

/**
	@brief HundredFNum::toRepresentedString
	@return the represented string of num
*/
QString HundredFNum::toRepresentedString(const QString num) const
{
	int numint = num.toInt();
	QString numstr = num;
	if (numint<100) {
		if (numint<10) {
			numstr.prepend("00");
		}
		else numstr.prepend("0");
	}
	return (numstr);
}

/**
	@brief HundredFNum::next
	@return the next NumerotationContext nc at position i
*/
NumerotationContext HundredFNum::next (const NumerotationContext &nc, const int i) const
{
	return (nextNumber(nc, i));
}

/**
	@brief HundredFNum::previous
	@return the previous NumerotationContext nc at posiiton i
*/
NumerotationContext HundredFNum::previous(const NumerotationContext &nc, const int i) const
{
	return (previousNumber(nc, i));
}

/**
	Constructor
*/
WrapNum::WrapNum (Diagram *d):
	NumStrategy (d)
{}

/**
	@brief WrapNum::toRepresentedString
	@return the represented string of num
*/
QString WrapNum::toRepresentedString(const QString num) const
{
	return (num);
}

/**
	@brief WrapNum::next
	Wraps this part's own value back to 0 every `modulus` values (carrying
	into the adjacent part is handled by NumerotationContextCommands::next(),
	which has visibility into the other parts).
	@return the next NumerotationContext nc at position i
*/
NumerotationContext WrapNum::next (const NumerotationContext &nc, const int i) const
{
	QStringList strl = nc.itemAt(i);
	NumerotationContext newnc;
	int increase = strl.at(2).toInt();
	int modulus = strl.size() > 4 ? strl.at(4).toInt() : 0;
	int new_value = strl.at(1).toInt() + increase;
	if (modulus > 0)
		new_value %= modulus;
	newnc.addValue(strl.at(0), QString::number(new_value), increase, strl.at(3).toInt(), modulus, NumerotationContext::formatOf(strl));
	return (newnc);
}

/**
	@brief WrapNum::previous
	@return the previous NumerotationContext nc at posiiton i
*/
NumerotationContext WrapNum::previous(const NumerotationContext &nc, const int i) const
{
	QStringList strl = nc.itemAt(i);
	NumerotationContext newnc;
	int increase = strl.at(2).toInt();
	int modulus = strl.size() > 4 ? strl.at(4).toInt() : 0;
	int new_value = strl.at(1).toInt() - increase;
	if (modulus > 0) {
		new_value %= modulus;
		if (new_value < 0)
			new_value += modulus;
	}
	newnc.addValue(strl.at(0), QString::number(new_value), increase, strl.at(3).toInt(), modulus, NumerotationContext::formatOf(strl));
	return (newnc);
}

/**
	Constructor
*/
StringNum::StringNum (Diagram *d):
	NumStrategy (d)
{}

/**
	@brief StringNum::toRepresentedString
	@return the represented string of num
*/
QString StringNum::toRepresentedString(const QString str) const
{
	return (str);
}

/**
	@brief StringNum::next
	@return the next NumerotationContext nc at position i
*/
NumerotationContext StringNum::next (const NumerotationContext &nc, const int i) const
{
	return (nextString(nc, i));
}

/**
	@brief StringNum::previous
	@return the previous NumerotationContext nc at posiiton i
*/
NumerotationContext StringNum::previous(const NumerotationContext &nc, const int i) const
{
	return (nextString(nc, i));
}

namespace
{
	/**
	 * @brief incrementAlpha
	 * Base-26 letter increment (a, b, ... z, aa, ab, ... az, ba, ...),
	 * the same algorithm as incrementing a spreadsheet column name.
	 * Carries right-to-left on 'z'/'Z' overflow; if the whole string
	 * overflows, a new leading letter is prepended (lowercase 'a').
	 * @param value : current alphabetic value; treated as "a" if empty.
	 * @return the next value.
	 */
	QString incrementAlpha(QString value)
	{
		if (value.isEmpty()) {
			return QStringLiteral("a");
		}

		int i = value.length() - 1;
		while (i >= 0 && (value.at(i) == QLatin1Char('z') || value.at(i) == QLatin1Char('Z'))) {
			value[i] = value.at(i).isUpper() ? QLatin1Char('A') : QLatin1Char('a');
			--i;
		}
		if (i < 0) {
			value.prepend(QLatin1Char('a'));
		} else {
			value[i] = QChar(value.at(i).unicode() + 1);
		}
		return value;
	}

	/**
	 * @brief decrementAlpha
	 * Inverse of incrementAlpha(): borrows right-to-left on 'a'/'A'
	 * underflow. Symmetric shrink case (e.g. "aa" -> "z"): once every
	 * position has borrowed, the leading letter is dropped rather than
	 * left as an extra 'z'. A single-letter value already at "a"/"A" has
	 * no representable predecessor and is left unchanged, the same way
	 * the numeric parts don't clamp but a blank label would be worse
	 * here than a value that stops decreasing.
	 * @param value : current alphabetic value; treated as "a" if empty.
	 * @return the previous value.
	 */
	QString decrementAlpha(QString value)
	{
		if (value.isEmpty()) {
			return QStringLiteral("a");
		}
		if (value.length() == 1) {
				//A single letter has no representable predecessor once it
				//reaches "a"/"A" -- clamp rather than mutate, since the loop
				//below would otherwise turn it into "z"/"Z" (borrowing past
				//the only position there is).
			if (value.at(0) == QLatin1Char('a') || value.at(0) == QLatin1Char('A')) {
				return value;
			}
			return QChar(value.at(0).unicode() - 1);
		}

		int i = value.length() - 1;
		while (i >= 0 && (value.at(i) == QLatin1Char('a') || value.at(i) == QLatin1Char('A'))) {
			value[i] = value.at(i).isUpper() ? QLatin1Char('Z') : QLatin1Char('z');
			--i;
		}
		if (i < 0) {
				//Every position borrowed: the whole value was "a...a", whose
				//predecessor is one fewer "z" (e.g. "aa" -> "z").
			value.remove(0, 1);
		} else {
			value[i] = QChar(value.at(i).unicode() - 1);
		}
		return value;
	}
}

/**
	Constructor
*/
AlphaNum::AlphaNum (Diagram *d):
	NumStrategy (d)
{}

/**
	@brief AlphaNum::toRepresentedString
	@return the represented string of str
*/
QString AlphaNum::toRepresentedString(const QString str) const
{
	return (str);
}

/**
	@brief AlphaNum::next
	@return the next NumerotationContext nc at position i
*/
NumerotationContext AlphaNum::next (const NumerotationContext &nc, const int i) const
{
	QStringList strl = nc.itemAt(i);
	NumerotationContext newnc;
	newnc.addValue(strl.at(0), incrementAlpha(strl.at(1)), strl.at(2).toInt(), 0, 0, NumerotationContext::formatOf(strl));
	return (newnc);
}

/**
	@brief AlphaNum::previous
	@return the previous NumerotationContext nc at posiiton i
*/
NumerotationContext AlphaNum::previous(const NumerotationContext &nc, const int i) const
{
	QStringList strl = nc.itemAt(i);
	NumerotationContext newnc;
	newnc.addValue(strl.at(0), decrementAlpha(strl.at(1)), strl.at(2).toInt(), 0, 0, NumerotationContext::formatOf(strl));
	return (newnc);
}

/**
	Constructor
*/
IdFolioNum::IdFolioNum (Diagram *d):
	NumStrategy (d)
{}

/**
	@brief IdFolioNum::toRepresentedString
	@return the represented string of num
*/
QString IdFolioNum::toRepresentedString(const QString str) const
{
	Q_UNUSED(str);
	return ("%id");
}

/**
	@brief IdFolioNum::next
	@return the next NumerotationContext nc at position i
*/
NumerotationContext IdFolioNum::next (const NumerotationContext &nc, const int i) const
{
	return (nextString(nc, i));
}

/**
	@brief IdFolioNum::previous
	@return the previous NumerotationContext nc at posiiton i
*/
NumerotationContext IdFolioNum::previous(const NumerotationContext &nc, const int i) const
{
	return (nextString(nc, i));
}

/**
	Constructor
*/
FolioNum::FolioNum (Diagram *d):
	NumStrategy (d)
{}

/**
	@brief FolioNum::toRepresentedString
	@return the represented string of folio
*/
QString FolioNum::toRepresentedString(const QString str) const
{
	Q_UNUSED(str);
	return ("%F");
}

/**
	@brief FolioNum::next
	@return the next NumerotationContext nc at position i
*/
NumerotationContext FolioNum::next (const NumerotationContext &nc, const int i) const
{
	return (nextString(nc, i));
}

/**
	@brief FolioNum::previous
	@return the previous NumerotationContext nc at posiiton i
*/
NumerotationContext FolioNum::previous(const NumerotationContext &nc, const int i) const
{
	return (nextString(nc, i));
}

/**
	Constructor
*/
PlantNum::PlantNum (Diagram *d):
	NumStrategy (d)
{}

/**
	@brief PlantNum::toRepresentedString
	@return the represented string of folio
*/
QString PlantNum::toRepresentedString(const QString str) const
{
	Q_UNUSED(str);
	return "%M";
}

/**
	@brief PlantNum::next
	@return the next NumerotationContext nc at position i
*/
NumerotationContext PlantNum::next (const NumerotationContext &nc, const int i) const
{
	return (nextString(nc, i));
}

/**
	@brief PlantNum::previous
	@return the previous NumerotationContext nc at posiiton i
*/
NumerotationContext PlantNum::previous(const NumerotationContext &nc, const int i) const
{
	return (nextString(nc, i));
}


/**
	Constructor
*/
LocmachNum::LocmachNum (Diagram *d):
	NumStrategy (d)
{}

/**
	@brief LocmachNum::toRepresentedString
	@return the represented string of folio
*/
QString LocmachNum::toRepresentedString(const QString str) const
{
	Q_UNUSED(str);
	return "%LM";
}

/**
	@brief LocmachNum::next
	@return the next NumerotationContext nc at position i
*/
NumerotationContext LocmachNum::next (const NumerotationContext &nc, const int i) const
{
	return (nextString(nc, i));
}

/**
	@brief LocmachNum::previous
	@return the previous NumerotationContext nc at posiiton i
*/
NumerotationContext LocmachNum::previous(const NumerotationContext &nc, const int i) const
{
	return (nextString(nc, i));
}


/**
	Constructor
*/
ElementLineNum::ElementLineNum (Diagram *d):
	NumStrategy (d)
{}

/**
	@brief ElementLineNum::toRepresentedString
	@return the represented string of folio
*/
QString ElementLineNum::toRepresentedString(const QString str) const
{
	Q_UNUSED(str);
	return "%l";
}

/**
	@brief ElementLineNum::next
	@return the next NumerotationContext nc at position i
*/
NumerotationContext ElementLineNum::next (const NumerotationContext &nc, const int i) const
{
	return (nextString(nc, i));
}

/**
	@brief ElementLineNum::previous
	@return the previous NumerotationContext nc at posiiton i
*/
NumerotationContext ElementLineNum::previous(const NumerotationContext &nc, const int i) const
{
	return (nextString(nc, i));
}

/**
	Constructor
*/
ElementColumnNum::ElementColumnNum (Diagram *d):
	NumStrategy (d)
{}

/**
	@brief ElementColumnNum::toRepresentedString
	@return the represented string of folio
*/
QString ElementColumnNum::toRepresentedString(const QString str) const
{
	Q_UNUSED(str);
	return "%c";
}

/**
	@brief ElementColumnNum::next
	@return the next NumerotationContext nc at position i
*/
NumerotationContext ElementColumnNum::next (const NumerotationContext &nc, const int i) const
{
	return (nextString(nc, i));
}

/**
	@brief ElementColumnNum::previous
	@return the previous NumerotationContext nc at posiiton i
*/
NumerotationContext ElementColumnNum::previous(const NumerotationContext &nc, const int i) const
{
	return (nextString(nc, i));
}

/**
	Constructor
*/
ElementPrefixNum::ElementPrefixNum (Diagram *d):
	NumStrategy (d)
{}

/**
	@brief ElementPrefixNum::toRepresentedString
	@return the represented string of folio
*/
QString ElementPrefixNum::toRepresentedString(const QString str) const
{
	Q_UNUSED(str);
	return "%prefix";
}

/**
	@brief ElementPrefixNum::next
	@return the next NumerotationContext nc at position i
*/
NumerotationContext ElementPrefixNum::next (const NumerotationContext &nc, const int i) const
{
	return (nextString(nc, i));
}

/**
	@brief ElementPrefixNum::previous
	@return the previous NumerotationContext nc at posiiton i
*/
NumerotationContext ElementPrefixNum::previous(const NumerotationContext &nc, const int i) const
{
	return (nextString(nc, i));
}

