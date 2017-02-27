/*
	Copyright 2006-2017 The QElectroTech Team
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
#include "diagrampropertiesdialog.h"
#include "borderpropertieswidget.h"
#include "titleblockpropertieswidget.h"
#include "conductorpropertieswidget.h"
#include "diagramcommands.h"
#include "projectpropertiesdialog.h"
#include "diagram.h"

/**
 * @brief DiagramPropertiesDialog::DiagramPropertiesDialog
 * Deafult constructor
 * @param diagram : diagram to edit properties
 * @param parent : parent widget
 */
DiagramPropertiesDialog::DiagramPropertiesDialog(Diagram *diagram, QWidget *parent) :
	QDialog (parent),
	m_diagram (diagram)
{
	bool diagram_is_read_only = diagram -> isReadOnly();

	// Get some properties of edited diagram
	TitleBlockProperties titleblock = diagram -> border_and_titleblock.exportTitleBlock();
	BorderProperties     border     = diagram -> border_and_titleblock.exportBorder();
	ConductorProperties  conductors = diagram -> defaultConductorProperties;

	setWindowModality(Qt::WindowModal);
#ifdef Q_OS_MAC
	setWindowFlags(Qt::Sheet);
#endif

    setWindowTitle(tr("Propriétés du folio", "window title"));

	//Border widget
	BorderPropertiesWidget *border_infos = new BorderPropertiesWidget(border, this);
	border_infos -> setReadOnly(diagram_is_read_only);

	//Title block widget
	TitleBlockPropertiesWidget  *titleblock_infos;

	if (QETProject *parent_project = diagram -> project())
		titleblock_infos  = new TitleBlockPropertiesWidget(parent_project -> embeddedTitleBlockTemplatesCollection(), titleblock, false, diagram->project(),  this);
	else
		titleblock_infos = new TitleBlockPropertiesWidget(titleblock, false, diagram->project(), this);

	titleblock_infos -> setReadOnly(diagram_is_read_only);
	connect(titleblock_infos,SIGNAL(openAutoNumFolioEditor(QString)),this,SLOT(editAutoFolioNum()));
	//titleblock_infos->setMinimumSize(590,480); //Minimum Size needed for correct display

		//Conductor widget
	m_cpw = new ConductorPropertiesWidget(conductors, this);
	m_cpw -> setReadOnly(diagram_is_read_only);
	
	QComboBox *autonum_combobox = m_cpw->autonumComboBox();
	autonum_combobox->addItems(diagram->project()->conductorAutoNum().keys());
	autonum_combobox->setCurrentIndex(autonum_combobox->findText(diagram->conductorsAutonumName()));
	
	connect(m_cpw->editAutonumPushButton(), &QPushButton::clicked, this, &DiagramPropertiesDialog::editAutonum);

		// Buttons
	QDialogButtonBox boutons(diagram_is_read_only ? QDialogButtonBox::Ok : QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
	connect(&boutons, SIGNAL(accepted()), this, SLOT(accept()));
	connect(&boutons, SIGNAL(rejected()), this, SLOT(reject()));
	
	QGridLayout *glayout = new QGridLayout;
	glayout->addWidget(border_infos,0,0);
	glayout->addWidget(titleblock_infos, 1, 0);
	glayout->addWidget(m_cpw, 0, 1, 0, 1);
	
	QVBoxLayout vlayout(this);
	vlayout.addLayout(glayout);
	vlayout.addWidget(&boutons);

	// if dialog is accepted
	if (this -> exec() == QDialog::Accepted && !diagram_is_read_only)
	{
		TitleBlockProperties new_titleblock = titleblock_infos  -> properties();
		BorderProperties     new_border     = border_infos -> properties();
		ConductorProperties  new_conductors = m_cpw -> properties();

		// Title block have change
		if (new_titleblock != titleblock) {
			diagram -> undoStack().push(new ChangeTitleBlockCommand(diagram, titleblock, new_titleblock));
		}

		// Border have change
		if (new_border != border) {
			diagram -> undoStack().push(new ChangeBorderCommand(diagram, border, new_border));
		}

		// Conducteur have change
		if (new_conductors != conductors) {
			/// TODO implement an undo command to allow the user to undo/redo this action
			diagram -> defaultConductorProperties = new_conductors;
		}

			// Conductor autonum name
		if (autonum_combobox->currentText() != diagram->conductorsAutonumName())
		{
			diagram->setConductorsAutonumName (autonum_combobox->currentText());
			diagram->project()->conductorAutoNumChanged();
		}
	}
}

/**
 * @brief DiagramPropertiesDialog::diagramPropertiesDialog
 * Static method to get a DiagramPropertiesDialog.
 * @param diagram : diagram to edit properties
 * @param parent : parent widget
 */
void DiagramPropertiesDialog::diagramPropertiesDialog(Diagram *diagram, QWidget *parent) {
	DiagramPropertiesDialog dialog(diagram, parent);
}

/**
 * @brief DiagramPropertiesDialog::editAutonum
 * Open conductor autonum editor
 */
void DiagramPropertiesDialog::editAutonum()
{
	ProjectPropertiesDialog ppd (m_diagram->project(), this);
	ppd.setCurrentPage(ProjectPropertiesDialog::Autonum);
	ppd.exec();
	m_cpw->autonumComboBox()->clear();
	m_cpw->autonumComboBox()->addItems(m_diagram->project()->conductorAutoNum().keys());
}

/**
 * @brief DiagramPropertiesDialog::editAutonum
 * Open folio autonum editor
 */
void DiagramPropertiesDialog::editAutoFolioNum () {
    ProjectPropertiesDialog ppd (m_diagram->project(), this);
    ppd.setCurrentPage(ProjectPropertiesDialog::Autonum);
    ppd.changeToFolio();
    ppd.exec();
}
