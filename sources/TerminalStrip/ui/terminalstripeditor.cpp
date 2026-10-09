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

#include <QApplication>
#include <QClipboard>
#include <QKeyEvent>

#include <algorithm>
#include <limits>

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

		//Copy, paste and delete of the cells with the keyboard
	ui->m_table_widget->installEventFilter(this);

		//Setup the bridge color
	ui->m_bridge_color_cb->setColors(TerminalStripBridge::bridgeColor().toList());

		//Call for update the state of child widgets
	selectionChanged();

		//Go the diagram of double clicked terminal
	connect(ui->m_table_widget, &QAbstractItemView::doubleClicked, this, [this](const QModelIndex &index)
	{
		if (m_model->columnTypeForIndex(index) == TerminalStripModel::XRef)
		{
			const auto mrtd = m_model->modelRealTerminalDataForIndex(index);
			if (mrtd.element_) {
				QetGraphicsItem::showItem(mrtd.element_);
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

void TerminalStripEditor::setProject(QETProject *project)
{
    m_project = project;
    setCurrentStrip(nullptr);
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
        disconnect(m_current_strip, &QObject::destroyed, this, &TerminalStripEditor::clear);
	}

	ui->m_move_to_cb->clear();

    if (!strip_) {
        clear();
	}
    else {
		ui->m_installation_le ->setText(strip_->installation());
		ui->m_location_le     ->setText(strip_->location());
		ui->m_name_le         ->setText(strip_->name());
		ui->m_comment_le      ->setText(strip_->comment());
		ui->m_description_te  ->setPlainText(strip_->description());
		ui->m_move_to_cb->addItem(tr("Independent terminals"), QUuid());

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
        connect(m_current_strip, &QObject::destroyed, this, &TerminalStripEditor::clear);
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
		m_project->undoStack()->beginMacro(tr("Modify terminal strip properties"));

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
					//The cable data are only stored when they are used
					const auto set_info = [&current_data](const QString &key, const QString &value) {
						if (!value.isEmpty() || current_data.m_informations.contains(key)) {
							current_data.m_informations.addValue(key, value);
						}
					};
					set_info(RealTerminal::cableInfoKey(), data_.cable_);
					set_info(RealTerminal::cableWireInfoKey(), data_.cable_wire);
					set_info(RealTerminal::shieldInfoKey(), data_.shield_ ? QStringLiteral("true") : QString());

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

void TerminalStripEditor::clear()
{
    ui->m_installation_le ->clear();
    ui->m_location_le     ->clear();
    ui->m_name_le         ->clear();
    ui->m_comment_le      ->clear();
    ui->m_description_te  ->clear();
    m_current_strip.clear();

    ui->m_table_widget->setModel(nullptr);
    if (m_model) {
        m_model->deleteLater();
        m_model = nullptr;
    }
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
		ui->m_cable_le          ->setDisabled(true);
		ui->m_cable_apply_pb    ->setDisabled(true);

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
		ui->m_cable_le      ->setDisabled(true);
		ui->m_cable_apply_pb->setDisabled(true);
	} else {
		ui->m_type_cb     ->setEnabled(true);
		ui->m_function_cb ->setEnabled(true);
		ui->m_led_cb      ->setEnabled(true);
		ui->m_cable_le      ->setEnabled(true);
		ui->m_cable_apply_pb->setEnabled(true);
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

namespace {
/**
 * @return true if the cell at @a index contain a free text
 * (label, cable or wire of the cable), so a text can be pasted in it.
 */
bool cellAcceptText(const QModelIndex &index)
{
	const auto column_ = TerminalStripModel::columnTypeForIndex(index);
	return column_ == TerminalStripModel::Label
			|| column_ == TerminalStripModel::Cable
			|| column_ == TerminalStripModel::CableWire;
}
}

/**
 * @brief TerminalStripEditor::eventFilter
 * Manage the shortcuts copy, paste and delete in the table
 * @param watched
 * @param event
 * @return
 */
bool TerminalStripEditor::eventFilter(QObject *watched, QEvent *event)
{
	if (watched == ui->m_table_widget &&
		event->type() == QEvent::KeyPress &&
		m_model)
	{
		const auto key_event = static_cast<QKeyEvent *>(event);
		if (key_event->matches(QKeySequence::Copy)) {
			copySelectionToClipboard();
			return true;
		}
		if (key_event->matches(QKeySequence::Paste)) {
			pasteFromClipboard();
			return true;
		}
		if (key_event->matches(QKeySequence::Delete)) {
			clearSelectedTexts();
			return true;
		}
	}

	return QWidget::eventFilter(watched, event);
}

/**
 * @brief TerminalStripEditor::copySelectionToClipboard
 * Copy the selected cells to the clipboard, as a text where the columns
 * are separated by a tabulation and the rows by a new line
 * (the format used by the spreadsheets).
 */
void TerminalStripEditor::copySelectionToClipboard()
{
	if (!m_model || !ui->m_table_widget->selectionModel()) {
		return;
	}

	const auto selection = ui->m_table_widget->selectionModel()->selectedIndexes();
	if (selection.isEmpty()) {
		return;
	}

	int top{std::numeric_limits<int>::max()}, left{std::numeric_limits<int>::max()}, bottom{-1}, right{-1};
	QHash<qint64, QString> texts;
	for (const auto &index : selection)
	{
		top    = std::min(top,    index.row());
		left   = std::min(left,   index.column());
		bottom = std::max(bottom, index.row());
		right  = std::max(right,  index.column());
		texts.insert((qint64(index.row()) << 32) | qint64(index.column()),
					 index.data(Qt::DisplayRole).toString());
	}

	QStringList lines;
	for (auto row = top ; row <= bottom ; ++row)
	{
		QStringList cells;
		for (auto column = left ; column <= right ; ++column) {
			cells << texts.value((qint64(row) << 32) | qint64(column));
		}
		lines << cells.join(QLatin1Char('\t'));
	}

	QApplication::clipboard()->setText(lines.join(QLatin1Char('\n')));
}

/**
 * @brief TerminalStripEditor::pasteFromClipboard
 * Paste the text of the clipboard in the table.
 * - If the text is only one value and several cells are selected,
 *   the value is set to every selected cell.
 * - Otherwise the text is read like a table (tabulation between the columns
 *   and new line between the rows) and written from the top left selected cell.
 * Only the label, cable and cable wire cells are modified.
 */
void TerminalStripEditor::pasteFromClipboard()
{
	if (!m_model || !ui->m_table_widget->selectionModel()) {
		return;
	}

	auto text = QApplication::clipboard()->text();
	if (text.isEmpty()) {
		return;
	}
	text.replace(QStringLiteral("\r\n"), QStringLiteral("\n"));
	text.replace(QLatin1Char('\r'), QLatin1Char('\n'));

	auto lines = text.split(QLatin1Char('\n'));
		//A spreadsheet end the last row with a new line
	if (lines.size() > 1 && lines.last().isEmpty()) {
		lines.removeLast();
	}

	const auto selection = ui->m_table_widget->selectionModel()->selectedIndexes();

		//One value, several cells : set the value to all of them
	if (lines.size() == 1 &&
		!lines.first().contains(QLatin1Char('\t')) &&
		selection.size() > 1)
	{
		for (const auto &index : selection)
		{
			if (cellAcceptText(index)) {
				m_model->setData(index, lines.first(), Qt::EditRole);
			}
		}
		return;
	}

		//Else write the text like a table, from the top left selected cell
	int first_row{-1}, first_column{-1};
	if (selection.isEmpty())
	{
		const auto current_ = ui->m_table_widget->currentIndex();
		if (!current_.isValid()) {
			return;
		}
		first_row = current_.row();
		first_column = current_.column();
	}
	else
	{
		first_row = std::numeric_limits<int>::max();
		first_column = std::numeric_limits<int>::max();
		for (const auto &index : selection) {
			first_row    = std::min(first_row,    index.row());
			first_column = std::min(first_column, index.column());
		}
	}

	for (auto i = 0 ; i < lines.size() ; ++i)
	{
		const auto row = first_row + i;
		if (row >= m_model->rowCount()) {
			break;
		}

		const auto cells = lines.at(i).split(QLatin1Char('\t'));
		for (auto j = 0 ; j < cells.size() ; ++j)
		{
			const auto column = first_column + j;
			if (column >= m_model->columnCount()) {
				break;
			}

			const auto index = m_model->index(row, column);
			if (cellAcceptText(index)) {
				m_model->setData(index, cells.at(j), Qt::EditRole);
			}
		}
	}
}

/**
 * @brief TerminalStripEditor::clearSelectedTexts
 * Erase the text of the selected cells who accept a text.
 */
void TerminalStripEditor::clearSelectedTexts()
{
	if (!m_model || !ui->m_table_widget->selectionModel()) {
		return;
	}

	const auto selection = ui->m_table_widget->selectionModel()->selectedIndexes();
	for (const auto &index : selection)
	{
		if (cellAcceptText(index)) {
			m_model->setData(index, QString(), Qt::EditRole);
		}
	}
}

/**
 * @brief TerminalStripEditor::on_m_cable_apply_pb_clicked
 * Set the cable name written in the line edit
 * to every selected terminal.
 */
void TerminalStripEditor::on_m_cable_apply_pb_clicked()
{
	if (m_model)
	{
		const auto index_list = ui->m_table_widget->selectionModel()->selectedIndexes();
		const auto cable_name = ui->m_cable_le->text();

		for (auto model_index : index_list)
		{
			const auto cable_index = m_model->index(model_index.row(), TerminalStripModel::Cable, model_index.parent());
			if (cable_index.isValid()) {
				m_model->setData(cable_index, cable_name, Qt::EditRole);
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

