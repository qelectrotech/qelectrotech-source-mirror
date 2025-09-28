/*
	Copyright 2006-2025 The QElectroTech Team
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
#include "../physicalterminal.h"
#include "../realterminal.h"
#include "../terminalstripbridge.h"
#include <QDebug>
#include <QBrush>
#include <QVector>
#include <QComboBox>
#include <QSpinBox>
#include <QPainter>
#include <QApplication>

/**
 * Some const int who describe what a column contain
 */
const int POS_CELL = 0;
const int LEVEL_CELL = 1;
const int LEVEL_0_CELL = 2;
const int LEVEL_1_CELL = 3;
const int LEVEL_2_CELL = 4;
const int LEVEL_3_CELL = 5;
const int LABEL_CELL = 6;
const int CONDUCTOR_CELL = 7;
const int XREF_CELL = 8;
const int CABLE_CELL = 9;
const int CABLE_WIRE_CELL = 10;
const int TYPE_CELL = 11;
const int FUNCTION_CELL = 12;
const int LED_CELL = 13;

const int COLUMN_COUNT = 14;

static QVector<bool> UNMODIFIED_CELL_VECTOR{false, false, false, false, false, false, false, false, false, false, false, false, false, false};

/**
 * @brief TerminalStripModel::levelForColumn
 * Return the terminal level for column @a column
 * or -1 if column is not a level column
 * @param column
 * @return
 */
int TerminalStripModel::levelForColumn(Column column)
{
	switch (column) {
		case Level0: return 0;
		case Level1: return 1;
		case Level2: return 2;
		case Level3: return 3;
		default: return -1;
	}
}

/**
 * @brief TerminalStripModel::columnTypeForIndex
 * @param index
 * @return the thing (pos, level, type, function etc...) for @a index
 */
TerminalStripModel::Column TerminalStripModel::columnTypeForIndex(const QModelIndex &index)
{
	if (index.isValid())
	{
		switch (index.column()) {
			case 0: return Pos;
			case 1: return Level;
			case 2 : return Level0;
			case 3 : return Level1;
			case 4 : return Level2;
			case 5 : return Level3;
			case 6 : return Label;
			case 7 : return Conductor;
			case 8 : return XRef;
			case 9 : return Cable;
			case 10 : return CableWire;
			case 11 : return Type;
			case 12 : return Function;
			case 13 : return Led;
			default : return Invalid;
		}
	}
	return Invalid;
}

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

	connect(terminal_strip, &TerminalStrip::bridgeColorChanged, this, [=] {
		emit dataChanged(index(0, LEVEL_0_CELL),
						 index(rowCount(), LEVEL_3_CELL));
	});
}

/**
 * @brief TerminalStripModel::setTerminalStrip
 * set the current terminal strip of this model to @a terminal_strip.
 * @param terminal_strip
 */
void TerminalStripModel::setTerminalStrip(TerminalStrip *terminal_strip)
{
	m_terminal_strip = terminal_strip;
	reload();
}

int TerminalStripModel::rowCount(const QModelIndex &parent) const
{
	Q_UNUSED(parent)

	if (!m_terminal_strip) {
		return 0;
	}

	auto count = 0;
	for (const auto &mptd : m_physical_data) {
		count += mptd.real_data.size();
	}

	return count;
}

int TerminalStripModel::columnCount(const QModelIndex &parent) const
{
	Q_UNUSED(parent)
	return COLUMN_COUNT;
}

QVariant TerminalStripModel::data(const QModelIndex &index, int role) const
{
	if (index.row() >= rowCount(QModelIndex())) {
		return QVariant();
	}

	const auto mrtd = dataAtRow(index.row());


	if (role == Qt::DisplayRole)
	{
		switch (index.column()) {
			case POS_CELL :        return physicalDataAtIndex(index.row()).pos_;
			case LEVEL_CELL :      return mrtd.level_;
			case LABEL_CELL :      return mrtd.label_;
			case CONDUCTOR_CELL :  return mrtd.conductor_;
			case XREF_CELL :       return mrtd.Xref_;
			case CABLE_CELL :      return mrtd.cable_;
			case CABLE_WIRE_CELL : return mrtd.cable_wire;
			case TYPE_CELL :       return ElementData::translatedTerminalType(mrtd.type_);
			case FUNCTION_CELL :   return ElementData::translatedTerminalFunction(mrtd.function_);
			default :              return QVariant();
		}
	}
	else if (role == Qt::EditRole)
	{
		switch (index.column()) {
			case LABEL_CELL : return mrtd.label_;
			default: return QVariant();

		}
	}
	else if (role == Qt::CheckStateRole &&
			   index.column() == LED_CELL)
	{
		return mrtd.led_ ? Qt::Checked : Qt::Unchecked;
	}
	else if (role == Qt::BackgroundRole && index.column() < COLUMN_COUNT )
	{
		if (m_modified_cell.contains(mrtd.element_) &&
			m_modified_cell.value(mrtd.element_).at(index.column()))
		{
			return QBrush(Qt::yellow);
		}
	}
	else if (role == Qt::DecorationRole &&
			 (index.column() == LEVEL_0_CELL ||
			 index.column() == LEVEL_1_CELL ||
			 index.column() == LEVEL_2_CELL ||
			 index.column() == LEVEL_3_CELL))
	{
		return bridgePixmapFor(index);
	}

	return QVariant();
}

