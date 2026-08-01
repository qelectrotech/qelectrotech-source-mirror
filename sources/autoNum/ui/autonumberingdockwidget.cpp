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
#include "autonumberingdockwidget.h"

#include "../../diagram.h"
#include "../../diagramview.h"
#include "../../qetapp.h"
#include "../../shortcutmanager.h"
#include "../../titleblockproperties.h"
#include "../../ui/projectpropertiesdialog.h"
#include "../numerotationcontext.h"
#include "ui_autonumberingdockwidget.h"

#include <QComboBox>

/**
	@brief AutoNumberingDockWidget::AutoNumberingDockWidget
	Constructor
	@param parent : parent widget
*/
AutoNumberingDockWidget::AutoNumberingDockWidget(QWidget *parent) :
	QDockWidget(parent),
	ui(new Ui::AutoNumberingDockWidget)
{
	ui->setupUi(this);
	this->setDisabled(true);
}

/**
	@brief AutoNumberingDockWidget::~AutoNumberingDockWidget
	Destructor
*/
AutoNumberingDockWidget::~AutoNumberingDockWidget()
{
	this->disconnect();
	delete ui;
}

/**
	@brief AutoNumberingDockWidget::clear
	Remove all combo box values
*/
void AutoNumberingDockWidget::clear()
{
	ui->m_conductor_cb->clear();
	ui->m_element_cb->clear();
	ui->m_folio_cb->clear();
}

void AutoNumberingDockWidget::projectClosed()
{
	m_project = nullptr;
	m_project_view = nullptr;
	clear();
	this->setDisabled(true);
}

/**
	@brief AutoNumberingDockWidget::setProject
	@param project: project to be setted
	@param projectview: projectview to be setted
	assign Project and ProjectView, connect all signals and setContext
*/
void AutoNumberingDockWidget::setProject(QETProject *project,
					 ProjectView *projectview)
{
		//Disconnect previous project
	if (m_project && m_project_view)
	{
			//Conductor Signals
		disconnect(m_project, SIGNAL(conductorAutoNumChanged()),
			   this,SLOT(conductorAutoNumChanged()));
		disconnect (m_project,SIGNAL(conductorAutoNumRemoved()),
			    this,SLOT(conductorAutoNumChanged()));
		disconnect (m_project,SIGNAL(conductorAutoNumAdded()),
			    this,SLOT(conductorAutoNumChanged()));
		disconnect(m_project_view,SIGNAL(diagramActivated(DiagramView*)),
			   this,SLOT(setConductorActive(DiagramView*)));
	
			//Element Signals
		disconnect (m_project,SIGNAL(elementAutoNumRemoved(QString)),
			    this,SLOT(elementAutoNumChanged()));
		disconnect (m_project,SIGNAL(elementAutoNumAdded(QString)),
			    this,SLOT(elementAutoNumChanged()));
	
			//Folio Signals
		disconnect (m_project,SIGNAL(folioAutoNumRemoved()),
			    this,SLOT(folioAutoNumChanged()));
		disconnect (m_project,SIGNAL(folioAutoNumAdded()),
			    this,SLOT(folioAutoNumChanged()));
		disconnect (this,
			    SIGNAL(folioAutoNumChanged(QString)),
			    &m_project_view->currentDiagram()->diagram()->border_and_titleblock,
			    SLOT (slot_setAutoPageNum(QString)));
		disconnect(m_project, SIGNAL(defaultTitleBlockPropertiesChanged()),
			   this,SLOT(setActive()));
	
			//Conductor, Element and Folio Signals
		disconnect(m_project, &QETProject::destroyed,
			   this, &AutoNumberingDockWidget::projectClosed);
	}
	
	m_project = project;
	m_project_view = projectview;
	this->setEnabled(true);

		//Conductor Signals
	connect(m_project, SIGNAL(conductorAutoNumChanged()),
		this,SLOT(conductorAutoNumChanged()));
	connect(m_project,SIGNAL(conductorAutoNumRemoved()),
		this,SLOT(conductorAutoNumChanged()));
	connect(m_project,SIGNAL(conductorAutoNumAdded()),
		this,SLOT(conductorAutoNumChanged()));
	connect(m_project_view,SIGNAL(diagramActivated(DiagramView*)),
		this,SLOT(setConductorActive(DiagramView*)));

		//Element Signals
	connect (m_project,SIGNAL(elementAutoNumRemoved(QString)),
		 this,SLOT(elementAutoNumChanged()));
	connect (m_project,SIGNAL(elementAutoNumAdded(QString)),
		 this,SLOT(elementAutoNumChanged()));

		//Folio Signals
	connect (m_project,SIGNAL(folioAutoNumRemoved()),
		 this,SLOT(folioAutoNumChanged()));
	connect (m_project,SIGNAL(folioAutoNumAdded()),
		 this,SLOT(folioAutoNumChanged()));
	connect (this,
		 SIGNAL(folioAutoNumChanged(QString)),
		 &m_project_view->currentDiagram()->diagram()->border_and_titleblock,
		 SLOT (slot_setAutoPageNum(QString)));
	connect(m_project, SIGNAL(defaultTitleBlockPropertiesChanged()),
		this,SLOT(setActive()));

		//Conductor, Element and Folio Signals
	connect(m_project, &QETProject::destroyed,
		this, &AutoNumberingDockWidget::projectClosed);

		//Set Combobox Context
	setContext();
	
	ShortcutManager::instance().registerAction(ui->m_configure_pb, "autonum.configure",
						    tr("Autonumérotation"), Qt::CTRL | Qt::SHIFT | Qt::Key_P);
}

