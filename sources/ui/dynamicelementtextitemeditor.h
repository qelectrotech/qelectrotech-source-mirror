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
	
	private:
		void dataEdited();
		void treeViewClicked(const QModelIndex &index);
    
    private slots:
        void on_m_add_text_clicked();
        void on_m_remove_selection_clicked();
		void on_m_add_group_clicked();
		
		private:
		Ui::DynamicElementTextItemEditor *ui;
        QTreeView *m_tree_view = nullptr;
        DynamicElementTextModel *m_model = nullptr;
};

#endif // DYNAMICELEMENTTEXTITEMEDITOR_H
