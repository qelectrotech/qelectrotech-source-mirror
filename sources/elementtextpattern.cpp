/*
	Copyright 2006-2017 The QElectroTech Team
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
#include "elementtextpattern.h"
#include "qetapp.h"
#include "element.h"
#include "qet.h"
#include "dynamicelementtextitem.h"
#include "elementtextitemgroup.h"
#include "addelementtextcommand.h"
#include "diagram.h"

#include <QDir>
#include <QInputDialog>
#include <QGraphicsScene>
#include <QMessageBox>
#include <QObject>
#include <QGraphicsView>
#include <QStringList>

/**
 * @brief ExportElementTextPattern::ExportElementTextPattern
 * Constructor
 * @param elmt
 */
ExportElementTextPattern::ExportElementTextPattern(Element *elmt) :
	m_element(elmt)
{
		//Create the conf dir if not yet.
	QDir dir(QETApp::configDir()+ "/element_texts_pattern");
	if(!dir.exists())
	{
		dir.cdUp();
		dir.mkdir("element_texts_pattern");
		dir.cd("element_texts_pattern");
	}
	
		//Get the name of conf
	bool ok;
	m_name = getName(ok);
	if(ok == false)
		return;
	
		//Check if a conf with the same name already exist
	if (QFileInfo::exists(dir.absoluteFilePath(m_name + ".xml")))
	{
		bool r = QMessageBox::question(parentWidget(),
									   QObject::tr("Configuration de textes"),
									   QObject::tr("Une configuration de textes nommé << %1 >> existe déjà.\n"
												   "Voulez-vous la remplacer ?").arg(m_name));
		
		if(r == false)
			return;
	}
	
	QDomDocument doc = xmlConf();
	QET::writeXmlFile(doc, dir.absoluteFilePath(m_name + ".xml"));
}

/**
 * @brief ExportElementTextConf::getName
 * Open a dialog to let user set the name of the conf and return it
 * @return 
 */
QString ExportElementTextPattern::getName(bool &ok) const
{	
	QString text = QInputDialog::getText(parentWidget(),
										 QObject::tr("Nom de la configuration"),
										 QObject::tr("Entrer le nom de la configuration à créer"),
										 QLineEdit::Normal,
										 QString(),
										 &ok);
	
	text.replace(" ", "_");
	text.replace(".","");
	return text;
}

QWidget *ExportElementTextPattern::parentWidget() const
{
	QWidget *parent = nullptr;
	if(m_element->scene() && !m_element->scene()->views().isEmpty())
		parent = m_element->scene()->views().first();
	
	return parent;
}

QDomDocument ExportElementTextPattern::xmlConf() const
{
	QDomDocument doc;
	QDomElement root = doc.createElement("Element_texts_pattern");
	root.setAttribute("name", m_name);
	doc.appendChild(root);
	
	QHash<Terminal *, int> H;
	QDomElement elmt = m_element->toXml(doc, H);
	QDomElement texts = elmt.firstChildElement("dynamic_texts");
	QDomElement groups = elmt.firstChildElement("texts_groups");
	
	if(texts.tagName() == "dynamic_texts")
		root.appendChild(texts);
	if(groups.tagName() == "texts_groups")
		root.appendChild(groups);
	
	return doc;
}

//*******************//
//******IMPORT*******//
//*******************//

ImportElementTextPattern::ImportElementTextPattern(Element *elmt):
	m_element(elmt)
{
	bool exist = true;
	QDir dir(QETApp::configDir()+ "/element_texts_pattern");
	
	if(!dir.exists())
		exist = false;

	QStringList entry = dir.entryList(QDir::Files | QDir::NoDotAndDotDot);
	QStringList result = entry.filter(".xml");
	if(result.isEmpty())
		exist = false;
	
	if(!exist)
	{
		QMessageBox::information(parentWidget(),
								 QObject::tr("Configuration de textes"),
								 QObject::tr("Aucune configuration de textes existante."));
		return;
	}
	
	bool ok=false;
		//Remove the .xml extention of the files
	result.replaceInStrings(".xml", "");
	QString name = getName(result, ok);
	
	if(!ok || name.isEmpty())
		return;
	else
		apply(name);
}

/**
 * @brief ImportElementTextPattern::getName
 * Open a dialog to let user select a conf
 * @param ok
 * @return 
 */
QString ImportElementTextPattern::getName(const QStringList& list, bool &ok) const
{
	return QInputDialog::getItem(parentWidget(),
								 QObject::tr("Séléctionner une configuration de textes"),
								 QObject::tr("Séléctionner la configuration de textes à ajouter à l'élément"),
								 list,
								 0,
								 false,
								 &ok);
}

QWidget *ImportElementTextPattern::parentWidget() const
{
	QWidget *parent = nullptr;
	if(m_element->scene() && !m_element->scene()->views().isEmpty())
		parent = m_element->scene()->views().first();
	
	return parent;
}

/**
 * @brief ImportElementTextPattern::apply
 * Apply the user choice
 * @param name : the name of the selected pattern
 */
void ImportElementTextPattern::apply(QString name) const
{
	if(!name.endsWith(".xml"))
		name.append(".xml");
	
	QFile conf_file(QETApp::configDir() + "/element_texts_pattern/" + name);
	if(!conf_file.open(QIODevice::ReadOnly | QIODevice::Text))
		return;
	
		//Load the content of the xml file
	QDomDocument xml_conf;
	if(!xml_conf.setContent(&conf_file))
		return;
	
	QDomElement root = xml_conf.firstChildElement("Element_texts_pattern");
		
		//Get the text
	QList <QDomElement> texts = QET::findInDomElement(root, "dynamic_texts", "dynamic_elmt_text");
	if (texts.isEmpty())
		return;
	
		//Replace the original uuid of texts in the xml description, by a new one for every texts
	QHash<QUuid, QUuid> uuid_hash;
	for(QDomElement text : texts)
	{
		QUuid original_uuid(text.attribute("uuid"));
		QUuid new_uuid = QUuid::createUuid();
		text.setAttribute("uuid", new_uuid.toString());
		
		uuid_hash.insert(original_uuid, new_uuid);
	}
	
		//In each group of the xml description, replace the original uuids, by the news created upper. 
	QList <QDomElement> groups = QET::findInDomElement(root, "texts_groups", "texts_group");
	for(const QDomElement& group : groups)
	{
		for(QDomElement text : QET::findInDomElement(group, "texts", "text"))
		{
			QUuid original_uuid(text.attribute("uuid"));
			QUuid new_uuid = uuid_hash.value(original_uuid);
			if(!new_uuid.isNull())
				text.setAttribute("uuid", new_uuid.toString());
		}
	}
	
	QUndoStack &undo_stack = m_element->diagram()->undoStack();
	undo_stack.beginMacro(QObject::tr("Importer la configuration de texte : %1").arg(name.remove(".xml")));
	
		//Add the texts to element
	for(const QDomElement& text : texts)
	{
		DynamicElementTextItem *deti = new DynamicElementTextItem(m_element);
		undo_stack.push(new AddElementTextCommand(m_element, deti));
		deti->fromXml(text);
	}
		//Add the groups to element
	for(const QDomElement& xml_group : groups)
		undo_stack.push(new AddTextsGroupCommand(m_element, xml_group));
	
	undo_stack.endMacro();
}
