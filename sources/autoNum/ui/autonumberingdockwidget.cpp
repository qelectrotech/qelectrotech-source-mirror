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
	m_project(project)
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

	m_project = project;
	m_project_view = projectview;

	//Conductor Signals
	connect(m_project, SIGNAL(conductorAutoNumChanged()),this,SLOT(conductorAutoNumChanged()));
	connect (m_project,SIGNAL(conductorAutoNumRemoved()), this,SLOT(conductorAutoNumChanged()));
	connect (m_project,SIGNAL(conductorAutoNumAdded()),   this,SLOT(conductorAutoNumChanged()));
	connect(m_project_view,SIGNAL(diagramActivated(DiagramView*)),this,SLOT(setConductorActive(DiagramView*)));

	//Element Signals
	connect (m_project,SIGNAL(elementAutoNumRemoved(QString)),   this,SLOT(elementAutoNumChanged()));
	connect (m_project,SIGNAL(elementAutoNumAdded(QString)),     this,SLOT(elementAutoNumChanged()));

	//Folio Signals
	connect (m_project,SIGNAL(folioAutoNumRemoved()),     this,SLOT(folioAutoNumChanged()));
	connect (m_project,SIGNAL(folioAutoNumAdded()),       this,SLOT(folioAutoNumChanged()));
	connect (this,
			 SIGNAL(folioAutoNumChanged(QString)),
			 &m_project_view->currentDiagram()->diagram()->border_and_titleblock,
			 SLOT (slot_setAutoPageNum(QString)));
	connect(m_project, SIGNAL(defaultTitleBlockPropertiesChanged()),this,SLOT(setActive()));

	//Conductor, Element and Folio Signals
	connect(m_project_view,SIGNAL(projectClosed(ProjectView*)),this,SLOT(clear()));

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
	QList <QString> keys_conductor = m_project->conductorAutoNum().keys();
	if (!keys_conductor.isEmpty()) {
		foreach (QString str, keys_conductor) { ui->m_conductor_cb-> addItem(str); }
	}

	//Element Combobox
	ui->m_element_cb->addItem("");
	QList <QString> keys_element = m_project->elementAutoNum().keys();
	if (!keys_element.isEmpty()) {
		foreach (QString str, keys_element) {ui->m_element_cb -> addItem(str);}
	}

	//Folio Combobox
	ui->m_folio_cb->addItem("");
	QList <QString> keys_folio = m_project->folioAutoNum().keys();
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

	if (m_project_view!=NULL) {
			//Conductor
		if (m_project_view->currentDiagram()) {
			QString conductor_autonum = m_project_view->currentDiagram()->diagram()->conductorsAutonumName();
			int conductor_index = ui->m_conductor_cb->findText(conductor_autonum);
			ui->m_conductor_cb->setCurrentIndex(conductor_index);
		}

			//Element
		QString element_formula = m_project->elementAutoNumCurrentFormula();
		QString active_element_autonum = m_project->elementCurrentAutoNum();
		int el_index = ui->m_element_cb->findText(active_element_autonum);
		ui->m_element_cb->setCurrentIndex(el_index);

			//Folio
		if (m_project->defaultTitleBlockProperties().folio == "%autonum") {
			QString page_autonum = m_project->defaultTitleBlockProperties().auto_page_num;
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
	QList <QString> keys_conductor = m_project->conductorAutoNum().keys();
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
void AutoNumberingDockWidget::on_m_conductor_cb_activated(int)
{
	QString current_autonum = ui->m_conductor_cb->currentText();
	QString current_formula = m_project->conductorAutoNumFormula(current_autonum);

	if (!current_autonum.isEmpty())  {
		m_project->setConductorAutoNumCurrentFormula(current_formula, current_autonum);
	}
	else {
		m_project->setConductorAutoNumCurrentFormula("","");
	}

	m_project_view->currentDiagram()->diagram()->setConductorsAutonumName(current_autonum);
	m_project_view->currentDiagram()->diagram()->loadCndFolioSeq();
}

/**
 * @brief AutoNumberingDockWidget::elementAutoNumChanged
 * Add new or remove element auto num from combobox
 */
void AutoNumberingDockWidget::elementAutoNumChanged() {

	ui->m_element_cb->clear();

	//Element Combobox
	ui->m_element_cb->addItem("");
	QList <QString> keys_element = m_project->elementAutoNum().keys();
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
void AutoNumberingDockWidget::on_m_element_cb_activated(int)
{
	m_project->setCurrrentElementAutonum(ui->m_element_cb->currentText());
	m_project_view->currentDiagram()->diagram()->loadElmtFolioSeq();
}

/**
 * @brief AutoNumberingDockWidget::folioAutoNumChanged
 * Add new or remove folio auto num from combobox
 */
void AutoNumberingDockWidget::folioAutoNumChanged() {

	ui->m_folio_cb->clear();

	//Folio Combobox
	ui->m_folio_cb->addItem("");
	QList <QString> keys_folio = m_project->folioAutoNum().keys();
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
	TitleBlockProperties ip = m_project -> defaultTitleBlockProperties();
	if (current_autonum != "") {
		ip.setAutoPageNum(current_autonum);
		ip.folio = "%autonum";
		m_project->setDefaultTitleBlockProperties(ip);
	}
	else {
		ip.folio = "%id/%total";
		m_project->setDefaultTitleBlockProperties(ip);
	}
		emit(folioAutoNumChanged(current_autonum));
}
