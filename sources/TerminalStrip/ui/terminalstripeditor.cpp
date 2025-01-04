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
#include "terminalstripeditor.h"
#include "ui_terminalstripeditor.h"
#include "../UndoCommand/addterminaltostripcommand.h"
#include "../../qetproject.h"
#include "../terminalstrip.h"
#include "../UndoCommand/changeterminalstripdata.h"
#include "../undocommand/changeelementdatacommand.h"
#include "terminalstripmodel.h"
#include "../diagram.h"
#include "../UndoCommand/sortterminalstripcommand.h"
#include "../UndoCommand/groupterminalscommand.h"
#include "../UndoCommand/changeterminallevel.h"
#include "../UndoCommand/bridgeterminalscommand.h"
#include "../UndoCommand/changeterminalstripcolor.h"
#include "../physicalterminal.h"
#include "../terminalstripbridge.h"

/**
 * @brief TerminalStripEditor::TerminalStripEditor
 * @param project : Project to manage the terminal strip
 * @param parent : paent widget
 */
TerminalStripEditor::TerminalStripEditor(QETProject *project, QWidget *parent) :
	QWidget{parent},
	ui{new Ui::TerminalStripEditor},
	m_project{project}
{
	ui->setupUi(this);

	ui->m_table_widget->setItemDelegate(new TerminalStripModelDelegate{this});

		//Setup the bridge color
	ui->m_bridge_color_cb->setColors(TerminalStripBridge::bridgeColor().toList());

		//Call for update the state of child widgets
	selectionChanged();

		//Go the diagram of double clicked terminal
	connect(ui->m_table_widget, &QAbstractItemView::doubleClicked, this, [=](const QModelIndex &index)
	{
		if (m_model->columnTypeForIndex(index) == TerminalStripModel::XRef)
		{
			auto mrtd = m_model->modelRealTerminalDataForIndex(index);
			if (mrtd.element_)
			{
				auto elmt = mrtd.element_;
				auto diagram = elmt->diagram();
				if (diagram)
				{
					diagram->showMe();
					if (diagram->views().size())
					{
						auto fit_view = elmt->sceneBoundingRect();
						fit_view.adjust(-200,-200,200,200);
						diagram->views().at(0)->fitInView(fit_view, Qt::KeepAspectRatioByExpanding);
					}
				}
			}
		}
	});
}

/**
 * @brief TerminalStripEditor::~TerminalStripEditor
 */
TerminalStripEditor::~TerminalStripEditor() {
	delete ui;
}

/**
 * @brief TerminalStripEditor::setCurrentStrip
 * Set the current terminal strip edited to \p strip_
 * @param strip_
 */
void TerminalStripEditor::setCurrentStrip(TerminalStrip *strip_)
{
	if (strip_ == m_current_strip) {
		return;
	}

	if (m_current_strip) {
		disconnect(m_current_strip, &TerminalStrip::orderChanged, this, &TerminalStripEditor::reload);
		disconnect(m_current_strip, &TerminalStrip::bridgeChanged, this, &TerminalStripEditor::reload);
	}

	ui->m_move_to_cb->clear();

	if (!strip_)
	{
		ui->m_installation_le ->clear();
		ui->m_location_le     ->clear();
		ui->m_name_le         ->clear();
		ui->m_comment_le      ->clear();
		ui->m_description_te  ->clear();
		m_current_strip = nullptr;

		ui->m_table_widget->setModel(nullptr);
		if (m_model) {
			m_model->deleteLater();
			m_model = nullptr;
		}		
	}
	else
	{
		ui->m_installation_le ->setText(strip_->installation());
		ui->m_location_le     ->setText(strip_->location());
		ui->m_name_le         ->setText(strip_->name());
		ui->m_comment_le      ->setText(strip_->comment());
		ui->m_description_te  ->setPlainText(strip_->description());
		ui->m_move_to_cb->addItem(tr("Bornes indépendantes"), QUuid());

		const auto project_{strip_->project()};
		if (project_)
		{
			const auto strip_vector = project_->terminalStrip();
			for (const auto &strip : strip_vector)
			{
				if (strip == strip_) {
					continue;
				}

				ui->m_move_to_cb->addItem(QString{strip->installation() + " " + strip->location() + " " + strip->name()},
										  strip->uuid());
			}
		}

		m_current_strip = strip_;

		if (m_model) {
			m_model->setTerminalStrip(strip_);
		}
		else
		{
			m_model = new TerminalStripModel{strip_, this};
			ui->m_table_widget->setModel(m_model);
			m_model->buildBridgePixmap(setUpBridgeCellWidth());
			connect(ui->m_table_widget->selectionModel(), &QItemSelectionModel::selectionChanged, this, &TerminalStripEditor::selectionChanged);
		}

		spanMultiLevelTerminals();
		selectionChanged();	//Used to update child widgets

		connect(m_current_strip, &TerminalStrip::orderChanged, this, &TerminalStripEditor::reload);
		connect(m_current_strip, &TerminalStrip::bridgeChanged, this, &TerminalStripEditor::reload);
	}
}

