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
	automatiquement recuperee.
	@param name   nom de la propriete modifiee
	@param part   partie modifiee
	@param prop   propriete modifiee
	@param new_v  nouvelle valeur
*/
void ElementItemEditor::addChangePartCommand(const QString &desc, CustomElementPart *part, const QString &prop, const QVariant &new_v) {
	QVariant old_v = part -> property(prop);
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

/// @return Le nom du type d'element edite
QString ElementItemEditor::elementTypeName() const {
	return(element_type_name);
}

/// @param name Nom du type d'element edite
void ElementItemEditor::setElementTypeName(const QString &name) {
	element_type_name = name;
}
