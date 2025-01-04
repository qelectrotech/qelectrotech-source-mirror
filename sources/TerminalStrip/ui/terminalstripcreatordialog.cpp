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
#include "terminalstripcreatordialog.h"
#include "ui_terminalstripcreatordialog.h"
#include "../terminalstrip.h"
#include "../../qetproject.h"

/**
 * @brief TerminalStripCreatorDialog::TerminalStripCreatorDialog
 * @param project : Project to add a new terminal strip
 * @param parent : parent widget
 */
TerminalStripCreatorDialog::TerminalStripCreatorDialog(QETProject *project, QWidget *parent) :
	QDialog(parent),
	ui(new Ui::TerminalStripCreatorDialog),
	m_project(project)
{
	ui->setupUi(this);
}

/**
 * @brief TerminalStripCreatorDialog::~TerminalStripCreatorDialog
 */
TerminalStripCreatorDialog::~TerminalStripCreatorDialog() {
	delete ui;
}

/**
 * @brief TerminalStripCreatorDialog::setInstallation
 * Set the installation field string
 * @param installation
 */
void TerminalStripCreatorDialog::setInstallation(const QString &installation) {
	ui->m_installation_le->setText(installation);
	setCursorToEmptyLine();
}

/**
 * @brief TerminalStripCreatorDialog::setLocation
 * Set the location field string
 * @param location
 */
void TerminalStripCreatorDialog::setLocation(const QString &location) {
	ui->m_location_le->setText(location);
	setCursorToEmptyLine();
}

/**
 * @brief TerminalStripCreatorDialog::generatedTerminalStrip
 * @return A new terminal Strip according to the value set by user.
 * The terminal strip is already added to the terminalStrip list of the project
 * so it's ready to use.
 */
TerminalStrip *TerminalStripCreatorDialog::generatedTerminalStrip() const
{
	QString installation_ = ui->m_installation_le->text();
	QString location_     = ui->m_location_le->text();
	QString name_         = ui->m_name_le->text();

	if (installation_.isEmpty()) {
		installation_ = QStringLiteral("=INST"); }
	if (location_.isEmpty()) {
		location_ = QStringLiteral("+LOC"); }
	if (name_.isEmpty()) {
		name_ = QStringLiteral("X"); }

	auto strip = m_project->newTerminalStrip(installation_,
											 location_,
											 name_);
	strip->setComment(ui->m_comment_le->text());
	strip->setDescription(ui->m_description_te->toPlainText());

	return strip;
}

/**
 * @brief TerminalStripCreatorDialog::setCursorToEmptyLine
 * Set the cursor to the first empty field.
 * It's useful when user creates a new terminal strip
 * with some value prefilled, to increase productivity.
 */
void TerminalStripCreatorDialog::setCursorToEmptyLine()
{
	if (ui->m_installation_le->text().isEmpty()) {
		return;
	}
	if (ui->m_location_le->text().isEmpty()) {
		ui->m_location_le->setFocus();
		return;
	}
	ui->m_name_le->setFocus();
}
