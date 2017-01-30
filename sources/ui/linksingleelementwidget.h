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
#ifndef LINKSINGLEELEMENTWIDGET_H
#define LINKSINGLEELEMENTWIDGET_H

#include "element.h"
#include "abstractelementpropertieseditorwidget.h"

class QTreeWidgetItem;

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
		void buildTree();

	public:
		bool setLiveEdit(bool live_edit);

	private :
		QList <Element *> availableElements();
		void setUpCompleter();
		void clearTreeWidget();
		void setUpHeaderLabels();

	private slots:
		void diagramWasRemovedFromProject();
		void showedElementWasDeleted();
		void linkTriggered();
		void hideButtons();
		void showButtons();
		void headerCustomContextMenuRequested(const QPoint &pos);
		
		void on_m_unlink_pb_clicked();
		void on_m_tree_widget_itemDoubleClicked(QTreeWidgetItem *item, int column);
		void on_m_tree_widget_customContextMenuRequested(const QPoint &pos);
		void on_m_show_linked_pb_clicked();
		void on_m_show_this_pb_clicked();
		
		void on_m_search_field_textEdited(const QString &arg1);
		
	private:
	Ui::LinkSingleElementWidget *ui;

	bool m_unlink = false;
	Element::kind m_filter;
	
	QHash <QTreeWidgetItem*, Element*> m_qtwi_elmt_hash;
	QHash <QTreeWidgetItem*, QStringList> m_qtwi_strl_hash;
	
	QTreeWidgetItem *m_qtwi_at_context_menu = nullptr,
					*m_pending_qtwi = nullptr;
	
	Element *m_showed_element = nullptr,
			*m_element_to_link = nullptr;
	
	QMenu *m_context_menu;
	QAction *m_link_action,
			*m_show_qtwi,
			*m_show_element,
			*m_save_header_state;
};

#endif // LINKSINGLEELEMENTWIDGET_H
