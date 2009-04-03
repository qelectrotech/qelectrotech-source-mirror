#include "qfilenameedit.h"
#include  "qetregexpvalidator.h"
#include <QKeyEvent>
#include <QRegExp>
#include <QToolTip>

/**
	Constructeur
	@param parent QWidget parent de ce champ de texte
*/
QFileNameEdit::QFileNameEdit(QWidget *parent) : QLineEdit(parent) {
	init();
}

/**
	Constructeur
	@param contents Contenu initial du champ
	@param parent QWidget parent de ce champ de texte
*/
QFileNameEdit::QFileNameEdit(const QString &contents, QWidget *parent) : QLineEdit(parent) {
	init();
	if (!contents.isEmpty() && regexp_.exactMatch(contents)) {
		setText(contents);
	}
}

/**
	Destructeur
*/
QFileNameEdit::~QFileNameEdit() {
}

/**
	@return true si le champ de texte est vide, false sinon
*/
bool QFileNameEdit::isEmpty() {
	return(text().isEmpty());
}

/**
	@return true si le champ de texte n'est pas vide et est valide
*/
bool QFileNameEdit::isValid() {
	return(regexp_.exactMatch(text()));
}

/**
	Construit l'objet
*/
void QFileNameEdit::init() {
	regexp_ = QRegExp("^[0-9a-z_\\-\\.]+$", Qt::CaseSensitive);
	validator_ = new QETRegExpValidator(regexp_, this);
	setValidator(validator_);
	tooltip_text_ = QString(
		tr(
			"Les caract\350res autoris\351s sont : \n"
			" - les chiffres [0-9]\n"
			" - les minuscules [a-z]\n"
			" - le tiret [-], l'underscore [_] et le point [.]\n",
			"tooltip content when editing a filename"
		)
	);
	connect(validator_, SIGNAL(validationFailed()), this, SLOT(validationFailed()));
}

/**
	Affiche l'info-bulle informant l'utilisateur des caracteres autorises.
*/
void QFileNameEdit::displayToolTip() {
	QToolTip::showText(
		mapToGlobal(QPoint(x() + width(), 0)),
		tooltip_text_,
		this,
		QRect()
	);
}

/**
	Gere le fait que la validation du champ de texte ait echoue.
*/
void QFileNameEdit::validationFailed() {
	displayToolTip();
}