bool TerminalStripModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
	auto mrtd = dataAtRow(index.row());
	bool modified_ = false;
	int modified_cell = -1;
	auto column_ = index.column();

	if (column_ == LEVEL_CELL &&
		role == Qt::EditRole)
	{
		mrtd.level_ = value.toInt();
		modified_ = true;
		modified_cell = LEVEL_CELL;
	}
	else if (column_ == LED_CELL)
	{
			mrtd.led_ = value.toBool();
			modified_ = true;
			modified_cell = LED_CELL;
	}
	else if (column_ == TYPE_CELL &&
			 role == Qt::EditRole)
	{
		mrtd.type_ = value.value<ElementData::TerminalType>();
		modified_ = true;
		modified_cell = TYPE_CELL;
	}
	else if (column_ == FUNCTION_CELL &&
			 role == Qt::EditRole)
	{
		mrtd.function_ = value.value<ElementData::TerminalFunction>();
		modified_ = true;
		modified_cell = FUNCTION_CELL;
	}
	else if (column_ == LABEL_CELL &&
			 role == Qt::EditRole &&
			 mrtd.label_ != value.toString())
	{
		mrtd.label_ = value.toString();
		modified_ = true;
		modified_cell = LABEL_CELL;
	}

		//Set the modification to the terminal data
	if (modified_)
	{
		replaceDataAtRow(mrtd, index.row());

		if (mrtd.element_)
		{
			QVector<bool> vector_;
			if (m_modified_cell.contains(mrtd.element_)) {
				vector_ = m_modified_cell.value(mrtd.element_);
			} else {
				vector_ = UNMODIFIED_CELL_VECTOR;
			}

			vector_.replace(modified_cell, true);
			m_modified_cell.insert(mrtd.element_, vector_);
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
				case LEVEL_0_CELL:    return QStringLiteral("0");
				case LEVEL_1_CELL:    return QStringLiteral("1");
				case LEVEL_2_CELL:    return QStringLiteral("2");
				case LEVEL_3_CELL:    return QStringLiteral("3");
				case LABEL_CELL:      return tr("Label");
				case CONDUCTOR_CELL:  return tr("Numéro de conducteur");
				case XREF_CELL:       return tr("Référence croisé");
				case CABLE_CELL:      return tr("Câble");
				case CABLE_WIRE_CELL: return tr("Couleur / numéro de fil câble");
				case TYPE_CELL:       return tr("Type");
				case FUNCTION_CELL :  return tr("Fonction");
				case LED_CELL:        return tr("led");
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
 * @return a vector of modified terminal.
 */
QVector<modelRealTerminalData> TerminalStripModel::modifiedmodelRealTerminalData() const
{
	QVector<modelRealTerminalData> returned_vector;

	const auto modified_real_terminal = m_modified_cell.keys();

	for (auto i = 0 ; i<m_physical_data.size() ; ++i)
	{
		auto ptd_ = m_physical_data.at(i);
		for (auto j = 0 ; j < ptd_.real_data.size() ; ++j)
		{
			auto mrtd_ = ptd_.real_data.at(j);
			if (modified_real_terminal.contains(mrtd_.element_))
			{
				returned_vector.append(m_physical_data.at(i).real_data.at(j));
			}
		}
	}

	return returned_vector;
}

/**
 * @brief TerminalStripModel::terminalsForIndex
 * @param index_list
 * @return A vector of PhysicalTerminalData represented by index_list.
 * If sereval index point to the same terminal the vector have only one PhysicalTerminalData
 */
QVector<modelPhysicalTerminalData> TerminalStripModel::modelPhysicalTerminalDataForIndex(QModelIndexList index_list) const
{
	QVector<modelPhysicalTerminalData> vector_;
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

	for (const auto i : set_)
	{
		const auto phy = physicalDataAtIndex(i);
		if (!vector_.contains(phy)) {
			vector_.append(phy);
		}
	}

	return vector_;
}

/**
 * @brief TerminalStripModel::realTerminalDataForIndex
 * @param index_list
 * @return
 */
QVector<modelRealTerminalData> TerminalStripModel::modelRealTerminalDataForIndex(QModelIndexList index_list) const
{
	QVector<modelRealTerminalData> vector_;
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

/**
 * @brief TerminalStripModel::realTerminalDataForIndex
 * @param index
 * @return modelRealTerminalData at index @a index or null modelRealTerminalData if invalid
 */
modelRealTerminalData TerminalStripModel::modelRealTerminalDataForIndex(const QModelIndex &index) const
{
	if (index.isValid()) {
		return realDataAtIndex(index.row());
	} else {
		return modelRealTerminalData();
	}
}

/**
 * @brief TerminalStripModel::buildBridgePixmap
 * Build the pixmap of bridge.
 * You should call this method when you know the
 * size to use in the bridge cell of a QTableView.
 * @param pixmap_size
 */
void TerminalStripModel::buildBridgePixmap(const QSize &pixmap_size)
{
	m_bridges_pixmaps.clear();
	for (auto color_ : TerminalStripBridge::bridgeColor())
	{
		QPen pen;
		pen.setColor(color_);
		pen.setWidth(1);

		QBrush brush;
		brush.setColor(color_);
		brush.setStyle(Qt::SolidPattern);

		QPixmap top_(pixmap_size);
		top_.fill(Qt::lightGray);
		QPainter top_p(&top_);
		top_p.setPen(pen);
		top_p.setBrush(brush);

		QPixmap middle_(pixmap_size);
		middle_.fill(Qt::lightGray);
		QPainter middle_p(&middle_);
		middle_p.setPen(pen);
		middle_p.setBrush(brush);

		QPixmap bottom_(pixmap_size);
		bottom_.fill(Qt::lightGray);
		QPainter bottom_p(&bottom_);
		bottom_p.setPen(pen);
		bottom_p.setBrush(brush);

		QPixmap none_(pixmap_size);
		none_.fill(Qt::lightGray);
		QPainter none_p(&none_);
		none_p.setPen(pen);
		none_p.setBrush(brush);


		auto w_ = pixmap_size.width();
		auto h_ = pixmap_size.height();

			//Draw circle
		top_p.drawEllipse(QPoint(w_/2, h_/2), w_/4, h_/4);
		middle_p.drawEllipse(QPoint(w_/2, h_/2), w_/4, h_/4);
		bottom_p.drawEllipse(QPoint(w_/2, h_/2), w_/4, h_/4);

			//Draw top line
		middle_p.drawRect((w_/2)-(w_/8), 0, w_/4, h_/2);
		bottom_p.drawRect((w_/2)-(w_/8), 0, w_/4, h_/2);
		none_p.drawRect((w_/2)-(w_/8), 0, w_/4, h_/2);

			//Draw bottom line
		top_p.drawRect((w_/2)-(w_/8), h_/2, w_/4, h_/2);
		middle_p.drawRect((w_/2)-(w_/8), h_/2, w_/4, h_/2);
		none_p.drawRect((w_/2)-(w_/8), h_/2, w_/4, h_/2);

		BridgePixmap bpxm;
		bpxm.top_ = top_;
		bpxm.middle_ = middle_;
		bpxm.bottom_ = bottom_;
		bpxm.none_ = none_;
		m_bridges_pixmaps.insert(color_, bpxm);
	}
}

/**
 * @brief TerminalStripModel::reload
 * Reload (reset) the model
 */
void TerminalStripModel::reload()
{
	beginResetModel();
	m_physical_data.clear();
	m_modified_cell.clear();
	fillPhysicalTerminalData();
	endResetModel();
}

void TerminalStripModel::fillPhysicalTerminalData()
{
		//Get all physical terminal
	if (m_terminal_strip)
	{
		for (const auto &phy_t : m_terminal_strip->physicalTerminal())
		{
			modelPhysicalTerminalData mptd;
			mptd.pos_ = phy_t->pos();
			mptd.uuid_ = phy_t->uuid();

			for (const auto &real_t : phy_t->realTerminals())
			{
				if (!real_t.isNull())
				{
					mptd.real_data.append(modelRealTerminalData::data(real_t));
				}
			}

			m_physical_data.append(mptd);
		}
	}
}

modelRealTerminalData TerminalStripModel::dataAtRow(int row) const
{
	if (row > rowCount(QModelIndex())) {
		return modelRealTerminalData();
	}
	else
	{
		auto current_row = 0;
		for (const auto &physical_data : std::as_const(m_physical_data))
		{
			for (const auto &real_data : physical_data.real_data)
			{
				if (current_row == row) {
					return real_data;
				} else {
					++current_row;
				}
			}
		}
	}

	return modelRealTerminalData();
}

/**
 * @brief TerminalStripModel::replaceDataAtRow
 * Replace the data at row \p row by \p data
 * @param data
 * @param row
 */
void TerminalStripModel::replaceDataAtRow(modelRealTerminalData data, int row)
{
	if (row > rowCount(QModelIndex())) {
		return;
	}
	else
	{
		auto current_row = 0;
		auto current_physical = 0;

		for (const auto &physical_data : std::as_const(m_physical_data))
		{
			auto current_real = 0;
			for (int i=0 ; i<physical_data.real_data.count() ; ++i)
			{
				if (current_row == row) {
					auto physical_data = m_physical_data.at(current_physical);
					physical_data.real_data.replace(current_real, data);
					m_physical_data.replace(current_physical, physical_data);
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
modelPhysicalTerminalData TerminalStripModel::physicalDataAtIndex(int index) const
{
	if (m_physical_data.isEmpty()) {
		return modelPhysicalTerminalData();
	}

	int current_checked_index = -1;
	int current_phy = -1;
	bool match_ = false;

	for (const auto &ptd_ : std::as_const(m_physical_data))
	{
		current_checked_index += ptd_.real_data.size();
		++current_phy;

		if (current_checked_index >= index) {
			match_ = true;
			break;
		}
	}

	if (match_) {
		return  m_physical_data.at(current_phy);
	} else {
		return modelPhysicalTerminalData();
	}
}

/**
 * @brief TerminalStripModel::realDataAtIndex
 * @param index
 * @return the realTerminalData at index \p index.
 */
modelRealTerminalData TerminalStripModel::realDataAtIndex(int index) const
{
	if (m_physical_data.isEmpty()) {
		return modelRealTerminalData();
	}

	int current_checked_index = -1;

	for (const auto & ptd_ : std::as_const(m_physical_data))
	{
		for (const auto & rtd_ : std::as_const(ptd_.real_data)) {
			++current_checked_index;
			if (current_checked_index == index) {
				return rtd_;
			}
		}
	}

	return modelRealTerminalData();
}

QPixmap TerminalStripModel::bridgePixmapFor(const QModelIndex &index) const
{
	if (!index.isValid() || m_terminal_strip.isNull()) {
		return QPixmap();
	}

	auto level_column = levelForColumn(columnTypeForIndex(index));
	if (level_column == -1) {
		return QPixmap();
	}

	auto mrtd = modelRealTerminalDataForIndex(index);

		//Terminal level correspond to the column level of index
	if (level_column == mrtd.level_)
	{
		if (mrtd.bridged_)
		{
			auto bridge_ = m_terminal_strip->isBridged(mrtd.real_terminal);
			if (bridge_)
			{
				const auto previous_t = m_terminal_strip->previousTerminalInLevel(mrtd.real_terminal);
				QSharedPointer<TerminalStripBridge> previous_bridge;
				if (previous_t)
					previous_bridge = previous_t->bridge();

				const auto next_t = m_terminal_strip->nextTerminalInLevel(mrtd.real_terminal);
				QSharedPointer<TerminalStripBridge> next_bridge;
				if (next_t)
					next_bridge = next_t->bridge();

				auto color_ = bridge_->color();
				auto pixmap_ = m_bridges_pixmaps.value(color_);

				//Current real terminal between two bridged terminal
				if ((bridge_ == previous_bridge) &&
					(bridge_ == next_bridge)) {
					return pixmap_.middle_;
				} else if (bridge_ == previous_bridge) {
					return pixmap_.bottom_;
				} else if (bridge_ == next_bridge) {
					return pixmap_.top_;
				}
			}
		}
	}
		//Terminal level ins't in the same column level of index
		//Check if we need to draw a none bridge pixmap

		//Check previous
	auto phy_t = mrtd.real_terminal.toStrongRef()->physicalTerminal();
	auto current_real_terminal = mrtd;
	auto current_phy_uuid = phy_t->uuid();
	bool already_jumped_to_previous = false;
	modelRealTerminalData previous_data;

	do {
		current_real_terminal = modelRealTerminalData::data(m_terminal_strip->previousRealTerminal(current_real_terminal.real_terminal));

		if (current_real_terminal.level_ == -1) {
			break;
		}

			//We are in the same physical terminal as previous loop
		if (current_phy_uuid == current_real_terminal.real_terminal.toStrongRef()->physicalTerminal()->uuid())
		{
			if (current_real_terminal.bridged_ &&
				current_real_terminal.level_ == level_column) {
				previous_data = current_real_terminal;
				break;
			}
		}
		else if (already_jumped_to_previous) { //We are not in same physical terminal as previous loop
			break;
		} else {
			already_jumped_to_previous = true;
			current_phy_uuid = current_real_terminal.real_terminal.toStrongRef()->physicalTerminal()->uuid();
			if (current_real_terminal.bridged_ &&
				current_real_terminal.level_ == level_column) {
				previous_data = current_real_terminal;
				break;
			}
		}
	} while(true);

		//Check next
	current_real_terminal = mrtd;
	current_phy_uuid = phy_t->uuid();
	bool already_jumped_to_next = false;
	modelRealTerminalData next_data;

	do {
		current_real_terminal = modelRealTerminalData::data(m_terminal_strip->nextRealTerminal(current_real_terminal.real_terminal));

		if (current_real_terminal.level_ == -1) {
			break;
		}

			//We are in the same physical terminal as previous loop
		if (current_phy_uuid == current_real_terminal.real_terminal.toStrongRef()->physicalTerminal()->uuid())
		{
			if (current_real_terminal.bridged_ &&
				current_real_terminal.level_ == level_column) {
				next_data = current_real_terminal;
				break;
			}
		}
		else if (already_jumped_to_next) { //We are not in same physical terminal as previous loop
			break;
		} else {
			already_jumped_to_next = true;
			current_phy_uuid = current_real_terminal.real_terminal.toStrongRef()->physicalTerminal()->uuid();
			if (current_real_terminal.bridged_ &&
				current_real_terminal.level_ == level_column) {
				next_data = current_real_terminal;
				break;
			}
		}
	} while(true);

	auto previous_bridge = m_terminal_strip->isBridged(previous_data.real_terminal);
	if (previous_bridge == m_terminal_strip->isBridged(next_data.real_terminal))
	{
		if (previous_bridge) {
			return m_bridges_pixmaps.value(previous_bridge->color()).none_;
		}
	}

	return QPixmap();
}

/***********************************************************
 * A little delegate for add a combobox to edit type
 * and a spinbox to edit the level of a terminal
 **********************************************************/

TerminalStripModelDelegate::TerminalStripModelDelegate(QObject *parent) :
	QStyledItemDelegate(parent)
{}

QWidget *TerminalStripModelDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
	if (index.column() == TYPE_CELL) {
		auto qcb = new QComboBox(parent);
		qcb->setObjectName(QStringLiteral("terminal_type"));
		qcb->addItem(ElementData::translatedTerminalType(ElementData::TTGeneric),   ElementData::TTGeneric);
		qcb->addItem(ElementData::translatedTerminalType(ElementData::TTFuse),      ElementData::TTFuse);
		qcb->addItem(ElementData::translatedTerminalType(ElementData::TTSectional), ElementData::TTSectional);
		qcb->addItem(ElementData::translatedTerminalType(ElementData::TTDiode),     ElementData::TTDiode);
		qcb->addItem(ElementData::translatedTerminalType(ElementData::TTGround),    ElementData::TTGround);

		return qcb;
	}
	if (index.column() == FUNCTION_CELL) {
		auto qcb = new QComboBox(parent);
		qcb->setObjectName(QStringLiteral("terminal_function"));
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

/**
 * @brief TerminalStripModelDelegate::paint
 * By default on a QTableView, Qt draw pixmap in cell with a little margin at left.
 * Override the function to draw the pixmap of bridge without the margin at left.
 * @param painter
 * @param option
 * @param index
 */
void TerminalStripModelDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
	auto column = index.column();
	if (column == LEVEL_0_CELL ||
		column == LEVEL_1_CELL ||
		column == LEVEL_2_CELL ||
		column == LEVEL_3_CELL)
	{
		auto variant = index.data(Qt::DecorationRole);
		if (variant.isNull()) {
			QStyledItemDelegate::paint(painter, option, index);
		}
		else
		{
			if (option.state & QStyle::State_Selected)
			{
				QStyleOptionViewItem opt_ = option;
				initStyleOption(&opt_, index);
				QStyle *style = QApplication::style();
				auto px = style->generatedIconPixmap(QIcon::Selected, variant.value<QPixmap>(), &opt_);
				style->drawItemPixmap(painter, option.rect, Qt::AlignLeft, px);
			}
			else {
				painter->drawPixmap(option.rect, variant.value<QPixmap>());
			}
		}
	}
	else {
		QStyledItemDelegate::paint(painter, option, index);
	}
}