/**
 * @brief TerminalStripEditor::reload
 * Reload this editor and and reset all
 * unapplied change.
 */
void TerminalStripEditor::reload()
{
	if (m_current_strip)
	{
		ui->m_installation_le ->setText(m_current_strip->installation());
		ui->m_location_le     ->setText(m_current_strip->location());
		ui->m_name_le         ->setText(m_current_strip->name());
		ui->m_comment_le      ->setText(m_current_strip->comment());
		ui->m_description_te  ->setPlainText(m_current_strip->description());
	}
	if (m_model)
	{
		m_model->reload();
		spanMultiLevelTerminals();
	}
}

/**
 * @brief TerminalStripEditor::apply
 * Apply current edited values.
 */
void TerminalStripEditor::apply()
{

	if (m_current_strip)
	{
		m_project->undoStack()->beginMacro(tr("Modifier des propriétés de borniers"));

		TerminalStripData data;
		data.m_installation = ui->m_installation_le->text();
		data.m_location     = ui->m_location_le->text();
		data.m_name         = ui->m_name_le->text();
		data.m_comment      = ui->m_comment_le->text();
		data.m_description  = ui->m_description_te->toPlainText();

		m_project->undoStack()->push(new ChangeTerminalStripData(m_current_strip, data, nullptr));

		if (m_model)
		{
			for (const auto &data_ : m_model->modifiedmodelRealTerminalData())
			{
				if (auto element = data_.element_)
				{
					auto current_data = element->elementData();
					current_data.setTerminalType(data_.type_);
					current_data.setTerminalFunction(data_.function_);
					current_data.setTerminalLED(data_.led_);
					current_data.m_informations.addValue(QStringLiteral("label"), data_.label_);

					if (element->elementData() != current_data)
						m_project->undoStack()->push(new ChangeElementDataCommand(element, current_data));
					if (data_.level_ != data_.real_terminal.toStrongRef()->level())
						m_project->undoStack()->push(new ChangeTerminalLevel(m_current_strip, data_.real_terminal, data_.level_));
				}
			}
		}

		m_project->undoStack()->endMacro();
	}

	reload();
}

/**
 * @brief TerminalStripEditor::spanMultiLevelTerminals
 * Span row of m_table_widget for multi-level terminal
 */
void TerminalStripEditor::spanMultiLevelTerminals()
{
	if (!m_current_strip) {
		return;
	}

	ui->m_table_widget->clearSpans();
	auto current_row = 0;
	for (auto i = 0 ; i < m_current_strip->physicalTerminalCount() ; ++i)
	{
		const auto level_count = m_current_strip->physicalTerminal(i)->realTerminalCount();
		if (level_count > 1) {
			ui->m_table_widget->setSpan(current_row, 0, level_count, 1);
		}
		current_row += level_count;
	}
}

/**
 * @brief TerminalStripEditor::selectionChanged
 * Update the editor according to the current selection
 */
