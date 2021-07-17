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
#include "../../qetgraphicsitem/element.h"
#include <QDebug>
#include <QBrush>
#include <QVector>
#include <QComboBox>
#include <QSpinBox>

/**
 * Some const int who describe what a column contain
 */
const int POS_CELL = 0;
const int LEVEL_CELL = 1;
const int LABEL_CELL = 2;
const int XREF_CELL = 3;
const int CABLE_CELL = 4;
const int CABLE_WIRE_CELL = 5;
const int TYPE_CELL = 6;
const int FUNCTION_CELL = 7;
const int LED_CELL = 8;
const int CONDUCTOR_CELL = 9;

const int ROW_COUNT = 9;

static QVector<bool> UNMODIFIED_CELL_VECTOR{false, false, false, false, false, false, false, false, false, false};

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
	return ROW_COUNT;
}

QVariant TerminalStripModel::data(const QModelIndex &index, int role) const
{
	if (index.row() >= m_real_terminal_data.size()) {
		return QVariant();
	}

	auto rtd = m_real_terminal_data.at(index.row());


	if (role == Qt::DisplayRole)
	{
		switch (index.column()) {
			case POS_CELL :        return rtd.pos_;
			case LEVEL_CELL :      return rtd.level_;
			case LABEL_CELL :      return rtd.label_;
			case XREF_CELL :       return rtd.Xref_;
			case CABLE_CELL :      return rtd.cable_;
			case CABLE_WIRE_CELL : return rtd.cable_wire_;
			case TYPE_CELL :       return ElementData::translatedTerminalType(rtd.type_);
			case FUNCTION_CELL :   return ElementData::translatedTerminalFunction(rtd.function_);
			case CONDUCTOR_CELL :  return rtd.conductor_;
			default :              return QVariant();
		}
	}
	else if (role == Qt::EditRole)
	{
		switch (index.column()) {
			case LABEL_CELL : return rtd.label_;
			default: return QVariant();

		}
	}
	else if (role == Qt::CheckStateRole &&
			   index.column() == LED_CELL)
	{
		return rtd.led_ ? Qt::Checked : Qt::Unchecked;
	}
	else if (role == Qt::BackgroundRole && index.column() <= CONDUCTOR_CELL )
	{
		if (m_modified_cell.contains(rtd.m_real_terminal) &&
			m_modified_cell.value(rtd.m_real_terminal).at(index.column()))
		{
			return QBrush(Qt::yellow);
		}
	}

	return QVariant();
}

bool TerminalStripModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
	auto rtd = m_real_terminal_data.at(index.row());
	bool modified_ = false;
	int modified_cell = -1;
	auto column_ = index.column();

	if (column_ == LED_CELL &&
		role == Qt::CheckStateRole)
	{
			rtd.led_ = value.toBool();
			modified_ = true;
			modified_cell = LED_CELL;
	}
	else if (column_ == TYPE_CELL &&
			 role == Qt::EditRole)
	{
		rtd.type_ = value.value<ElementData::TerminalType>();
		modified_ = true;
		modified_cell = TYPE_CELL;
	}
	else if (column_ == FUNCTION_CELL &&
			 role == Qt::EditRole)
	{
		rtd.function_ = value.value<ElementData::TerminalFunction>();
		modified_ = true;
		modified_cell = FUNCTION_CELL;
	}

		//Set the modification to the terminal data
	if (modified_)
	{
		m_real_terminal_data.replace(index.row(), rtd);

		if (rtd.m_real_terminal)
		{
			QVector<bool> vector_;
			if (m_modified_cell.contains(rtd.m_real_terminal)) {
				vector_ = m_modified_cell.value(rtd.m_real_terminal);
			} else {
				vector_ = UNMODIFIED_CELL_VECTOR;
			}

			vector_.replace(modified_cell, true);
			m_modified_cell.insert(rtd.m_real_terminal, vector_);
		}
		return true;
	}

	return false;
}

QVariant TerminalStripModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	if (role == Qt::DisplayRole)
	{
		if (orientation == Qt::Horizontal)
		{
			switch (section) {
				case POS_CELL:        return tr("Position");
				case LEVEL_CELL:      return tr("Étage");
				case LABEL_CELL:      return tr("Label");
				case XREF_CELL:       return tr("Référence croisé");
				case CABLE_CELL:      return tr("Câble");
				case CABLE_WIRE_CELL: return tr("Couleur / numéro de fil câble");
				case TYPE_CELL:       return tr("Type");
				case FUNCTION_CELL :  return tr("Fonction");
				case LED_CELL:        return tr("led");
				case CONDUCTOR_CELL:  return tr("Numéro de conducteur");
				default : return QVariant();
			}
		}
	}

	return QVariant();
}

