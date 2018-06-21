/*
	Copyright 2006-2017 The QElectroTech Team
	This file is part of QElectroTech.

	QElectroTech is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 2 of the License, or
	(at your option) any later version.

	QElectroTech is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with QElectroTech. If not, see <http://www.gnu.org/licenses/>.
*/
#ifndef DYNAMICELEMENTTEXTMODEL_H
#define DYNAMICELEMENTTEXTMODEL_H

#include <QStandardItemModel>
#include <qstyleditemdelegate.h>
#include "dynamicelementtextitem.h"

class QUndoCommand;
class ElementTextItemGroup;
class Element;

/**
 * @brief The DynamicElementTextModel class
 * A model to use with QtView.
 * This model display and can edit the value of dynamic text of an element.
 * Set the delegate DynamicTextItemDelegate as delegate of this model.
 */
class DynamicElementTextModel : public QStandardItemModel
{
	Q_OBJECT
	
	public:
        enum ValueType {
            textFrom =1,
			userText,
			infoText,
			compositeText,
			txtAlignment,
            size,
            color,
			pos,
			frame,
			rotation,
			textWidth,
			grpAlignment,
			grpPos,
			grpRotation,
			grpVAdjust,
			grpName,
			grpHoldBottom
        };
        
		DynamicElementTextModel(Element *element, QObject *parent = nullptr);
		~DynamicElementTextModel() override;
		
		bool indexIsInGroup(const QModelIndex &index) const;
        DynamicElementTextItem *textFromIndex(const QModelIndex &index) const;
        DynamicElementTextItem *textFromItem(QStandardItem *item) const;
		QModelIndex indexFromText(DynamicElementTextItem *text) const;
		QUndoCommand *undoForEditedText(DynamicElementTextItem *deti, QUndoCommand *parent_undo = nullptr) const;
		QUndoCommand *undoForEditedGroup(ElementTextItemGroup *group, QUndoCommand *parent_undo = nullptr) const;
		
		ElementTextItemGroup *groupFromIndex(const QModelIndex &index) const;
		ElementTextItemGroup *groupFromItem(QStandardItem *item) const;
		QModelIndex indexFromGroup(ElementTextItemGroup *group) const;
		bool indexIsText(const QModelIndex &index) const;
		bool indexIsGroup(const QModelIndex &index) const;
		
		bool canDropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent) const override;
		bool dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent) override;
		QMimeData *mimeData(const QModelIndexList &indexes) const override;
		QStringList mimeTypes() const override;
		
	signals:
		void dataChanged();
        
    private:
		QList<QStandardItem *> itemsForText(DynamicElementTextItem *deti);
		void addText(DynamicElementTextItem *deti);
		void removeText(DynamicElementTextItem *deti);
		void addGroup(ElementTextItemGroup *group);
		void removeGroup(ElementTextItemGroup *group);
		void addTextToGroup(DynamicElementTextItem *deti, ElementTextItemGroup *group);
		void removeTextFromGroup(DynamicElementTextItem *deti, ElementTextItemGroup *group);
		void enableSourceText(DynamicElementTextItem *deti, DynamicElementTextItem::TextFrom tf );
		void enableGroupRotationAndPos(ElementTextItemGroup *group);
        void itemDataChanged(QStandardItem *qsi);
		void setConnection(DynamicElementTextItem *deti, bool set);
		void setConnection(ElementTextItemGroup *group, bool set);
		void updateDataFromText(DynamicElementTextItem *deti, DynamicElementTextModel::ValueType type);
		void updateDataFromGroup(ElementTextItemGroup *group, DynamicElementTextModel::ValueType type);
		
	private:
		QPointer<Element> m_element;
		QHash <DynamicElementTextItem *, QStandardItem *> m_texts_list;
		QHash <ElementTextItemGroup *, QStandardItem *> m_groups_list;
		QHash <DynamicElementTextItem *, QList<QMetaObject::Connection>> m_hash_text_connect;
		QHash <ElementTextItemGroup *, QList<QMetaObject::Connection>> m_hash_group_connect;
		bool m_block_dataChanged = false;
};

class DynamicTextItemDelegate : public QStyledItemDelegate
{
	Q_OBJECT
	
    public:
        DynamicTextItemDelegate(QObject *parent = Q_NULLPTR);
        
        QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
        void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const override;
		
	protected:
		bool eventFilter(QObject *object, QEvent *event) override;
		
	private:
		QStringList availableInfo(DynamicElementTextItem *deti) const;
};

#endif // DYNAMICELEMENTTEXTMODEL_H