void TerminalStripEditor::selectionChanged()
{
	if (!m_model) {
		ui->m_auto_ordering_pb  ->setDisabled(true);
		ui->m_group_terminals_pb->setDisabled(true);
		ui->m_ungroup_pb        ->setDisabled(true);
		ui->m_level_sb          ->setDisabled(true);
		ui->m_type_cb           ->setDisabled(true);
		ui->m_function_cb       ->setDisabled(true);
		ui->m_led_cb            ->setDisabled(true);

		ui->m_bridge_terminals_pb  ->setDisabled(true);
		ui->m_unbridge_terminals_pb->setDisabled(true);
		ui->m_bridge_color_cb      ->setDisabled(true);
		return;
	}

	ui->m_auto_ordering_pb->setEnabled(true);

	const auto index_list = ui->m_table_widget->selectionModel()->selectedIndexes();

	if (index_list.isEmpty()) {
		ui->m_type_cb     ->setDisabled(true);
		ui->m_function_cb ->setDisabled(true);
		ui->m_led_cb      ->setDisabled(true);
	} else {
		ui->m_type_cb     ->setEnabled(true);
		ui->m_function_cb ->setEnabled(true);
		ui->m_led_cb      ->setEnabled(true);
	}

	const auto model_physical_terminal_vector = m_model->modelPhysicalTerminalDataForIndex(index_list);
	const auto model_real_terminal_vector = m_model->modelRealTerminalDataForIndex(index_list);

		//Enable/disable group button
	ui->m_group_terminals_pb->setEnabled(model_physical_terminal_vector.size() > 1 ? true : false);

		//Enable/disable ungroup button
	auto it_= std::find_if(model_physical_terminal_vector.constBegin(), model_physical_terminal_vector.constEnd(), [](const modelPhysicalTerminalData &data)
	{
		if (data.real_data.size() >= 2) {
			return true;
		} else {
			return false;
		}
	});
	ui->m_ungroup_pb->setDisabled(it_ == model_physical_terminal_vector.constEnd());

		//Enable/disable level spinbox
	bool enable_ = false;
	for (const auto &physical : model_physical_terminal_vector)
	{
		if (physical.real_data.size() > 1) {
			enable_ = true;
			break;
		}
	}
	ui->m_level_sb->setEnabled(enable_);

		//Enable/disable bridge and unbridge
	bool enable_bridge = false;
	bool enable_unbridge = false;
	bool enable_bridge_color = false;

		//One column must be selected and the column must be a level column
	int level_ = TerminalStripModel::levelForColumn(isSingleColumnSelected());
	if (level_ >= 0 && m_current_strip)
	{
			//Select only terminals of corresponding level cell selection
		QVector<QSharedPointer<RealTerminal>> real_terminal_in_level_vector;
		for (const auto &mrtd : model_real_terminal_vector)
		{
			if (mrtd.level_ == level_) {
				real_terminal_in_level_vector.append(mrtd.real_terminal.toStrongRef());
				if (!enable_bridge_color && mrtd.bridged_) {
					enable_bridge_color = true;
				}
			}
		}
		enable_bridge = m_current_strip->isBridgeable(real_terminal_in_level_vector);
		enable_unbridge = m_current_strip->canUnBridge(real_terminal_in_level_vector);
	}
	ui->m_bridge_terminals_pb->setEnabled(enable_bridge);
	ui->m_unbridge_terminals_pb->setEnabled(enable_unbridge);
	ui->m_bridge_color_cb->setEnabled(enable_bridge_color);

		//Enable or not the 'move to' buttons
	bool enabled_move_to{!model_physical_terminal_vector.isEmpty()};
	for (const auto &model_physical : model_physical_terminal_vector)
	{
		for (const auto &model_real_data : model_physical.real_data)
		{
			if (model_real_data.bridged_) {
				enabled_move_to = false;
				break;
			}
		}
	}
	ui->m_move_to_label->setEnabled(enabled_move_to);
	ui->m_move_to_cb->setEnabled(enabled_move_to);
	ui->m_move_to_pb->setEnabled(enabled_move_to);
}

