/*
	Copyright 2006-2016 The QElectroTech Team
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
#include "autonumberingdockwidget.h"
#include "qetapp.h"
#include "ui_autonumberingdockwidget.h"
#include "diagramview.h"
#include "diagram.h"
#include "titleblockproperties.h"
#include "numerotationcontext.h"

/**
 * @brief AutoNumberingDockWidget::AutoNumberingDockWidget
 * Constructor
 * @param parent : parent widget
 */
AutoNumberingDockWidget::AutoNumberingDockWidget(QWidget *parent, QETProject *project) :
	QDockWidget(parent),
	ui(new Ui::AutoNumberingDockWidget),
	project_(project)
{
	ui->setupUi(this);
}

/**
 * @brief AutoNumberingDockWidget::~AutoNumberingDockWidget
 * Destructor
 */
AutoNumberingDockWidget::~AutoNumberingDockWidget()
{
	this->disconnect();
	delete ui;
}

/**
 * @brief AutoNumberingDockWidget::clear
 * Remove all combo box values
 */
void AutoNumberingDockWidget::clear()
{
	ui->m_conductor_cb->clear();
	ui->m_element_cb->clear();
	ui->m_folio_cb->clear();
}

/**
 * @brief AutoNumberingDockWidget::setProject
 * @param project: project to be setted
 * @param projectview: projectview to be setted
 * assign Project and ProjectView, connect all signals and setContext
 */
void AutoNumberingDockWidget::setProject(QETProject *project, ProjectView *projectview) {

	project_ = project;
	projectview_ = projectview;

	//Conductor Signals
	connect(project_, SIGNAL(conductorAutoNumChanged()),this,SLOT(conductorAutoNumChanged()));
	connect (project_,SIGNAL(conductorAutoNumRemoved()), this,SLOT(conductorAutoNumChanged()));
	connect (project_,SIGNAL(conductorAutoNumAdded()),   this,SLOT(conductorAutoNumChanged()));
	connect(projectview_,SIGNAL(diagramActivated(DiagramView*)),this,SLOT(setConductorActive(DiagramView*)));

	//Element Signals
	connect (project_,SIGNAL(elementAutoNumRemoved()),   this,SLOT(elementAutoNumChanged()));
	connect (project_,SIGNAL(elementAutoNumAdded()),     this,SLOT(elementAutoNumChanged()));

	//Folio Signals
	connect (project_,SIGNAL(folioAutoNumRemoved()),     this,SLOT(folioAutoNumChanged()));
	connect (project_,SIGNAL(folioAutoNumAdded()),       this,SLOT(folioAutoNumChanged()));
	connect (this,
			 SIGNAL(folioAutoNumChanged(QString)),
			 &projectview_->currentDiagram()->diagram()->border_and_titleblock,
			 SLOT (slot_setAutoPageNum(QString)));
	connect(project_, SIGNAL(defaultTitleBlockPropertiesChanged()),this,SLOT(setActive()));

	//Conductor, Element and Folio Signals
	connect(projectview_,SIGNAL(projectClosed(ProjectView*)),this,SLOT(clear()));

	//Set Combobox Context
	setContext();
}

/**
 * @brief AutoNumberingDockWidget::setContext
 * Add all itens to comboboxes
 */
void AutoNumberingDockWidget::setContext() {

	this->clear();

	//Conductor Combobox
	ui->m_conductor_cb->addItem("");
	QList <QString> keys_conductor = project_->conductorAutoNum().keys();
	if (!keys_conductor.isEmpty()) {
		foreach (QString str, keys_conductor) { ui->m_conductor_cb-> addItem(str); }
	}

	//Element Combobox
	ui->m_element_cb->addItem("");
	QList <QString> keys_element = project_->elementAutoNum().keys();
	if (!keys_element.isEmpty()) {
		foreach (QString str, keys_element) {ui->m_element_cb -> addItem(str);}
	}

	//Folio Combobox
	ui->m_folio_cb->addItem("");
	QList <QString> keys_folio = project_->folioAutoNum().keys();
	if (!keys_folio.isEmpty()) {
		foreach (QString str, keys_folio) { ui->m_folio_cb -> addItem(str);}
	}

	this->setActive();
}

/**
 * @brief AutoNumberingDockWidget::setConductorActive
 * @param dv: activated diagramview
 */
void AutoNumberingDockWidget::setConductorActive(DiagramView* dv) {
	if (dv!=NULL) {
		QString conductor_autonum = dv->diagram()->conductorsAutonumName();
		int conductor_index = ui->m_conductor_cb->findText(conductor_autonum);
		ui->m_conductor_cb->setCurrentIndex(conductor_index);
	}
}

