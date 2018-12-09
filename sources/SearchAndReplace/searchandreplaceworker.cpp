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
#include "searchandreplaceworker.h"
#include "diagram.h"
#include "changetitleblockcommand.h"
#include "changeelementinformationcommand.h"
#include "element.h"
#include "qetapp.h"
#include "independenttextitem.h"
#include "diagramcommands.h"
#include "QPropertyUndoCommand/qpropertyundocommand.h"


SearchAndReplaceWorker::SearchAndReplaceWorker()
{
	m_conductor_properties = invalidConductorProperties();
}

/**
 * @brief SearchAndReplaceWorker::replaceDiagram
 * Replace all properties of each diagram in @diagram_list,
 * by the current titleblock propertie of this worker
 * @param diagram_list, list of diagram to be changed, all diagrams must belong to the same project;
 */
void SearchAndReplaceWorker::replaceDiagram(QList<Diagram *> diagram_list)
{
	if (diagram_list.isEmpty()) {
		return;
	}
	
	QETProject *project = diagram_list.first()->project();
	for (Diagram *d : diagram_list) {
		if (d->project() != project) {
			return;
		}
	}
	
	QUndoStack *us = project->undoStack();
	us->beginMacro(QObject::tr("Chercher/remplacer les propriétés de folio"));
	for (Diagram *d : diagram_list)
	{
		TitleBlockProperties old_propertie = d->border_and_titleblock.exportTitleBlock();
		TitleBlockProperties new_properties = old_propertie;
		
		new_properties.title = applyChange(new_properties.title, m_titleblock_properties.title);
		new_properties.author = applyChange(new_properties.author, m_titleblock_properties.author);
		new_properties.filename = applyChange(new_properties.filename, m_titleblock_properties.filename);
		new_properties.plant = applyChange(new_properties.plant, m_titleblock_properties.plant);
		new_properties.locmach = applyChange(new_properties.locmach, m_titleblock_properties.locmach);
		new_properties.indexrev = applyChange(new_properties.indexrev, m_titleblock_properties.indexrev);
		new_properties.folio = applyChange(new_properties.folio, m_titleblock_properties.folio);

		if (m_titleblock_properties.date.isValid())
		{
			if (m_titleblock_properties.date == eraseDate()) {
				new_properties.date = QDate();
			} else {
				new_properties.date = m_titleblock_properties.date;
			}
		}
		
		new_properties.context.add(m_titleblock_properties.context);
		
		if (old_propertie != new_properties) {
			project->undoStack()->push(new ChangeTitleBlockCommand(d, old_propertie, new_properties));
		}
	}
	us->endMacro();
}

void SearchAndReplaceWorker::replaceDiagram(Diagram *diagram)
{
	QList<Diagram *> list;
	list.append(diagram);
	replaceDiagram(list);
}

/**
 * @brief SearchAndReplaceWorker::replaceElement
 * Replace all properties of each elements in @list
 * All element must belong to the same project, if not this function do nothing.
 * All change are made through a undo command append to undo list of the project.
 * @param list
 */
void SearchAndReplaceWorker::replaceElement(QList<Element *> list)
{
	if (list.isEmpty() || !list.first()->diagram()) {
		return;
	}
	
	QETProject *project_ = list.first()->diagram()->project();
	for (Element *elmt : list)
	{
		if (elmt->diagram()) {
			if (elmt->diagram()->project() != project_) {
				return;
			}
		}
	}
	
	project_->undoStack()->beginMacro(QObject::tr("Chercher/remplacer les propriétés d'éléments."));
	for (Element *elmt : list)
	{
			//We apply change only for master, slave, and terminal element.
		if (elmt->linkType() == Element::Master ||
			elmt->linkType() == Element::Simple ||
			elmt->linkType() == Element::Terminale)
		{
			DiagramContext old_context;
			DiagramContext new_context =  old_context = elmt->elementInformations();
			for (QString key : QETApp::elementInfoKeys())
			{
				new_context.addValue(key, applyChange(old_context.value(key).toString(),
													  m_element_context.value(key).toString()));
			}
			
			if (old_context != new_context)
			{
				ChangeElementInformationCommand *undo = new ChangeElementInformationCommand(elmt, old_context, new_context);
				project_->undoStack()->push(undo);
			}
		}
	}
	project_->undoStack()->endMacro();
}

void SearchAndReplaceWorker::replaceElement(Element *element)
{
	QList<Element *>list;
	list.append(element);
	replaceElement(list);
}

/**
 * @brief SearchAndReplaceWorker::replaceIndiText
 * Replace all displayed text of independent text of @list
 * Each must belong to the same project, if not this function do nothing
 * @param list
 */
void SearchAndReplaceWorker::replaceIndiText(QList<IndependentTextItem *> list)
{
	if (list.isEmpty() || !list.first()->diagram()) {
		return;
	}
	QETProject *project_ = list.first()->diagram()->project();
	for (IndependentTextItem *text : list) {
		if (!text->diagram() ||
			text->diagram()->project() != project_) {
			return;
		}
	}
	
	project_->undoStack()->beginMacro(QObject::tr("Chercher/remplacer des textes independants"));
	for (IndependentTextItem *text : list)
	{
		QString before = text->toPlainText();
		text->setPlainText(m_indi_text);
		project_->undoStack()->push(new ChangeDiagramTextCommand(text, before, m_indi_text));
	}
	project_->undoStack()->endMacro();
}

