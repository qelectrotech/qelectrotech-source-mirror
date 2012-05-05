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
#ifndef ELEMENT_ITEM_EDITOR_H
#define ELEMENT_ITEM_EDITOR_H
#include <QtGui>
class QETElementEditor;
class ElementScene;
class CustomElementPart;
/**
	Cette classe est la classe de base pour les editeurs de parties dans
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
	virtual void detach();
	virtual bool setPart(CustomElementPart *) = 0;
	virtual CustomElementPart *currentPart() const = 0;
	virtual void updateForm() = 0;
	
	// attributs
	private:
	QETElementEditor *element_editor;
	QString element_type_name;
};
#endif
