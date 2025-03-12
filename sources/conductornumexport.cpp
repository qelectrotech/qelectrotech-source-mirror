/*
	Copyright 2006-2025 The QElectroTech Team
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
#include "conductornumexport.h"

#include "qetapp.h"
#include "diagram.h"
#include "diagramcontent.h"
#include "qetgraphicsitem/conductor.h"
#include "qetgraphicsitem/conductortextitem.h"
#include "qetgraphicsitem/element.h"
#include "qetgraphicsitem/terminal.h"

#include <QFileDialog>

/**
	@brief ConductorNumExport::ConductorNumExport
	@param project : the project to export the conductors num
	@param parent : parent widget
*/
ConductorNumExport::ConductorNumExport(QETProject *project, QWidget *parent) :
	m_project(project),
	m_parent_widget(parent)
{
	fillHash();
}

/**
	@brief ConductorNumExport::toCsv
	Export the num of conductors into a csv file.
	@return true if suceesfully exported.
*/
bool ConductorNumExport::toCsv()
{
		//save in csv file in same directory as project by default
	QString dir = m_project->currentDir();
	if (dir.isEmpty()) dir = QETApp::documentDir();
	QString name = dir + "/" + QObject::tr("numero_de_fileries_") + m_project->title() + ".csv";
	//    if(!name.endsWith(".csv")) {
	//        name += ".csv";
	//    }

	QString filename = QFileDialog::getSaveFileName(m_parent_widget, QObject::tr("Enregister sous... "), name, QObject::tr("Fichiers csv (*.csv)"));
	QFile file(filename);
	if(!filename.isEmpty())
	{
		if(QFile::exists(filename))
		{
			// if file already exist -> delete it
			if(!QFile::remove(filename))
			{
				QMessageBox::critical(m_parent_widget, QObject::tr("Erreur"),
									  QObject::tr("Impossible de remplacer le fichier!\n\n")+
									  "Destination : "+filename+"\n");
				return false;
			}
		}
		if (file.open(QIODevice::WriteOnly | QIODevice::Text))
		{
			QTextStream stream(&file);
			stream << wiresNum() << &Qt::endl(stream);
		}
		else {
			return false;
		}
	}
	else {
		return false;
	}

	return true;
}

/**
	@brief ConductorNumExport::wiresNum
	@return the wire num formatted in csv
*/
QString ConductorNumExport::wiresNum() const
{
	QString csv;

	QStringList list = m_hash.keys();
	list.sort();
	for (QString key : list)
	{
		for (int i=0; i<m_hash.value(key) ; ++i) {
			csv.append(key + "\n");
		}
	}

	return csv;
}

/**
	@brief ConductorNumExport::fillHash
	make/fill of m_hash
*/
void ConductorNumExport::fillHash()
{
	//We used this rx to avoid insert num composed only withe white space.
	QRegularExpression rx("^ *$");
	for (Diagram *d : m_project->diagrams())
	{
		DiagramContent dc(d, false);
		for (Conductor *c : dc.conductors())
		{
			QString num = c->textItem()->toPlainText();
			if (num.isEmpty() || num.contains(rx)) {
				continue;
			}

			//We must define if the connected terminal is a folio report, if it is the case
			//we don't add the num to the hash because the terminal doesn't represent a real terminal.
			if(!(c->terminal1->parentElement()->linkType() & Element::AllReport))
			{
				int value = m_hash.value(num, 0);
				++value;
				m_hash.insert(num, value);
			}
			if(!(c->terminal2->parentElement()->linkType() & Element::AllReport))
			{
				int value = m_hash.value(num, 0);
				++value;
				m_hash.insert(num, value);
			}
		}
	}
}
