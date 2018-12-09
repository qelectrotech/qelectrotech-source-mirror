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
#ifndef SEARCHANDREPLACEWIDGET_H
#define SEARCHANDREPLACEWIDGET_H

#include <QWidget>
#include <QTreeWidgetItemIterator>

#include "element.h"
#include "independenttextitem.h"
#include "searchandreplaceworker.h"

class QTreeWidgetItem;

namespace Ui {
	class SearchAndReplaceWidget;
}

class QETDiagramEditor;

class SearchAndReplaceWidget : public QWidget
{
	Q_OBJECT
	
	public:
		explicit SearchAndReplaceWidget(QWidget *parent = nullptr);
		~SearchAndReplaceWidget();
	
		bool event(QEvent *event) override;
		void clear();
		void setEditor(QETDiagramEditor *editor);
	
	private:
		void setUpTreeItems();
		void setHideAdvanced(bool hide) const;
		void fillItemsList();
		void addElement(Element *element);
		void search();
		
		void setVisibleAllParents(QTreeWidgetItem *item, bool expend_parent = true);
		QTreeWidgetItem *nextItem(QTreeWidgetItem *item=nullptr, QTreeWidgetItemIterator::IteratorFlag flags = QTreeWidgetItemIterator::All) const;
		QTreeWidgetItem *previousItem(QTreeWidgetItem *item=nullptr, QTreeWidgetItemIterator::IteratorFlag flags = QTreeWidgetItemIterator::All) const;
		void updateNextPreviousButtons();
		void itemChanged(QTreeWidgetItem *item, int column);
		void setChildCheckState(QTreeWidgetItem *item, Qt::CheckState check, bool deep = true);
		void updateParentCheckState(QTreeWidgetItem *item, bool all_parents = true);
		void activateNextChecked();
		QStringList searchTerms(Diagram *diagram) const;
		QStringList searchTerms(Element *element) const;
		
	private slots:
		void on_m_quit_button_clicked();
		void on_m_advanced_pb_toggled(bool checked);
		void on_m_tree_widget_itemDoubleClicked(QTreeWidgetItem *item, int column);
		void on_m_reload_pb_clicked();	
		void on_m_tree_widget_currentItemChanged(QTreeWidgetItem *current, QTreeWidgetItem *previous);
		void on_m_next_pb_clicked();
		void on_m_previous_pb_clicked();
		void on_m_folio_pb_clicked();
		void on_m_replace_pb_clicked();
		void on_m_replace_all_pb_clicked();
		void on_m_element_pb_clicked();
		void on_m_mode_cb_currentIndexChanged(int index);
		void on_m_case_sensitive_cb_stateChanged(int arg1);
		void on_m_conductor_pb_clicked();
		
		private:
		Ui::SearchAndReplaceWidget *ui;
		QETDiagramEditor *m_editor;
		QTreeWidgetItem *m_root_qtwi          = nullptr,
						*m_folio_qtwi         = nullptr,
						*m_indi_text_qtwi     = nullptr,
						*m_elements_qtwi      = nullptr,
						*m_simple_elmt_qtwi   = nullptr,
						*m_master_elmt_qtwi   = nullptr,
						*m_slave_elmt_qtwi    = nullptr,
						*m_report_elmt_qtwi   = nullptr,
						*m_terminal_elmt_qtwi = nullptr,
						*m_conductor_qtwi     = nullptr;
		QList<QTreeWidgetItem *> m_qtwi_elmts;
		QList<QTreeWidgetItem *> m_category_qtwi;
		QHash<QTreeWidgetItem *, QPointer <Element>> m_element_hash;
		QHash<QTreeWidgetItem *, QPointer <IndependentTextItem>> m_text_hash;
		QHash<QTreeWidgetItem *, QPointer <Conductor>> m_conductor_hash;
		QPointer<Element> m_highlighted_element;
		QPointer<QGraphicsObject> m_last_selected;
		QHash<QTreeWidgetItem *, QPointer <Diagram>> m_diagram_hash;
		SearchAndReplaceWorker m_worker;
};

#endif // SEARCHANDREPLACEWIDGET_H
