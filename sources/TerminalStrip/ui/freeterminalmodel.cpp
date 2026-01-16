/*
	Copyright 2006-2026 The QElectroTech Team
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
#include <QComboBox>

#include "freeterminalmodel.h"
#include "../../elementprovider.h"
#include "../../utils/qetutils.h"
#include "../../qetgraphicsitem/terminalelement.h"
#include "../realterminal.h"
#include "../../qetinformation.h"

const int LABEL_CELL = 0;
const int XREF_CELL = 1;
const int TYPE_CELL = 2;
const int FUNCTION_CELL = 3;
const int LED_CELL = 4;

const int COLUMN_COUNT = 5;

static QVector<bool> UNMODIFIED_CELL_VECTOR{false, false, false, false, false};

FreeTerminalModel::Column FreeTerminalModel::columnTypeForIndex(const QModelIndex &index)
{
	if (index.isValid())
	{
		switch (index.column()) {
			case 0 : return Label;
			case 1 : return XRef;
			case 2 : return Type;
			case 3 : return Function;
			case 4 : return Led;
			default : return Invalid;
		}
	}
	return Invalid;
}

/**
 * @brief FreeTerminalModel::FreeTerminalModel
 * @param project
 * @param parent
 */
FreeTerminalModel::FreeTerminalModel(QETProject *project, QObject *parent) :
    QAbstractTableModel(parent) {
    setProject(project);
}

/**
 * @brief FreeTerminalModel::setProject
 * Set @project as project handled by this model.
 * If a previous project was setted, everything is clear.
 * This function track the destruction of the project,
 * that  mean if the project pointer is deleted
 * no need to call this function with a nullptr,
 * everything is made inside this class.
 * @param project
 */
void FreeTerminalModel::setProject(QETProject *project)
{
    if(m_project) {
        disconnect(m_project, &QObject::destroyed, this, &FreeTerminalModel::clear);
    }
    m_project = project;
    if (m_project) {
        connect(m_project, &QObject::destroyed, this, &FreeTerminalModel::clear);
    }
    clear();
}

/**
 * @brief FreeTerminalModel::rowCount
 * @param parent
 */
int FreeTerminalModel::rowCount(const QModelIndex &parent) const
{
	Q_UNUSED(parent)

	return m_terminal_vector.size();
}

/**
 * @brief FreeTerminalModel::columnCount
 * @param parent
 */
int FreeTerminalModel::columnCount(const QModelIndex &parent) const
{
	Q_UNUSED(parent)
	return COLUMN_COUNT;
}

/**
 * @brief FreeTerminalModel::data
 * @param index
 * @param role
 * @return
 */
QVariant FreeTerminalModel::data(const QModelIndex &index, int role) const
{
	if (index.row() >= rowCount(QModelIndex())) {
		return QVariant();
	}

	const auto real_t_data = m_real_t_data.at(index.row());

	if (role == Qt::DisplayRole)
	{

		switch (index.column())
		{
			case LABEL_CELL: return real_t_data.label_;
			case XREF_CELL : return real_t_data.Xref_;
			case TYPE_CELL : return ElementData::translatedTerminalType(real_t_data.type_);
			case FUNCTION_CELL: return ElementData::translatedTerminalFunction(real_t_data.function_);
			default : return QVariant();
		}
	}
	else if (role == Qt::EditRole)
	{
		switch (index.column()) {
			case LABEL_CELL : return real_t_data.label_;
			default : return QVariant();
		}
	}
	else if (role == Qt::CheckStateRole &&
			 index.column() == LED_CELL) {
		return real_t_data.led_ ? Qt::Checked : Qt::Unchecked;
	}
	else if (role == Qt::BackgroundRole && index.column() <= LED_CELL)
	{
		if (m_modified_cell.contains(real_t_data.real_terminal) &&
			m_modified_cell.value(real_t_data.real_terminal).at(index.column()))
		{
			return QBrush(Qt::yellow);
		}
	}

	return QVariant();
}

