#include "elementscategorydeleter.h"

/**
	Constructeur
	@param category_path Chemin du dossier representant la categorie a supprimer
	@param parent QWidget parent
*/
ElementsCategoryDeleter::ElementsCategoryDeleter(const QString &category_path, QWidget *parent) :
	QWidget(parent),
	cat(category_path),
	empty_category_path(category_path.isNull())
{
}

/// Destructeur
ElementsCategoryDeleter::~ElementsCategoryDeleter() {
}

/**
	Supprime la categorie et ses elements : verifie l'existence du dossier,
	demande deux fois confirmation a l'utilisateur et avertit ce dernier si la
	suppression a echoue.
*/
void ElementsCategoryDeleter::exec() {
	// verifie l'existence de la categorie
	if (!cat.exists() || empty_category_path) return;
	
	// confirmation #1
	QMessageBox::StandardButton answer_1 = QMessageBox::question(
		this,
		tr("Supprimer la cat\351gorie ?"),
		tr("\312tes-vous s\373r de vouloir supprimer cette cat\351gorie ?\n"
		"Tous les \351l\351ments et les cat\351gories contenus dans cette "
		"cat\351gorie seront supprim\351s"),
		QMessageBox::Yes|QMessageBox::No|QMessageBox::Cancel
	);
	if (answer_1 != QMessageBox::Yes) return;
	
	// confirmation #2
	QMessageBox::StandardButton answer_2 = QMessageBox::question(
		this,
		tr("Supprimer la cat\351gorie ?"),
		tr("\312tes-vous vraiment s\373r de vouloir supprimer cette "
		"cat\351gorie ?\nLes changements seront d\351finitifs."),
		QMessageBox::Yes|QMessageBox::No|QMessageBox::Cancel
	);
	if (answer_2 != QMessageBox::Yes) return;
	
	// supprime la categorie
	if (!cat.remove()) {
		QMessageBox::warning(
			this,
			tr("Suppression de la cat\351gorie"),
			tr("La suppression de la cat\351gorie a \351chou\351.\n"
			"V\351rifiez vos droits sur le dossier ") + cat.absolutePath() + tr(".")
		);
	}
}
