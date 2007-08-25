#ifndef ELEMENT_ITEM_EDITOR_H
#define ELEMENT_ITEM_EDITOR_H
#include <QtGui>
class QETElementEditor;
class ElementScene;
class CustomElementPart;
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
