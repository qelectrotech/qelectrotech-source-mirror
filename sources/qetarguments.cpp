/*
	Copyright 2006-2015 The QElectroTech Team
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
#include "qetarguments.h"
#include "titleblock/templatescollection.h"

/**
	Constructeur par defaut
	Cree un objet sans argument.
*/
QETArguments::QETArguments(QObject *parent) :
	QObject(parent),
	print_help_(false),
	print_license_(false),
	print_version_(false)
{
}

/**
	Constructeur
	@param args Arguments a analyser et memoriser
	@param parent QObject parent
*/
QETArguments::QETArguments(const QList<QString> &args, QObject *parent) :
	QObject(parent),
	print_help_(false),
	print_license_(false),
	print_version_(false)
{
	parseArguments(args);
}

/**
	Constructeur de copie - la copie reprend le parent de l'original
	@param qet_arguments Objet a copier
*/
QETArguments::QETArguments(const QETArguments &qet_arguments) :
	QObject(qet_arguments.parent()),
	project_files_(qet_arguments.project_files_),
	element_files_(qet_arguments.element_files_),
	tbt_files_(qet_arguments.tbt_files_),
	options_(qet_arguments.options_),
	unknown_options_(qet_arguments.unknown_options_),
#ifdef QET_ALLOW_OVERRIDE_CED_OPTION
	common_elements_dir_(qet_arguments.common_elements_dir_),
#endif
#ifdef QET_ALLOW_OVERRIDE_CTBTD_OPTION
common_tbt_dir_(qet_arguments.common_tbt_dir_),
#endif
#ifdef QET_ALLOW_OVERRIDE_CD_OPTION
	config_dir_(qet_arguments.config_dir_),
#endif
	lang_dir_(qet_arguments.lang_dir_),
	print_help_(qet_arguments.print_help_),
	print_license_(qet_arguments.print_license_),
	print_version_(qet_arguments.print_version_)
{
}

/**
	Operateur d'affectation - la copie ne reprend pas le parent de l'original
	@param qet_arguments Objet a copier
*/
QETArguments &QETArguments::operator=(const QETArguments &qet_arguments) {
	project_files_   = qet_arguments.project_files_;
	element_files_   = qet_arguments.element_files_;
	tbt_files_       = qet_arguments.tbt_files_;
	options_         = qet_arguments.options_;
	unknown_options_ = qet_arguments.unknown_options_;
#ifdef QET_ALLOW_OVERRIDE_CED_OPTION
	common_elements_dir_ = qet_arguments.common_elements_dir_;
#endif
#ifdef QET_ALLOW_OVERRIDE_CTBTD_OPTION
	common_tbt_dir_ = qet_arguments.common_tbt_dir_;
#endif
#ifdef QET_ALLOW_OVERRIDE_CD_OPTION
	config_dir_ = qet_arguments.config_dir_;
#endif
	lang_dir_        = qet_arguments.lang_dir_;
	print_help_      = qet_arguments.print_help_;
	print_license_   = qet_arguments.print_license_;
	print_version_   = qet_arguments.print_version_;
	return(*this);
}

/**
	Destructeur
*/
QETArguments::~QETArguments() {
}

/**
	Definit les arguments de cet objet.
	Si cet objet contenait deja des arguments, ceux-ci sont oublies.
	@param args Arguments a analyser et memoriser
*/
void QETArguments::setArguments(const QList<QString> &args) {
	parseArguments(args);
}

/**
	@return tous les arguments (projets et elements) passes en parametres
	dans l'ordre suivant : options connues puis inconnues, fichiers de types
	projet puis element.
*/
QList<QString> QETArguments::arguments() const {
	return(options_ + unknown_options_ + project_files_ + element_files_ + tbt_files_);
}

/**
	@return tous les fichiers (projets et elements) passes en parametres.
	Les fichiers de type projet viennent avant les fichiers de type element.
*/
QList<QString> QETArguments::files() const {
	return(project_files_ + element_files_ + tbt_files_);
}

/**
	@return les fichiers de type projet
*/
QList<QString> QETArguments::projectFiles() const {
	return(project_files_);
}

/**
	@return les fichiers de type element
*/
QList<QString> QETArguments::elementFiles() const {
	return(element_files_);
}

/**
	@return title block template files
*/
QList<QString> QETArguments::titleBlockTemplateFiles() const {
	return(tbt_files_);
}

/**
	@return les options reconnues
*/
QList<QString> QETArguments::options() const {
	return(options_);
}

/**
	@return les options non reconnues
*/
QList<QString> QETArguments::unknownOptions() const {
	return(unknown_options_);
}

/**
	Oublie tous les arguments de cet objet
*/
void QETArguments::clear() {
	project_files_.clear();
	element_files_.clear();
	options_.clear();
	unknown_options_.clear();
#ifdef QET_ALLOW_OVERRIDE_CED_OPTION
	common_elements_dir_.clear();
#endif
#ifdef QET_ALLOW_OVERRIDE_CTBTD_OPTION
	common_tbt_dir_.clear();
#endif
#ifdef QET_ALLOW_OVERRIDE_CD_OPTION
	config_dir_.clear();
#endif
}

