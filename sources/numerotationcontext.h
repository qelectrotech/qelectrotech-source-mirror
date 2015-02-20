/*
	Copyright 2006-2015 The QElectroTech Team
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
#ifndef NUMEROTATIONCONTEXT_H
#define NUMEROTATIONCONTEXT_H

#include <QStringList>
#include <QVariant>
#include <QDomElement>

/**
	This class represents a numerotation context, i.e. the data (type, value, increase)
	of a numerotation at a given time. It is notably used by conductor
	to store the informations they need to do their autonumerotation.
 */
class NumerotationContext
{
	public:
	NumerotationContext ();
	NumerotationContext (QDomElement &);
	void clear();
	bool addValue(const QString &, const QVariant & = QVariant(1), const int = 1);
	QString operator[] (const int &) const;
	void operator << (const NumerotationContext &);
	int size() const;
	bool isEmpty() const;
	QStringList itemAt(const int) const;
	QString validRegExpNum () const;
	QString validRegExpNumber() const;
	bool keyIsAcceptable (const QString &) const;
	bool keyIsNumber(const QString &) const;
	QDomElement toXml(QDomDocument &, QString);
	void fromXml(QDomElement &);

	private:
	QStringList content_;
};

#endif // NUMEROTATIONCONTEXT_H
