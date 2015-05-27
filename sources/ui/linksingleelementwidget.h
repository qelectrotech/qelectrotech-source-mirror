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
#ifndef LINKSINGLEELEMENTWIDGET_H
#define LINKSINGLEELEMENTWIDGET_H

#include "element.h"
#include "abstractelementpropertieseditorwidget.h"


class Diagram;
class QLineEdit;
class ElementSelectorWidget;

namespace Ui {
	class LinkSingleElementWidget;
}

/**
* @brief The LinkSingleElementWidget class
 * this class provide a widget to select an element to be linked
 * to the element given in the constructor.
 * The element given in constructor must be linked with only one other element (like report or slave element).
 * This widget detect automaticaly the kind of element given in the constructor and
 * search all element that can be linked with it.
 * If the element is already linked, the widget ask user to unlink.
 * This widget embedded the diagram command for undo/redo the action
 */
class LinkSingleElementWidget : public AbstractElementPropertiesEditorWidget
{
	Q_OBJECT

	///Methods
	public:
		explicit LinkSingleElementWidget(Element *elmt, QWidget *parent = 0);
		~LinkSingleElementWidget();

		void setElement (Element *element);
		void apply();
		QUndoCommand *associatedUndo() const;
		QString title() const;

	public slots:
		void updateUi();

	public:
		bool setLiveEdit(bool live_edit);

	private :
		void enableLiveEdit();
		void disableLiveEdit();
		void buildList();
		void buildSearchField();
		QList <Element *> availableElements();
		void setUpCompleter();

	private slots:
		void setNewList();
		void unlinkClicked();
		void on_button_this_clicked();
		void on_button_linked_clicked();
		void diagramWasRemovedFromProject();

	///Attributes
	private:
	Ui::LinkSingleElementWidget *ui;
	ElementSelectorWidget *esw_;
	QList <Diagram *> diagram_list;
	bool unlink_;
	Element::kind filter_;
	QLineEdit *search_field;
};

#endif // LINKSINGLEELEMENTWIDGET_H
