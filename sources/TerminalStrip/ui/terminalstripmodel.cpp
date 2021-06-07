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
#include "terminalstripmodel.h"
#include "../terminalstrip.h"
#include <QDebug>

/**
 * @brief TerminalStripModel::TerminalStripModel
 * @param terminal_strip
 * @param parent
 */
TerminalStripModel::TerminalStripModel(TerminalStrip *terminal_strip, QObject *parent) :
    QAbstractTableModel(parent),
    m_terminal_strip(terminal_strip)
{
	fillRealTerminalData();
}

int TerminalStripModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)

    if (!m_terminal_strip) {
        return 0;
    }

    return m_terminal_strip->realTerminalCount();
}

int TerminalStripModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
	return 9;
}

QVariant TerminalStripModel::data(const QModelIndex &index, int role) const
{
	if (role != Qt::DisplayRole ||
		index.row() >= m_real_terminal_data.size()) {
		return QVariant();
	}

	auto rtd = m_real_terminal_data.at(index.row());
	switch (index.column()) {
		case 0 : return rtd.pos_;
		case 1 : return rtd.level_;
		case 2 : return rtd.label_;
		case 3 : return rtd.Xref_;
		case 4 : return rtd.cable_;
		case 5 : return rtd.cable_wire_;
		case 6 : return ElementData::translatedTerminalType(rtd.type_);
		case 7 : return rtd.led_;
		case 8 : return rtd.conductor_;
		default : return QVariant();
	}
}

QVariant TerminalStripModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	if (orientation != Qt::Horizontal ||
		role != Qt::DisplayRole) {
		return QVariant();
	}

	switch (section) {
		case 0: return tr("Position");
		case 1: return tr("Étage");
		case 2: return tr("Label");
		case 3: return tr("Référence croisé");
		case 4: return tr("Câble");
		case 5: return tr("Couleur / numéro de fil câble");
		case 6: return tr("Type");
		case 7: return tr("led");
		case 8: return tr("Numéro de conducteur");
		default : return QVariant();
	}
}

void TerminalStripModel::fillRealTerminalData()
{
	if (m_terminal_strip) {
		for (int i=0 ; i < m_terminal_strip->realTerminalCount() ; ++i) {
			m_real_terminal_data.append(m_terminal_strip->realTerminalData(i));
		}
	}
}