QSize TerminalStripEditor::setUpBridgeCellWidth()
{
	if (ui->m_table_widget->verticalHeader() &&
		m_model)
	{
		auto section_size = ui->m_table_widget->verticalHeader()->defaultSectionSize();
		auto h_header = ui->m_table_widget->horizontalHeader();

		for (int i = TerminalStripModel::Level0 ; i<(TerminalStripModel::Level3+1) ; ++i) {
			ui->m_table_widget->setColumnWidth(i, section_size);
			h_header->setSectionResizeMode(i, QHeaderView::Fixed);
		}

		return QSize(section_size, section_size);
	}

	return QSize(0,0);
}

/**
 * @brief TerminalStripEditor::isSingleColumnSelected
 * If all current QModelIndex are in the same column
 * return the column type
 * @sa TerminalStripModel::Column
 * @return the column or TerminalStripModel::Invalid if several column are selected
 */
TerminalStripModel::Column TerminalStripEditor::isSingleColumnSelected() const
{
	if (m_current_strip &&
		ui->m_table_widget->selectionModel())
	{
		const auto index_list = ui->m_table_widget->selectionModel()->selectedIndexes();
		if (index_list.isEmpty()) {
			return TerminalStripModel::Invalid;
		}

		auto column_ = index_list.first().column();
		for (const auto &index : index_list) {
			if (index.column() != column_) {
				return TerminalStripModel::Invalid;
			}
		}

		return TerminalStripModel::columnTypeForIndex(index_list.first());
	}

	return TerminalStripModel::Invalid;
}

/**
 * @brief TerminalStripEditor::singleColumnData
 * @return a QPair with for first value the column and for second value the data
 * of selected cell of the table widget, only if the selected cells are
 * in the same column. If selected cells are not in the same column the first value
 * of the QPair is TerminalStripModel::Invalid.
 */
QPair<TerminalStripModel::Column, QVector<modelRealTerminalData> > TerminalStripEditor::singleColumnData() const
{
	if (m_current_strip)
	{
		auto level_ = isSingleColumnSelected();
		if (level_ != TerminalStripModel::Invalid)
		{
			const auto index_list = ui->m_table_widget->selectionModel()->selectedIndexes();
			const auto mrtd_vector = m_model->modelRealTerminalDataForIndex(index_list);
			return qMakePair(level_, mrtd_vector);
		}
	}

	return qMakePair(TerminalStripModel::Invalid, QVector<modelRealTerminalData>());
}

/**
 * @brief TerminalStripEditor::on_m_auto_pos_pb_clicked
 */
void TerminalStripEditor::on_m_auto_ordering_pb_clicked()
{
	if (m_project && m_current_strip) {
		m_project->undoStack()->push(new SortTerminalStripCommand(m_current_strip));
	}
}

/**
 * @brief TerminalStripEditor::on_m_group_terminals_pb_clicked
 */
void TerminalStripEditor::on_m_group_terminals_pb_clicked()
{
	if (m_model && m_current_strip && m_project)
	{
		auto mrtd_vector = m_model->modelRealTerminalDataForIndex(ui->m_table_widget->selectionModel()->selectedIndexes());
		if (mrtd_vector.size() >= 2)
		{
				//At this step get the first physical terminal as receiver
			auto receiver_ = mrtd_vector.first().real_terminal.toStrongRef()->physicalTerminal();

			QVector<QSharedPointer<RealTerminal>> vector_;
			int count_ = 0;
			for (const auto & mrtd : mrtd_vector)
			{
				const auto real_t = mrtd.real_terminal.toStrongRef();
				vector_.append(real_t);

					//Get the better physical terminal as receiver
					//(physical terminal with the max of real terminal)
				const auto current_physical = real_t->physicalTerminal();
				int real_t_count = current_physical->realTerminalCount();
				if (real_t_count > 1 && real_t_count > count_) {
					count_ = real_t_count;
					receiver_ = real_t->physicalTerminal();
				}

			}

				//Now we remove from vector_ all real terminal of receiver
			for (const auto &real_t : receiver_->realTerminals()) {
				vector_.removeOne(real_t);
			}

			m_project->undoStack()->push(new GroupTerminalsCommand(m_current_strip,
																   receiver_,
																   vector_));
		}
	}
}

