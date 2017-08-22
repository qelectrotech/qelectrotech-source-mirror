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
            size,
            tagg,
            color
        };
        
		DynamicElementTextModel(QObject *parent = nullptr);
		~DynamicElementTextModel() override;
		
		void addText(DynamicElementTextItem *deti);
		void removeText(DynamicElementTextItem *deti);
        DynamicElementTextItem *textFromIndex(const QModelIndex &index) const;
        DynamicElementTextItem *textFromItem(QStandardItem *item) const;
		QModelIndex indexFromText(DynamicElementTextItem *text) const;
		QUndoCommand *undoForEditedText(DynamicElementTextItem *deti) const;
        
    private:
		void enableSourceText(DynamicElementTextItem *deti, DynamicElementTextItem::TextFrom tf );
        void dataEdited(QStandardItem *qsi);
		void setConnection(DynamicElementTextItem *deti, bool set);
		void updateDataFromText(DynamicElementTextItem *deti, DynamicElementTextModel::ValueType type);
		
	private:
		QHash <DynamicElementTextItem *, QStandardItem *> m_texts_list;
		QHash <DynamicElementTextItem *, QList<QMetaObject::Connection>> m_hash_text_connect;
};

class DynamicTextItemDelegate : public QStyledItemDelegate
{
    public:
        DynamicTextItemDelegate(QObject *parent = Q_NULLPTR);
        
        QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
        void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const override;
		
	private:
		QStringList availableInfo(DynamicElementTextItem *deti) const;
};

#endif // DYNAMICELEMENTTEXTMODEL_H
