/*
	Copyright 2006-2020 The QElectroTech Team
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

#include <QObject>
#include <QHash>
#include "qetinformation.h"

/** Default information related to element **/
static QString ELMT_LABEL                        = "label";
static QString ELMT_FORMULA                      = "formula";
static QString ELMT_COMMENT                      = "comment";
static QString ELMT_FUNCTION                     = "function";
static QString ELMT_AUX1                         = "auxiliary1";
static QString ELMT_AUX2                         = "auxiliary2";
static QString ELMT_DESCRIPTION                  = "description";
static QString ELMT_DESIGNATION                  = "designation";
static QString ELMT_MANUFACTURER                 = "manufacturer";
static QString ELMT_MANUFACTURER_REF             = "manufacturer_reference";
static QString ELMT_MACHINE_MANUFACTURER_REF     = "machine_manufacturer_reference";
static QString ELMT_SUPPLIER                     = "supplier";
static QString ELMT_QUANTITY                     = "quantity";
static QString ELMT_UNITY                        = "unity";
static QString ELMT_PLANT                        = "plant";
static QString ELMT_LOCATION                     = "location";

//Voir la suite dans QETApp::elementInfoKeys()

/** Default information related to conductor **/
static QString COND_FUNCTION             = "function";
static QString COND_TENSION_PROTOCOL     = "tension/protocol";
static QString COND_COLOR                = "conductor_color";
static QString COND_SECTION              = "conductor_section";
static QString COND_FORMULA              = "formula";
static QString COND_TEXT                 = "text";

/** Default information related to diagram **/
static QString DIA_AUTHOR            = "author";
static QString DIA_DATE              = "date";
static QString DIA_DISPLAY_FOLIO     = "display_folio";
static QString DIA_FILENAME          = "filename";
static QString DIA_FOLIO             = "folio";
static QString DIA_INDEX_REV         = "indexrev";
static QString DIA_LOCMACH           = "locmach";
static QString DIA_PLANT             = "plant";
static QString DIA_POS               = "pos";
static QString DIA_TITLE             = "title";

/**
	@brief QETInformation::titleblockInfoKeys
	@return all available key for use with a titleblock
*/
QStringList QETInformation::titleblockInfoKeys()
{
	QStringList info_list;
	info_list << DIA_AUTHOR;
	info_list << DIA_DATE;
	info_list << DIA_TITLE;
	info_list << DIA_FILENAME;
	info_list << DIA_PLANT;
	info_list << DIA_LOCMACH;
	info_list << DIA_INDEX_REV;
	info_list << "version";
	info_list << DIA_FOLIO;
	info_list << "folio-id";
	info_list << "folio-total";
	info_list << "previous-folio-num";
	info_list << "next-folio-num";
	info_list << "projecttitle";
	info_list << "projectpath";
	info_list << "projectfilename";
	info_list << "saveddate";
	info_list << "saveddate-eu";
	info_list << "saveddate-us";
	info_list << "savedtime";
	info_list << "savedfilename";
	info_list << "savedfilepath";

	return info_list;
}

/**
	@brief QETInformation::titleblockInfoKeysToVar
	@param info
	@return var in form %{my-var} corresponding to the info.
	if there is not available var for the given info, the returned var is %{void}
*/
QString QETInformation::titleblockInfoKeysToVar(const QString &info)
{
	if (titleblockInfoKeys().contains(info))
		return infoToVar(info);
	else
		return QString("%{void}");
}

/**
	@brief QETInformation::titleblockTranslatedKeyHashVar
	@return a QHash with for key, the translated information key of title block,
	and for value the corresponding var.
*/
QHash<QString, QString> QETInformation::titleblockTranslatedKeyHashVar()
{
	QHash <QString, QString> hash_;
	for (QString str : titleblockInfoKeys()) {
		hash_.insert(translatedInfoKey(str), titleblockInfoKeysToVar(str));
	}
	return hash_;
}

/**
 * @brief QETInformation::folioReportInfoKey
 * @return The info key available for dynamic element text item of a folio report
 */
QStringList QETInformation::folioReportInfoKeys()
{
	QStringList list = {ELMT_LABEL,
						COND_FUNCTION,
						COND_TENSION_PROTOCOL,
						COND_COLOR,
						COND_SECTION};

	return list;
}
QHash<QString, QString> QETInformation::folioReportInfoKeyToVar()
{
	QHash <QString, QString> H_;
	H_.insert(ELMT_LABEL,            infoToVar(ELMT_LABEL));
	H_.insert(COND_FUNCTION,         infoToVar(COND_FUNCTION));
	H_.insert(COND_TENSION_PROTOCOL, infoToVar(COND_TENSION_PROTOCOL));
	H_.insert(COND_COLOR,            infoToVar(COND_COLOR));
	H_.insert(COND_SECTION,          infoToVar(COND_SECTION));

	return H_;
}

QStringList QETInformation::conductorInfoKeys()
{
	QStringList list = 	{COND_FORMULA,
						 COND_TEXT,
						 COND_FUNCTION,
						 COND_TENSION_PROTOCOL,
						 COND_COLOR,
						 COND_SECTION};

	return  list;
}

QStringList QETInformation::diagramInfoKeys()
{
	QStringList list = {DIA_TITLE,
					   DIA_AUTHOR,
					   DIA_FILENAME,
					   DIA_FOLIO,
					   DIA_PLANT,
					   DIA_LOCMACH,
					   DIA_INDEX_REV,
					   DIA_DATE,
					   DIA_DISPLAY_FOLIO};
	return list;
}

