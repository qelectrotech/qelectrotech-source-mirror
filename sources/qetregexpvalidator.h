/*
	Copyright 2006-2025 The QElectroTech Team
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
#include <QRegularExpressionValidator>
/**
	This class acts like a QRegularExpressionValidator
	except it emits a signal when the input validation fails.
*/
class QETRegExpValidator : public QRegularExpressionValidator {
	Q_OBJECT

	// constructors, destructor
	public:
	QETRegExpValidator(QObject *);
	QETRegExpValidator(const QRegularExpression &, QObject *);
	~QETRegExpValidator() override;
	private:
	QETRegExpValidator(const QETRegExpValidator &);

	// methods
	public:
	QValidator::State validate(QString &, int &) const override;

	signals:
	void validationFailed() const;
};
#endif