void SearchAndReplaceWorker::replaceIndiText(IndependentTextItem *text)
{
	QList<IndependentTextItem *>list;
	list.append(text);
	replaceIndiText(list);
}

/**
 * @brief SearchAndReplaceWorker::replaceConductor
 * Replace all properties of each conductor in @list
 * All conductor must belong to the same project, if not this function do nothing.
 * All change are made through a undo command append to undo list of the project.
 * @param list
 */
void SearchAndReplaceWorker::replaceConductor(QList<Conductor *> list)
{
	if (list.isEmpty() || !list.first()->diagram()) {
		return;
	}
	
	QETProject *project_ = list.first()->diagram()->project();
	for (Conductor *c : list) {
		if (!c->diagram() ||
			c->diagram()->project() != project_) {
			return;
		}
	}
	
	project_->undoStack()->beginMacro(QObject::tr("Chercher/remplacer les propriétés de conducteurs."));
	for (Conductor *c : list)
	{
		ConductorProperties cp = applyChange(c->properties(), m_conductor_properties);
		
		if (cp != c->properties())
		{
			QSet <Conductor *> conductors_list = c->relatedPotentialConductors(true);
			conductors_list << c;
			for (Conductor *cc : conductors_list)
			{
				QVariant old_value, new_value;
				old_value.setValue(cc->properties());
				new_value.setValue(cp);
				project_->undoStack()->push(new QPropertyUndoCommand(cc, "properties", old_value, new_value));
			}
		}
	}
	project_->undoStack()->endMacro();
}

void SearchAndReplaceWorker::replaceConductor(Conductor *conductor)
{
	QList<Conductor *>list;
	list.append(conductor);
	replaceConductor(list);
}

/**
 * @brief SearchAndReplaceWorker::setupLineEdit
 * With search and replace, when the variable to edit is a text,
 * the editor is always the same no matter if it is for a folio, element or conductor.
 * The editor is a QLineEdit to edit the text and checkbox to erase the text if checked.
 * This function fill the editor, from the current string
 * @param l
 * @param cb
 * @param str
 */
void SearchAndReplaceWorker::setupLineEdit(QLineEdit *l, QCheckBox *cb, QString str)
{
	l->setText(str);
	cb->setChecked(str == eraseText() ? true : false);
	l->setDisabled(str == eraseText() ? true : false);
}

ConductorProperties SearchAndReplaceWorker::invalidConductorProperties()
{
	ConductorProperties cp;
	
		//init with invalid value the conductor properties
	cp.text_size = 0;
	cp.text.clear();
	cp.m_vertical_alignment = Qt::AlignAbsolute;
	cp.m_horizontal_alignment = Qt::AlignAbsolute;
	cp.verti_rotate_text = -1;
	cp.horiz_rotate_text = -1;
	cp.color = QColor();
	cp.style = Qt::NoPen;
	cp.cond_size = 0;
	cp.m_color_2 = QColor();
	cp.m_dash_size = 0;
	
	return cp;
}

/**
 * @brief SearchAndReplaceWorker::applyChange
 * @param original : the original properties
 * @param change : the change properties, to be merged with @original
 * @return a new conductor properties with the change applyed.
 */
ConductorProperties SearchAndReplaceWorker::applyChange(const ConductorProperties &original, const ConductorProperties &change)
{
	ConductorProperties new_properties = original;
	
	if (change.text_size > 2) {new_properties.text_size = change.text_size;}
	new_properties.m_formula = applyChange(new_properties.m_formula, change.m_formula);
	new_properties.text = applyChange(new_properties.text, change.text);
	new_properties.m_show_text = change.m_show_text;
	new_properties.m_function = applyChange(new_properties.m_function, change.m_function);
	new_properties.m_tension_protocol = applyChange(new_properties.m_tension_protocol, change.m_tension_protocol);
	if(change.m_vertical_alignment == Qt::AlignLeft ||
	   change.m_vertical_alignment == Qt::AlignRight) {new_properties.m_vertical_alignment = change.m_vertical_alignment;}
	if(change.m_horizontal_alignment == Qt::AlignTop ||
	   change.m_horizontal_alignment == Qt::AlignBottom) {new_properties.m_horizontal_alignment = change.m_horizontal_alignment;}
	if (change.verti_rotate_text >= 0) {new_properties.verti_rotate_text = change.verti_rotate_text;}
	if (change.horiz_rotate_text >= 0) {new_properties.horiz_rotate_text = change.horiz_rotate_text;}
	if (change.color.isValid()) {new_properties.color = change.color;}
	if (change.style != Qt::NoPen) {new_properties.style = change.style;}
	if (change.cond_size >= 0.4) {new_properties.cond_size = change.cond_size;}
	new_properties.m_bicolor = change.m_bicolor;
	if (change.m_color_2.isValid()) {new_properties.m_color_2 = change.m_color_2;}
	if (change.m_dash_size >= 2) {new_properties.m_dash_size = change.m_dash_size;}
	new_properties.singleLineProperties = change.singleLineProperties;
	
	return new_properties;
}

/**
 * @brief SearchAndReplaceWorker::applyChange
 * @param original : the original string
 * @param change : the changed string:
 * @return the string to be use in the properties
 */
QString SearchAndReplaceWorker::applyChange(const QString &original, const QString &change)
{
	if (change.isEmpty())           {return original;}
	else if (change == eraseText()) {return QString();}
	else                            {return change;}
}
