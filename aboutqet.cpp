#include "aboutqet.h"

/**
	Constructeur
	@param parent Le QWidget parent de la boite de dialogue
*/
AboutQET::AboutQET(QWidget *parent) : QDialog(parent) {
	// Titre, taille, comportement...
	setWindowTitle(tr("\300 propos de QElectrotech"));
	setMinimumWidth(680);
	setMinimumHeight(350);
	setModal(true);
	
	// Trois onglets
	QTabWidget *onglets = new QTabWidget(this);
	onglets -> addTab(ongletAPropos(), tr("\300 &propos"));
	onglets -> addTab(ongletAuteurs(), tr("A&uteurs"));
	onglets -> addTab(ongletLicence(), tr("&Accord de licence"));
	
	// Un bouton pour fermer la boite de dialogue
	QDialogButtonBox *boutons = new QDialogButtonBox(QDialogButtonBox::Close);
	connect(boutons, SIGNAL(accepted()), this, SLOT(accept()));
	connect(boutons, SIGNAL(rejected()), this, SLOT(accept()));
	
	// Le tout dans une disposition verticale
	QVBoxLayout *disposition = new QVBoxLayout();
	disposition -> addWidget(titre());
	disposition -> addWidget(onglets);
	disposition -> addWidget(boutons);
	setLayout(disposition);
}

/**
	@return Le titre QElectroTech avec son icone
*/
QWidget *AboutQET::titre() {
	QWidget *icone_et_titre = new QWidget();
	// icone
	QLabel *icone = new QLabel();
	icone -> setPixmap(QIcon(":/ico/qelectrotech.png").pixmap(48, 48));
	// label "QElectroTech"
	QLabel *titre = new QLabel("<span style=\"font-weight:0;font-size:16pt;\">QElectroTech</span>");
	titre -> setTextFormat(Qt::RichText);
	// le tout dans une grille
	QGridLayout *dispo_horiz = new QGridLayout();
	dispo_horiz -> addWidget(icone, 0, 0);
	dispo_horiz -> addWidget(titre, 0, 1);
	dispo_horiz -> setColumnStretch(0, 1);
	dispo_horiz -> setColumnStretch(1, 100);
	icone_et_titre -> setLayout(dispo_horiz);
	return(icone_et_titre);
}

/**
	@return Le widget contenu par l'onglet « A propos »
*/
QWidget *AboutQET::ongletAPropos() {
	QLabel *apropos = new QLabel(
		tr(
			"QElectroTech, une application de "
			"r\351alisation de sch\351mas \351lectriques.\n\n\251 2006 Les "
			"d\351veloppeurs de QElectroTech\n\nMerde on n'a pas de site web"
		)
	);
	apropos -> setAlignment(Qt::AlignCenter);
	return(apropos);
}

/**
	@return Le widget contenu par l'onglet « Auteurs »
*/
QWidget *AboutQET::ongletAuteurs() {
	QLabel *auteurs = new QLabel(
		tr(
			"Id\351e originale : Beno\356t Ansieau <benoit.ansieau@gmail.com>\n\n"
			"Programmation : Xavier Guerrin <xavier.guerrin@gmail.com>"
		)
	);
	auteurs -> setAlignment(Qt::AlignCenter);
	return(auteurs);
}

/**
	@return Le widget contenu par l'onglet « Accord de Licence »
*/
QWidget *AboutQET::ongletLicence() {
	QWidget *licence = new QWidget();
	// label
	QLabel *titre_licence = new QLabel(tr("Ce programme est sous licence GNU/GPL."));
	
	// Recuperation du texte de la GNU/GPL dans un fichier externe
	QFile *fichier_gpl = new QFile("./gnugpl.txt");
	QString txt_gpl;
	// verifie que le fichier existe
	if (!fichier_gpl -> exists()) {
		txt_gpl = QString(tr("Le fichier texte contenant la licence GNU/GPL est introuvable - bon bah de toute fa\347on, vous la connaissez par coeur non ?"));
	} else {
		// ouvre le fichier en mode texte et en lecture seule
		if (!fichier_gpl -> open(QIODevice::ReadOnly | QIODevice::Text)) {
			txt_gpl = QString(tr("Le fichier texte contenant la licence GNU/GPL existe mais n'a pas pu \352tre ouvert - bon bah de toute fa\347on, vous la connaissez par coeur non ?"));
		} else {
			// charge le contenu du fichier dans une QString
			QTextStream in(fichier_gpl);
			txt_gpl = QString("");
			while (!in.atEnd()) txt_gpl += in.readLine()+"\n";
			// ferme le fichier
			fichier_gpl -> close();
		}
	}
	
	// texte de la GNU/GPL dans une zone de texte scrollable non editable
	QTextEdit *texte_licence = new QTextEdit();
	texte_licence -> setPlainText(txt_gpl);
	texte_licence -> setReadOnly(true);
	
	// le tout dans une disposition verticale
	QVBoxLayout *dispo_licence = new QVBoxLayout();
	dispo_licence -> addWidget(titre_licence);
	dispo_licence -> addWidget(texte_licence);
	licence -> setLayout(dispo_licence);
	return(licence);
}
