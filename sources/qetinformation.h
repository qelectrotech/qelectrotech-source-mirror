/*
	Copyright 2006-2025 The QElectroTech Team
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
#ifndef QETINFORMATION_H
#define QETINFORMATION_H

#include <QStringList>
#include <QHash>

/**
 * Inside this namespace you will find all information used in QElectrotech for
 * element, conductor and diagram.
 * Each information have 3 values :
 * #1 the info key = the key of an information as a QString used in the code (example : label)
 * #2 the info key to variable = the key in form of a variable.
 * This is used by the user to replace a variable by the string of this variable (example : %{label})
 * #3 the info key translated to the current local (example label in dutch = Betriebsmittelkennzeichen)
 */
namespace QETInformation
{
	/** Default information related to element **/
	static QString ELMT_LABEL                        = "label";
	static QString ELMT_FORMULA                      = "formula";
	static QString ELMT_COMMENT                      = "comment";
	static QString ELMT_FUNCTION                     = "function";
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
	static QString ELMT_AUX1                         = "auxiliary1";
	static QString ELMT_DESCRIPTION_AUX1                 = "description_auxiliary1";
	static QString ELMT_DESIGNATION_AUX1                  = "designation_auxiliary1";
	static QString ELMT_MANUFACTURER_AUX1                 = "manufacturer_auxiliary1";
	static QString ELMT_MANUFACTURER_REF_AUX1             = "manufacturer_reference_auxiliary1";
	static QString ELMT_MACHINE_MANUFACTURER_REF_AUX1     = "machine_manufacturer_reference_auxiliary1";
	static QString ELMT_SUPPLIER_AUX1                     = "supplier_auxiliary1";
	static QString ELMT_QUANTITY_AUX1                     = "quantity_auxiliary1";
	static QString ELMT_UNITY_AUX1                        = "unity_auxiliary1";
	static QString ELMT_AUX2                         = "auxiliary2";
	static QString ELMT_DESCRIPTION_AUX2                 = "description_auxiliary2";
	static QString ELMT_DESIGNATION_AUX2                  = "designation_auxiliary2";
	static QString ELMT_MANUFACTURER_AUX2                 = "manufacturer_auxiliary2";
	static QString ELMT_MANUFACTURER_REF_AUX2             = "manufacturer_reference_auxiliary2";
	static QString ELMT_MACHINE_MANUFACTURER_REF_AUX2     = "machine_manufacturer_reference_auxiliary2";
	static QString ELMT_SUPPLIER_AUX2                     = "supplier_auxiliary2";
	static QString ELMT_QUANTITY_AUX2                     = "quantity_auxiliary2";
	static QString ELMT_UNITY_AUX2                        = "unity_auxiliary2";
	static QString ELMT_AUX3                         = "auxiliary3";
	static QString ELMT_DESCRIPTION_AUX3                 = "description_auxiliary3";
	static QString ELMT_DESIGNATION_AUX3                  = "designation_auxiliary3";
	static QString ELMT_MANUFACTURER_AUX3                 = "manufacturer_auxiliary3";
	static QString ELMT_MANUFACTURER_REF_AUX3             = "manufacturer_reference_auxiliary3";
	static QString ELMT_MACHINE_MANUFACTURER_REF_AUX3     = "machine_manufacturer_reference_auxiliary3";
	static QString ELMT_SUPPLIER_AUX3                     = "supplier_auxiliary3";
	static QString ELMT_QUANTITY_AUX3                     = "quantity_auxiliary3";
	static QString ELMT_UNITY_AUX3                        = "unity_auxiliary3";
	static QString ELMT_AUX4                         = "auxiliary4";
	static QString ELMT_DESCRIPTION_AUX4                 = "description_auxiliary4";
	static QString ELMT_DESIGNATION_AUX4                  = "designation_auxiliary4";
	static QString ELMT_MANUFACTURER_AUX4                 = "manufacturer_auxiliary4";
	static QString ELMT_MANUFACTURER_REF_AUX4             = "manufacturer_reference_auxiliary4";
	static QString ELMT_MACHINE_MANUFACTURER_REF_AUX4     = "machine_manufacturer_reference_auxiliary4";
	static QString ELMT_SUPPLIER_AUX4                     = "supplier_auxiliary4";
	static QString ELMT_QUANTITY_AUX4                     = "quantity_auxiliary4";
	static QString ELMT_UNITY_AUX4                        = "unity_auxiliary4";


	/** Default information related to conductor **/
	static QString COND_FUNCTION             = "function";
	static QString COND_TENSION_PROTOCOL     = "tension_protocol";
	static QString COND_COLOR                = "conductor_color";
	static QString COND_SECTION              = "conductor_section";
	static QString COND_FORMULA              = "formula";
	static QString COND_TEXT                 = "text";

	/** Default information related to diagram **/
	static QString DIA_AUTHOR             = "author";
	static QString DIA_DATE               = "date";
	static QString DIA_DISPLAY_FOLIO      = "display_folio";
	static QString DIA_FILENAME           = "filename";
	static QString DIA_FOLIO              = "folio";
	static QString DIA_INDEX_REV          = "indexrev";
	static QString DIA_LOCMACH            = "locmach";
	static QString DIA_PLANT              = "plant";
	static QString DIA_POS                = "pos";
	static QString DIA_TITLE              = "title";
	static QString DIA_FOLIO_ID           = "folio-id";
	static QString DIA_PREVIOUS_FOLIO_NUM = "previous-folio-num";
	static QString DIA_NEXT_FOLIO_NUM     = "next-folio-num";

	/** Default information related to project **/
	static QString PROJECT_FOLIO_TOTAL     = "folio-total";
	static QString PROJECT_TITLE           = "projecttitle";
	static QString PROJECT_PATH            = "projectpath";
	static QString PROJECT_FILE_NAME       = "projectfilename";
	static QString PROJECT_SAVE_DATE       = "saveddate";
	static QString PROJECT_SAVE_DATE_EU    = "saveddate-eu";
	static QString PROJECT_SAVE_DATE_US    = "saveddate-us";
	static QString PROJECT_SAVE_TIME       = "savedtime";
	static QString PROJECT_SAVED_FILE_NAME = "savedfilename";
	static QString PROJECT_SAVED_FILE_PATH = "savedfilepath";


	/** Default information related to QElectroTech **/
	static QString QET_VERSION = "version";





	QStringList titleblockInfoKeys();
	QString titleblockInfoKeysToVar(const QString &info);
	QHash <QString, QString> titleblockTranslatedKeyHashVar();

	QStringList folioReportInfoKeys();
	QHash <QString, QString> folioReportInfoKeyToVar();
	QString folioReportInfoToVar(const QString &info);

	QStringList conductorInfoKeys();
	QStringList diagramInfoKeys();

	QStringList elementInfoKeys();
	QStringList elementEditorElementInfoKeys();
	QString elementInfoToVar(const QString &info);

	QStringList terminalElementInfoKeys();

	QString infoToVar(const QString &info);
	QString translatedInfoKey(const QString &info);
}

#endif // QETINFORMATION_H
