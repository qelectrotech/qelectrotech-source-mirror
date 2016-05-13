/*
	Copyright 2006-2016 The QElectroTech Team
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
#include "diagram.h"

/**
 * @brief Constructor
 */
NumerotationContextCommands::NumerotationContextCommands(const NumerotationContext &nc, Diagram *d):
	diagram_ (d),
	context_ (nc),
	strategy_ (NULL)
{}

/**
 * @brief Destructor
 */
NumerotationContextCommands::~NumerotationContextCommands() {
	if (strategy_) delete strategy_;
}

/**
 * @brief NumerotationContextCommands::next
 * @return the next numerotation context
 */
NumerotationContext NumerotationContextCommands::next() {
	NumerotationContext contextnum;

	for (int i=0; i<context_.size(); ++i) {
		QStringList str = context_.itemAt(i);
		setNumStrategy(str.at(0));
		contextnum << strategy_ -> next(context_, i);
	}
	return contextnum;
}

/**
 * @brief NumerotationContextCommands::previous
 * @return the previous numerotation context
 */
NumerotationContext NumerotationContextCommands::previous() {
	NumerotationContext contextnum;

	for (int i=0; i<context_.size(); ++i) {
		QStringList str = context_.itemAt(i);
		setNumStrategy(str.at(0));
		contextnum << strategy_ -> previous(context_, i);
	}
	return contextnum;
}

/**
 * @brief NumerotationContextCommands::toFinalString
 * @return the string represented by the numerotation context
 */
QString NumerotationContextCommands::toRepresentedString() {
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
 * @brief NumerotationContextCommands::setNumStrategy
 * apply the good strategy relative to @str
 */
void NumerotationContextCommands::setNumStrategy(const QString &str) {
	if (strategy_) delete strategy_;
	if (str == "unit") {
		strategy_ = new UnitNum(diagram_);
		return;
	}
	else if (str == "ten") {
		strategy_ = new TenNum (diagram_);
		return;
	}
	else if (str == "hundred") {
		strategy_ = new HundredNum (diagram_);
		return;
	}
	else if (str == "string") {
		strategy_ = new StringNum (diagram_);
		return;
	}
	else if (str == "folio") {
		strategy_ = new FolioNum (diagram_);
		return;
	}
}



/**
 * Constructor
 */
NumStrategy::NumStrategy (Diagram *d):
	diagram_ (d)
{}

NumStrategy::~NumStrategy() {}

/**
 * @brief NumStrategy::nextString
 * @return the next value of @nc at position @i
 */
NumerotationContext NumStrategy::nextString (const NumerotationContext &nc, const int i) const {
	QStringList strl = nc.itemAt(i);
	NumerotationContext newnc;
	newnc.addValue(strl.at(0), strl.at(1), strl.at(2).toInt());
	return (newnc);
}

/**
 * @brief NumStrategy::nextNumber
 * @return the next value of @nc at position @i
 */
NumerotationContext NumStrategy::nextNumber (const NumerotationContext &nc, const int i) const {
	QStringList strl = nc.itemAt(i);
	NumerotationContext newnc;
	QString value = QString::number( (strl.at(1).toInt()) + (strl.at(2).toInt()) );
	newnc.addValue(strl.at(0), value, strl.at(2).toInt());
	return (newnc);
}

/**
 * @brief NumStrategy::previousNumber
 * @return  the previous value of @nc at position @i
 */
NumerotationContext NumStrategy::previousNumber(const NumerotationContext &nc, const int i) const {
	QStringList strl = nc.itemAt(i);
	NumerotationContext newnc;
	QString value = QString::number( (strl.at(1).toInt()) - (strl.at(2).toInt()) );
	newnc.addValue(strl.at(0), value, strl.at(2).toInt());
	return (newnc);
}

/**
 * Constructor
 */
UnitNum::UnitNum(Diagram *d):
	NumStrategy(d)
{}

/**
 * @brief UnitNum::toRepresentedString
 * @return the represented string of num
 */
QString UnitNum::toRepresentedString(const QString num) const {
	return (num);
}

/**
 * @brief UnitNum::next
 * @return the next NumerotationContext nc at position i
 */
NumerotationContext UnitNum::next (const NumerotationContext &nc, const int i) const {
	return (nextNumber(nc, i));
}

/**
 * @brief UnitNum::previous
 * @return the previous NumerotationContext nc at posiiton i
 */
NumerotationContext UnitNum::previous(const NumerotationContext &nc, const int i) const {
	return (previousNumber(nc, i));
}

/**
 * Constructor
 */
TenNum::TenNum (Diagram *d):
	NumStrategy (d)
{}

/**
 * @brief TenNum::toRepresentedString
 * @return the represented string of num
 */
QString TenNum::toRepresentedString(const QString num) const {
	int numint = num.toInt();
	QString numstr = num;
	if (numint<10) numstr.prepend("0");
	return (numstr);
}

/**
 * @brief TenNum::next
 * @return the next NumerotationContext nc at position i
 */
NumerotationContext TenNum::next (const NumerotationContext &nc, const int i) const {
	return (nextNumber(nc, i));
}

/**
 * @brief TenNum::previous
 * @return the previous NumerotationContext nc at posiiton i
 */
NumerotationContext TenNum::previous(const NumerotationContext &nc, const int i) const {
	return (previousNumber(nc, i));
}

/**
 * Constructor
 */
HundredNum::HundredNum (Diagram *d):
	NumStrategy (d)
{}

/**
 * @brief HundredNum::toRepresentedString
 * @return the represented string of num
 */
QString HundredNum::toRepresentedString(const QString num) const {
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
 * @brief HundredNum::next
 * @return the next NumerotationContext nc at position i
 */
NumerotationContext HundredNum::next (const NumerotationContext &nc, const int i) const {
	return (nextNumber(nc, i));
}

/**
 * @brief HundredNum::previous
 * @return the previous NumerotationContext nc at posiiton i
 */
NumerotationContext HundredNum::previous(const NumerotationContext &nc, const int i) const {
	return (previousNumber(nc, i));
}

/**
 * Constructor
 */
StringNum::StringNum (Diagram *d):
	NumStrategy (d)
{}

/**
 * @brief StringNum::toRepresentedString
 * @return the represented string of num
 */
QString StringNum::toRepresentedString(const QString str) const {
	return (str);
}

/**
 * @brief StringNum::next
 * @return the next NumerotationContext nc at position i
 */
NumerotationContext StringNum::next (const NumerotationContext &nc, const int i) const {
	return (nextString(nc, i));
}

/**
 * @brief StringNum::previous
 * @return the previous NumerotationContext nc at posiiton i
 */
NumerotationContext StringNum::previous(const NumerotationContext &nc, const int i) const {
	return (nextString(nc, i));
}


/**
 * Constructor
 */
FolioNum::FolioNum (Diagram *d):
	NumStrategy (d)
{}

/**
 * @brief FolioNum::toRepresentedString
 * @return the represented string of num
 */
QString FolioNum::toRepresentedString(const QString str) const {
	Q_UNUSED(str);
	return (QString::number(diagram_ -> folioIndex() + 1));
}

/**
 * @brief FolioNum::next
 * @return the next NumerotationContext nc at position i
 */
NumerotationContext FolioNum::next (const NumerotationContext &nc, const int i) const {
	return (nextString(nc, i));
}

/**
 * @brief FolioNum::previous
 * @return the previous NumerotationContext nc at posiiton i
 */
NumerotationContext FolioNum::previous(const NumerotationContext &nc, const int i) const {
	return (nextString(nc, i));
}

