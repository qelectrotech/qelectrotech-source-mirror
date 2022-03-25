/*
		Copyright 2006-2022 The QElectroTech Team
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
void FreeTerminalEditor::reload() {
	m_model->clear();
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
