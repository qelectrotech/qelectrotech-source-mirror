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
#include "aboutqet.h"
#include "qet.h"
#include "qeticons.h"

#include <QTabWidget>
#include <QDialogButtonBox>
#include <QVBoxLayout>
#include <QLabel>
#include <QTextEdit>
#include <QScrollArea>

/**
	Constructeur
	@param parent The parent of the dialog QWidget
*/
AboutQET::AboutQET(QWidget *parent) : QDialog(parent) {
	// Title, size, behavior ...
	setWindowTitle(tr("À propos de QElectrotech", "window title"));
	setFixedSize (690, 610);
	setModal(true);

	// Three tabs
	QTabWidget *tabs = new QTabWidget(this);
	tabs -> addTab(aboutTab(),        tr("À &propos",          "tab title"));
	tabs -> addTab(authorsTab(),      tr("A&uteurs",           "tab title"));
	tabs -> addTab(translatorsTab(),  tr("&Traducteurs",       "tab title"));
	tabs -> addTab(contributorsTab(), tr("&Contributeurs",     "tab title"));
	tabs -> addTab(titleTab(),        tr("&Version",           "tab title"));
	tabs -> addTab(licenseTab(),      tr("&Accord de licence", "tab title"));

	// A button to close the dialog box
	QDialogButtonBox *buttons = new QDialogButtonBox(QDialogButtonBox::Close);
	connect(buttons, SIGNAL(accepted()), this, SLOT(accept()));
	connect(buttons, SIGNAL(rejected()), this, SLOT(accept()));
	
	// All in a vertical arrangement
	QVBoxLayout *vlayout = new QVBoxLayout();
	vlayout -> addWidget(tabs);
	setLayout(vlayout);

	QScrollArea *scrollArea = new QScrollArea(this);
	scrollArea->setWidgetResizable(true);
	scrollArea->setFixedSize (690, 610);
	scrollArea->setWidget(tabs);


}

/**
	Destructeur
*/
AboutQET::~AboutQET() {
}

/**
	@return The title QElectroTech with its icon
*/
QWidget *AboutQET::titleTab() const {
	// label "QElectroTech"
	QLabel *title = new QLabel("<span style=\"font-weight:0;font-size:16pt;\">QElectroTech V " + QET::displayedVersion + "</span>");
	QString compilation_info = "<br />" + tr("Compilation : ");
#ifdef __GNUC__
	compilation_info += "  GCC " + QString(__VERSION__);
	compilation_info += " - built with Qt " + QString(QT_VERSION_STR);
	compilation_info += " - run with Qt "+ QString(qVersion());
#endif
	title -> setAlignment(Qt::AlignCenter);
	title -> setText(title->text() + compilation_info);
	title -> setTextFormat(Qt::RichText);
	title ->setTextInteractionFlags(Qt::TextSelectableByMouse);
	return(title);
}

/**
	@return The widget content tab "About"
*/
QWidget *AboutQET::aboutTab() const {
	QLabel *about = new QLabel(
		tr("QElectroTech, une application de réalisation de schémas électriques.", "about tab, description line") +
		"<br><br>" +
		tr("2006-2016 Les développeurs de QElectroTech", "about tab, developers line") +
		"<br><br>"
		"<a href=\"https://qelectrotech.org/\">https://qelectrotech.org/</a>"
		"<br><br>" +
		tr("Contact : <a href=\"mailto:qet@lists.tuxfamily.org\">qet@lists.tuxfamily.org</a>", "about tab, contact line")
	);
	about -> setAlignment(Qt::AlignCenter);
	about -> setOpenExternalLinks(true);
	about -> setTextFormat(Qt::RichText);
	about -> setFixedSize (690, 610);
	return(about);
}

