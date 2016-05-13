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
#include "qetresult.h"

/**
	Construct a default "true" QET result without an error message.
*/
QETResult::QETResult() :
	result_(true)
{
}

/**
	Construct a QET result embedding \a error_message and \a result (defaults
	to false).
*/
QETResult::QETResult(const QString &error_message, bool result) :
	result_(result),
	error_message_(error_message)
{
}

/**
	Destructor
*/
QETResult::~QETResult() {
}

/**
	@return the boolean value embedded within this result.
*/
bool QETResult::isOk() const {
	return(result_);
}

/**
	Embed \a result.
*/
void QETResult::setResult(bool result) {
	result_ = result;
}

/**
	@return the error message embedded within this result.
*/
QString QETResult::errorMessage() const {
	return(error_message_);
}

/**
	Embed \a error_message wihthin this result.
*/
void QETResult::setErrorMessage(const QString &error_message) {
	error_message_ = error_message;
}