bool FreeTerminalModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
	auto mrtd = dataAtRow(index.row());
	bool modified_ = false;
	int modified_cell = -1;
	auto column_ = index.column();


	if (column_ == LABEL_CELL &&
		role == Qt::EditRole &&
		mrtd.label_ != value.toString())
	{
		mrtd.label_ = value.toString();
		modified_ = true;
		modified_cell = LABEL_CELL;
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
	else if (column_ == LED_CELL)
	{
		mrtd.led_ = value.toBool();
		modified_ = true;
		modified_cell = LED_CELL;
	}

		//Set the modification to the terminal data
	if (modified_)
	{
		m_real_t_data.replace(index.row(), mrtd);

		QVector<bool> vector_;
		if (m_modified_cell.contains(mrtd.real_terminal)) {
			vector_ = m_modified_cell.value(mrtd.real_terminal);
		} else {
			vector_ = UNMODIFIED_CELL_VECTOR;
		}

		vector_.replace(modified_cell, true);
		m_modified_cell.insert(mrtd.real_terminal, vector_);
		emit dataChanged(index, index);
		return true;
	}

	return false;
}

/**
 * @brief FreeTerminalModel::headerData
 * @param section
 * @param orientation
 * @param role
 * @return
 */
QVariant FreeTerminalModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	if (role == Qt::DisplayRole)
	{
		if (orientation == Qt::Horizontal)
		{
			switch (section) {
				case LABEL_CELL:    return tr("Label");
				case XREF_CELL:     return tr("Référence croisé");
				case TYPE_CELL:     return tr("Type");
				case FUNCTION_CELL: return tr("Fonction");
				case LED_CELL:      return tr("led");
				default : return QVariant();
			}
		}
	}

	return QVariant();
}

Qt::ItemFlags FreeTerminalModel::flags(const QModelIndex &index) const
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
 * @brief FreeTerminalModel::clear
 * Clear the model and set it as the current
 * state of the project
 */
void FreeTerminalModel::clear()
{
	beginResetModel();
	m_terminal_vector.clear();
	m_real_t_data.clear();
	m_modified_cell.clear();
	fillTerminalVector();
	endResetModel();
}

/**
 * @brief FreeTerminalModel::modifiedModelRealTerminalData
 * @return a vector of modified terminal.
 */
QVector<modelRealTerminalData> FreeTerminalModel::modifiedModelRealTerminalData() const
{
	QVector<modelRealTerminalData> returned_vector;

	for (const auto &real_t_data : m_real_t_data) {
		if (m_modified_cell.contains(real_t_data.real_terminal)) {
			returned_vector.append(real_t_data);
		}
	}

	return returned_vector;
}

/**
 * @brief FreeTerminalModel::dataAtRow
 * @param row
 * @return the current data at row @a row
 */
modelRealTerminalData FreeTerminalModel::dataAtRow(int row) const
{
	if (row >= m_terminal_vector.size()) {
		return modelRealTerminalData();
	} else {
		return m_real_t_data.at(row);
	}
}

/**
 * @brief FreeTerminalModel::realTerminalForIndex
 * @param index_list
 * @return The QSharedPointer<RealTerminal> associated with the index found in @a index_list
 */
QVector<QSharedPointer<RealTerminal> > FreeTerminalModel::realTerminalForIndex(const QModelIndexList &index_list) const
{
	QVector<QSharedPointer<RealTerminal>> vector_;
	for (const auto &index : index_list)
	{
		if (index.isValid()
			&& index.model() == this
			&& index.row() < m_terminal_vector.size())
		{
			const auto rt_ = m_terminal_vector.at(index.row());
			if (!vector_.contains(rt_)) {
				vector_.append(m_terminal_vector.at(index.row()));
			}
		}
	}

	return vector_;
}

/**
 * @brief FreeTerminalModel::fillTerminalVector
 */
void FreeTerminalModel::fillTerminalVector()
{
    if (m_project) {
        ElementProvider provider_(m_project);
        auto free_terminal_vector = provider_.freeTerminal();

        std::sort(free_terminal_vector.begin(), free_terminal_vector.end(),
                  [](TerminalElement *a, TerminalElement *b)
                  {
                      return QETUtils::sortBeginIntString(a->elementData().m_informations.value(QETInformation::ELMT_LABEL).toString(),
                                                          b->elementData().m_informations.value(QETInformation::ELMT_LABEL).toString());
                  });

        for (const auto &terminal_ : free_terminal_vector) {
            m_terminal_vector.append(terminal_->realTerminal());
            m_real_t_data.append(modelRealTerminalData::data(terminal_->realTerminal()));
        }
    }
}

/****************************************************************
 * A little delegate for add a combobox to edit type and function
 ****************************************************************/

FreeTerminalModelDelegate::FreeTerminalModelDelegate(QObject *parent) :
	QStyledItemDelegate(parent)
{}

QWidget *FreeTerminalModelDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const
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

void FreeTerminalModelDelegate::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
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
