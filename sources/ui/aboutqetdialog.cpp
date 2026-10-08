/*
	Copyright 2006-2026 The QElectroTech Team
	This file is part of QElectroTech.

	QElectroTech is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 2 of the License, or
	(at your option) any later version.

	QElectroTech is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with QElectroTech. If not, see <http://www.gnu.org/licenses/>.
*/
#include "aboutqetdialog.h"

#include "../machine_info.h"
#include "../qet.h"
#include "../qetapp.h"
#include "ui_aboutqetdialog.h"
#include "../qetversion.h"

#include <QDate>

/**
	@brief AboutQETDialog::AboutQETDialog
	@param parent
*/
AboutQETDialog::AboutQETDialog(QWidget *parent) :
	QDialog(parent),
	ui(new Ui::AboutQETDialog)
{
	ui->setupUi(this);
	QET::trackDialogGeometry(this);
	setAbout();
	setAuthors();
	setTranslators();
	setContributors();
	setVersion();
	setAnnexProject();
	setLibraries();
	setLicenses();
	setLoginfo();
}

/**
	@brief AboutQETDialog::~AboutQETDialog
*/
AboutQETDialog::~AboutQETDialog()
{
	delete ui;
}

/**
	@brief AboutQETDialog::setAbout
*/
void AboutQETDialog::setAbout()
{

	QString str  = tr("QElectroTech, an application to design electric diagrams.", "about tab, description line") +
			"<br><br>© 2006-"+QDate::currentDate().toString("yyyy")+
			tr(" The developers of QElectroTech", "about tab, developers line") +
			"<br><br>"
			"<a href=\"https://qelectrotech.org/\">https://qelectrotech.org/</a>"
			"<br><br>" +
			"The program is provided AS IS with NO WARRANTY OF ANY KIND,"
			"<br>"
			" INCLUDING THE WARRANTY OF DESIGN, "
			"<br>"
			"MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.";
			"<br>"
			"<br><br>" +
			tr("Contact : <a href=\"mailto:qet@lists.tuxfamily.org\">qet@lists.tuxfamily.org</a>", "about tab, contact line");



	ui->m_about_label->setText(str);
}

/**
	@brief AboutQETDialog::setAuthors
*/
void AboutQETDialog::setAuthors()
{
	addAuthor(ui->m_author_label, "Benoît Ansieau",     "benoit@qelectrotech.org",         tr("Original concept"));
	addAuthor(ui->m_author_label, "Laurent Trinques",   "scorpio@qelectrotech.org",        tr("Development"));
	addAuthor(ui->m_author_label, "Joshua Claveau",     "Joshua@qelectrotech.org",         tr("Development"));
	addAuthor(ui->m_author_label, "Davi Fochi",         "davi@fochi.com.br",               tr("Development"));
	addAuthor(ui->m_author_label, "Ronny Desmedt",      "r.desmedt@live.be",               tr("DXF converter"));
	addAuthor(ui->m_author_label, "Raul Roda",          "raulroda8@gmail.com",             tr("Terminal Block plugin"));
	addAuthor(ui->m_author_label, "Abhishek Bansal",    "abhishek@qelectrotech.org",       tr("Development"));
	addAuthor(ui->m_author_label, "Simon De Backer",    "debacker@qelectrotech.org",       tr("Development"));
	addAuthor(ui->m_author_label, "David Varley",       "David.Varley@cborn.com",          tr("Development"));
	addAuthor(ui->m_author_label, "Damian Caceres",     "damiancaceresmoreno@yahoo.es",    tr("Development"));
	addAuthor(ui->m_author_label, "Martin Marmsoler",   "martin.marmsoler@gmail.com",      tr("Development"));
	addAuthor(ui->m_author_label, "Sébastien Deffaux",  "s.deffaux@live.fr",               tr("Collection"));
}

