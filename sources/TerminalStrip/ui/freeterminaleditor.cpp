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
#include "freeterminaleditor.h"
#include "ui_freeterminaleditor.h"

#include "../undocommand/changeelementdatacommand.h"
#include "../../diagram.h"
#include "../../elementprovider.h"
#include "freeterminalmodel.h"
#include "../terminalstrip.h"
#include "../UndoCommand/addterminaltostripcommand.h"

/**
 * @brief FreeTerminalEditor::FreeTerminalEditor
 * @param project
 * @param parent
 */
FreeTerminalEditor::FreeTerminalEditor(QETProject *project, QWidget *parent) :
	QWidget(parent),
	ui(new Ui::FreeTerminalEditor),
	m_project(project)
{
	ui->setupUi(this);
	ui->m_table_view->setItemDelegate(new FreeTerminalModelDelegate(ui->m_table_view));

	m_model = new FreeTerminalModel(m_project, this);
	ui->m_table_view->setModel(m_model);
	ui->m_table_view->setCurrentIndex(m_model->index(0,0));

		//Disabled the move if the table is currently edited (yellow cell)
	connect(m_model, &FreeTerminalModel::dataChanged, this, [=] {
		this->setDisabledMove();
	});

	connect(ui->m_table_view, &QAbstractItemView::doubleClicked, this, [=](const QModelIndex &index)
	{
		if (m_model->columnTypeForIndex(index) == FreeTerminalModel::XRef)
		{
			auto mrtd = m_model->dataAtRow(index.row());
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
 * @brief FreeTerminalEditor::~FreeTerminalEditor
 */
FreeTerminalEditor::~FreeTerminalEditor()
{
	delete ui;
}

/**
 * @brief FreeTerminalEditor::reload
 * Reload the editor to be up to date with
 * the current state of the project.
 * Every not applied change will be lost.
 */
void FreeTerminalEditor::reload()
{
	m_model->clear();
	ui->m_move_in_cb->clear();

	if (m_project)
	{
		const auto strip_vector = m_project->terminalStrip();
		for (const auto &strip : strip_vector)
		{
			QString str(strip->installation() + " " + strip->location() + " " + strip->name());
			ui->m_move_in_cb->addItem(str, strip->uuid());
		}
		setDisabledMove(false);
	}
}

/**
 * @brief FreeTerminalEditor::apply
 * Applu current edited values
 */
void FreeTerminalEditor::apply()
{
	const auto modified_data = m_model->modifiedModelRealTerminalData();
	if (modified_data.size())
	{
		m_project->undoStack()->beginMacro(tr("Modifier des propriétés de borniers"));

		for (const auto &data_ : modified_data)
		{
			if (auto element_ = data_.element_)
			{
				auto current_data = element_->elementData();
				current_data.setTerminalType(data_.type_);
				current_data.setTerminalFunction(data_.function_);
				current_data.setTerminalLED(data_.led_);
				current_data.m_informations.addValue(QStringLiteral("label"), data_.label_);

				if (element_->elementData() != current_data) {
					m_project->undoStack()->push(new ChangeElementDataCommand(element_, current_data));
				}
			}
		}

		m_project->undoStack()->endMacro();
	}

	reload();
}

void FreeTerminalEditor::on_m_type_cb_activated(int index)
{
	if (m_model)
	{
		const auto index_list = ui->m_table_view->selectionModel()->selectedIndexes();

		for (auto model_index : index_list)
		{
			auto type_index = m_model->index(model_index.row(), FreeTerminalModel::Type, model_index.parent());
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


void FreeTerminalEditor::on_m_function_cb_activated(int index)
{
	if (m_model)
	{
		const auto index_list = ui->m_table_view->selectionModel()->selectedIndexes();

		for (auto model_index : index_list)
		{
			auto function_index = m_model->index(model_index.row(), FreeTerminalModel::Function, model_index.parent());
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


void FreeTerminalEditor::on_m_led_cb_activated(int index)
{
	if (m_model)
	{
		const auto index_list = ui->m_table_view->selectionModel()->selectedIndexes();

		for (auto model_index : index_list)
		{
			auto led_index = m_model->index(model_index.row(), FreeTerminalModel::Led, model_index.parent());

			if (led_index.isValid()) {
				m_model->setData(led_index,
								 index == 0 ? false : true);
			}
		}
	}
}


void FreeTerminalEditor::on_m_move_pb_clicked()
{
		//Get the selected real terminal
	const auto index_list = ui->m_table_view->selectionModel()->selectedIndexes();
	const auto real_t_vector = m_model->realTerminalForIndex(index_list);
	if (real_t_vector.isEmpty()) {
		return;
	}

		//Get the terminal strip who receive the real terminal
	const auto strip_uuid = ui->m_move_in_cb->currentData().toUuid();
	TerminalStrip *terminal_strip{nullptr};
	for (const auto &strip : m_project->terminalStrip()) {
		if (strip->uuid() == strip_uuid) {
			terminal_strip = strip;
			break;
		}
	}

	if (!terminal_strip) {
		return;
	}

	m_project->undoStack()->push(new AddTerminalToStripCommand(real_t_vector, terminal_strip));

	reload();
}

void FreeTerminalEditor::setDisabledMove(bool b)
{
	ui->m_move_label->setDisabled(b);
	ui->m_move_in_cb->setDisabled(b);
	ui->m_move_pb->setDisabled(b);
}

