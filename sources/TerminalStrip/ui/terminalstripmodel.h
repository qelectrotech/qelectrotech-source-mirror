/*
        Copyright 2006-2021 The QElectroTech Team
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
#ifndef TERMINALSTRIPMODEL_H
#define TERMINALSTRIPMODEL_H

#include <QAbstractTableModel>
#include <QObject>
#include <QPointer>
#include <QStyledItemDelegate>
#include <QHash>
#include <QColor>

#include "../terminalstrip.h"
#include "../../qetgraphicsitem/element.h"

//Code to use QColor as key for QHash
inline uint qHash(const QColor &key, uint seed) {
	return qHash(key.name(), seed);
}

//needed to use QPointer<Element> as key of QHash
inline uint qHash(const QPointer<Element> &key, uint seed) {
	if (key)
		return qHash(key->uuid(), seed);
	else
		return qHash(nullptr, seed);
}

class TerminalStrip;


struct modelRealTerminalData
{
		int level_ = -1;
		QString label_;
		QString Xref_;
		QString cable_;
		QString cable_wire;
		QString conductor_;
		bool led_ = false;
		bool bridged_ = false;

		ElementData::TerminalType type_ = ElementData::TerminalType::TTGeneric;
		ElementData::TerminalFunction function_ = ElementData::TerminalFunction::TFGeneric;
		QPointer<Element> element_;

		QWeakPointer<RealTerminal> real_terminal;

};

struct modelPhysicalTerminalData
{
		QVector<modelRealTerminalData> real_data;
		int pos_ = -1;
		QUuid uuid_;
};

inline bool operator == (const modelPhysicalTerminalData &data_1, const modelPhysicalTerminalData &data_2) {
	return data_1.uuid_ == data_2.uuid_;
}

class TerminalStripModel : public QAbstractTableModel
{
	public:
		enum Column {
			Pos = 0,
			Level = 1,
			Level0 = 2,
			Level1 = 3,
			Level2 = 4,
			Level3 = 5,
			Label = 6,
			XRef = 7,
			Cable = 8,
			CableWire = 9,
			Type = 10,
			Function = 11,
			Led = 12,
			Conductor = 13,
			Invalid = 99
		};

		static int levelForColumn(TerminalStripModel::Column column);
		static TerminalStripModel::Column columnTypeForIndex(const QModelIndex &index);

        Q_OBJECT
    public:
        TerminalStripModel(TerminalStrip *terminal_strip, QObject *parent = nullptr);

        virtual int rowCount    (const QModelIndex &parent = QModelIndex()) const override;
        virtual int columnCount (const QModelIndex &parent = QModelIndex()) const override;
        virtual QVariant data   (const QModelIndex &index, int role = Qt::DisplayRole) const override;
		virtual bool setData (const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;
		virtual QVariant headerData (int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
		virtual Qt::ItemFlags flags (const QModelIndex &index) const override;
		QVector<modelRealTerminalData> modifiedmodelRealTerminalData() const;

		QVector<modelPhysicalTerminalData> modelPhysicalTerminalDataForIndex(QModelIndexList index_list) const;
		QVector<modelRealTerminalData> modelRealTerminalDataForIndex(QModelIndexList index_list) const;
		modelRealTerminalData modelRealTerminalDataForIndex(const QModelIndex &index) const;

		void buildBridgePixmap(const QSize &pixmap_size);

	private:
		void fillPhysicalTerminalData();
		modelRealTerminalData dataAtRow(int row) const;
		void replaceDataAtRow(modelRealTerminalData data, int row);
		modelPhysicalTerminalData physicalDataAtIndex(int index) const;
		modelRealTerminalData realDataAtIndex(int index) const;
		QPixmap bridgePixmapFor(const QModelIndex &index) const;

		static modelRealTerminalData modelRealData(const RealTerminalData &data);

    private:
        QPointer<TerminalStrip> m_terminal_strip;
		QHash<QPointer<Element>, QVector<bool>> m_modified_cell;
		QVector<modelPhysicalTerminalData> m_physical_data;
		struct BridgePixmap
		{
				QPixmap top_,
						middle_,
						bottom_,
						none_;
		};

		QHash<QColor, BridgePixmap> m_bridges_pixmaps;

};

class TerminalStripModelDelegate : public QStyledItemDelegate
{
	Q_OBJECT

	public:
		TerminalStripModelDelegate(QObject *parent = Q_NULLPTR);

		QWidget *createEditor(
				QWidget *parent,
				const QStyleOptionViewItem &option,
				const QModelIndex &index) const override;
		void setModelData(
				QWidget *editor,
				QAbstractItemModel *model,
				const QModelIndex &index) const override;

		void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
};

#endif // TERMINALSTRIPMODEL_H
