/*
	Copyright 2006-2018 The QElectroTech Team
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
#include "qetinformation.h"

#include <QObject>
#include <QHash>

/**
 * @brief QETInformation::titleblockInfoKeys
 * @return all available key for use with a titleblock
 */
QStringList QETInformation::titleblockInfoKeys()
{
	QStringList info_list;
	info_list << "author";
	info_list << "date";
	info_list << "title";
	info_list << "filename";
	info_list << "plant";
	info_list << "locmach";
	info_list << "indexrev";
	info_list << "version";
	info_list << "folio";
	info_list << "folio-id";
	info_list << "folio-total";
	info_list << "previous-folio-num";
	info_list << "next-folio-num";
	info_list << "projecttitle";
	info_list << "projectpath";
	info_list << "projectfilename";
	info_list << "saveddate";
	info_list << "savedtime";
	info_list << "savedfilename";
	info_list << "savedfilepath";
	
	return info_list;
}

/**
 * @brief QETInformation::titleblockTranslatedInfoKey
 * @param info : info key to be translated
 * @return the translated information given by @info
 * If  @info don't match, return an empty string
 */
QString QETInformation::titleblockTranslatedInfoKey(const QString &info)
{
	if      (info == "author")             return QObject::tr("Auteur");
	else if (info == "date")               return QObject::tr("Date");
	else if (info == "title")              return QObject::tr("Titre");
	else if (info == "filename")           return QObject::tr("Fichier");
	else if (info == "plant")              return QObject::tr("Installation (=)");
	else if (info == "locmach")            return QObject::tr("Localisation (+)");
	else if (info == "indexrev")           return QObject::tr("Indice de révision");
	else if (info == "version")            return QObject::tr("Version de QElectroTech");
	else if (info == "folio")              return QObject::tr("Numéro de folio");
	else if (info == "folio-id")           return QObject::tr("Position du folio");
	else if (info == "folio-total")        return QObject::tr("Nombre de folio");
	else if (info == "previous-folio-num") return QObject::tr("Numéro du folio précédent");
	else if (info == "next-folio-num")     return QObject::tr("Numéro du folio suivant");
	else if (info == "projecttitle")       return QObject::tr("Titre du projet");
	else if (info == "projectpath")        return QObject::tr("Chemin du fichier du projet");
	else if (info == "projectfilename")    return QObject::tr("Nom du fichier");
	else if (info == "saveddate")          return QObject::tr("Date d'enregistrement du fichier");
	else if (info == "savedtime")          return QObject::tr("Heure d'enregistrement du fichier");
	else if (info == "savedfilename")      return QObject::tr("Nom du fichier enregistré");
	else if (info == "savedfilepath")      return QObject::tr("Chemin du fichier enregistré");
	else return QString();
}

/**
 * @brief QETInformation::titleblockInfoKeysToVar
 * @param info
 * @return var in form %{my-var} corresponding to the info.
 * if there is not available var for the given info, the returned var is %{void}
 */
QString QETInformation::titleblockInfoKeysToVar(const QString &info)
{
	if      (info == "author")             return QString("%author");
	else if (info == "date")               return QString("%date");
	else if (info == "title")              return QString("%title");
	else if (info == "filename")           return QString("%filename");
	else if (info == "plant")              return QString("%plant");
	else if (info == "locmach")            return QString("%locmach");
	else if (info == "indexrev")           return QString("%indexrev");
	else if (info == "version")            return QString("%version");
	else if (info == "folio")              return QString("%folio");
	else if (info == "folio-id")           return QString("%folio-id");
	else if (info == "folio-total")        return QString("%folio-total");
	else if (info == "previous-folio-num") return QString("%previous-folio-num");
	else if (info == "next-folio-num")     return QString("%next-folio-num");
	else if (info == "projecttitle")       return QString("%projecttitle");
	else if (info == "projectpath")        return QString("%projectpath");
	else if (info == "projectfilename")    return QString("%projectfilename");
	else if (info == "saveddate")          return QString("%saveddate");
	else if (info == "savedtime")          return QString("%savedtime");
	else if (info == "savedfilename")      return QString("%savedfilename");
	else if (info == "savedfilepath")      return QString("%savedfilepath");
	else return QString("%void");
}

/**
 * @brief QETInformation::titleblockTranslatedKeyHashVar
 * @return a QHash with for key, the translated information key of title block,
 * and for value the corresponding var.
 */
QHash<QString, QString> QETInformation::titleblockTranslatedKeyHashVar()
{
	QHash <QString, QString> hash_;
	for (QString str : titleblockInfoKeys()) {
		hash_.insert(titleblockTranslatedInfoKey(str), titleblockInfoKeysToVar(str));
	}
	return hash_;
}
