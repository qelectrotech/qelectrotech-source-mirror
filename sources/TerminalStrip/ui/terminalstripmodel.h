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
#include "../terminalstrip.h"

class TerminalStrip;

class TerminalStripModel : public QAbstractTableModel
{
        Q_OBJECT
    public:
        TerminalStripModel(TerminalStrip *terminal_strip, QObject *parent = nullptr);

        virtual int rowCount    (const QModelIndex &parent = QModelIndex()) const override;
        virtual int columnCount (const QModelIndex &parent = QModelIndex()) const override;
        virtual QVariant data   (const QModelIndex &index, int role = Qt::DisplayRole) const override;
		virtual QVariant headerData (int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

	private:
		void fillRealTerminalData();

    private:
        QPointer<TerminalStrip> m_terminal_strip;
		QVector<RealTerminalData> m_real_terminal_data;
};

#endif // TERMINALSTRIPMODEL_H
