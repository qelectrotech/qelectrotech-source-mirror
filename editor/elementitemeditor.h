#ifndef ELEMENT_ITEM_EDITOR_H
#define ELEMENT_ITEM_EDITOR_H
#include <QtGui>
class QETElementEditor;
class ElementScene;
class CustomElementPart;
/**
	Cette classe est la classe de base pour les editeurs de aprties dans
	l'editeur d'element. Elle fournit des methodes pour acceder facilement
	a l'editeur, a la pile d'annulation, a la scene d'edition ou encore pour
	ajouter facilement une annulation de type ChangePartCommand.
*/
class ElementItemEditor : public QWidget {
	Q_OBJECT
	// constructeurs, destructeur
	public:
	ElementItemEditor(QETElementEditor *, QWidget * = 0);
	virtual ~ElementItemEditor() {};
	private:
	ElementItemEditor(const ElementItemEditor &);
	
	// methodes
	public:
	virtual QETElementEditor *elementEditor() const;
	virtual ElementScene *elementScene() const;
	virtual QUndoStack &undoStack() const;
	virtual void addChangePartCommand(const QString &, CustomElementPart *, const QString &, const QVariant &);
	virtual QString elementTypeName() const;
	virtual void setElementTypeName(const QString &);
	
	// attributs
	private:
	QETElementEditor *element_editor;
	QString element_type_name;
};
#endif