/**
	@brief AboutQETDialog::setTranslators
*/
void AboutQETDialog::setTranslators()
{
	addAuthor(ui->m_translators_label, "Alfredo Carreto",                           "electronicos_mx@yahoo.com.mx",   tr("Spanish translation"));
	addAuthor(ui->m_translators_label, "Edgar Robles Najar",                        "tgo.edrobles@gmail.com",         tr("Spanish translation"));
	addAuthor(ui->m_translators_label, "Yuriy Litkevich",                           "yuriy@qelectrotech.org",         tr("Russian translation"));
	addAuthor(ui->m_translators_label, "Evgeny Kozlov",                             "Evgeny.Kozlov.mailbox@gmail.com",tr("Russian translation"));
	addAuthor(ui->m_translators_label, "José Carlos Martins",                       "jose@qelectrotech.org",          tr("Portuguese translation"));
	addAuthor(ui->m_translators_label, "Pavel Fric",                                "pavelfric@seznam.cz",            tr("Czech translation"));
	addAuthor(ui->m_translators_label, "Pawe&#x0142; &#x015A;miech",                "pawel32640@gmail.com",           tr("Polish translation"));
	addAuthor(ui->m_translators_label, "Markus Budde & Jonas Stein & Noah Braden",  "news@jonasstein.de",             tr("German translation"));
	addAuthor(ui->m_translators_label, "Nuri",                                      "nuri@qelectrotech.org",          tr("German translation"));
	addAuthor(ui->m_translators_label, "Gabi Mandoc",                               "gabriel.mandoc@gic.ro",          tr("Romanian translation"));
	addAuthor(ui->m_translators_label, "Alessandro Conti & Silvio",                 "silvio@qelectrotech.org",        tr("Italian translation"));
	addAuthor(ui->m_translators_label, "Mohamed Souabni",                           "souabnimohamed@yahoo.fr",        tr("Arabic translation"));
	addAuthor(ui->m_translators_label, "Antun Marakovi&#x0107;",                    "antun.marakovic@lolaribar.hr",   tr("Croatian translation"));
	addAuthor(ui->m_translators_label, "Eduard Amorós",                             "amoros@marmenuda.com",           tr("Catalan translation"));
	addAuthor(ui->m_translators_label, "Nikos Papadopoylos",                        "231036448@freemail.gr",          tr("Greek translation"));
	addAuthor(ui->m_translators_label, "Yannis Gyftomitros",                        "yang@hellug.gr",                 tr("Greek translation"));
	addAuthor(ui->m_translators_label, "Paul Van Deelen",                           "shooter@home.nl",                tr("Dutch translation"));
	addAuthor(ui->m_translators_label, "Dik Leenheer",                              "dleenheer@suzerein.nl",          tr("Dutch translation"));
	addAuthor(ui->m_translators_label, "Ronny Desmedt",                             "r.desmedt@live.be",              tr("Flemish translation"));
	addAuthor(ui->m_translators_label, "OSS au2mation",                             "OSSau2mation@OSSau2mation.dk",   tr("Danish translation"));
	addAuthor(ui->m_translators_label, "Hilario Silveira & Gleisson J.J.Cardoso",   "hilario@soliton.com.br",         tr("Brazilian translation"));
	addAuthor(ui->m_translators_label, "Aziz Karabudak",                            "aziz.karabudak@argevi.com",      tr("Turkish translation"));
	addAuthor(ui->m_translators_label, "Emir Izmiroglu",                            "emirizmiroglu@gmail.com",        tr("Turkish translation"));
	addAuthor(ui->m_translators_label, "Gábor Gubányi",                             "gubanyig@gmail.com",             tr("Hungarian translation"));
	addAuthor(ui->m_translators_label, "",                                          "",                               tr("Serbian Translation"));
	addAuthor(ui->m_translators_label, "Yaroslav",                                  "",                               tr("Ukrainian translation"));
	addAuthor(ui->m_translators_label, "JoelAs",                                    "",                               tr("Norwegian Translation"));
	addAuthor(ui->m_translators_label, "Yuki",                                      "yuki.atoh@gmail.com",            tr("Japanese translation"));
	addAuthor(ui->m_translators_label, "Jung Kwang-Ho",                             "jkh2rokmc@daum.net",             tr("Translation into Korean"));
	addAuthor(ui->m_translators_label, "Nathalie",                                  "nathalie.roussier@giz.de",       tr("Mongolian Translation"));
	addAuthor(ui->m_translators_label, "Uroš Platiše",                              "uros.platise@energycon.eu",      tr("Slovenian translation"));
}