/**
	Analyse des arguments et les retient dans cet objet.
	Si cet objet contenait deja des arguments, ceux-ci sont oublies.
	@param arguments Arguments a analyser
*/
void QETArguments::parseArguments(const QList<QString> &arguments) {
	// oublie les eventuels arguments precedents
	clear();
	
	// separe les fichiers des options
	foreach(QString argument, arguments) {
		QFileInfo argument_info(argument);
		if (argument_info.exists()) {
		// on exprime les chemins des fichiers en absolu
			QString can_argument = argument_info.canonicalFilePath();
			handleFileArgument(can_argument);
		} else {
			handleOptionArgument(argument);
		}
	}
}

/**
	Gere les arguments correspondant a un fichier existant.
*/
void QETArguments::handleFileArgument(const QString &file) {
	if (file.endsWith(".elmt")) {
		if (!element_files_.contains(file)) {
			element_files_ << file;
		}
	} else if (file.endsWith(TITLEBLOCKS_FILE_EXTENSION)) {
		if (!tbt_files_.contains(file)) {
			tbt_files_ << file;
		}
	} else {
		if (!project_files_.contains(file)) {
			project_files_ << file;
		}
	}
}

/**
	Gere les arguments correspondant potentiellement a une option.
	Les options reconnues sont : 
	  * --common-elements-dir=
	  * --common-tbt-dir
	  * --config-dir=
	  * --lang-dir=
	  * --help
	  * --version
	  * -v
	  * --license
*/
void QETArguments::handleOptionArgument(const QString &option) {
	if (option == QString("--help")) {
		print_help_ = true;
		options_ << option;
		return;
	} else if (option == QString("--version") || option == QString("-v")) {
		print_version_ = true;
		options_ << option;
		return;
	} else if (option == QString("--license")) {
		print_license_ = true;
		options_ << option;
		return;
	}
	
#ifdef QET_ALLOW_OVERRIDE_CED_OPTION
	QString ced_arg("--common-elements-dir=");
	if (option.startsWith(ced_arg)) {
		common_elements_dir_ = option.mid(ced_arg.length());
		return;
	}
	
#endif
#ifdef QET_ALLOW_OVERRIDE_CTBTD_OPTION
	QString ctbtd_arg("--common-tbt-dir=");
	if (option.startsWith(ctbtd_arg)) {
		common_tbt_dir_ = option.mid(ctbtd_arg.length());
		return;
	}
#endif
#ifdef QET_ALLOW_OVERRIDE_CD_OPTION
	QString cd_arg("--config-dir=");
	if (option.startsWith(cd_arg)) {
		config_dir_ = option.mid(cd_arg.length());
		return;
	}
	
#endif
	
	QString ld_arg("--lang-dir=");
	if (option.startsWith(ld_arg)) {
		lang_dir_ = option.mid(ld_arg.length());
		return;
	}
	
	// a ce stade, l'option est inconnue
	unknown_options_ << option;
}

#ifdef QET_ALLOW_OVERRIDE_CED_OPTION
/**
	@return true si l'utilisateur a specifie un dossier pour la collection
	commune.
*/
bool QETArguments::commonElementsDirSpecified() const {
	return(!common_elements_dir_.isEmpty());
}

/**
	@return le dossier de la collection commune specifie par l'utilisateur.
	Si l'utilisateur n'en a pas specifie, une chaine vide est retournee.
*/
QString QETArguments::commonElementsDir() const {
	return(common_elements_dir_);
}
#endif

#ifdef QET_ALLOW_OVERRIDE_CTBTD_OPTION
/**
	@return true if the user has specified a directory for the common title
	block templates collection
*/
bool QETArguments::commonTitleBlockTemplatesDirSpecified() const {
	return(!common_tbt_dir_.isEmpty());
}

/**
	@return the directory of the common title block templates collection
	specified by the user. If none were specified, return an empty string.
*/
QString QETArguments::commonTitleBlockTemplatesDir() const {
	return(common_tbt_dir_);
}
#endif

#ifdef QET_ALLOW_OVERRIDE_CD_OPTION
/**
	@return true si l'utilisateur a specifie un dossier pour la configuration.
*/
bool QETArguments::configDirSpecified() const {
	return(!config_dir_.isEmpty());
}

/**
	@return le dossier de configuration specifie par l'utilisateur.
	Si l'utilisateur n'en a pas specifie, une chaine vide est retournee.
*/
QString QETArguments::configDir() const {
	return(config_dir_);
}
#endif

/**
	@return true si l'utilisateur a specifie un dossier pour les fichiers de langue
*/
bool QETArguments::langDirSpecified() const {
	return(!lang_dir_.isEmpty());
}

/**
	@return le dossier de langue specifie par l'utilisateur.
	Si l'utilisateur n'en a pas specifie, une chaine vide est retournee.
*/
QString QETArguments::langDir() const {
	return(lang_dir_);
}

/**
	@return true si les arguments comportent une demande d'affichage de l'aide,
	false sinon
*/
bool QETArguments::printHelpRequested() const {
	return(print_help_);
}

/**
	@return true si les arguments comportent une demande d'affichage de la
	licence, false sinon
*/
bool QETArguments::printLicenseRequested() const {
	return(print_license_);
}

/**
	@return true si les arguments comportent une demande d'affichage de la
	version, false sinon
*/
bool QETArguments::printVersionRequested() const {
	return(print_version_);
}