/**
 * @brief AutoNumberingDockWidget::setActive
 * Set current used autonumberings
 */
void AutoNumberingDockWidget::setActive() {

	if (projectview_!=NULL) {
		//Conductor
		QString conductor_autonum = projectview_->currentDiagram()->diagram()->conductorsAutonumName();
		int conductor_index = ui->m_conductor_cb->findText(conductor_autonum);
		ui->m_conductor_cb->setCurrentIndex(conductor_index);

		//Element
		QString current_element_autonum = project_->elementCurrentAutoNum();
		QString element_formula = project_->elementAutoNumFormula();
		QString active_element_autonum = project_->elementAutoNumHash().key(element_formula);
		int el_index = ui->m_element_cb->findText(active_element_autonum);
		ui->m_element_cb->setCurrentIndex(el_index);

		//Folio
		if (project_->defaultTitleBlockProperties().folio == "%autonum") {
			QString page_autonum = project_->defaultTitleBlockProperties().auto_page_num;
			int folio_index = ui->m_folio_cb->findText(page_autonum);
			ui->m_folio_cb->setCurrentIndex(folio_index);
		}
	}
}

/**
 * @brief AutoNumberingDockWidget::conductorAutoNumChanged
 * Add new or remove conductor auto num from combobox
 */
void AutoNumberingDockWidget::conductorAutoNumChanged() {

	ui->m_conductor_cb->clear();

	//Conductor Combobox
	ui->m_conductor_cb->addItem("");
	QList <QString> keys_conductor = project_->conductorAutoNum().keys();
	if (!keys_conductor.isEmpty()) {
		foreach (QString str, keys_conductor) { ui->m_conductor_cb-> addItem(str); }
	}
	setActive();
}

/**
 * @brief AutoNumberingDockWidget::on_m_conductor_cb_activated
 * @param unused
 * Set new conductor AutoNum
 */
void AutoNumberingDockWidget::on_m_conductor_cb_activated(int) {
	QString current_autonum = ui->m_conductor_cb->currentText();
	projectview_->currentDiagram()->diagram()->setConductorsAutonumName(current_autonum);
}

/**
 * @brief AutoNumberingDockWidget::elementAutoNumChanged
 * Add new or remove element auto num from combobox
 */
void AutoNumberingDockWidget::elementAutoNumChanged() {

	ui->m_element_cb->clear();

	//Element Combobox
	ui->m_element_cb->addItem("");
	QList <QString> keys_element = project_->elementAutoNum().keys();
	if (!keys_element.isEmpty()) {
		foreach (QString str, keys_element) {ui->m_element_cb -> addItem(str);}
	}
	setActive();
}

/**
 * @brief AutoNumberingDockWidget::on_m_element_cb_activated
 * @param unused
 * Set new element AutoNum
 */
void AutoNumberingDockWidget::on_m_element_cb_activated(int) {
	QString current_autonum = ui->m_element_cb->currentText();
	QString current_formula = project_->elementAutoNumFormula(current_autonum);
	if (current_autonum != "") {
		project_->setElementAutoNumCurrentFormula(current_formula, current_autonum);
	}
	else
		project_->setElementAutoNumCurrentFormula("","");
}

/**
 * @brief AutoNumberingDockWidget::folioAutoNumChanged
 * Add new or remove folio auto num from combobox
 */
void AutoNumberingDockWidget::folioAutoNumChanged() {

	ui->m_folio_cb->clear();

	//Folio Combobox
	ui->m_folio_cb->addItem("");
	QList <QString> keys_folio = project_->folioAutoNum().keys();
	if (!keys_folio.isEmpty()) {
		foreach (QString str, keys_folio) { ui->m_folio_cb -> addItem(str);}
	}
	setActive();
}

/**
 * @brief AutoNumberingDockWidget::on_m_folio_cb_activated
 * @param unused
 * Set new folio AutoNum
 */
void AutoNumberingDockWidget::on_m_folio_cb_activated(int) {
	QString current_autonum = ui->m_folio_cb->currentText();
	TitleBlockProperties ip = project_ -> defaultTitleBlockProperties();
	if (current_autonum != "") {
		ip.setAutoPageNum(current_autonum);
		ip.folio = "%autonum";
		project_->setDefaultTitleBlockProperties(ip);
	}
	else {
		ip.folio = "%id/%total";
		project_->setDefaultTitleBlockProperties(ip);
	}
		emit(folioAutoNumChanged(current_autonum));
}
