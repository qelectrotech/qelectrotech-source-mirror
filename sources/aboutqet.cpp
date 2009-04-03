/*
	Copyright 2006-2009 Xavier Guerrin
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
#include <QtGui>
#include "qettabwidget.h"
#include "aboutqet.h"
#include "qet.h"

/**
	Constructeur
	@param parent Le QWidget parent de la boite de dialogue
*/
AboutQET::AboutQET(QWidget *parent) : QDialog(parent) {
	// Titre, taille, comportement...
	setWindowTitle(tr("\300 propos de QElectrotech", "window title"));
	setMinimumWidth(680);
	setMinimumHeight(350);
	setModal(true);
	
	// Trois onglets
	QETTabWidget *onglets = new QETTabWidget(this);
	onglets -> addTab(ongletAPropos(), tr("\300 &propos","tab title"));
	onglets -> addTab(ongletAuteurs(), tr("A&uteurs", "tab title"));
	onglets -> addTab(ongletLicence(), tr("&Accord de licence", "tab title"));
	
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
	Destructeur
*/
AboutQET::~AboutQET() {
}

/**
	@return Le titre QElectroTech avec son icone
*/
QWidget *AboutQET::titre() const {
	QWidget *icone_et_titre = new QWidget();
	// icone
	QLabel *icone = new QLabel();
	icone -> setPixmap(QIcon(":/ico/qelectrotech.png").pixmap(48, 48));
	// label "QElectroTech"
	QLabel *titre = new QLabel("<span style=\"font-weight:0;font-size:16pt;\">QElectroTech v" + QET::displayedVersion + "</span>");
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
QWidget *AboutQET::ongletAPropos() const {
	QLabel *apropos = new QLabel(
		tr("QElectroTech, une application de r\351alisation de sch\351mas \351lectriques.") +
		"<br><br>" +
		tr("\251 2006-2009 Les d\351veloppeurs de QElectroTech") +
		"<br><br>"
		"<a href=\"http://qelectrotech.org/\">"
		"http://qelectrotech.org/</a>"
	);
	apropos -> setAlignment(Qt::AlignCenter);
	apropos -> setOpenExternalLinks(true);
	apropos -> setTextFormat(Qt::RichText);
	return(apropos);
}

/**
	@return Le widget contenu par l'onglet « Auteurs »
*/
QWidget *AboutQET::ongletAuteurs() const {
	QLabel *auteurs = new QLabel(
		"<span style=\"text-decoration: underline;\">" +
		tr("Id\351e originale") +
		"</span> : Beno\356t Ansieau "
		"&lt;<a href=\"mailto:benoit.ansieau@gmail.com\">"
		"benoit.ansieau@gmail.com</a>&gt;"
		"<br><br>"
		"<span style=\"text-decoration: underline;\">" +
		tr("Programmation") +
		"</span> : Xavier Guerrin "
		"&lt;<a href=\"mailto:xavier.guerrin@gmail.com\">"
		"xavier.guerrin@gmail.com</a>&gt;"
	);
	auteurs -> setAlignment(Qt::AlignCenter);
	auteurs -> setOpenExternalLinks(true);
	auteurs -> setTextFormat(Qt::RichText);
	return(auteurs);
}

/**
	@return Le widget contenu par l'onglet « Accord de Licence »
*/
QWidget *AboutQET::ongletLicence() const {
	QWidget *licence = new QWidget();
	// label
	QLabel *titre_licence = new QLabel(tr("Ce programme est sous licence GNU/GPL."));
	
	// texte de la GNU/GPL dans une zone de texte scrollable non editable
	QTextEdit *texte_licence = new QTextEdit();
	texte_licence -> setPlainText(QET::license());
	texte_licence -> setReadOnly(true);
	
	// le tout dans une disposition verticale
	QVBoxLayout *dispo_licence = new QVBoxLayout();
	dispo_licence -> addWidget(titre_licence);
	dispo_licence -> addWidget(texte_licence);
	licence -> setLayout(dispo_licence);
	return(licence);
}