/**
	@return The widget content by "Authors" tab
*/
QWidget *AboutQET::authorsTab() const {
	QLabel *authors = new QLabel();
	addAuthor(authors, "Benoît Ansieau",     "benoit@qelectrotech.org",     tr("Idée originale"));
	addAuthor(authors, "Laurent Trinques",   "scorpio@qelectrotech.org",    tr("Développement"));
	addAuthor(authors, "Joshua Claveau",     "Joshua@qelectrotech.org",     tr("Développement"));




	authors -> setOpenExternalLinks(true);
	authors -> setTextFormat(Qt::RichText);

	QWidget *authors_widget = new QWidget();
	QHBoxLayout *authors_layout = new QHBoxLayout(authors_widget);
	authors_layout -> addWidget(authors, 0, Qt::AlignCenter);
	return(authors_widget);
}

/**
	@return The widget content via the "Translators" tab
*/
QWidget *AboutQET::translatorsTab() const {
	QLabel *translators = new QLabel();
	
	addAuthor(translators, "Alfredo Carreto",                           "electronicos_mx@yahoo.com.mx",   tr("Traduction en espagnol"));
	addAuthor(translators, "Yuriy Litkevich",                           "yuriy@qelectrotech.org",         tr("Traduction en russe"));
	addAuthor(translators, "Evgeny Kozlov",                             "Evgeny.Kozlov.mailbox@gmail.com",tr("Traduction en russe"));
	addAuthor(translators, "José Carlos Martins",                       "jose@qelectrotech.org",          tr("Traduction en portugais"));
	addAuthor(translators, "Pavel Fric",                                "pavelfric@seznam.cz",            tr("Traduction en tchèque"));
	addAuthor(translators, "Pawe&#x0142; &#x015A;miech",                "pawel32640@gmail.com",           tr("Traduction en polonais"));
	addAuthor(translators, "Markus Budde & Jonas Stein & Noah Braden",  "news@jonasstein.de",             tr("Traduction en allemand"));
	addAuthor(translators, "Nuri",                                      "nuri@qelectrotech.org",          tr("Traduction en allemand"));
	addAuthor(translators, "Gabi Mandoc",                               "gabriel.mandoc@gic.ro",          tr("Traduction en roumain"));
	addAuthor(translators, "Alessandro Conti & Silvio",                 "silvio@qelectrotech.org",        tr("Traduction en italien"));
	addAuthor(translators, "Mohamed Souabni",                           "souabnimohamed@yahoo.fr",        tr("Traduction en arabe"));
	addAuthor(translators, "Antun Marakovi&#x0107;",                    "antun.marakovic@lolaribar.hr",   tr("Traduction en croate"));
	addAuthor(translators, "Eduard Amorós",                             "amoros@marmenuda.com",           tr("Traduction en catalan"));
	addAuthor(translators, "Nikos Papadopoylos",                        "231036448@freemail.gr",          tr("Traduction en grec"));
	addAuthor(translators, "Yannis Gyftomitros",                        "yang@hellug.gr",                 tr("Traduction en grec"));
	addAuthor(translators, "Paul Van Deelen",                           "shooter@home.nl",                tr("Traduction en néerlandais"));
	addAuthor(translators, "Ronny Desmedt",                             "r.desmedt@live.be",              tr("Traduction en flamand"));
	addAuthor(translators, "OSS au2mation",                             "OSSau2mation@OSSau2mation.dk",   tr("Traduction en danois"));
	addAuthor(translators, "Hilario Silveira",                          "hilario@soliton.com.br",         tr("Traduction en brézilien"));



	
	translators -> setOpenExternalLinks(true);
	translators -> setTextFormat(Qt::RichText);
	
	QWidget *translators_widget = new QWidget();
	QHBoxLayout *translators_layout = new QHBoxLayout(translators_widget);
	translators_layout -> addWidget(translators, 0, Qt::AlignCenter);
	return(translators_widget);
}

