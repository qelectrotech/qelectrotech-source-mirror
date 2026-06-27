/*
	Copyright 2006-2026 The QElectroTech Team
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
	inline const QString ELMT_LABEL = QStringLiteral("label");
	inline const QString ELMT_FORMULA = QStringLiteral("formula");
	inline const QString ELMT_COMMENT = QStringLiteral("comment");
	inline const QString ELMT_FUNCTION = QStringLiteral("function");
	inline const QString ELMT_DESCRIPTION = QStringLiteral("description");
	inline const QString ELMT_DESIGNATION = QStringLiteral("designation");
	inline const QString ELMT_MANUFACTURER = QStringLiteral("manufacturer");
	inline const QString ELMT_MANUFACTURER_REF = QStringLiteral("manufacturer_reference");
	inline const QString ELMT_MACHINE_MANUFACTURER_REF = QStringLiteral("machine_manufacturer_reference");
	inline const QString ELMT_SUPPLIER = QStringLiteral("supplier");
	inline const QString ELMT_QUANTITY = QStringLiteral("quantity");
	inline const QString ELMT_UNITY = QStringLiteral("unity");
	inline const QString ELMT_PLANT = QStringLiteral("plant");
	inline const QString ELMT_LOCATION = QStringLiteral("location");
	inline const QString ELMT_AUX1 = QStringLiteral("auxiliary1");
	inline const QString ELMT_DESCRIPTION_AUX1 = QStringLiteral("description_auxiliary1");
	inline const QString ELMT_DESIGNATION_AUX1 = QStringLiteral("designation_auxiliary1");
	inline const QString ELMT_MANUFACTURER_AUX1 = QStringLiteral("manufacturer_auxiliary1");
	inline const QString ELMT_MANUFACTURER_REF_AUX1 = QStringLiteral("manufacturer_reference_auxiliary1");
	inline const QString ELMT_MACHINE_MANUFACTURER_REF_AUX1 = QStringLiteral("machine_manufacturer_reference_auxiliary1");
	inline const QString ELMT_SUPPLIER_AUX1 = QStringLiteral("supplier_auxiliary1");
	inline const QString ELMT_QUANTITY_AUX1 = QStringLiteral("quantity_auxiliary1");
	inline const QString ELMT_UNITY_AUX1 = QStringLiteral("unity_auxiliary1");
	inline const QString ELMT_AUX2 = QStringLiteral("auxiliary2");
	inline const QString ELMT_DESCRIPTION_AUX2 = QStringLiteral("description_auxiliary2");
	inline const QString ELMT_DESIGNATION_AUX2 = QStringLiteral("designation_auxiliary2");
	inline const QString ELMT_MANUFACTURER_AUX2 = QStringLiteral("manufacturer_auxiliary2");
	inline const QString ELMT_MANUFACTURER_REF_AUX2 = QStringLiteral("manufacturer_reference_auxiliary2");
	inline const QString ELMT_MACHINE_MANUFACTURER_REF_AUX2 = QStringLiteral("machine_manufacturer_reference_auxiliary2");
	inline const QString ELMT_SUPPLIER_AUX2 = QStringLiteral("supplier_auxiliary2");
	inline const QString ELMT_QUANTITY_AUX2 = QStringLiteral("quantity_auxiliary2");
	inline const QString ELMT_UNITY_AUX2 = QStringLiteral("unity_auxiliary2");
	inline const QString ELMT_AUX3 = QStringLiteral("auxiliary3");
	inline const QString ELMT_DESCRIPTION_AUX3 = QStringLiteral("description_auxiliary3");
	inline const QString ELMT_DESIGNATION_AUX3 = QStringLiteral("designation_auxiliary3");
	inline const QString ELMT_MANUFACTURER_AUX3 = QStringLiteral("manufacturer_auxiliary3");
	inline const QString ELMT_MANUFACTURER_REF_AUX3 = QStringLiteral("manufacturer_reference_auxiliary3");
	inline const QString ELMT_MACHINE_MANUFACTURER_REF_AUX3 = QStringLiteral("machine_manufacturer_reference_auxiliary3");
	inline const QString ELMT_SUPPLIER_AUX3 = QStringLiteral("supplier_auxiliary3");
	inline const QString ELMT_QUANTITY_AUX3 = QStringLiteral("quantity_auxiliary3");
	inline const QString ELMT_UNITY_AUX3 = QStringLiteral("unity_auxiliary3");
	inline const QString ELMT_AUX4 = QStringLiteral("auxiliary4");
	inline const QString ELMT_DESCRIPTION_AUX4 = QStringLiteral("description_auxiliary4");
	inline const QString ELMT_DESIGNATION_AUX4 = QStringLiteral("designation_auxiliary4");
	inline const QString ELMT_MANUFACTURER_AUX4 = QStringLiteral("manufacturer_auxiliary4");
	inline const QString ELMT_MANUFACTURER_REF_AUX4 = QStringLiteral("manufacturer_reference_auxiliary4");
	inline const QString ELMT_MACHINE_MANUFACTURER_REF_AUX4 = QStringLiteral("machine_manufacturer_reference_auxiliary4");
	inline const QString ELMT_SUPPLIER_AUX4 = QStringLiteral("supplier_auxiliary4");
	inline const QString ELMT_QUANTITY_AUX4 = QStringLiteral("quantity_auxiliary4");
	inline const QString ELMT_UNITY_AUX4 = QStringLiteral("unity_auxiliary4");


	/** Default information related to conductor **/
	inline const QString COND_FUNCTION = QStringLiteral("function");
	inline const QString COND_TENSION_PROTOCOL = QStringLiteral("tension_protocol");
	inline const QString COND_COLOR = QStringLiteral("conductor_color");
	inline const QString COND_SECTION = QStringLiteral("conductor_section");
	inline const QString COND_FORMULA = QStringLiteral("formula");
	inline const QString COND_TEXT = QStringLiteral("text");

	/** Default information related to diagram **/
	inline const QString DIA_AUTHOR = QStringLiteral("author");
	inline const QString DIA_DATE = QStringLiteral("date");
	inline const QString DIA_DISPLAY_FOLIO = QStringLiteral("display_folio");
	inline const QString DIA_FILENAME = QStringLiteral("filename");
	inline const QString DIA_FOLIO = QStringLiteral("folio");
	inline const QString DIA_INDEX_REV = QStringLiteral("indexrev");
	inline const QString DIA_LOCMACH = QStringLiteral("locmach");
	inline const QString DIA_PLANT = QStringLiteral("plant");
	inline const QString DIA_POS = QStringLiteral("pos");
	inline const QString DIA_TITLE = QStringLiteral("title");
	inline const QString DIA_FOLIO_ID = QStringLiteral("folio-id");
	inline const QString DIA_PREVIOUS_FOLIO_NUM = QStringLiteral("previous-folio-num");
	inline const QString DIA_NEXT_FOLIO_NUM = QStringLiteral("next-folio-num");

	/** Default information related to project **/
	inline const QString PROJECT_FOLIO_TOTAL = QStringLiteral("folio-total");
	inline const QString PROJECT_TITLE = QStringLiteral("projecttitle");
	inline const QString PROJECT_PATH = QStringLiteral("projectpath");
	inline const QString PROJECT_FILE_NAME = QStringLiteral("projectfilename");
	inline const QString PROJECT_SAVE_DATE = QStringLiteral("saveddate");
	inline const QString PROJECT_SAVE_DATE_EU = QStringLiteral("saveddate-eu");
	inline const QString PROJECT_SAVE_DATE_US = QStringLiteral("saveddate-us");
	inline const QString PROJECT_SAVE_TIME = QStringLiteral("savedtime");
	inline const QString PROJECT_SAVED_FILE_NAME = QStringLiteral("savedfilename");
	inline const QString PROJECT_SAVED_FILE_PATH = QStringLiteral("savedfilepath");


	/** Default information related to QElectroTech **/
	inline const QString QET_VERSION = QStringLiteral("version");





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
