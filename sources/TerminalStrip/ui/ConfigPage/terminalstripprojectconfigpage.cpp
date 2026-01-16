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
#include "terminalstripprojectconfigpage.h"
#include "../../../qeticons.h"
#include "../terminalstriplayouteditor.h"
#include "../../../qetproject.h"

#include <QVBoxLayout>

TerminalStripProjectConfigPage::TerminalStripProjectConfigPage(QETProject *project,
															   QWidget *parent) :
	ProjectConfigPage { project, parent }
{
	initWidgets();
}

QString TerminalStripProjectConfigPage::title() const {
	return tr("Plan de bornes");
}

QIcon TerminalStripProjectConfigPage::icon() const {
	return QET::Icons::TerminalStrip;
}

void TerminalStripProjectConfigPage::initWidgets()
{
	m_layout_editor = new TerminalStripLayoutEditor{ project()->projectPropertiesHandler().terminalStripLayoutHandler().defaultLayout(),
													this };

	auto v_layout = new QVBoxLayout { this };
	v_layout->addWidget(m_layout_editor);
	setLayout(v_layout);
}
