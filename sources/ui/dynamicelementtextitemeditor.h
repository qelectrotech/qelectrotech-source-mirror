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
#ifndef DYNAMICELEMENTTEXTITEMEDITOR_H
#define DYNAMICELEMENTTEXTITEMEDITOR_H

#include "abstractelementpropertieseditorwidget.h"

class DynamicElementTextItem;
class DynamicElementTextModel;
class QTreeView;
class QStandardItem;
class QMenu;
class QSignalMapper;
class ElementTextItemGroup;

namespace Ui {
	class DynamicElementTextItemEditor;
}

class DynamicElementTextItemEditor : public AbstractElementPropertiesEditorWidget
{
    Q_OBJECT
    
	public:
	   explicit DynamicElementTextItemEditor(Element *element, QWidget *parent = nullptr);
		~DynamicElementTextItemEditor() override;
	
		void setElement(Element *element) override;
        QString title() const override {return tr("Textes");}
		bool setLiveEdit(bool live_edit) override;
		void apply() override;
		void setCurrentText(DynamicElementTextItem *text);
		void setCurrentGroup(ElementTextItemGroup *group);
		QUndoCommand *associatedUndo() const override;
		bool eventFilter(QObject *watched, QEvent *event) override;
	
	private:
		void setUpAction();
		void dataEdited(DynamicElementTextItem *deti);
		void addCurrentTextToGroup(QString name);
    
    private slots:
        void on_m_add_text_clicked();
        void on_m_remove_text_clicked();
        
    private:
		Ui::DynamicElementTextItemEditor *ui;
        QTreeView *m_tree_view = nullptr;
        DynamicElementTextModel *m_model = nullptr;
		QMenu *m_context_menu = nullptr;
		QSignalMapper *m_signal_mapper = nullptr;
		QAction *m_add_to_group = nullptr,
				*m_remove_text_from_group = nullptr,
				*m_new_group = nullptr,
				*m_remove_current_text = nullptr,
				*m_remove_current_group = nullptr;
		QList<QAction *> m_actions_list;
};

#endif // DYNAMICELEMENTTEXTITEMEDITOR_H
