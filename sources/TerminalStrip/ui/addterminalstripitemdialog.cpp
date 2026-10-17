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
#include "addterminalstripitemdialog.h"
#include "ui_addterminalstripitemdialog.h"
#include "../../qetproject.h"
#include "../../undocommand/addgraphicsobjectcommand.h"
#include "../terminalstrip.h"
#include "../GraphicsItem/terminalstripitem.h"
#include "../../diagram.h"

#include "../../qet.h"

#include <algorithm>
void AddTerminalStripItemDialog::openDialog(Diagram *diagram, QWidget *parent)
{
	AddTerminalStripItemDialog d(diagram->project(), parent);
	if (d.exec())
	{
		const auto strip_{d.selectedTerminalStrip()};
		if (strip_)
		{
			auto item_ = new TerminalStripItem(strip_);
			item_->setDrawStyle(static_cast<TerminalStripLayoutPattern::DrawStyle>(d.selectedDrawStyle()));
			diagram->addItem(item_);
			item_->setPos(50, 50);

			diagram->project()->undoStack()->push(new AddGraphicsObjectCommand(item_, diagram, QPointF{50, 50}));
		}
	}
}

AddTerminalStripItemDialog::AddTerminalStripItemDialog(QETProject *project, QWidget *parent) :
	QDialog{parent},
	m_project{project},
	ui{new Ui::AddTerminalStripItemDialog}
{
	ui->setupUi(this);
	QET::trackDialogGeometry(this);
	fillComboBox();
}

AddTerminalStripItemDialog::~AddTerminalStripItemDialog()
{
	delete ui;
}

/**
 * @brief AddTerminalStripItemDialog::selectedTerminalStrip
 * @return The selected terminal strip or nullptr if no one is selected
 * or error encounted.
 */
TerminalStrip *AddTerminalStripItemDialog::selectedTerminalStrip() const
{
	if (m_project)
	{
		const QUuid uuid_{ui->m_terminal_strip_cb->currentData().toUuid()};
		for (auto &&strip_ : m_project->terminalStrip())
		{
			if (strip_->uuid() == uuid_) {
				return strip_;
			}
		}
	}

	return nullptr;
}

/**
 * @brief AddTerminalStripItemDialog::selectedDrawStyle
 * @return The drawing style chosen for the new item,
 * as an int value of TerminalStripLayoutPattern::DrawStyle.
 * The index of the combo box is the value of the enum.
 */
int AddTerminalStripItemDialog::selectedDrawStyle() const {
	return std::clamp(ui->m_draw_style_cb->currentIndex(), 0, 2);
}

void AddTerminalStripItemDialog::fillComboBox()
{
	if (m_project)
	{
		for (auto &&strip_ : m_project->terminalStrip())
		{
			const auto text{strip_->installation() + " " + strip_->location() + " " + strip_->name()};
			ui->m_terminal_strip_cb->addItem(text, strip_->uuid());
		}
	}
}
