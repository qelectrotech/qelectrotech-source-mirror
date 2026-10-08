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
#include "diagrampropertiesdialog.h"

#include "../diagram.h"
#include "../diagramcommands.h"
#include "../qetgraphicsitem/conductor.h"
#include "../qetgraphicsitem/conductortextitem.h"
#include "../undocommand/changetitleblockcommand.h"
#include "borderpropertieswidget.h"
#include "conductorpropertieswidget.h"
#include "projectpropertiesdialog.h"
#include "titleblockpropertieswidget.h"

/**
	@brief DiagramPropertiesDialog::DiagramPropertiesDialog
	Default constructor
	@param diagram : diagram to edit properties
	@param parent : parent widget
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
#ifdef Q_OS_MACOS
	setWindowFlags(Qt::Sheet);
#endif

	setWindowTitle(tr("Propriétés du folio", "window title"));

	//Border widget
	BorderPropertiesWidget *border_infos = new BorderPropertiesWidget(border, this);
	border_infos -> setReadOnly(diagram_is_read_only);

	//Cabinet layout
	QGroupBox *cabinet_layout_infos = new QGroupBox(tr("Vue de disposition"), this);
	cabinet_layout_infos->setCheckable(true);
	cabinet_layout_infos->setChecked(diagram->cabinetLayoutEnabled());
	cabinet_layout_infos->setEnabled(!diagram_is_read_only);

	auto *cabinet_layout_px_sb = new QDoubleSpinBox(cabinet_layout_infos);
	cabinet_layout_px_sb->setDecimals(4);
	cabinet_layout_px_sb->setRange(0.0001, 100000.0);
	cabinet_layout_px_sb->setSuffix(tr(" px"));
	cabinet_layout_px_sb->setMaximumWidth(100);
	cabinet_layout_px_sb->setValue(diagram->cabinetLayoutScale());

	auto *cabinet_layout_mm_sb = new QDoubleSpinBox(cabinet_layout_infos);
	cabinet_layout_mm_sb->setDecimals(4);
	cabinet_layout_mm_sb->setRange(0.0001, 100000.0);
	cabinet_layout_mm_sb->setSuffix(tr(" mm"));
	cabinet_layout_mm_sb->setMaximumWidth(100);
	cabinet_layout_mm_sb->setValue(1.0);

	auto *cabinet_layout_scale_row = new QWidget(cabinet_layout_infos);
	auto *cabinet_layout_scale_layout = new QHBoxLayout(cabinet_layout_scale_row);
	cabinet_layout_scale_layout->setContentsMargins(0, 0, 0, 0);
	cabinet_layout_scale_layout->setSpacing(4);
	cabinet_layout_scale_layout->addWidget(cabinet_layout_px_sb);
	cabinet_layout_scale_layout->addWidget(new QLabel(tr("pour"), cabinet_layout_scale_row));
	cabinet_layout_scale_layout->addWidget(cabinet_layout_mm_sb);
	cabinet_layout_scale_layout->addStretch();

	auto *cabinet_layout_view_cb = new QComboBox(cabinet_layout_infos);
	cabinet_layout_view_cb->addItem(tr("Face"), Diagram::CabinetLayoutFront);
	cabinet_layout_view_cb->addItem(tr("Côté"), Diagram::CabinetLayoutSide);
	cabinet_layout_view_cb->setSizeAdjustPolicy(QComboBox::AdjustToContents);
	cabinet_layout_view_cb->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Fixed);
	cabinet_layout_view_cb->setCurrentIndex(
		diagram->cabinetLayoutView() == Diagram::CabinetLayoutSide ? 1 : 0);

	auto *cabinet_layout_form = new QFormLayout;
	//cabinet_layout_form->setLabelAlignment(Qt::AlignLeft);
	cabinet_layout_form->addRow(tr("Échelle :"), cabinet_layout_scale_row);
	cabinet_layout_form->addRow(tr("Vue :"), cabinet_layout_view_cb);
	cabinet_layout_infos->setLayout(cabinet_layout_form);

	//Title block widget
	TitleBlockPropertiesWidget  *titleblock_infos;

	if (QETProject *parent_project = diagram -> project())
		titleblock_infos = new TitleBlockPropertiesWidget(parent_project -> embeddedTitleBlockTemplatesCollection(), titleblock, false, diagram->project(), this);
	else
		titleblock_infos = new TitleBlockPropertiesWidget(titleblock, false, diagram->project(), this);

	titleblock_infos -> setReadOnly(diagram_is_read_only);
	connect(titleblock_infos, &TitleBlockPropertiesWidget::openAutoNumFolioEditor, this, &DiagramPropertiesDialog::editAutoFolioNum);
	border_infos -> setTitleBlockSize(titleblock_infos -> currentTitleBlockHeight(),
									  titleblock_infos -> currentTitleBlockEdge());
	connect(titleblock_infos, &TitleBlockPropertiesWidget::titleBlockSizeChanged,
			border_infos, &BorderPropertiesWidget::setTitleBlockSize);
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
	connect(&boutons, &QDialogButtonBox::accepted, this, &DiagramPropertiesDialog::accept);
	connect(&boutons, &QDialogButtonBox::rejected, this, &DiagramPropertiesDialog::reject);

	QGridLayout *glayout = new QGridLayout;
	glayout->addWidget(border_infos,0,0);
	glayout->addWidget(cabinet_layout_infos, 1, 0);
	glayout->addWidget(titleblock_infos, 2, 0);
	glayout->addWidget(m_cpw, 0, 1, 0, 1);

	QVBoxLayout vlayout(this);
	vlayout.addLayout(glayout);
	vlayout.addWidget(&boutons);

	QET::trackDialogGeometry(this);

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
#if TODO_LIST
#pragma message("@TODO implement an undo command to allow the user to undo/redo this action")
#endif
			/// TODO implement an undo command to allow the user to undo/redo this action
			diagram -> defaultConductorProperties = new_conductors;

				// "One text per potential" changed: show or hide the
				// conductor texts now, not at the next edit or reload.
			if (new_conductors.m_one_text_per_folio != conductors.m_one_text_per_folio)
			{
				const QList<Conductor *> conductor_list = diagram -> conductors();
				for (Conductor *c : conductor_list)
					c -> updateTextVisibility();
				for (Conductor *c : conductor_list)
					c -> calculateTextItemPosition();
			}
		}

			// Conductor autonum name
		if (autonum_combobox->currentText() != diagram->conductorsAutonumName())
		{
			diagram->setConductorsAutonumName (autonum_combobox->currentText());
			diagram->project()->conductorAutoNumChanged();
		}

		//Cabinet layout
		//No dedicated undo command yet, same as the conductor
		diagram->setCabinetLayoutEnabled(cabinet_layout_infos->isChecked());
		if (cabinet_layout_mm_sb->value() > 0.0)
			diagram->setCabinetLayoutScale(cabinet_layout_px_sb->value() / cabinet_layout_mm_sb->value());
		diagram->setCabinetLayoutView(
			cabinet_layout_view_cb->currentData().toInt() == Diagram::CabinetLayoutSide
				? Diagram::CabinetLayoutSide : Diagram::CabinetLayoutFront);
	}
}

/**
	@brief DiagramPropertiesDialog::diagramPropertiesDialog
	Static method to get a DiagramPropertiesDialog.
	@param diagram : diagram to edit properties
	@param parent : parent widget
*/
void DiagramPropertiesDialog::diagramPropertiesDialog(Diagram *diagram, QWidget *parent) {
	DiagramPropertiesDialog dialog(diagram, parent);
}

/**
	@brief DiagramPropertiesDialog::editAutonum
	Open conductor autonum editor
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
	@brief DiagramPropertiesDialog::editAutonum
	Open folio autonum editor
*/
void DiagramPropertiesDialog::editAutoFolioNum ()
{
	ProjectPropertiesDialog ppd (m_diagram->project(), this);
	ppd.setCurrentPage(ProjectPropertiesDialog::Autonum);
	ppd.changeToFolio();
	ppd.exec();
}