/**
	@brief AutoNumberingDockWidget::setContext
	Add all itens to comboboxes
*/
void AutoNumberingDockWidget::setContext()
{

	this->clear();

	//Conductor Combobox
	ui->m_conductor_cb->addItem("");
	QList <QString> keys_conductor = m_project->conductorAutoNum().keys();
	if (!keys_conductor.isEmpty()) {
		foreach (QString str, keys_conductor)
		{ ui->m_conductor_cb-> addItem(str); }
	}

	//Element Combobox
	ui->m_element_cb->addItem("");
	QList <QString> keys_element = m_project->elementAutoNum().keys();
	if (!keys_element.isEmpty()) {
		foreach (QString str, keys_element)
		{ui->m_element_cb -> addItem(str);}
	}

	//Folio Combobox
	ui->m_folio_cb->addItem("");
	QList <QString> keys_folio = m_project->folioAutoNum().keys();
	if (!keys_folio.isEmpty()) {
		foreach (QString str, keys_folio)
		{ ui->m_folio_cb -> addItem(str);}
	}

	this->setActive();
}

/**
	@brief AutoNumberingDockWidget::setConductorActive
	@param dv: activated diagramview
*/
void AutoNumberingDockWidget::setConductorActive(DiagramView* dv) {
	if (dv!=nullptr) {
		QString conductor_autonum = dv->diagram()->conductorsAutonumName();
		int conductor_index = ui->m_conductor_cb->findText(conductor_autonum);
		ui->m_conductor_cb->setCurrentIndex(conductor_index);
	}
}