QStringList QETInformation::elementInfoKeys()
{
	QStringList list = { ELMT_FORMULA,
						 ELMT_LABEL,
						 ELMT_PLANT,
						 ELMT_LOCATION,
						 ELMT_COMMENT,
						 ELMT_FUNCTION,
						 ELMT_AUX1,
						 ELMT_AUX2,
						 ELMT_DESCRIPTION,
						 ELMT_DESIGNATION,
						 ELMT_MANUFACTURER,
						 ELMT_MANUFACTURER_REF,
						 ELMT_MACHINE_MANUFACTURER_REF,
						 ELMT_SUPPLIER,
						 ELMT_QUANTITY,
						 ELMT_UNITY};
	return list;
}

/**
 * @brief QETInformation::elementInfoToVar
 * @param info
 * @return The info to a variable or %{void} if @info
 * is not an element information
 */
QString QETInformation::elementInfoToVar(const QString &info)
{
	if (QETInformation::elementInfoKeys().contains(info))
		return infoToVar(info);
	else
		return (QString ("%{void}"));
}

/**
 * @brief QETInformation::infoToVar
 * @param info
 * @return return the string @info prepended by %{ ans appended by }
 * exemple : if info is label, the returned string is %{label}
 */
QString QETInformation::infoToVar(const QString &info)
{
	return QString("%{")+info+QString("}");
}

/**
 * @brief QETInformation::translatedInfoKey
 * @param info
 * @return the information translated to the current local.
 * This function translate all information : element, conductor, diagram
 * Return an empty QString if don't match
 */
QString QETInformation::translatedInfoKey(const QString &info)
{
	if      (info == DIA_AUTHOR)                       return QObject::tr("Auteur");
	else if (info == DIA_DATE)                         return QObject::tr("Date");
	else if (info == DIA_TITLE)                        return QObject::tr("Titre");
	else if (info == DIA_FILENAME)                     return QObject::tr("Fichier");
	else if (info == DIA_PLANT)                        return QObject::tr("Installation (=)");
	else if (info == DIA_LOCMACH)                      return QObject::tr("Localisation (+)");
	else if (info == DIA_INDEX_REV)                    return QObject::tr("Indice de révision");
	else if (info == DIA_POS)                          return QObject::tr("Position");
	else if (info == "version")                        return QObject::tr("Version de QElectroTech");
	else if (info == "folio")                          return QObject::tr("Numéro de folio");
	else if (info == "folio-id")                       return QObject::tr("Position du folio");
	else if (info == "folio-total")                    return QObject::tr("Nombre de folio");
	else if (info == "previous-folio-num")             return QObject::tr("Numéro du folio précédent");
	else if (info == "next-folio-num")                 return QObject::tr("Numéro du folio suivant");
	else if (info == "projecttitle")                   return QObject::tr("Titre du projet");
	else if (info == "projectpath")                    return QObject::tr("Chemin du fichier du projet");
	else if (info == "projectfilename")                return QObject::tr("Nom du fichier");
	else if (info == "saveddate")                      return QObject::tr("Date d'enregistrement du fichier format local");
	else if (info == "saveddate-eu")                   return QObject::tr("Date d'enregistrement du fichier format dd-MM-yyyy");
	else if (info == "saveddate-us")                   return QObject::tr("Date d'enregistrement du fichier format yyyy-MM-dd");
	else if (info == "savedtime")                      return QObject::tr("Heure d'enregistrement du fichier");
	else if (info == "savedfilename")                  return QObject::tr("Nom du fichier enregistré");
	else if (info == "savedfilepath")                  return QObject::tr("Chemin du fichier enregistré");
	else if (info == ELMT_LABEL)                       return QObject::tr("Formule du label");
	else if (info == ELMT_LABEL)                       return QObject::tr("Label");
	else if (info == ELMT_COMMENT)                     return QObject::tr("Commentaire");
	else if (info == ELMT_FUNCTION)                    return QObject::tr("Fonction");
	else if (info == ELMT_AUX1)                        return QObject::tr("Bloc auxiliaire 1");
	else if (info == ELMT_AUX2)                        return QObject::tr("Bloc auxiliaire 2");
	else if (info == ELMT_DESCRIPTION)                 return QObject::tr("Description textuelle");
	else if (info == ELMT_DESIGNATION)                 return QObject::tr("Numéro d'article");
	else if (info == ELMT_MANUFACTURER)                return QObject::tr("Fabricant");
	else if (info == ELMT_MANUFACTURER_REF)            return QObject::tr("Numéro de commande");
	else if (info == ELMT_MACHINE_MANUFACTURER_REF)    return QObject::tr("Numéro interne");
	else if (info == ELMT_SUPPLIER)                    return QObject::tr("Fournisseur");
	else if (info == ELMT_QUANTITY)                    return QObject::tr("Quantité");
	else if (info == ELMT_UNITY)                       return QObject::tr("Unité");
	else if (info == COND_FUNCTION)                    return QObject::tr("Fonction");
	else if (info == COND_TENSION_PROTOCOL)            return QObject::tr("Tension / Protocole");
	else if (info == COND_COLOR)                       return QObject::tr("Couleur du fil");
	else if (info == COND_SECTION)                     return QObject::tr("Section du fil");
	else if (info == COND_TEXT)                        return QObject::tr("Texte");
	else if (info == COND_FORMULA)                     return QObject::tr("Formule du texte");
	else return QString();
}