/**
	@brief AboutQETDialog::setContributors
*/
void AboutQETDialog::setContributors()
{
	addAuthor(ui->m_contrib_label, "Remi Collet",         "remi@fedoraproject.org",              tr("Fedora and Red Hat packages"));
	addAuthor(ui->m_contrib_label, "David Geiger",        "david.david@mageialinux-online.org",  tr("Mageia packages"));
	addAuthor(ui->m_contrib_label, "Laurent Trinques",    "scorpio@qelectrotech.org",            tr("Debian packages"));
	addAuthor(ui->m_contrib_label, "Denis Briand",        "debian@denis-briand.fr",              tr("Debian packages"));
	addAuthor(ui->m_contrib_label, "W. Martin Borgert",   "debacle@debian.org",                  tr("Debian packages"));
	addAuthor(ui->m_contrib_label, "Markos Chandras",     "hwoarang@gentoo.org.",                tr("Gentoo packages"));
	addAuthor(ui->m_contrib_label, "Mbit",                "",                                    tr("Gentoo packages"));
	addAuthor(ui->m_contrib_label, "Elbert",              "",                                    tr("OS/2 packages"));
	addAuthor(ui->m_contrib_label, "zloidemon",           "",                                    tr("FreeBSD packages"));
	addAuthor(ui->m_contrib_label, "Yoann Varenne",       "yoann@tuxfamily.org",                 tr("MAC OS X packages"));
	addAuthor(ui->m_contrib_label, "Chipsterjulien",      "",                                    tr("Archlinux AUR packages"));
	addAuthor(ui->m_contrib_label, "Nuno Pinheiro",       "nuno@nuno-icons.com",                 tr("Icons"));
	addAuthor(ui->m_contrib_label, "Cyril Frausti",       "cyril@qelectrotech.org",              tr("Development"));
	addAuthor(ui->m_contrib_label, "Fernando Mateu Palou de Comasema", "fdomateu@gmail.com",     tr("Documentation"));
	addAuthor(ui->m_contrib_label, "Arun Kishore Eswara", "eswara.arun@gmail.com",               tr("Documentation"));
	addAuthor(ui->m_contrib_label, "René Negre",          "runsys@qelectrotech.org",             tr("Development"));
	addAuthor(ui->m_contrib_label, "Nuri",                "nuri@qelectrotech.org",               tr("Collection of elements"));
	addAuthor(ui->m_contrib_label, "Maximilian Federle",   "",                                   tr("Snap package"));
}

/**
	@brief AboutQETDialog::setVersion
*/
void AboutQETDialog::setVersion()
{
	QString str = "<span style=\"font-weight:bold;font-size:16pt;\">QElectroTech V "
		+ QetVersion::displayedVersion()
			+ "</span>";
	ui->m_version_label->setText(str + MachineInfo::instance()->compilation_info());
}

void AboutQETDialog::setAnnexProject()
{
	addLibrary(ui->m_annex_project_label, tr("Elements  resizer", "Element scaler"),
	 "https://github.com/plc-user/QET_ElementScaler");
	addLibrary(ui->m_annex_project_label, tr("Lambda element generator", "Lambda element generator"),
	 "https://github.com/asnigma/qet_gen_element");
	addLibrary(ui->m_annex_project_label, tr("DXF Element Converter", "Dxf2elmt"),
	 "https://github.com/Vadoola/dxf2elmt");
	addLibrary(ui->m_annex_project_label, tr("Elements translation tool", "Qet_translate"),
	 "https://qelectrotech.org/forum/viewtopic.php?pid=16027#p16027");

}

/**
	@brief AboutQETDialog::setLibraries
*/
void AboutQETDialog::setLibraries()
{
	addLibrary(ui->m_libraries_label, "KDE lib",            "https://api.kde.org");
	addLibrary(ui->m_libraries_label, "Single application", "https://github.com/itay-grudev/SingleApplication");
	addLibrary(ui->m_libraries_label, "pugixml",            "https://pugixml.org");
	addLibrary(ui->m_libraries_label, "Liberation fonts",   "https://github.com/liberationfonts/liberation-fonts");
	addLibrary(ui->m_libraries_label, "osifont",            "https://github.com/hikikomori82/osifont");
}