/**
	@brief AutoNumberingDockWidget::setActive
	Set current used autonumberings
*/
void AutoNumberingDockWidget::setActive()
{

	if (m_project_view!=nullptr) {
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
	@brief AutoNumberingDockWidget::conductorAutoNumChanged
	Add new or remove conductor auto num from combobox
*/
void AutoNumberingDockWidget::conductorAutoNumChanged()
{
	ui->m_conductor_cb->clear();

	//Conductor Combobox
	ui->m_conductor_cb->addItem("");
	QList <QString> keys_conductor = m_project->conductorAutoNum().keys();
	if (!keys_conductor.isEmpty()) {
		foreach (QString str, keys_conductor)
		{ ui->m_conductor_cb-> addItem(str); }
	}
	setActive();
}

/**
	@brief AutoNumberingDockWidget::on_m_conductor_cb_activated
	Set new conductor AutoNum
*/
void AutoNumberingDockWidget::on_m_conductor_cb_activated(int)
{
	QString current_autonum = ui->m_conductor_cb->currentText();

	m_project->setCurrentConductorAutoNum(current_autonum);
	m_project_view->currentDiagram()->diagram()->setConductorsAutonumName(current_autonum);
	m_project_view->currentDiagram()->diagram()->loadCndFolioSeq();
}

/**
	@brief AutoNumberingDockWidget::elementAutoNumChanged
	Add new or remove element auto num from combobox
*/
void AutoNumberingDockWidget::elementAutoNumChanged()
{

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
	@brief AutoNumberingDockWidget::on_m_element_cb_activated
	Set new element AutoNum
*/
void AutoNumberingDockWidget::on_m_element_cb_activated(int)
{
	m_project->setCurrrentElementAutonum(ui->m_element_cb->currentText());
	m_project_view->currentDiagram()->diagram()->loadElmtFolioSeq();
}

/**
	@brief AutoNumberingDockWidget::folioAutoNumChanged
	Add new or remove folio auto num from combobox
*/
void AutoNumberingDockWidget::folioAutoNumChanged()
{

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
	@brief AutoNumberingDockWidget::on_m_folio_cb_activated
	Set new folio AutoNum
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

void AutoNumberingDockWidget::on_m_configure_pb_clicked()
{
	if (m_project)
	{
		ProjectPropertiesDialog ppd (m_project, this);
		ppd.setCurrentPage(ProjectPropertiesDialog::Autonum);
		ppd.exec();
	}
}

void AutoNumberingDockWidget::on_m_conductor_reset_start_pb_clicked()
{
	resetAutoNum(ui->m_conductor_cb, AutoNumCategory::Conductor, false);
}

void AutoNumberingDockWidget::on_m_conductor_reset_placeholder_pb_clicked()
{
	resetAutoNum(ui->m_conductor_cb, AutoNumCategory::Conductor, true);
}

void AutoNumberingDockWidget::on_m_element_reset_start_pb_clicked()
{
	resetAutoNum(ui->m_element_cb, AutoNumCategory::Element, false);
}

void AutoNumberingDockWidget::on_m_element_reset_placeholder_pb_clicked()
{
	resetAutoNum(ui->m_element_cb, AutoNumCategory::Element, true);
}

void AutoNumberingDockWidget::on_m_folio_reset_start_pb_clicked()
{
	resetAutoNum(ui->m_folio_cb, AutoNumCategory::Folio, false);
}

void AutoNumberingDockWidget::on_m_folio_reset_placeholder_pb_clicked()
{
	resetAutoNum(ui->m_folio_cb, AutoNumCategory::Folio, true);
}

/**
	@brief AutoNumberingDockWidget::resetAutoNum
	Reset the numerotation context currently selected in combo_box (for
	category) either to a per-type starting value (to_placeholder = false)
	or to the literal placeholder "?" on every part (to_placeholder =
	true), then write it back so the existing refresh signals fire as
	normal. Does nothing if no context is selected.

	"Reset to start" only touches parts that actually represent a
	progressing counter (numeric types, wrap, alpha): folio-anchored
	numeric types go back to their own stored initialvalue, plain numeric
	types and wrap go back to "1", alpha goes back to "a". Non-incrementing
	types (string, plant, locmach, idfolio, folio, elementline,
	elementcolumn, elementprefix) are left untouched -- there's no
	meaningful "start" distinct from whatever the user configured for a
	fixed/contextual value. "Reset to ?" applies to every part
	unconditionally, since its purpose is marking the whole context as
	needing manual attention.
*/
void AutoNumberingDockWidget::resetAutoNum(QComboBox *combo_box, AutoNumCategory category, bool to_placeholder)
{
	if (!m_project || combo_box->currentText().isEmpty())
		return;

	const QString key = combo_box->currentText();
	NumerotationContext context;
	switch (category) {
		case AutoNumCategory::Conductor: context = m_project->conductorAutoNum(key); break;
		case AutoNumCategory::Element:   context = m_project->elementAutoNum(key);   break;
		case AutoNumCategory::Folio:     context = m_project->folioAutoNum(key);     break;
	}

	for (int i = 0; i < context.size(); ++i)
	{
		if (to_placeholder)
		{
			context.replaceValue(i, QStringLiteral("?"));
			continue;
		}

		const QStringList item = context.itemAt(i);
		const QString &type = item.at(0);
		if (type == QLatin1String("unitfolio")
				|| type == QLatin1String("tenfolio")
				|| type == QLatin1String("hundredfolio"))
			context.replaceValue(i, item.size() > 3 ? item.at(3) : QStringLiteral("1"));
		else if (type == QLatin1String("unit")
				|| type == QLatin1String("ten")
				|| type == QLatin1String("hundred")
				|| type == QLatin1String("wrap"))
			context.replaceValue(i, QStringLiteral("1"));
		else if (type == QLatin1String("alpha"))
			context.replaceValue(i, QStringLiteral("a"));
	}

	switch (category) {
		case AutoNumCategory::Conductor: m_project->addConductorAutoNum(key, context); break;
		case AutoNumCategory::Element:   m_project->addElementAutoNum(key, context);   break;
		case AutoNumCategory::Folio:     m_project->addFolioAutoNum(key, context);     break;
	}
}
