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
#include <QtDebug>

#include "nomenclature.h"
#include "elementprovider.h"
#define PR(x) qDebug() << #x " = " << x;

/**
		Constructor
		@param an project (QETProject) of QET file 
*/
nomenclature::nomenclature(QETProject *project, QWidget *parent):
	QObject(),
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
bool nomenclature::saveToCSVFile() {
	// SAVE IN FILE
	QString name = tr("nomenclature_") + QString(m_project  -> title());
	if (!name.endsWith(".csv")) {
		name += ".csv";
	}
	QString filename = QFileDialog::getSaveFileName(this->m_parent, tr("Enregister sous... "), name, tr("Fichiers csv (*.csv)"));
	QFile file(filename);
	if( !filename.isEmpty() ) {
		if(QFile::exists ( filename )){
			// if file already exist -> delete it
			if(!QFile::remove ( filename ) ){
				QMessageBox::critical(this->m_parent, tr("Erreur"),
									  tr("Impossible de remplacer le fichier!\n\n")+
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
QString nomenclature::getNomenclature() {
	//Process...
	QString data = tr("NOMENCLATURE : ") + m_project -> title() + "\n\n";
	data += tr("N° de folio") +";"
	""+ tr("Titre de folio") +";"
	""+ tr("Désignation qet") +";"
	""+ tr("Label") +";"
	""+ tr("Désignation") +";"
	""+ tr("Commentaire") +";"
	""+ tr("Fabricant") +";"
	""+ tr("Reference") +";"
	""+ tr("Machine-reference") +";"
	""+ tr("Localisation") +";"
	""+ tr("Fonction") +"\n";


	if(m_list_diagram.isEmpty()) return data;

	foreach (Diagram *d, m_list_diagram) {
		//Get only simple, master and unlinked slave element.
		ElementProvider ep(d);
		QList <Element *> list_elements;
		list_elements << ep.find(Element::Simple | Element::Master);
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
QString nomenclature::getElementInfo(const Element *elmt) {
	QString info;

	Diagram *diagram = elmt -> diagram();
	DiagramContext elmt_info = elmt -> elementInformations();

	info += QString::number(diagram -> folioIndex()+1) + ";";
	info += diagram -> title() + ";";
	info += elmt -> name() + ";";
	info += elmt_info["label"].toString() + ";";
	info += elmt_info["designation"].toString() + ";";
	info += elmt_info["comment"].toString() + ";";
	info += elmt_info["manufacturer"].toString() + ";";
	info += elmt_info["manufacturer-reference"].toString() + ";";
	info += elmt_info["machine-manufacturer-reference"].toString() + ";";
	info += elmt_info["location"].toString() + ";";
	info += elmt_info["function"].toString() + "\n";

	return info;
}
