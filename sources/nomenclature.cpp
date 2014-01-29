/*
		Copyright 2006-2014 The QElectroTech Team
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
	if(!m_project->isEmpty()){
		//get list of schema present in project
		m_list_diagram = m_project -> diagrams();
	}
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
	if(m_list_diagram.isEmpty()) return false;
	
	//Process...
	QString data = tr("NOMENCLATURE : ") + m_project -> title() + "\n\n";
	data += tr("Ref") +";"+ tr("Folio") +";"+ tr("Sch\351ma") +";"+ tr("D\351signation\n");
	QStringList rows;
	for(int i=0; i<m_list_diagram.count(); i++){
		rows = getRows(m_list_diagram.at(i));
		for(int j=0;j<rows.count();j++){
			data += rows.at(j);
		}
	}
	
	// SAVE IN FILE
	QString name = tr("nomenclature_") + QString(m_project  -> title());
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
			stream << data << endl;
		}
		else return false;
	}
	else return false;
	
	return true;
}

/**
		gets rows of nomenclature
		@return the list of rows
*/
QStringList nomenclature::getRows(Diagram *schema) {
	QString row;
	QStringList list;
	QList<Element *> elements_list;
	//QList<CustomElement *> elements_list;
	
	//elements_list = schema->customElements();
	elements_list = schema->content().elements.toList();
	for(int j=0;j<elements_list.count();j++){
		row += QString::number(0) + ";";
		row += QString::number(0) + ";";
		row += schema->title() + ";";
		row += elements_list.at(j)->name() + "\n";
		list << row;
	}
	return list;
}

