/*
	Copyright 2006-2012 Xavier Guerrin
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
#ifndef QET_REGEXP_VALIDATOR_H
#define QET_REGEXP_VALIDATOR_H
#include <QRegExpValidator>
/**
	Cette classe agit comme un QRegExpValidator a ceci pres qu'elle emet un
	signal lorsqu'elle ne valide pas une saisie.
*/
class QETRegExpValidator : public QRegExpValidator {
	Q_OBJECT
	
	// constructeurs, destructeur
	public:
	QETRegExpValidator(QObject *);
	QETRegExpValidator(const QRegExp &, QObject *);
	virtual ~QETRegExpValidator();
	private:
	QETRegExpValidator(const QETRegExpValidator &);
	
	// methodes
	public:
	virtual QValidator::State validate(QString &, int &) const;
	
	signals:
	void validationFailed() const;
};
#endif
