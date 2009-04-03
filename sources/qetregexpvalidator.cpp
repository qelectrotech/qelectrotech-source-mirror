#include "qetregexpvalidator.h"

/**
	Constructeur
	@param object QObject parent
*/
QETRegExpValidator::QETRegExpValidator(QObject *parent) : QRegExpValidator(parent) {
}

/**
	Constructeur
	@param regexp Expression reguliere a valider
	@param object QObject parent
*/
QETRegExpValidator::QETRegExpValidator(const QRegExp &regexp, QObject *parent) : QRegExpValidator(regexp, parent) {
}

/**
	Destructeur
*/
QETRegExpValidator::~QETRegExpValidator() {
}

/**
	@see QRegExpValidator::validate
	@see validationFailed()
	Emet le signal validationFailed si la validation echoue
*/
QValidator::State QETRegExpValidator::validate(QString &input, int &pos) const {
	QValidator::State result = QRegExpValidator::validate(input, pos);
	if (result == QValidator::Invalid) emit(validationFailed());
	return(result);
}