/**
	@brief AboutQETDialog::setLicenses
*/
void AboutQETDialog::setLicenses()
{
	ui->m_licenses_comboBox->addItem("QElectroTech");
	ui->m_licenses_comboBox->addItem("QET-Elements");
	ui->m_licenses_comboBox->addItem("liberation-fonts");
	ui->m_licenses_comboBox->addItem("osifont");
}

/**
	@brief AboutQETDialog::setLoginfo
	fills the m_log_comboBox with log files
*/
void AboutQETDialog::setLoginfo()
{
	const QString path = QETApp::dataDir() + "/";
	QString filter("%1%1%1%1%1%1%1%1.log"); // pattern
	filter = filter.arg("[0123456789]"); // valid characters
	Q_FOREACH (auto fileInfo,
		   QDir(path).entryInfoList(
			   QStringList(filter),
			   QDir::Files))
	{
		ui->m_log_comboBox->addItem(fileInfo.absoluteFilePath());
	}
	ui->m_log_comboBox->setCurrentIndex(ui->m_log_comboBox->count() - 1);
}

/**
	@brief AboutQETDialog::addAuthor
	Adds a person to the list of authors
	@param label : QLabel which will add the person
	@param name : Name of person
	@param email : E-mail address of the person
	@param work : Function / work done by the person
*/
void AboutQETDialog::addAuthor(QLabel *label, const QString &name, const QString &email, const QString &work)
{
	QString new_text = label->text();

	QString author_template = "<span style=\"text-decoration: underline;\">%1</span> : %2 &lt;<a href=\"mailto:%3\">%3</a>&gt;&lrm;<br/><br/>";

		// Add the function of the person
	new_text += author_template.arg(work).arg(name).arg(email);
	label->setText(new_text);
}

/**
	@brief AboutQETDialog::addLibrary
	@param label
	@param name
	@param link
*/
void AboutQETDialog::addLibrary(QLabel *label, const QString &name, const QString &link)
{
	QString new_text = label->text();

	QString Library_template = "<span style=\"text-decoration: underline;\">%1</span> : &lt;<a href=\"%3\">%3</a>&gt;&lrm;<br/><br/>";

		// Add the function of the person
	new_text += Library_template.arg(name).arg(link);
	label->setText(new_text);
}

/**
	@brief Updates the displayed license text when a different one is selected

	This slot is called when the user selects a different license in the 
	licenses combo box. It retrieves the selected license text from QET's 
	license collection and displays it in the text edit widgets.

	@param license_name The identifier of the selected license
*/
void AboutQETDialog::on_m_licenses_comboBox_currentTextChanged(
		const QString &license_name)
{
	std::tuple<QString, QString> license_info = QET::licenses(license_name);
	ui->m_licenses_notice_plainTextEdit->setPlainText(std::get<0>(license_info));
	ui->m_licenses_license_plainTextEdit->setPlainText(std::get<1>(license_info));
	//adjust height of notice-field:
	int LineCount = ui->m_licenses_notice_plainTextEdit->blockCount();
	if (LineCount <= 4) {
		QFontMetrics qfm (ui->m_licenses_notice_plainTextEdit->font());
		int LineHeight = qfm.lineSpacing();
		ui->m_licenses_notice_plainTextEdit->setMaximumHeight((LineCount + 1) * LineHeight);
	} else {
		ui->m_licenses_notice_plainTextEdit->setMaximumHeight(16777215);
	}
}

void AboutQETDialog::on_m_log_comboBox_currentTextChanged(const QString &arg1)
{
	QFile log_File(arg1);
	if(log_File.open(QIODevice::ReadOnly)){
		ui->m_log_textEdit->setPlainText(log_File.readAll());
	}
	log_File.close();
	ui->m_log_textEdit->moveCursor(QTextCursor::End);
}
