#include "customelementpart.h"
#include "customelement.h"
#include "qetelementeditor.h"

/// @return le QETElementEditor auquel cet editeur appartient
QETElementEditor *CustomElementPart::elementEditor() const {
	return(element_editor);
}

/// @return l'ElementScene contenant les parties editees par cet editeur
ElementScene *CustomElementPart::elementScene() const {
	return(element_editor -> elementScene());
}

/// @return la QUndoStack a utiliser pour les annulations
QUndoStack &CustomElementPart::undoStack() const {
	return(elementScene() -> undoStack());
}
