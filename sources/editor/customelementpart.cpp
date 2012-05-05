/*
	Copyright 2006-2012 Xavier Guerrin
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
#include "customelementpart.h"
#include "customelement.h"
#include "qetelementeditor.h"

/// @return le QETElementEditor auquel cet editeur appartient
QETElementEditor *CustomElementPart::elementEditor() const {
	return(element_editor);
}

/**
	Appelle le slot updateCurrentPartEditor de l'editeur
	@see QETElementEditor::updateCurrentPartEditor()
*/
void CustomElementPart::updateCurrentPartEditor() const {
	if (element_editor) {
		element_editor -> updateCurrentPartEditor();
	}
}

/// @return l'ElementScene contenant les parties editees par cet editeur
ElementScene *CustomElementPart::elementScene() const {
	return(element_editor -> elementScene());
}

/// @return la QUndoStack a utiliser pour les annulations
QUndoStack &CustomElementPart::undoStack() const {
	return(elementScene() -> undoStack());
}