/**
 * @brief TerminalStripEditor::on_m_ungroup_pb_clicked
 */
void TerminalStripEditor::on_m_ungroup_pb_clicked()
{
	if (m_model && m_current_strip)
	{
		const auto mrtd_vector = m_model->modelRealTerminalDataForIndex(ui->m_table_widget->selectionModel()->selectedIndexes());

		QVector<QSharedPointer<RealTerminal>> vector_;
		for (const auto &mrtd : mrtd_vector) {
			vector_.append(mrtd.real_terminal.toStrongRef());
		}
		m_project->undoStack()->push(new UnGroupTerminalsCommand(m_current_strip,vector_));
	}
}

/**
 * @brief TerminalStripEditor::on_m_level_sb_valueChanged
 * @param arg1
 */
void TerminalStripEditor::on_m_level_sb_valueChanged(int arg1)
{
	if (m_model)
	{
		const auto index_list = ui->m_table_widget->selectionModel()->selectedIndexes();

		for (auto index : index_list)
		{
			auto level_index = m_model->index(index.row(), TerminalStripModel::Level, index.parent());
			if (level_index.isValid())
			{
				m_model->setData(level_index, arg1);
			}
		}
	}
}

void TerminalStripEditor::on_m_type_cb_activated(int index)
{
	if (m_model)
	{
		const auto index_list = ui->m_table_widget->selectionModel()->selectedIndexes();

		for (auto model_index : index_list)
		{
			auto type_index = m_model->index(model_index.row(), TerminalStripModel::Type, model_index.parent());
			if (type_index.isValid())
			{
				ElementData::TerminalType override_type;
				switch (index) {
					case 0:
						override_type = ElementData::TTGeneric; break;
					case 1:
						override_type = ElementData::TTFuse; break;
					case 2:
						override_type = ElementData::TTSectional; break;
					case 3:
						override_type = ElementData::TTDiode; break;
					case 4:
						override_type = ElementData::TTGround; break;
					default:
						override_type = ElementData::TTGeneric; break;
				}
				m_model->setData(type_index, override_type);
			}
		}
	}
}


void TerminalStripEditor::on_m_function_cb_activated(int index)
{
	if (m_model)
	{
		const auto index_list = ui->m_table_widget->selectionModel()->selectedIndexes();

		for (auto model_index : index_list)
		{
			auto function_index = m_model->index(model_index.row(), TerminalStripModel::Function, model_index.parent());
			if (function_index.isValid())
			{
				ElementData::TerminalFunction override_function;
				switch (index) {
					case 0:
						override_function = ElementData::TFGeneric; break;
					case 1:
						override_function = ElementData::TFPhase; break;
					case 2:
						override_function = ElementData::TFNeutral; break;
					default:
						override_function = ElementData::TFGeneric; break;
				}
				m_model->setData(function_index, override_function);
			}
		}
	}
}


void TerminalStripEditor::on_m_led_cb_activated(int index)
{
	if (m_model)
	{
		const auto index_list = ui->m_table_widget->selectionModel()->selectedIndexes();

		for (auto model_index : index_list)
		{
			auto led_index = m_model->index(model_index.row(), TerminalStripModel::Led, model_index.parent());

			if (led_index.isValid()) {
				m_model->setData(led_index,
								 index == 0 ? false : true);
			}
		}
	}
}

/**
 * @brief TerminalStripEditor::on_m_bridge_terminals_pb_clicked
 */
void TerminalStripEditor::on_m_bridge_terminals_pb_clicked()
{
	if (m_current_strip)
	{
		int level_ = isSingleColumnSelected();
		if (level_ >= TerminalStripModel::Level0 &&
			level_ <= TerminalStripModel::Level3)
		{
			if(level_ == TerminalStripModel::Level0){level_ = 0;}
			else if(level_ == TerminalStripModel::Level1){level_ = 1;}
			else if(level_ == TerminalStripModel::Level2){level_ = 2;}
			else if(level_ == TerminalStripModel::Level3){level_ = 3;}

			const auto index_list = ui->m_table_widget->selectionModel()->selectedIndexes();
			const auto mrtd_vector = m_model->modelRealTerminalDataForIndex(index_list);
			QVector <QSharedPointer<RealTerminal>> match_vector;
			for (const auto &mrtd : mrtd_vector)
			{
				if (mrtd.level_ == level_) {
					match_vector.append(mrtd.real_terminal.toStrongRef());
				}
			}

			if (m_current_strip->isBridgeable(match_vector)) {
				m_project->undoStack()->push(new BridgeTerminalsCommand(m_current_strip, match_vector));
			}
		}
	}
}