Qt::ItemFlags TerminalStripModel::flags(const QModelIndex &index) const
{
	Qt::ItemFlags flags = Qt::ItemIsEnabled | Qt::ItemIsSelectable;

	auto c = index.column();
	if (c == LABEL_CELL || c == TYPE_CELL || c == FUNCTION_CELL)
		flags = flags | Qt::ItemIsEditable;
	if (c == LED_CELL) {
		flags = flags | Qt::ItemIsUserCheckable;
	}
	return flags;
}

/**
 * @brief TerminalStripModel::editedTerminals
 * @return a hash with for keys the edited element and for value the ElementData with modified properties
 */
QHash<Element *, ElementData> TerminalStripModel::editedTerminalsData() const
{
	QHash<Element *, ElementData> returned_hash;

	QVector<RealTerminalData> rtd_vector = m_real_terminal_data;

	const auto modified_real_terminal = m_modified_cell.keys();
	for (auto const &rt : modified_real_terminal) //loop over modified real terminal
	{
		for (auto const &rtd : rtd_vector) //loop over real terminal data to retrieve the data associated with real terminal
		{
			if (rtd.m_real_terminal == rt)
			{
				auto element = m_terminal_strip->elementForRealTerminal(rt);
				if (element) {
					returned_hash.insert(element, modifiedData(element->elementData(), rtd));
				}
				break;
			}
		}
	}

	return returned_hash;
}

void TerminalStripModel::fillRealTerminalData()
{
	if (m_terminal_strip) {
		for (int i=0 ; i < m_terminal_strip->realTerminalCount() ; ++i) {
			m_real_terminal_data.append(m_terminal_strip->realTerminalData(i));
		}
	}
}

/**
 * @brief TerminalStripModel::modifiedData
 * @param previous_data
 * @param edited_data
 * @return an ElementData with the change made in \p edited_data applied to \p original_data
 */
ElementData TerminalStripModel::modifiedData(const ElementData &original_data, const RealTerminalData &edited_data)
{
	ElementData returned_data = original_data;

	returned_data.setTerminalType(edited_data.type_);
	returned_data.setTerminalFunction(edited_data.function_);
	returned_data.setTerminalLED(edited_data.led_);

	return returned_data;
}

/***********************************************************
 * Alittle delegate for add a combobox to edit type
 * and a spinbox to edit the level of a terminal
 **********************************************************/

TerminalStripModelDelegate::TerminalStripModelDelegate(QObject *parent) :
	QStyledItemDelegate(parent)
{}

QWidget *TerminalStripModelDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
	if (index.column() == TYPE_CELL) {
		auto qcb = new QComboBox(parent);
		qcb->setObjectName("terminal_type");
		qcb->addItem(ElementData::translatedTerminalType(ElementData::TTGeneric),   ElementData::TTGeneric);
		qcb->addItem(ElementData::translatedTerminalType(ElementData::TTFuse),      ElementData::TTFuse);
		qcb->addItem(ElementData::translatedTerminalType(ElementData::TTSectional), ElementData::TTSectional);
		qcb->addItem(ElementData::translatedTerminalType(ElementData::TTDiode),     ElementData::TTDiode);
		qcb->addItem(ElementData::translatedTerminalType(ElementData::TTGround),    ElementData::TTGround);

		return qcb;
	}
	if (index.column() == FUNCTION_CELL) {
		auto qcb = new QComboBox(parent);
		qcb->setObjectName("terminal_function");
		qcb->addItem(ElementData::translatedTerminalFunction(ElementData::TFGeneric), ElementData::TFGeneric);
		qcb->addItem(ElementData::translatedTerminalFunction(ElementData::TFPhase),   ElementData::TFPhase);
		qcb->addItem(ElementData::translatedTerminalFunction(ElementData::TFNeutral), ElementData::TFNeutral);

		return qcb;
	}

	return QStyledItemDelegate::createEditor(parent, option, index);
}

void TerminalStripModelDelegate::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
	if (index.isValid())
	{
		if (editor->objectName() == QLatin1String("terminal_type"))
		{
			if (auto qcb = dynamic_cast<QComboBox *>(editor)) {
				model->setData(index, qcb->currentData(), Qt::EditRole);
			}
		}
		else if (editor->objectName() == QLatin1String("terminal_function"))
		{
			if (auto qcb = dynamic_cast<QComboBox *>(editor)) {
				model->setData(index, qcb->currentData(), Qt::EditRole);
			}
		}
	}
}