/**
	@return The widget content via the "Contributors" tab
*/
QWidget *AboutQET::contributorsTab() const {
	QLabel *contributors = new QLabel();
	
	addAuthor(contributors, "Remi Collet",         "remi@fedoraproject.org",              tr("Paquets Fedora et Red Hat"));
	addAuthor(contributors, "David Geiger",        "david.david@mageialinux-online.org",  tr("Paquets Mageia"));
	addAuthor(contributors, "Laurent Trinques",    "scorpio@qelectrotech.org",            tr("Paquets Debian"));
	addAuthor(contributors, "Denis Briand",        "debian@denis-briand.fr",              tr("Paquets Debian"));
	addAuthor(contributors, "Markos Chandras",     "hwoarang@gentoo.org.",                tr("Paquets Gentoo"));
	addAuthor(contributors, "Mbit",                "",                                    tr("Paquets Gentoo"));
	addAuthor(contributors, "Elbert",              "",                                    tr("Paquets OS/2"));
	addAuthor(contributors, "zloidemon",           "",                                    tr("Paquets FreeBSD"));
	addAuthor(contributors, "Yoann Varenne",       "yoann@tuxfamily.org",                 tr("Paquets MAC OS X"));
	addAuthor(contributors, "Chipsterjulien",      "",                                    tr("Paquets Archlinux AUR"));
	addAuthor(contributors, "Nuno Pinheiro",       "nuno@nuno-icons.com",                 tr("Icônes"));
	addAuthor(contributors, "Cyril Frausti",       "cyril@qelectrotech.org",              tr("Développement"));
	addAuthor(contributors, "Arun Kishore Eswara", "eswara.arun@gmail.com",               tr("Documentation"));
	addAuthor(contributors, "René Negre",          "runsys@qelectrotech.org",             tr("Développement"));
	addAuthor(contributors, "Nuri",                "nuri@qelectrotech.org",               tr("Collection d'éléments"));
	addAuthor(contributors, "Ronny Desmedt",       "r.desmedt@live.be",                   tr("Convertisseur DXF"));
	addAuthor(contributors, "Abhishek Bansal",     "abhishek@qelectrotech.org",           tr("Développement"));
	addAuthor(contributors, "Davi Fochi",          "davi@fochi.com.br",                   tr("Développement"));


	
	contributors -> setOpenExternalLinks(true);
	contributors -> setTextFormat(Qt::RichText);
	
	QWidget *contributors_widget = new QWidget();
	QHBoxLayout *contributors_layout = new QHBoxLayout(contributors_widget);
	contributors_layout -> addWidget(contributors, 0, Qt::AlignCenter);
	return(contributors_widget);
}

/**
	@return The widget content via the "License Agreement" tab
*/
QWidget *AboutQET::licenseTab() const {
	QWidget *license = new QWidget();
	// label
	QLabel *title_license = new QLabel(tr("Ce programme est sous licence GNU/GPL."));
	
	// Text of the GNU/GPL in a scrollable text box not editable
	QTextEdit *text_license = new QTextEdit();
	text_license -> setPlainText(QET::license());
	text_license -> setReadOnly(true);
	
	// All in a vertical arrangement
	QVBoxLayout *license_layout = new QVBoxLayout();
	license_layout -> addWidget(title_license);
	license_layout -> addWidget(text_license);
	license -> setLayout(license_layout);
	return(license);
}

/**
	Adds a person to the list of authors
	@param label QLabel which will add the person
	@param name  Name of person
	@param email E-mail address of the person
	@param work  Function / work done by the person
*/
void AboutQET::addAuthor(QLabel *label, const QString &name, const QString &email, const QString &work) const {
	QString new_text = label -> text();
	
	QString author_template = "<span style=\"text-decoration: underline;\">%1</span> : %2 &lt;<a href=\"mailto:%3\">%3</a>&gt;&lrm;<br/><br/>";
	
	// Add the function of the person
	new_text += author_template.arg(work).arg(name).arg(email);
	label -> setText(new_text);
}
