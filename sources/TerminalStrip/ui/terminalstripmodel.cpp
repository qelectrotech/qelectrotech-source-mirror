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
	fillPhysicalTerminalData();
}

int TerminalStripModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)

    if (!m_terminal_strip) {
        return 0;
    }

	auto count = 0;
	for (const auto &ptd : m_edited_terminal_data) {
		count += ptd.real_terminals_vector.size();
	}

	return count;
}

int TerminalStripModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
	return ROW_COUNT;
}

QVariant TerminalStripModel::data(const QModelIndex &index, int role) const
{
	if (index.row() >= rowCount(QModelIndex())) {
		return QVariant();
	}

	const auto rtd = dataAtRow(index.row());


	if (role == Qt::DisplayRole)
	{
		switch (index.column()) {
			case POS_CELL :        return physicalDataAtIndex(index.row()).pos_;
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
		if (m_modified_cell.contains(rtd.element_) &&
			m_modified_cell.value(rtd.element_).at(index.column()))
		{
			return QBrush(Qt::yellow);
		}
	}

	return QVariant();
}

bool TerminalStripModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
	auto rtd = dataAtRow(index.row());
	bool modified_ = false;
	int modified_cell = -1;
	auto column_ = index.column();

	if (column_ == LEVEL_CELL &&
		role == Qt::EditRole)
	{
		rtd.level_ = value.toInt();
		modified_ = true;
		modified_cell = LEVEL_CELL;
	}
	else if (column_ == LED_CELL &&
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
	else if (column_ == LABEL_CELL &&
			 role == Qt::EditRole &&
			 rtd.label_ != value.toString())
	{
		rtd.label_ = value.toString();
		modified_ = true;
		modified_cell = LABEL_CELL;
	}

		//Set the modification to the terminal data
	if (modified_)
	{
		replaceDataAtRow(rtd, index.row());

		if (rtd.element_)
		{
			QVector<bool> vector_;
			if (m_modified_cell.contains(rtd.element_)) {
				vector_ = m_modified_cell.value(rtd.element_);
			} else {
				vector_ = UNMODIFIED_CELL_VECTOR;
			}

			vector_.replace(modified_cell, true);
			m_modified_cell.insert(rtd.element_, vector_);
		}
		emit dataChanged(index, index);
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
 * @brief TerminalStripModel::modifiedRealTerminalData
 * @return a vector of QPair of modified terminal.
 * the first value of the QPair is the original data, the second value is the edited data
 */
QVector<QPair<RealTerminalData, RealTerminalData>> TerminalStripModel::modifiedRealTerminalData() const
{
	QVector<QPair<RealTerminalData, RealTerminalData>> returned_vector;

	const auto modified_real_terminal = m_modified_cell.keys();

	for (auto i = 0 ; i<m_edited_terminal_data.size() ; ++i)
	{
		auto ptd_ = m_edited_terminal_data.at(i);
		for (auto j = 0 ; j < ptd_.real_terminals_vector.size() ; ++j)
		{
			auto rtd_ = ptd_.real_terminals_vector.at(j);
			if (modified_real_terminal.contains(rtd_.element_))
			{
				returned_vector.append(qMakePair(m_original_terminal_data.at(i).real_terminals_vector.at(j),
												 m_edited_terminal_data.at(i).real_terminals_vector.at(j)));
			}
		}
	}

	return returned_vector;
}

/**
 * @brief TerminalStripModel::isXrefCell
 * @param index
 * @param elmt : Pointer of a pointer element
 * @return true if the index is the Xref cell, if true the pointer \p element
 * will be set to the element associated to the cell.
 */
bool TerminalStripModel::isXrefCell(const QModelIndex &index, Element **element)
{
	if (index.model() == this
		&& index.isValid()
		&& index.column() == XREF_CELL)
	{
		if (index.row() < rowCount())
		{
			if (auto data = dataAtRow(index.row()) ; data.element_) {
				*element = data.element_.data();
			}
		}
		return true;
	}

	return false;
}

/**
 * @brief TerminalStripModel::terminalsForIndex
 * @param index_list
 * @return A vector of PhysicalTerminalData represented by index_list.
 * If sereval index point to the same terminal the vector have only one PhysicalTerminalData
 */
QVector<PhysicalTerminalData> TerminalStripModel::physicalTerminalDataForIndex(QModelIndexList index_list) const
{
	QVector<PhysicalTerminalData> vector_;
	if (index_list.isEmpty()) {
		return vector_;
	}

	QSet<int> set_;

		//We use a QSet to avoid insert several time the same terminal.
	for (auto index : index_list) {
		if (index.isValid()) {
			set_.insert(index.row());
		}
	}

	for (auto i : set_) {
		vector_.append(physicalDataAtIndex(i));
	}

	return vector_;
}

/**
 * @brief TerminalStripModel::realTerminalDataForIndex
 * @param index_list
 * @return
 */
QVector<RealTerminalData> TerminalStripModel::realTerminalDataForIndex(QModelIndexList index_list) const
{
	QVector<RealTerminalData> vector_;
	if (index_list.isEmpty()) {
		return vector_;
	}

	QSet<int> set_;
		//We use a QSet to avoid insert several time the same terminal.
	for (auto index : index_list) {
		if (index.isValid()) {
			set_.insert(index.row());
		}
	}

	for (auto i : set_) {
		const auto rtd_ = realDataAtIndex(i);
		if (rtd_.level_ > -1) {
			vector_.append(realDataAtIndex(i));
		}
	}

	return vector_;
}

void TerminalStripModel::fillPhysicalTerminalData()
{
		//Get all physical terminal
	if (m_terminal_strip) {
		for (auto i=0 ; i < m_terminal_strip->physicalTerminalCount() ; ++i) {
			m_original_terminal_data.append(m_terminal_strip->physicalTerminalData(i));
		}
		m_edited_terminal_data = m_original_terminal_data;
	}
}

RealTerminalData TerminalStripModel::dataAtRow(int row) const
{
	if (row > rowCount(QModelIndex())) {
		return RealTerminalData();
	}
	else
	{
		auto current_row = 0;
		for (const auto &physical_data : m_edited_terminal_data)
		{
			for (const auto &real_data : physical_data.real_terminals_vector)
			{
				if (current_row == row) {
					return real_data;
				} else {
					++current_row;
				}
			}
		}
	}

	return RealTerminalData();
}

/**
 * @brief TerminalStripModel::replaceDataAtRow
 * Replace the data at row \p row by \p data
 * @param data
 * @param row
 */
void TerminalStripModel::replaceDataAtRow(RealTerminalData data, int row)
{
	if (row > rowCount(QModelIndex())) {
		return;
	}
	else
	{
		auto current_row = 0;
		auto current_physical = 0;

		for (const auto &physical_data : qAsConst(m_edited_terminal_data))
		{
			auto current_real = 0;
			for (int i=0 ; i<physical_data.real_terminals_vector.count() ; ++i)
			{
				if (current_row == row) {
					auto physical_data = m_edited_terminal_data.at(current_physical);
					physical_data.real_terminals_vector.replace(current_real, data);
					m_edited_terminal_data.replace(current_physical, physical_data);
					return;
				} else {
					++current_real;
					++current_row;
				}
			}
			++current_physical;
		}
	}
}

/**
 * @brief TerminalStripModel::physicalDataAtIndex
 * @param index
 * @return the physical terminal data at \p index.
 * We need to use this method because the model can have more index than physical
 * terminal, because physical terminal can be multi-level and each level
 * have is own row.
 * If \p index is out of range, return a default PhysicalTerminalData (pos_ is set to -1 by default)
 */
PhysicalTerminalData TerminalStripModel::physicalDataAtIndex(int index) const
{
	if (m_edited_terminal_data.isEmpty()) {
		return PhysicalTerminalData();
	}

	int current_checked_index = -1;
	int current_phy = -1;
	bool match_ = false;

	for (const auto &ptd_ : qAsConst(m_edited_terminal_data))
	{
		current_checked_index += ptd_.real_terminals_vector.size();
		++current_phy;

		if (current_checked_index >= index) {
			match_ = true;
			break;
		}
	}

	if (match_) {
		return  m_edited_terminal_data.at(current_phy);
	} else {
		return PhysicalTerminalData();
	}
}

/**
 * @brief TerminalStripModel::realDataAtIndex
 * @param index
 * @return the realTerminalData at index \p index.
 */
RealTerminalData TerminalStripModel::realDataAtIndex(int index) const
{
	if (m_edited_terminal_data.isEmpty()) {
		return RealTerminalData();
	}

	int current_checked_index = -1;

	for (const auto & ptd_ : qAsConst(m_edited_terminal_data))
	{
		for (const auto & rtd_ : qAsConst(ptd_.real_terminals_vector)) {
			++current_checked_index;
			if (current_checked_index == index) {
				return rtd_;
			}
		}
	}

	return RealTerminalData();
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
		qcb->setObjectName(QLatin1String("terminal_type"));
		qcb->addItem(ElementData::translatedTerminalType(ElementData::TTGeneric),   ElementData::TTGeneric);
		qcb->addItem(ElementData::translatedTerminalType(ElementData::TTFuse),      ElementData::TTFuse);
		qcb->addItem(ElementData::translatedTerminalType(ElementData::TTSectional), ElementData::TTSectional);
		qcb->addItem(ElementData::translatedTerminalType(ElementData::TTDiode),     ElementData::TTDiode);
		qcb->addItem(ElementData::translatedTerminalType(ElementData::TTGround),    ElementData::TTGround);

		return qcb;
	}
	if (index.column() == FUNCTION_CELL) {
		auto qcb = new QComboBox(parent);
		qcb->setObjectName(QLatin1String("terminal_function"));
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
		else {
			QStyledItemDelegate::setModelData(editor, model, index);
		}
	}
}