/**
 * @brief TerminalStripEditor::on_m_unbridge_terminals_pb_clicked
 */
void TerminalStripEditor::on_m_unbridge_terminals_pb_clicked()
{
	if (m_current_strip)
	{
		int level_ = isSingleColumnSelected();
		if (level_ >= TerminalStripModel::Level0 &&
			level_ <= TerminalStripModel::Level3)
		{
			if(level_ == TerminalStripModel::Level0){level_ = 0;}
			else if(level_ == TerminalStripModel::Level1){level_ = 1;}
			else if(level_ == TerminalStripModel::Level2){level_ = 2;}
			else if(level_ == TerminalStripModel::Level3){level_ = 3;}

			const auto index_list = ui->m_table_widget->selectionModel()->selectedIndexes();
			const auto mrtd_vector = m_model->modelRealTerminalDataForIndex(index_list);
			QVector<QSharedPointer<RealTerminal>> match_vector;
			for (const auto &mrtd : mrtd_vector)
			{
				if (mrtd.level_ == level_
					&& mrtd.bridged_) {
					match_vector.append(mrtd.real_terminal.toStrongRef());
				}
			}
			m_project->undoStack()->push(new UnBridgeTerminalsCommand(m_current_strip, match_vector));
		}
	}
}


void TerminalStripEditor::on_m_bridge_color_cb_activated(const QColor &col)
{
	const auto data_vector = singleColumnData();
	const auto column_ = data_vector.first;
	if (column_ == TerminalStripModel::Level0 ||
		column_ == TerminalStripModel::Level1 ||
		column_ == TerminalStripModel::Level2 ||
		column_ == TerminalStripModel::Level3)
	{
		const auto level_ = TerminalStripModel::levelForColumn(column_);
		for (const auto &mrtd : data_vector.second)
		{
			if (mrtd.level_ == level_ && mrtd.bridged_) {
				auto bridge_ = mrtd.real_terminal.toStrongRef()->bridge();
				if (bridge_->color() != col)
					m_project->undoStack()->push(new ChangeTerminalStripColor(bridge_, col));
				break;
			}
		}
	}
}


void TerminalStripEditor::on_m_move_to_pb_clicked()
{
	if (!m_model || !m_current_strip || !m_current_strip->project()) {
		return;
	}

		//Get selected physical terminal
	const auto index_vector = m_model->modelPhysicalTerminalDataForIndex(ui->m_table_widget->selectionModel()->selectedIndexes());
	QVector<QSharedPointer<PhysicalTerminal>> phy_vector;
	for (const auto &index : index_vector)
	{
		const auto shared_{m_current_strip->physicalTerminal(index.uuid_)};
		if (shared_)
			phy_vector.append(shared_);
	}

	if (phy_vector.isEmpty()) {
		return;
	}

	const auto uuid_{ui->m_move_to_cb->currentData().toUuid()};
		//Uuid is null we move the selected terminal to indepandant terminal
	if (uuid_.isNull()) {
		m_current_strip->project()->undoStack()->push(new RemoveTerminalFromStripCommand(phy_vector, m_current_strip));
	}
	else
	{
		TerminalStrip *receiver_strip{nullptr};
		for (const auto &strip_ : m_current_strip->project()->terminalStrip())
		{
			if (strip_->uuid() == uuid_) {
				receiver_strip = strip_;
				break;
			}
		}

		if (!receiver_strip) {
			return;
		}

		m_current_strip->project()->undoStack()->push(new MoveTerminalCommand(phy_vector, m_current_strip, receiver_strip));
	}
}

