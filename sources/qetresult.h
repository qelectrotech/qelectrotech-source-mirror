/*
	Copyright 2006-2017 The QElectroTech Team
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
#ifndef QET_RESULT_H
#define QET_RESULT_H
#include <QString>

/**
	This class represents the result of a lambda operation. Technically, it is
	a mere boolean+error message pair.
*/
class QETResult {
	// Constructor, destructor
	public:
	QETResult();
	QETResult(const QString &error_message, bool = false);
	virtual ~QETResult();
	
	// methods
	public:
	bool isOk() const;
	void setResult(bool);
	QString errorMessage() const;
	void setErrorMessage(const QString &);
	
	// attributes
	private:
	bool result_;           ///< Embedded boolean value
	QString error_message_; ///< Embedded error message, typically used to explain what failed to users
};

#endif
