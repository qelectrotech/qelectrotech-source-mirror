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
#include <QtDebug>

#include "nomenclature.h"
#include "elementprovider.h"
#define PR(x) qDebug() << #x " = " << x;

/**
		Constructor
		@param an project (QETProject) of QET file 
*/
nomenclature::nomenclature(QETProject *project, QWidget *parent):
	m_project(project)
{
	m_parent = parent;
	//get list of schema present in project
	m_list_diagram = m_project -> diagrams();
}

/**
		Destructor
*/
nomenclature::~nomenclature() {
}

/**
		Save to csv file
		@param true if success
*/
bool nomenclature::saveToCSVFile()
{
	// SAVE IN FILE
	QString name = QObject::tr("nomenclature_") + QString(m_project  -> title());
	if (!name.endsWith(".csv")) {
		name += ".csv";
	}
	QString filename = QFileDialog::getSaveFileName(this->m_parent, QObject::tr("Enregister sous... "), name, QObject::tr("Fichiers csv (*.csv)"));
	QFile file(filename);
	if( !filename.isEmpty() ) {
		if(QFile::exists ( filename )){
			// if file already exist -> delete it
			if(!QFile::remove ( filename ) ){
				QMessageBox::critical(this->m_parent, QObject::tr("Erreur"),
									  QObject::tr("Impossible de remplacer le fichier!\n\n")+
									  "Destination: "+filename+"\n");
				return false;
			}	
		}
		if (file.open(QIODevice::WriteOnly | QIODevice::Text)){
			QTextStream stream(&file);
			stream << getNomenclature() << endl;
		}
		else return false;
	}
	else return false;
	
	return true;
}

/**
 * @brief nomenclature::getNomenclature
 * Create and formated a nomenclature to csv file.
 * @return The QString of nomenclature
 */
QString nomenclature::getNomenclature()
{
	//Process...
	QString data = QObject::tr("NOMENCLATURE : ") + m_project -> title() + "\n\n";
	data += QObject::tr("N° de folio") +";"
	""+ QObject::tr("Titre de folio") +";"
	""+ QObject::tr("Label de folio") +";"
	""+ QObject::tr("Désignation qet") +";"
	""+ QObject::tr("Position") +";"
	""+ QObject::tr("Label") +";"
	""+ QObject::tr("Désignation") +";"
	""+ QObject::tr("Commentaire") +";"
	""+ QObject::tr("Fabricant") +";"
	""+ QObject::tr("Reference") +";"
	""+ QObject::tr("Bloc auxiliaire 1") +";"
	""+ QObject::tr("Bloc auxiliaire 2") +";"
	""+ QObject::tr("Machine-reference") +";"
	""+ QObject::tr("Localisation") +";"
	""+ QObject::tr("Fonction") +"\n";


	if(m_list_diagram.isEmpty()) return data;

	foreach (Diagram *d, m_list_diagram) {
		//Get only simple, master and unlinked slave element.
		ElementProvider ep(d);
		QSettings settings;
		QList <Element *> list_elements;
		
		if (settings.value("nomenclature/terminal-exportlist", true).toBool()){
		list_elements << ep.find(Element::Simple | Element::Master | Element::Terminale);
		
		}else{
		
		list_elements << ep.find(Element::Simple | Element::Master);
		
		}
		
		list_elements << ep.freeElement(Element::Slave);

		foreach (Element *elmt, list_elements) {
			data += getElementInfo(elmt);
		}
	}

	return data;
}

/**
 * @brief nomenclature::getElementInfo
 * @param elmt : the element to getinfo
 * @return : QString with information about element formated to csv file
 */
QString nomenclature::getElementInfo(Element *elmt) {
	QString info;

	Diagram *diagram = elmt -> diagram();
	DiagramContext elmt_info = elmt -> elementInformations();

	info += QString::number(diagram -> folioIndex()+1) + ";";
	info += diagram -> title() + ";";
	info += diagram -> border_and_titleblock.folio() + ";";
	info += elmt -> name() + ";";
	info += elmt-> diagram()-> convertPosition(elmt -> scenePos()).toString() + ";";
	info += elmt->assignVariables(elmt_info["label"].toString(), elmt) + ";";
	info += elmt->assignVariables(elmt_info["designation"].toString(), elmt) + ";";
	info += elmt->assignVariables(elmt_info["comment"].toString(), elmt) + ";";
	info += elmt->assignVariables(elmt_info["manufacturer"].toString(), elmt) + ";";
	info += elmt->assignVariables(elmt_info["manufacturer-reference"].toString(), elmt) + ";";
	info += elmt->assignVariables(elmt_info["auxiliary1"].toString(), elmt) + ";";
	info += elmt->assignVariables(elmt_info["auxiliary2"].toString(), elmt) + ";";
	info += elmt->assignVariables(elmt_info["machine-manufacturer-reference"].toString(), elmt) + ";";
	info += elmt->assignVariables(elmt_info["location"].toString(), elmt) + ";";
	info += elmt->assignVariables(elmt_info["function"].toString(), elmt) + "\n";
	return info;
}
