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
#include "elementitemeditor.h"
#include "qetelementeditor.h"
#include "editorcommands.h"

/**
	Constructeur
	@param editor QETElementEditor auquel cet editeur appartient
	@param parent QWidget parent de cet editeur
*/
ElementItemEditor::ElementItemEditor(QETElementEditor *editor, QWidget *parent) :
	QWidget(parent),
	element_editor(editor)
{
}

/// @return le QETElementEditor auquel cet editeur appartient
QETElementEditor *ElementItemEditor::elementEditor() const {
	return(element_editor);
}

/// @return l'ElementScene contenant les parties editees par cet editeur
ElementScene *ElementItemEditor::elementScene() const {
	return(element_editor -> elementScene());
}

/// @return la QUndoStack a utiliser pour les annulations
QUndoStack &ElementItemEditor::undoStack() const {
	return(elementScene() -> undoStack());
}

/**
	Ajoute une ChangePartCommand a l'UndoStack. L'ancienne valeur sera
	automatiquement recuperee. A noter que cette methode ne fait rien si
	l'ancienne valeur et la nouvelle sont egales ou encore si part vaut 0
	@param desc   nom de la propriete modifiee
	@param part   partie modifiee
	@param prop   propriete modifiee
	@param new_v  nouvelle valeur
*/
void ElementItemEditor::addChangePartCommand(const QString &desc, CustomElementPart *part, const char *prop, const QVariant &new_v) {
	// ne fait rien si part vaut 0
	if (!part) return;
	
	// recupere l'ancienne valeur
	QVariant old_v = part -> property(prop);
	
	// ne fait rien si l'ancienne valeur et la nouvelle sont egales 
	if (old_v == new_v) return;
	
	undoStack().push(
		new ChangePartCommand(
			desc + " " + element_type_name,
			part,
			prop,
			old_v,
			new_v
		)
	);
}

/**
 * @brief ElementItemEditor::addChangePartCommand
 * Add a ChangePartCommand with child for each part of part_list to the undo stack
 * @param undo_text : text of undo command to display
 * @param part_list : list of parts to modify
 * @param property : QProperty (of CustomElementPart) to modify
 * @param new_value : the new value of the QProperty
 */
void ElementItemEditor::addChangePartCommand(const QString &undo_text, QList<CustomElementPart *> part_list, const char *property, const QVariant &new_value)
{
	if (part_list.isEmpty()) return;

		//Get only the parts concerned  by modification
	QList <CustomElementPart *> updated_part;
	foreach (CustomElementPart *cep, part_list)
		if (cep->property(property) != new_value)
			updated_part << cep;

		//There is not part to modify
	if(updated_part.isEmpty()) return;

		//Set the first part has parent undo
	CustomElementPart *p_cep = updated_part.takeFirst();
	QUndoCommand *parent_undo = new ChangePartCommand (undo_text, p_cep, property, p_cep->property(property), new_value);

		//And other parts are just child of parent
	foreach (CustomElementPart *cep, updated_part)
		new ChangePartCommand (undo_text, cep, property, cep->property(property), new_value, parent_undo);

	undoStack().push(parent_undo);
}

/// @return Le nom du type d'element edite
QString ElementItemEditor::elementTypeName() const {
	return(element_type_name);
}

/// @param name Nom du type d'element edite
void ElementItemEditor::setElementTypeName(const QString &name) {
	element_type_name = name;
}

/**
	Detache l'editeur de la primitive qu'il edite.
	Equivaut a setPart(0)
	@see setPart
*/
void ElementItemEditor::detach() {
	setPart(0);
}
