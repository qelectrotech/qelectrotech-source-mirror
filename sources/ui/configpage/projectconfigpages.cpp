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
#include "projectconfigpages.h"

#include "../autoNum/autonumschemecommand.h"
#include "../autoNum/elementautonumschemecommand.h"
#include "../autoNum/ui/counterwarning.h"
#include "../autoNum/ui/renumberpreviewdialog.h"
#include "../autoNum/numerotationcontext.h"
#include "../autoNum/ui/autonumberingmanagementw.h"
#include "../autoNum/ui/folioautonumbering.h"
#include "../autoNum/ui/formulaautonumberingw.h"
#include "../autoNum/ui/selectautonumw.h"
#include "../project/projectpropertieshandler.h"
#include "../qet.h"
#include "../qeticons.h"
#include "../qetproject.h"
#include "../wiringrules.h"
#include "../wiringruleswarning.h"
#include "../qetgraphicsitem/element.h"
#include "../diagram.h"
#include "../borderpropertieswidget.h"
#include "../conductorpropertieswidget.h"
#include "../diagramcontextwidget.h"
#include "../reportpropertiewidget.h"
#include "../titleblockpropertieswidget.h"
#include "../xrefpropertieswidget.h"

//#include "ui_autonumberingmanagementw.h"

#include <QtWidgets>
#include <optional>

/**
	Constructor
	@param project Project this page is editing.
	@param parent Parent QWidget
*/
ProjectConfigPage::ProjectConfigPage(QETProject *project, QWidget *parent) :
	ConfigPage(parent),
	m_project(project)
{
}

/**
	Destructor
*/
ProjectConfigPage::~ProjectConfigPage()
{
}

/**
	@return the project being edited by this page
*/
QETProject *ProjectConfigPage::project() const
{
	return(m_project);
}

/**
	@brief ProjectConfigPage::setProject
	Set \a new_project as the project being edited by this page.
	@param new_project : True to read values from the project
	into widgets before setting them read only accordingly,
	false otherwise. Defaults to true.
	@param read_values
	@return the former project
*/
QETProject *ProjectConfigPage::setProject(QETProject *new_project,
					  bool read_values) {
	if (new_project == m_project) return(m_project);
	
	QETProject *former_project = m_project;
	m_project = new_project;
	if (m_project && read_values) {
		readValuesFromProject();
		adjustReadOnly();
	}
	return(former_project);
}

/**
	Apply the configuration after user input
*/
void ProjectConfigPage::applyConf()
{
	if (!m_project || m_project -> isReadOnly()) return;
	applyProjectConf();
}

/**
	Initialize the page by calling initWidgets() and initLayout(). Also call
	readValuesFromProject() and adjustReadOnly() if a non-zero project has been
	set. Typically, you should call this function in your subclass constructor.
*/
void ProjectConfigPage::init()
{
	initWidgets();
	initLayout();
	if (m_project) {
		readValuesFromProject();
		adjustReadOnly();
	}
}

//######################################################################################//

/**
	Constructor
	@param project Project this page is editing.
	@param parent Parent QWidget
*/
ProjectMainConfigPage::ProjectMainConfigPage(QETProject *project, QWidget *parent) :
	ProjectConfigPage(project, parent)
{
	init();
}

/**
	Destructor
*/
ProjectMainConfigPage::~ProjectMainConfigPage()
{
}

/**
	@return the title for this page
*/
QString ProjectMainConfigPage::title() const
{
	return(tr("General", "configuration page title"));
}

/**
	@return the icon for this page
*/
QIcon ProjectMainConfigPage::icon() const
{
	return(QET::Icons::Settings);
}

/**
	Apply the configuration after user input
*/
void ProjectMainConfigPage::applyProjectConf()
{
	bool modified_project = false;
	
	QString new_title = title_value_ -> text();
	if (m_project -> title() != new_title) {
		m_project -> setTitle(new_title);
		modified_project = true;
	}
	
	DiagramContext new_properties = project_variables_ -> context();
	if (m_project -> projectProperties() != new_properties) {
		m_project -> setProjectProperties(new_properties);
		modified_project = true;
	}

	ProjectUsageTracker &usage_tracker = m_project -> projectPropertiesHandler().usageTracker();
	if (usage_tracker.isEnabled() != usage_enabled_cb_ -> isChecked()) {
		usage_tracker.setEnabled(usage_enabled_cb_ -> isChecked());
		modified_project = true;
	}

	const auto wire_hops = WireHops::fromString(wire_hops_cb_ -> currentData().toString());
	if (m_project -> wireHops() != wire_hops) {
		m_project -> setWireHops(wire_hops);
		modified_project = true;
	}

	if (m_project -> uprightSymbolTexts() != upright_symbol_texts_cb_ -> isChecked()) {
		m_project -> setUprightSymbolTexts(upright_symbol_texts_cb_ -> isChecked());
		modified_project = true;
	}

		//Following the application's rules stores nothing in the project
	WiringRules::Settings wiring_rules;
	if (!use_application_rules_cb_ -> isChecked()) {
		wiring_rules.own = true;
		wiring_rules.max_wires = max_wires_sb_ -> value();
		wiring_rules.one_wire_per_report = one_wire_per_report_cb_ -> isChecked();
	}
	if (m_project -> projectWiringRules() != wiring_rules) {
		const WiringRules::Settings before = m_project -> wiringRules();
		m_project -> setWiringRules(wiring_rules);
		modified_project = true;
		if (WiringRules::masterEnabled()
			&& WiringRules::turnsRuleOn(before, m_project -> wiringRules())) {
			WiringRulesWarning::show(this);
		}
	}

	if (modified_project) {
		m_project -> setModified(true);
	}
}

/**
	@return the project title entered by the user
*/
QString ProjectMainConfigPage::projectTitle() const
{
	return(title_value_ -> text());
}

/**
	Initialize widgets displayed by the page.
*/
void ProjectMainConfigPage::initWidgets()
{
	title_label_ = new QLabel(tr("Project title :", "label when configuring"));
	title_value_ = new QLineEdit();
	title_information_ = new QLabel(tr("This title is made available to all child sheets as %projecttitle.", "informative label"));
	project_variables_label_ = new QLabel(
		tr(
			"You may define below custom properties that will be made available to all sheets of the project (typically to use within title blocks).",
			 "informative label"
		)
	);
	project_variables_label_ -> setWordWrap(true);
	project_variables_ = new DiagramContextWidget();
	project_variables_ -> setContext(DiagramContext());

	usage_label_ = new QLabel(tr("Time spent on this project:", "label when configuring"));
	usage_value_ = new QLabel();
	usage_enabled_cb_ = new QCheckBox(tr("Track the time spent on this project (recorded locally in this file only)", "checkbox label"));
	usage_reset_pb_ = new QPushButton(tr("Reset", "button label"));
	connect(usage_reset_pb_, &QPushButton::clicked, this, &ProjectMainConfigPage::resetUsageTracker);

		//Hops where two conductors cross without being connected (issue #436)
	wire_hops_label_ = new QLabel(tr("Conductor crossings:", "label when configuring"));
	wire_hops_cb_ = new QComboBox();
	wire_hops_cb_ -> addItem(tr("No hop", "wire crossings"),
							 WireHops::toString(WireHops::Mode::None));
	wire_hops_cb_ -> addItem(tr("Hop on horizontal conductors", "wire crossings"),
							 WireHops::toString(WireHops::Mode::Horizontal));
	wire_hops_cb_ -> addItem(tr("Hop on vertical conductors", "wire crossings"),
							 WireHops::toString(WireHops::Mode::Vertical));
	wire_hops_cb_ -> setToolTip(tr("Draws a small arc where two conductors cross without being connected. Only the "
								   "drawing changes: no element is added and no conductor is cut.",
								   "tooltip"));

		//Texts of turned symbols kept horizontal
	upright_symbol_texts_cb_ = new QCheckBox(tr("Keep the texts of rotated elements horizontal",
												"checkbox label"));
	upright_symbol_texts_cb_ -> setToolTip(tr("Texts drawn in an element and the names of its terminals stay readable "
											  "when the element is rotated: their frame turns with the element, not the "
											  "text. Uncheck to make them turn with the element, as before.",
											  "tooltip"));

		//How many wires a terminal may take (discussion #1158)
	wiring_rules_gb_ = new QGroupBox(tr("Conductors per terminal", "group box title"));
	use_application_rules_cb_ = new QCheckBox(tr("Use the application settings", "checkbox label"));
	use_application_rules_cb_ -> setToolTip(tr("The settings in Configure QElectroTech > General apply. Untick to give this "
											   "project its own settings, saved in the project.",
											   "tooltip"));
	connect(use_application_rules_cb_, &QCheckBox::toggled, this, [this](bool use) {
			//Show the values that will apply: the application's, or the
			//application's as a starting point for the project's own
		if (use) {
			const WiringRules::Settings application = WiringRules::applicationSettings();
			max_wires_sb_ -> setValue(application.max_wires);
			one_wire_per_report_cb_ -> setChecked(application.one_wire_per_report);
		}
		updateWiringRulesWidgets();
	});
	max_wires_label_ = new QLabel(tr("Maximum conductors per terminal:", "label when configuring"));
	max_wires_sb_ = new QSpinBox();
	max_wires_sb_ -> setRange(0, 99);
	max_wires_sb_ -> setSpecialValueText(tr("No limit", "wires per terminal"));
	max_wires_sb_ -> setToolTip(tr("A new conductor that would take a terminal past this number is refused. "
								   "Conductors already drawn are not changed. 4 means two twin ferrules, one "
								   "on each side of the screw.",
								   "tooltip"));
	one_wire_per_report_cb_ = new QCheckBox(tr("Only one conductor per sheet reference", "checkbox label"));
	one_wire_per_report_cb_ -> setToolTip(tr("A sheet reference is a virtual point: it takes only one conductor, the "
											 "one that continues on the other sheet.",
											 "tooltip"));
	wiring_rules_off_label_ = new QLabel(tr("These rules are turned off for every project "
											"(Configure QElectroTech > General).",
											"informative label"));
	wiring_rules_off_label_ -> setWordWrap(true);
}

/**
	Initialize the layout of this page.
*/
void ProjectMainConfigPage::initLayout()
{
	QVBoxLayout *main_layout0 = new QVBoxLayout();
	QHBoxLayout *title_layout0 = new QHBoxLayout();
	title_layout0 -> addWidget(title_label_);
	title_layout0 -> addWidget(title_value_);
	main_layout0 -> addLayout(title_layout0);
	main_layout0 -> addWidget(title_information_);
	main_layout0 -> addSpacing(10);
	main_layout0 -> addWidget(project_variables_label_);
	main_layout0 -> addWidget(project_variables_);
	main_layout0 -> addSpacing(10);

	QHBoxLayout *usage_layout0 = new QHBoxLayout();
	usage_layout0 -> addWidget(usage_label_);
	usage_layout0 -> addWidget(usage_value_);
	usage_layout0 -> addStretch();
	usage_layout0 -> addWidget(usage_reset_pb_);
	main_layout0 -> addLayout(usage_layout0);
	main_layout0 -> addWidget(usage_enabled_cb_);
	main_layout0 -> addSpacing(10);

	QHBoxLayout *wire_hops_layout0 = new QHBoxLayout();
	wire_hops_layout0 -> addWidget(wire_hops_label_);
	wire_hops_layout0 -> addWidget(wire_hops_cb_);
	wire_hops_layout0 -> addStretch();
	main_layout0 -> addLayout(wire_hops_layout0);
	main_layout0 -> addSpacing(10);

	main_layout0 -> addWidget(upright_symbol_texts_cb_);
	main_layout0 -> addSpacing(10);

	QVBoxLayout *wiring_rules_layout = new QVBoxLayout(wiring_rules_gb_);
	wiring_rules_layout -> addWidget(use_application_rules_cb_);
	QHBoxLayout *max_wires_layout = new QHBoxLayout();
	max_wires_layout -> addWidget(max_wires_label_);
	max_wires_layout -> addWidget(max_wires_sb_);
	max_wires_layout -> addStretch();
	wiring_rules_layout -> addLayout(max_wires_layout);
	wiring_rules_layout -> addWidget(one_wire_per_report_cb_);
	wiring_rules_layout -> addWidget(wiring_rules_off_label_);
	main_layout0 -> addWidget(wiring_rules_gb_);

	setLayout(main_layout0);
	this -> setMinimumWidth(680);

}

/**
	Read properties from the edited project then fill widgets with them.
*/
void ProjectMainConfigPage::readValuesFromProject()
{
	title_value_ -> setText(m_project -> title());
	project_variables_ -> setContext(m_project -> projectProperties());

	const ProjectUsageTracker &usage_tracker = m_project -> projectPropertiesHandler().usageTracker();
	const qint64 total_seconds = usage_tracker.secondsSpent();
	usage_value_ -> setText(tr("%1 h %2 min", "hours and minutes of time spent on a project")
							 .arg(total_seconds / 3600)
							 .arg((total_seconds % 3600) / 60));
	usage_enabled_cb_ -> setChecked(usage_tracker.isEnabled());

	const int wire_hops_index = wire_hops_cb_ -> findData(WireHops::toString(m_project -> wireHops()));
	wire_hops_cb_ -> setCurrentIndex(qMax(0, wire_hops_index));
	upright_symbol_texts_cb_ -> setChecked(m_project -> uprightSymbolTexts());

		//The rules that apply: the project's own or the application's
	const WiringRules::Settings wiring_rules = m_project -> wiringRules();
	{
		const QSignalBlocker blocker(use_application_rules_cb_);
		use_application_rules_cb_ -> setChecked(!wiring_rules.own);
	}
	max_wires_sb_ -> setValue(wiring_rules.max_wires);
	one_wire_per_report_cb_ -> setChecked(wiring_rules.one_wire_per_report);
	updateWiringRulesWidgets();
}

/**
	@brief ProjectMainConfigPage::updateWiringRulesWidgets
	Enable the wiring rule fields only when they can change something: the
	master switch is on and the project sets its own rules.
*/
void ProjectMainConfigPage::updateWiringRulesWidgets()
{
		//The master switch wins over every project: say so rather than
		//let the user set a rule that does nothing.
	const bool master = WiringRules::masterEnabled();
	const bool own = master && !use_application_rules_cb_ -> isChecked();
	wiring_rules_off_label_ -> setVisible(!master);
	use_application_rules_cb_ -> setEnabled(master);
	max_wires_label_ -> setEnabled(own);
	max_wires_sb_ -> setEnabled(own);
	one_wire_per_report_cb_ -> setEnabled(own);
}

/**
	@brief ProjectMainConfigPage::resetUsageTracker
	Reset the accumulated "time spent on this project" counter to zero and
	refresh its displayed value. Applies immediately (not staged behind
	OK/Cancel like the other fields on this page), since it isn't
	destructive to any actual project content.
*/
void ProjectMainConfigPage::resetUsageTracker()
{
	m_project -> projectPropertiesHandler().usageTracker().resetSecondsSpent();
	m_project -> setModified(true);
	usage_value_ -> setText(tr("%1 h %2 min", "hours and minutes of time spent on a project").arg(0).arg(0));
}

/**
	Set the content of this page read only if the project is read only,
	editable if the project is editable.
*/
void ProjectMainConfigPage::adjustReadOnly()
{
	bool is_read_only = m_project -> isReadOnly();
	title_value_ -> setReadOnly(is_read_only);
	usage_enabled_cb_ -> setDisabled(is_read_only);
	usage_reset_pb_ -> setDisabled(is_read_only);
	wire_hops_cb_ -> setDisabled(is_read_only);
	upright_symbol_texts_cb_ -> setDisabled(is_read_only);
	wiring_rules_gb_ -> setDisabled(is_read_only);
}

//######################################################################################//

/**
	@brief ProjectAutoNumConfigPage::ProjectAutoNumConfigPage
	Default constructor
	@param project : project to edit
	@param parent : parent widget
*/
ProjectAutoNumConfigPage::ProjectAutoNumConfigPage (QETProject *project,
						    QWidget *parent) :
	ProjectConfigPage(project, parent)
{
	// Follow the same contract ProjectMainConfigPage's constructor does --
	// init() is documented to be the thing a subclass constructor calls
	// (see ProjectConfigPage::init()'s doc comment). Calling the pieces
	// individually here used to skip both initLayout() and adjustReadOnly(),
	// and called readValuesFromProject() even when m_project was null.
	// buildConnections() itself is invoked from the end of initWidgets()
	// below, in the same relative position it held here.
	init();
}

/**
	@brief ProjectAutoNumConfigPage::title
	Title of this config page
	@return
*/
QString ProjectAutoNumConfigPage::title() const
{
	return tr("Auto Numbering");
}

/**
	@brief ProjectAutoNumConfigPage::icon
	Icon of this config pafe
	@return
*/
QIcon ProjectAutoNumConfigPage::icon() const
{
	return QIcon (QET::Icons::AutoNum);
}

/**
	@brief ProjectAutoNumConfigPage::applyProjectConf
*/
void ProjectAutoNumConfigPage::applyProjectConf()
{}

namespace {
/**
	A tab of a numbering list over a table of the folios which follow the
	numbering shown
*/
QWidget *schemeTab(SelectAutonumW *saw, QGroupBox *&box, QTableWidget *&table)
{
	auto *tab = new QWidget();
	auto *layout = new QVBoxLayout(tab);
	layout->setContentsMargins(0, 0, 0, 0);
	layout->addWidget(saw);

	box = new QGroupBox(tab);
	auto *box_layout = new QVBoxLayout(box);
	table = new QTableWidget(0, 2, box);
	table->setHorizontalHeaderLabels({QObject::tr("Sheet"), QObject::tr("Title")});
	table->setEditTriggers(QAbstractItemView::NoEditTriggers);
	table->setSelectionBehavior(QAbstractItemView::SelectRows);
	table->verticalHeader()->setVisible(false);
	table->horizontalHeader()->setStretchLastSection(true);
	table->setAlternatingRowColors(true);
	box_layout->addWidget(table);
	layout->addWidget(box, 1);
	return tab;
}
} // namespace

/**
	@brief ProjectAutoNumConfigPage::initWidgets
	Init some widget of this page
*/
void ProjectAutoNumConfigPage::initWidgets()
{
	QTabWidget *tab_widget = new QTabWidget(this);
	m_tab_widget = tab_widget;
	
		//Management tab
	m_amw = new AutoNumberingManagementW(project());
	tab_widget->addTab(m_amw, tr("Management"));
	
		//Conductor tab
	m_saw_conductor = new SelectAutonumW(1);
	m_saw_conductor->setExplicitNaming();
	tab_widget->addTab(schemeTab(m_saw_conductor, m_conductor_users_box, m_conductor_users), tr("Conductors"));
	
		//Element tab
	m_saw_element = new SelectAutonumW(0);
	m_saw_element->setExplicitNaming();
	auto *element_tab = new QWidget(this);
	auto *element_layout = new QVBoxLayout(element_tab);
	element_layout->setContentsMargins(0, 0, 0, 0);
	element_layout->addWidget(m_saw_element);

		//The elements which follow the numbering shown: what an edit or a
		//renumbering would touch, and which of them stop it
	m_element_users_box = new QGroupBox(element_tab);
	auto *users_layout = new QVBoxLayout(m_element_users_box);
	m_element_users = new QTableWidget(0, 5, m_element_users_box);
	m_element_users->setHorizontalHeaderLabels({tr("No."), tr("Name"), tr("Sheet"), tr("Element"), tr("Frozen")});
	m_element_users->setEditTriggers(QAbstractItemView::NoEditTriggers);
	m_element_users->setSelectionBehavior(QAbstractItemView::SelectRows);
	m_element_users->verticalHeader()->setVisible(false);
	m_element_users->horizontalHeader()->setStretchLastSection(true);
	m_element_users->setAlternatingRowColors(true);
	users_layout->addWidget(m_element_users);
	m_assign_number_pb = new QPushButton(tr("Assign a free number…"), m_element_users_box);
	m_assign_number_pb->setToolTip(tr("Give the selected element a number nobody has: it keeps its "
									  "numbering, only its number changes."));
	m_assign_number_pb->setEnabled(false);
	users_layout->addWidget(m_assign_number_pb, 0, Qt::AlignLeft);
	connect(m_assign_number_pb, &QPushButton::clicked, this, &ProjectAutoNumConfigPage::assignFreeNumber);
	connect(m_element_users, &QTableWidget::itemSelectionChanged,
			this, &ProjectAutoNumConfigPage::updateAssignNumberButton);
	element_layout->addWidget(m_element_users_box, 1);
	tab_widget->addTab(element_tab, tr("Elements"));
	
		//Folio Tab
	m_saw_folio = new SelectAutonumW(2);
	m_saw_folio->setExplicitNaming();
	tab_widget->addTab(schemeTab(m_saw_folio, m_folio_users_box, m_folio_users), tr("Sheets"));
	
		//AutoNumbering Tab
	m_faw = new FolioAutonumberingW(project());
	tab_widget->addTab(m_faw, tr("Sheet Auto Numbering"));
	
	m_import_pb = new QPushButton(
				tr("Import from another project..."), this);
	m_import_pb->setToolTip(
				tr("Reuse the automatic numbering saved in "
				   "another project"));

	QHBoxLayout *button_layout = new QHBoxLayout();
	button_layout->addStretch();
	button_layout->addWidget(m_import_pb);

	QVBoxLayout *main_layout = new QVBoxLayout();
	main_layout->addWidget(tab_widget);
	main_layout->addLayout(button_layout);
	setLayout(main_layout);

	buildConnections();
}

/**
	@brief ProjectAutoNumConfigPage::readValuesFromProject
	Read value stored on project, and update display
*/
void ProjectAutoNumConfigPage::readValuesFromProject()
{
		// This is called again after an import, so start from an empty
		// combo box instead of appending a second copy of every name.
	m_saw_conductor->contextComboBox()->clear();
	m_saw_element->contextComboBox()->clear();
	m_saw_folio->contextComboBox()->clear();

		//Conductor Tab
	refreshSchemes(SchemeKind::Conductor, m_project->conductorCurrentAutoNum());
	
		//Element Tab
	refreshElementSchemes(m_project->elementCurrentAutoNum());
	
		//Folio Tab
	refreshSchemes(SchemeKind::Folio, QString());
	
		//Folio AutoNumbering Tab
	m_faw->setContext(m_project->folioAutoNum().keys());
}

/**
	@brief ProjectAutoNumConfigPage::adjustReadOnly
	set this config page disable if project is read only
*/
void ProjectAutoNumConfigPage::adjustReadOnly()
{
	if (m_import_pb && m_project) {
		m_import_pb->setDisabled(m_project->isReadOnly());
	}
}

/**
	@brief ProjectAutoNumConfigPage::buildConnections
	setup some connections
*/
void ProjectAutoNumConfigPage::buildConnections()
{
		//Management Tab
	connect(m_amw, &AutoNumberingManagementW::applyPressed, this, &ProjectAutoNumConfigPage::applyManagement);

		//Conductor Tab
	connect(m_saw_conductor, &SelectAutonumW::applyPressed,  this, &ProjectAutoNumConfigPage::saveContextConductor);
	connect(m_saw_conductor, &SelectAutonumW::removeClicked, this, &ProjectAutoNumConfigPage::removeContextConductor);
	connect(m_saw_conductor, &SelectAutonumW::newClicked,    this, &ProjectAutoNumConfigPage::newContextConductor);
	connect(m_saw_conductor, &SelectAutonumW::renameClicked, this, &ProjectAutoNumConfigPage::renameContextConductor);
	connect(m_saw_conductor->contextComboBox(), &QComboBox::textActivated, this, &ProjectAutoNumConfigPage::updateContextConductor);

		//Element Tab
	connect(m_saw_element, &SelectAutonumW::applyPressed,  this, &ProjectAutoNumConfigPage::saveContextElement);
	connect(m_saw_element, &SelectAutonumW::removeClicked, this, &ProjectAutoNumConfigPage::removeContextElement);
	connect(m_saw_element, &SelectAutonumW::newClicked,    this, &ProjectAutoNumConfigPage::newContextElement);
	connect(m_saw_element, &SelectAutonumW::renameClicked, this, &ProjectAutoNumConfigPage::renameContextElement);
	connect(m_saw_element->contextComboBox(), &QComboBox::textActivated, this, &ProjectAutoNumConfigPage::updateContextElement);

		//Folio Tab
	connect(m_saw_folio, &SelectAutonumW::applyPressed,  this, &ProjectAutoNumConfigPage::saveContextFolio);
	connect(m_saw_folio, &SelectAutonumW::removeClicked, this, &ProjectAutoNumConfigPage::removeContextFolio);
	connect(m_saw_folio, &SelectAutonumW::newClicked,    this, &ProjectAutoNumConfigPage::newContextFolio);
	connect(m_saw_folio, &SelectAutonumW::renameClicked, this, &ProjectAutoNumConfigPage::renameContextFolio);
	connect(m_saw_folio->contextComboBox(), &QComboBox::textActivated, this, &ProjectAutoNumConfigPage::updateContextFolio);

		//	Auto Folio Numbering
	connect(m_faw, &FolioAutonumberingW::applyPressed, this, &ProjectAutoNumConfigPage::applyAutoNum);

		//Import from another project
	connect(m_import_pb, &QPushButton::clicked, this, &ProjectAutoNumConfigPage::importFromProject);
}

/**
	@brief ProjectAutoNumConfigPage::updateContext_conductor
	Display the current selected context for conductor
	@param str : key of context stored in project
*/
void ProjectAutoNumConfigPage::updateContextConductor(const QString& str) {
	m_saw_conductor->setContext(AutoNumSchemeCommand::contextOf(m_project, SchemeKind::Conductor, str));
	refreshSchemeUsers(SchemeKind::Conductor);
}
void ProjectAutoNumConfigPage::updateContextFolio(const QString& str) {
	m_saw_folio->setContext(AutoNumSchemeCommand::contextOf(m_project, SchemeKind::Folio, str));
	refreshSchemeUsers(SchemeKind::Folio);
}

/**
	@brief ProjectAutoNumConfigPage::updateContextElement
	Display the current selected context for element
	@param str : key of context stored in project
*/
void ProjectAutoNumConfigPage::updateContextElement(const QString& str)
{
	if (str.isEmpty() || !m_project->elementAutoNum().contains(str))
	{
		m_saw_element->setContext(NumerotationContext());
	}
	else
	{
		m_saw_element->setContext(m_project->elementAutoNum(str));
	}
	refreshElementUsers();
}

/**
	@brief ProjectAutoNumConfigPage::refreshElementUsers
	List, in folio and position order, the elements which follow the
	element numbering shown, with their label and whether it is frozen.
*/
void ProjectAutoNumConfigPage::refreshElementUsers()
{
	if (!m_element_users || !m_element_users_box) {
		return;
	}
	const QString title = m_saw_element->contextComboBox()->currentText();
	QVector<Element *> users = m_project->elementsUsingElementAutoNum(title);
	std::sort(users.begin(), users.end(),
			  [](Element *a, Element *b) { return comparPos(a, b); });

		//With a single sequence of numbers the elements are listed by number,
		//and the numbers no element has between the lowest and the highest
		//are shown too: where an element was deleted or given another number
	using Support = ElementAutoNumSchemeCommand::NumberSupport;
	const Support support = m_project->elementAutoNum().contains(title)
			? ElementAutoNumSchemeCommand::numberSupport(m_project->elementAutoNum().value(title))
			: Support();
	struct Entry {
		int number = 0;
		int to = 0;               ///< the end of a range of free numbers
		Element *element = nullptr;
	};
	QVector<Entry> entries;
	QVector<Entry> unnumbered;
	for (Element *el : std::as_const(users)) {
		const auto number = support.supported ? ElementAutoNumSchemeCommand::numberOf(support, el)
											  : std::nullopt;
		if (support.supported && !number) {
			unnumbered << Entry{0, 0, el};
		} else {
			entries << Entry{number.value_or(0), 0, el};
		}
	}
	if (support.supported) {
		for (const auto &gap : ElementAutoNumSchemeCommand::gapRanges(m_project, title)) {
			entries << Entry{gap.from, gap.to, nullptr};
		}
		std::stable_sort(entries.begin(), entries.end(),
						 [](const Entry &a, const Entry &b) { return a.number < b.number; });
		entries += unnumbered;
	}

	int frozen = 0;
	int gaps = 0;
	m_element_users->setRowCount(0);
	m_element_rows.clear();
	for (const Entry &entry : std::as_const(entries))
	{
		const int row = m_element_users->rowCount();
		m_element_users->insertRow(row);
		m_element_rows << QPointer<Element>(entry.element);

		QStringList cells;
		if (entry.element)
		{
			const bool is_frozen = entry.element->isFreezeLabel();
			if (is_frozen) ++frozen;
			const auto number = support.supported ? ElementAutoNumSchemeCommand::numberOf(support, entry.element)
												  : std::nullopt;
			cells = QStringList{
				number ? QString::number(*number) : QString(),
				entry.element->elementInformations().value(QStringLiteral("label")).toString(),
				entry.element->diagram() ? QString::number(entry.element->diagram()->folioIndex() + 1) : QString(),
				entry.element->name(),
				is_frozen ? tr("frozen") : QString()};
		}
		else
		{
			++gaps;
			cells = QStringList{
				entry.to > entry.number ? QStringLiteral("%1 – %2").arg(entry.number).arg(entry.to)
										: QString::number(entry.number),
				tr("— free —"), QString(),
				tr("no element has this number (deleted or renumbered)"), QString()};
		}
		for (int column = 0 ; column < cells.size() ; ++column) {
			auto *item = new QTableWidgetItem(cells.at(column));
			if (!entry.element) {
				QFont font = item->font();
				font.setItalic(true);
				item->setFont(font);
				item->setForeground(QBrush(Qt::gray));
			}
			m_element_users->setItem(row, column, item);
		}
	}
	m_element_users->setColumnHidden(0, !support.supported);
	m_element_users->resizeColumnsToContents();
	m_element_users_box->setTitle(
				users.isEmpty()
				? tr("No element follows this numbering")
				: tr("%n elements follow this numbering, %1 with a frozen name", "", users.size())
				  .arg(frozen)
				  + (gaps ? tr("; %n numbers without an element", "", gaps) : QString()));
	updateAssignNumberButton();
}

/**
	@brief ProjectAutoNumConfigPage::updateAssignNumberButton
	The button which gives an element a free number needs an element row
	selected, in a numbering whose numbers are one sequence.
*/
void ProjectAutoNumConfigPage::updateAssignNumberButton()
{
	if (!m_assign_number_pb) {
		return;
	}
	const int row = m_element_users->currentRow();
	const Element *element = row >= 0 ? m_element_rows.value(row).data() : nullptr;
	const QString title = m_saw_element->contextComboBox()->currentText();
	const bool supported = m_project->elementAutoNum().contains(title)
			&& ElementAutoNumSchemeCommand::numberSupport(m_project->elementAutoNum().value(title)).supported;
	m_assign_number_pb->setEnabled(element && supported && !m_project->isReadOnly());
	m_assign_number_pb->setToolTip(
				!supported
				? tr("This numbering has several numbers (or one number per sheet): one "
					 "cannot be chosen by hand.")
				: tr("Give the selected element a number nobody has: it keeps its "
					 "numbering, only its number changes."));
}

/**
	@brief ProjectAutoNumConfigPage::assignFreeNumber
	Give the element selected in the table a free number, chosen in the list
	of those which are, each with the label it would give. The element keeps
	following the numbering. One undo step.
*/
void ProjectAutoNumConfigPage::assignFreeNumber()
{
	const int row = m_element_users->currentRow();
	Element *element = row >= 0 ? m_element_rows.value(row).data() : nullptr;
	const QString title = m_saw_element->contextComboBox()->currentText();
	const QString caption = tr("Assign a free number");
	if (!element || m_project->isReadOnly()) {
		return;
	}
	if (element->isFreezeLabel()) {
		QMessageBox::information(this, caption,
								 tr("This element's name is frozen: unfreeze it first."));
		return;
	}
	const QList<int> numbers = ElementAutoNumSchemeCommand::freeNumbers(m_project, title, element);
	if (numbers.isEmpty()) {
		QMessageBox::information(this, caption, tr("No number is free."));
		return;
	}
	QStringList items;
	for (int n : numbers) {
		items << tr("%1  →  %2").arg(n).arg(ElementAutoNumSchemeCommand::labelForNumber(
												m_project, title, element, n));
	}
	bool ok = false;
	const QString chosen = QInputDialog::getItem(
				this, caption,
				tr("Free number for element “%1” (%2):")
				.arg(element->elementInformations().value(QStringLiteral("label")).toString(), element->name()),
				items, 0, false, &ok);
	if (!ok) {
		return;
	}
	QString problem;
	auto *cmd = ElementAutoNumSchemeCommand::assignNumber(
				m_project, element, numbers.at(items.indexOf(chosen)), &problem);
	if (!cmd) {
		QMessageBox::warning(this, caption, problem);
		return;
	}
	m_project->undoStack()->push(cmd);
	updateContextElement(title);   // the counter may have moved; lists the elements again
}

/**
	@brief ProjectAutoNumConfigPage::refreshElementSchemes
	Fill the list of element numberings from the project and show
	@p selected, or the first one if there is no such numbering
*/
void ProjectAutoNumConfigPage::refreshElementSchemes(const QString &selected)
{
	QComboBox *cb = m_saw_element->contextComboBox();
	const QSignalBlocker blocker(cb);
	cb->clear();
	QStringList titles(m_project->elementAutoNum().keys());
	titles.sort(Qt::CaseInsensitive);
	cb->addItems(titles);
	const int index = cb->findText(selected);
	cb->setCurrentIndex(index >= 0 ? index : 0);
	updateContextElement(cb->currentText());
}

/**
	@brief ProjectAutoNumConfigPage::askElementSchemeName
	Ask the user for the name of an element numbering until it is an
	acceptable one or the user gives up.
	@param title : title of the dialog
	@param name : name proposed first
	@param ignored_title : the numbering being renamed, if any
	@return the name, empty if the user cancelled
*/
QString ProjectAutoNumConfigPage::askElementSchemeName(const QString &title,
													   QString name,
													   const QString &ignored_title)
{
	for (;;)
	{
		bool ok = false;
		name = QInputDialog::getText(this, title, tr("Numbering name:"),
									 QLineEdit::Normal, name, &ok);
		if (!ok) {
			return QString();
		}
		const QString problem = ElementAutoNumSchemeCommand::nameProblem(
									m_project, name, ignored_title);
		if (problem.isEmpty()) {
			return QETProject::normalizedAutoNumName(name);
		}
		QMessageBox::warning(this, title, problem);
	}
}

/**
	@brief ProjectAutoNumConfigPage::pushElementSchemeCommand
	Push @p cmd on the project's undo stack, after the user agreed when it
	changes the label of elements.
	@return true if pushed; @p cmd is deleted otherwise
*/
bool ProjectAutoNumConfigPage::pushElementSchemeCommand(ElementAutoNumSchemeCommand *cmd)
{
	if (!cmd) {
		return false;
	}
	if (const int changed = cmd->changedElementCount())
	{
		if (!RenumberPreviewDialog::confirm(
					this,
					tr("Modify numbering"),
					tr("%n elements follow this numbering and will change "
					   "formula and name.", "", changed),
					cmd->changes(), {})) {
			delete cmd;
			return false;
		}
	}
	m_project->undoStack()->push(cmd);
	return true;
}

/**
	@brief ProjectAutoNumConfigPage::newContextElement
	Create an element numbering under a name asked to the user, with a
	default definition, and show it to be defined.
*/
void ProjectAutoNumConfigPage::newContextElement()
{
	if (m_project->isReadOnly()) {
		return;
	}
	const QString name = askElementSchemeName(tr("New numbering"),
											  QString(), QString());
	if (name.isEmpty()) {
		return;
	}
	m_saw_element->setContext(NumerotationContext());
	if (pushElementSchemeCommand(ElementAutoNumSchemeCommand::create(
									 m_project, name,
									 m_saw_element->toNumContext(),
									 QUuid(), false))) {
		refreshElementSchemes(name);
	}
}

/**
	@brief ProjectAutoNumConfigPage::renameContextElement
	Rename the shown element numbering; the elements following it keep
	following it.
*/
void ProjectAutoNumConfigPage::renameContextElement()
{
	const QString old_title = m_saw_element->contextComboBox()->currentText();
	if (m_project->isReadOnly() || !m_project->elementAutoNum().contains(old_title)) {
		return;
	}
	const QString name = askElementSchemeName(tr("Rename numbering"),
											  old_title, old_title);
	if (name.isEmpty() || name == old_title) {
		return;
	}
	if (pushElementSchemeCommand(ElementAutoNumSchemeCommand::edit(
									 m_project, old_title, name,
									 m_project->elementAutoNum(old_title),
									 false))) {
		refreshElementSchemes(name);
	}
}

/**
	@brief ProjectAutoNumConfigPage::saveContextElement
	Save the current displayed Element formula in project
*/
void ProjectAutoNumConfigPage::saveContextElement()
{
	if (m_project->isReadOnly()) {
		return;
	}
	const QString title = m_saw_element->contextComboBox()->currentText();
	ElementAutoNumSchemeCommand *cmd = nullptr;
	QString shown = title;
	if (!m_project->elementAutoNum().contains(title))
	{
			//No numbering yet: the definition needs a name
		shown = askElementSchemeName(tr("New numbering"),
									 QString(), QString());
		if (shown.isEmpty()) {
			return;
		}
		cmd = ElementAutoNumSchemeCommand::create(
				  m_project, shown, m_saw_element->toNumContext());
	}
	else
	{
		const NumerotationContext wanted = m_saw_element->toNumContext();
		if (const int frozen = ElementAutoNumSchemeCommand::editBlockedBy(
					m_project, title, wanted).size())
		{
			QMessageBox::warning(
						this, tr("Modify numbering"),
						tr("The “%1” numbering cannot be modified: %n elements with a "
						   "frozen name follow it.\n"
						   "Unfreeze them first (see the list), or just rename the "
						   "numbering.", "", frozen).arg(title));
			refreshElementSchemes(title);   //Back to the definition the project has
			return;
		}
			//A counter put back on numbers in use: the next elements skip them
		if (!CounterWarning::confirm(this, m_project, title, wanted))
		{
			refreshElementSchemes(title);   //Back to the definition the project has
			return;
		}
		cmd = ElementAutoNumSchemeCommand::edit(
				  m_project, title, title, wanted);
	}
	if (pushElementSchemeCommand(cmd)) {
		refreshElementSchemes(shown);
	}
}

/**
	@brief ProjectAutoNumConfigPage::importFromProject
	Read the automatic numbering rules stored in another .qet project and
	copy the ones the user selects into this project.

	The source file is parsed as plain XML rather than opened as a
	QETProject: opening it would run the whole load path, including the
	modal dialog raised when the file was written by a different version
	of QElectroTech.
*/
void ProjectAutoNumConfigPage::importFromProject()
{
	if (!m_project || m_project->isReadOnly()) {
		return;
	}

	const QString path = QFileDialog::getOpenFileName(
				this,
				tr("Import numbering from a project"),
				m_project->currentDir(),
				tr("Project QElectroTech (*.qet)"));
	if (path.isEmpty()) {
		return;
	}

	QFile file(path);
	if (!file.open(QIODevice::ReadOnly)) {
		QMessageBox::warning(this, tr("Import not possible"),
					 tr("Unable to open %1").arg(path));
		return;
	}

	QDomDocument doc;
	if (!doc.setContent(&file)) {
		QMessageBox::warning(this, tr("Import not possible"),
					 tr("%1 is not a valid QElectroTech project.")
					 .arg(QFileInfo(path).fileName()));
		return;
	}
	file.close();

	const QDomNodeList newdiagrams =
			doc.elementsByTagName(QStringLiteral("newdiagrams"));
	if (newdiagrams.isEmpty()) {
		QMessageBox::information(this, tr("No numbering"),
					 tr("This project does not contain any automatic numbering."));
		return;
	}
	const QDomElement root = newdiagrams.at(0).toElement();

		// tag of the group, tag of one entry, label shown to the user
	struct Category {
		QString group_tag;
		QString item_tag;
		QString label;
	};
	const QList<Category> categories {
		{QStringLiteral("conductors_autonums"),
		 QStringLiteral("conductor_autonum"), tr("Conductors")},
		{QStringLiteral("element_autonums"),
		 QStringLiteral("element_autonum"), tr("Elements")},
		{QStringLiteral("folio_autonums"),
		 QStringLiteral("folio_autonum"), tr("Sheets")}
	};

	QDialog dialog(this);
	dialog.setWindowTitle(tr("Numbering to import"));
	QVBoxLayout *layout = new QVBoxLayout(&dialog);
	layout->addWidget(new QLabel(
				  tr("Numbering found in %1:")
				  .arg(QFileInfo(path).fileName()), &dialog));

	QListWidget *list = new QListWidget(&dialog);
	layout->addWidget(list);

		// NumerotationContext is not a QVariant type, so the list item
		// carries an index into this instead of the context itself.
	QList<NumerotationContext> contexts;
	for (int i = 0 ; i < categories.count() ; ++i)
	{
		const Category &category = categories.at(i);
		QDomElement group;
		for (QDomNode n = root.firstChild() ; !n.isNull() ; n = n.nextSibling()) {
			if (n.toElement().tagName() == category.group_tag) {
				group = n.toElement();
				break;
			}
		}
		if (group.isNull()) {
			continue;
		}

		for (QDomElement entry : QET::findInDomElement(group, category.item_tag))
		{
			const QString title = entry.attribute(QStringLiteral("title"));
			if (title.isEmpty()) {
				continue;
			}

			bool exists = false;
			switch (i) {
				case 0: exists = m_project->conductorAutoNum().contains(title); break;
				case 1: exists = !m_project->elementAutoNumNameClash(title).isEmpty(); break;
				default: exists = m_project->folioAutoNum().contains(title); break;
			}

			QListWidgetItem *item = new QListWidgetItem(
						exists ? tr("%1: %2 (already exists)")
							 .arg(category.label, title)
					       : QStringLiteral("%1 : %2")
							 .arg(category.label, title),
						list);
			item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
			item->setCheckState(exists ? Qt::Unchecked : Qt::Checked);
			item->setData(Qt::UserRole, i);
			item->setData(Qt::UserRole + 1, title);
			item->setData(Qt::UserRole + 3, entry.attribute(QStringLiteral("id")));

			NumerotationContext nc;
			nc.fromXml(entry);
			item->setData(Qt::UserRole + 2, contexts.count());
			contexts << nc;
		}
	}

	if (contexts.isEmpty()) {
		QMessageBox::information(this, tr("No numbering"),
					 tr("This project does not contain any automatic numbering."));
		return;
	}

	QCheckBox *overwrite_cb = new QCheckBox(
				tr("Replace numbering with the same name"), &dialog);
	layout->addWidget(overwrite_cb);

	QDialogButtonBox *buttons = new QDialogButtonBox(
				QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
	layout->addWidget(buttons);
	connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
	connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

	if (dialog.exec() != QDialog::Accepted) {
		return;
	}

	int imported = 0, skipped = 0, conductors = 0, relabelled = 0, blocked = 0;
	m_project->undoStack()->beginMacro(tr("Import numberings"));
	for (int row = 0 ; row < list->count() ; ++row)
	{
		QListWidgetItem *item = list->item(row);
		if (item->checkState() != Qt::Checked) {
			continue;
		}

		const int category = item->data(Qt::UserRole).toInt();
		const QString title = item->data(Qt::UserRole + 1).toString();

		bool exists = false;
		switch (category) {
			case 0: exists = !AutoNumSchemeCommand::nameClash(m_project, SchemeKind::Conductor, title).isEmpty(); break;
			case 1: exists = !m_project->elementAutoNumNameClash(title).isEmpty(); break;
			default: exists = !AutoNumSchemeCommand::nameClash(m_project, SchemeKind::Folio, title).isEmpty(); break;
		}
		if (exists && !overwrite_cb->isChecked()) {
			++skipped;
			continue;
		}

		const NumerotationContext &nc =
				contexts.at(item->data(Qt::UserRole + 2).toInt());
		switch (category) {
			case 0:
			{
				const QString clash = AutoNumSchemeCommand::nameClash(m_project, SchemeKind::Conductor, title);
				auto *cmd = clash.isEmpty()
						? AutoNumSchemeCommand::create(m_project, SchemeKind::Conductor, title, nc)
						: AutoNumSchemeCommand::edit(m_project, SchemeKind::Conductor, clash, clash, nc);
				if (cmd) m_project->undoStack()->push(cmd);
				++conductors;
				break;
			}
			case 1:
			{
					//Undoable, and an existing numbering is edited, not
					//replaced: the elements following it follow the
					//imported definition. A new one keeps the id it has
					//in the other project when free here, so elements
					//pasted from there follow it too.
				const QString clash = m_project->elementAutoNumNameClash(title);
				if (!clash.isEmpty()
						&& !ElementAutoNumSchemeCommand::editBlockedBy(m_project, clash, nc).isEmpty()) {
						//Elements with a frozen label follow it: kept as it is
					++blocked;
					continue;
				}
				ElementAutoNumSchemeCommand *cmd = clash.isEmpty()
						? ElementAutoNumSchemeCommand::create(
							  m_project, title, nc,
							  QUuid(item->data(Qt::UserRole + 3).toString()), false)
						: ElementAutoNumSchemeCommand::edit(
							  m_project, clash, clash, nc, false);
				if (cmd) {
					relabelled += cmd->changedElementCount();
					m_project->undoStack()->push(cmd);
				}
				break;
			}
			default:
			{
				const QString clash = AutoNumSchemeCommand::nameClash(m_project, SchemeKind::Folio, title);
				auto *cmd = clash.isEmpty()
						? AutoNumSchemeCommand::create(m_project, SchemeKind::Folio, title, nc)
						: AutoNumSchemeCommand::edit(m_project, SchemeKind::Folio, clash, clash, nc);
				if (cmd) m_project->undoStack()->push(cmd);
				break;
			}
		}
		++imported;
	}
	m_project->undoStack()->endMacro();

	readValuesFromProject();
	if (conductors) {
		m_project->conductorAutoNumAdded();
	}

	QMessageBox::information(
				this, tr("Import complete"),
				(skipped ? tr("%1 numbering(s) imported, %2 "
						  "kept unchanged.")
					   .arg(imported).arg(skipped)
					 : tr("%1 numbering(s) imported.").arg(imported))
				+ (blocked ? QStringLiteral("\n")
							 + tr("%n numberings followed by elements with a frozen name were "
								  "not replaced.", "", blocked)
						   : QString())
				+ (relabelled ? QStringLiteral("\n")
								+ tr("%n elements changed formula.", "", relabelled)
							  : QString()));
}

/**
	@brief ProjectAutoNumConfigPage::removeContextElement
	Remove from project the current element numerotation context
*/
void ProjectAutoNumConfigPage::removeContextElement()
{
	const QString title = m_saw_element->contextComboBox()->currentText();
	if (m_project->isReadOnly() || !m_project->elementAutoNum().contains(title)) {
		return;
	}
	if (const int used = m_project->elementsUsingElementAutoNum(title).size())
	{
		QMessageBox::information(
					this, tr("Delete numbering"),
					tr("%n elements follow the “%1” numbering, so it cannot be "
					   "deleted.\n"
					   "Give them another numbering or a fixed name first.", "", used)
					.arg(title));
		return;
	}
	if (pushElementSchemeCommand(ElementAutoNumSchemeCommand::remove(m_project, title))) {
		refreshElementSchemes(m_project->elementCurrentAutoNum());
	}
}

/**
	@brief ProjectAutoNumConfigPage::saveContext_conductor
	Save the current displayed conductor context in project
*/
void ProjectAutoNumConfigPage::saveContextConductor()
{
	saveScheme(SchemeKind::Conductor);
}

/**
	@brief ProjectAutoNumConfigPage::saveContextFolio
	Apply the definition shown to the folio numbering shown
*/
void ProjectAutoNumConfigPage::saveContextFolio()
{
	saveScheme(SchemeKind::Folio);
}

/**
	@brief ProjectAutoNumConfigPage::applyAutoNum
	Apply auto folio numbering, New Folios or Selected Folios
*/
void ProjectAutoNumConfigPage::applyAutoNum()
{

	if (m_faw->newFolios){
		int foliosRemaining = m_faw->newFoliosNumber();
		emit (saveCurrentTbp());
		emit (setAutoNum(m_faw->autoNumSelected()));
		while (foliosRemaining > 0){
			project()->autoFolioNumberingNewFolios();
			foliosRemaining = foliosRemaining-1;
		}
		emit (loadSavedTbp());
	}
	else{
		QString autoNum   = m_faw->autoNumSelected();
		int fromFolio = m_faw->fromFolio();
		int toFolio   = m_faw->toFolio();
		m_project->autoFolioNumberingSelectedFolios(fromFolio,toFolio,autoNum);
	}
}

/**
	@brief ProjectAutoNumConfigPage::applyAutoManagement
	Apply Management Options in Selected Folios
*/
void ProjectAutoNumConfigPage::applyManagement()
{
	//	int from;
	//	int to;
	//	//Apply to Entire Project
	//	if (m_amw->ui->m_apply_project_rb->isChecked()) {
	//		from = 0;
	//		to = project()->diagrams().size() - 1;
	//	}
	//	//Apply to selected Folios
	//	else {
	//		from =
	//m_amw->ui->m_from_folios_cb->itemData(m_amw->ui->m_from_folios_cb->currentIndex()).toInt();
	//		to =
	//m_amw->ui->m_to_folios_cb->itemData(m_amw->ui->m_to_folios_cb->currentIndex()).toInt();
	//	}

	//	//Conductor Autonumbering Update Policy
	//	//Allow Both Existent and New Conductors
	//	if (m_amw->ui->m_both_conductor_rb->isChecked()) {
	//		//Unfreeze Existent and New Conductors
	//		project()->freezeExistentConductorLabel(false, from,to);
	//		project()->freezeNewConductorLabel(false, from,to);
	//		project()->setFreezeNewConductors(false);
	//	}
	//	//Allow Only New
	//	else if (m_amw->ui->m_new_conductor_rb->isChecked()) {
	//		//Freeze Existent and Unfreeze New Conductors
	//		project()->freezeExistentConductorLabel(true, from,to);
	//		project()->freezeNewConductorLabel(false, from,to);
	//		project()->setFreezeNewConductors(false);
	//	}
	//	//Allow Only Existent
	//	else if (m_amw->ui->m_existent_conductor_rb->isChecked()) {
	//		//Freeze Existent and Unfreeze New Conductors
	//		project()->freezeExistentConductorLabel(false, from,to);
	//		project()->freezeNewConductorLabel(true, from,to);
	//		project()->setFreezeNewConductors(true);
	//	}
	//	//Disable
	//	else if (m_amw->ui->m_disable_conductor_rb->isChecked()) {
	//		//Freeze Existent and New Elements, Set Freeze Element Project Wide
	//		project()->freezeExistentConductorLabel(true, from,to);
	//		project()->freezeNewConductorLabel(true, from,to);
	//		project()->setFreezeNewConductors(true);
	//	}

	//	//Element Autonumbering Update Policy
	//	//Allow Both Existent and New Elements
	//	if (m_amw->ui->m_both_element_rb->isChecked()) {
	//		//Unfreeze Existent and New Elements
	//		project()->freezeExistentElementLabel(false, from,to);
	//		project()->freezeNewElementLabel(false, from,to);
	//		project()->setFreezeNewElements(false);
	//	}
	//	//Allow Only New
	//	else if (m_amw->ui->m_new_element_rb->isChecked()) {
	//		//Freeze Existent and Unfreeze New Elements
	//		project()->freezeExistentElementLabel(true, from,to);
	//		project()->freezeNewElementLabel(false, from,to);
	//		project()->setFreezeNewElements(false);
	//	}
	//	//Allow Only Existent
	//	else if (m_amw->ui->m_existent_element_rb->isChecked()) {
	//		//Freeze New and Unfreeze Existent Elements, Set Freeze Element
	//Project Wide 		project()->freezeExistentElementLabel(false, from,to);
	//		project()->freezeNewElementLabel(true, from,to);
	//		project()->setFreezeNewElements(true);
	//	}
	//	//Disable
	//	else if (m_amw->ui->m_disable_element_rb->isChecked()) {
	//		//Freeze Existent and New Elements, Set Freeze Element Project Wide
	//		project()->freezeExistentElementLabel(true, from,to);
	//		project()->freezeNewElementLabel(true, from,to);
	//		project()->setFreezeNewElements(true);
	//	}

	//	//Folio Autonumbering Status
	//	if (m_amw->ui->m_both_folio_rb->isChecked()) {

	//	}
	//	else if (m_amw->ui->m_new_folio_rb->isChecked()) {

	//	}
	//	else if (m_amw->ui->m_existent_folio_rb->isChecked()) {

	//	}
	//	else if (m_amw->ui->m_disable_folio_rb->isChecked()) {

	//	}
}

/**
	@brief ProjectAutoNumConfigPage::removeContext
	Remove from project the current conductor numerotation context
*/
void ProjectAutoNumConfigPage::removeContextConductor()
{
	removeScheme(SchemeKind::Conductor);
}

/**
	@brief ProjectAutoNumConfigPage::removeContextFolio
	Remove from project the folio numerotation context shown
*/
void ProjectAutoNumConfigPage::removeContextFolio()
{
	removeScheme(SchemeKind::Folio);
}

void ProjectAutoNumConfigPage::newContextConductor()    {newScheme(SchemeKind::Conductor);}
void ProjectAutoNumConfigPage::renameContextConductor() {renameScheme(SchemeKind::Conductor);}
void ProjectAutoNumConfigPage::newContextFolio()        {newScheme(SchemeKind::Folio);}
void ProjectAutoNumConfigPage::renameContextFolio()     {renameScheme(SchemeKind::Folio);}

SelectAutonumW *ProjectAutoNumConfigPage::sawFor(SchemeKind kind) const
{
	return kind == SchemeKind::Conductor ? m_saw_conductor : m_saw_folio;
}

/**
	@brief ProjectAutoNumConfigPage::refreshSchemes
	Fill the list of numberings of @p kind from the project and show
	@p selected, or the first one if there is no such numbering
*/
void ProjectAutoNumConfigPage::refreshSchemes(SchemeKind kind, const QString &selected)
{
	QComboBox *cb = sawFor(kind)->contextComboBox();
	{
		const QSignalBlocker blocker(cb);
		cb->clear();
		cb->addItems(AutoNumSchemeCommand::titles(m_project, kind));
		const int index = cb->findText(selected);
		cb->setCurrentIndex(index >= 0 ? index : 0);
	}
	sawFor(kind)->setContext(AutoNumSchemeCommand::contextOf(m_project, kind, cb->currentText()));
	refreshSchemeUsers(kind);
}

/**
	@brief ProjectAutoNumConfigPage::refreshSchemeUsers
	List the folios which follow the numbering of @p kind shown
*/
void ProjectAutoNumConfigPage::refreshSchemeUsers(SchemeKind kind)
{
	QTableWidget *table = kind == SchemeKind::Conductor ? m_conductor_users : m_folio_users;
	QGroupBox *box = kind == SchemeKind::Conductor ? m_conductor_users_box : m_folio_users_box;
	if (!table || !box) {
		return;
	}
	const QString title = sawFor(kind)->contextComboBox()->currentText();
		//A folio numbering is named by the title blocks it numbered, whose
		//folio field holds the number since: they are what is listed
	const QList<Diagram *> users = AutoNumSchemeCommand::usersOf(
				m_project, kind, title, kind == SchemeKind::Folio);

	table->setRowCount(0);
	for (Diagram *d : users) {
		const int row = table->rowCount();
		table->insertRow(row);
		table->setItem(row, 0, new QTableWidgetItem(QString::number(d->folioIndex() + 1)));
		table->setItem(row, 1, new QTableWidgetItem(d->border_and_titleblock.title()));
	}
	table->resizeColumnsToContents();
	if (kind == SchemeKind::Folio) {
		box->setTitle(users.isEmpty()
					  ? tr("No sheet names this numbering")
					  : tr("%n sheets name this numbering", "", users.size()));
	} else {
		box->setTitle(users.isEmpty()
					  ? tr("No sheet follows this numbering")
					  : tr("%n sheets follow this numbering", "", users.size()));
	}
}

QString ProjectAutoNumConfigPage::askSchemeName(SchemeKind kind, const QString &title,
												QString name, const QString &ignored_title)
{
	for (;;)
	{
		bool ok = false;
		name = QInputDialog::getText(this, title, tr("Numbering name:"),
									 QLineEdit::Normal, name, &ok);
		if (!ok) {
			return QString();
		}
		const QString problem = AutoNumSchemeCommand::nameProblem(m_project, kind, name, ignored_title);
		if (problem.isEmpty()) {
			return QETProject::normalizedAutoNumName(name);
		}
		QMessageBox::warning(this, title, problem);
	}
}

/**
	@brief ProjectAutoNumConfigPage::newScheme
	Create a numbering under a name asked to the user, and show it to be defined.
*/
void ProjectAutoNumConfigPage::newScheme(SchemeKind kind)
{
	if (m_project->isReadOnly()) {
		return;
	}
	const QString name = askSchemeName(kind, tr("New numbering"), QString(), QString());
	if (name.isEmpty()) {
		return;
	}
	sawFor(kind)->setContext(NumerotationContext());
	if (auto *cmd = AutoNumSchemeCommand::create(m_project, kind, name,
												 sawFor(kind)->toNumContext(), true)) {
		m_project->undoStack()->push(cmd);
		refreshSchemes(kind, name);
	}
}

/**
	@brief ProjectAutoNumConfigPage::renameScheme
	Rename the numbering shown; the folios which follow it follow it under its new name.
*/
void ProjectAutoNumConfigPage::renameScheme(SchemeKind kind)
{
	const QString old_title = sawFor(kind)->contextComboBox()->currentText();
	if (m_project->isReadOnly() || !AutoNumSchemeCommand::contains(m_project, kind, old_title)) {
		return;
	}
	const QString name = askSchemeName(kind, tr("Rename numbering"), old_title, old_title);
	if (name.isEmpty() || name == old_title) {
		return;
	}
	if (auto *cmd = AutoNumSchemeCommand::edit(
				m_project, kind, old_title, name,
				AutoNumSchemeCommand::contextOf(m_project, kind, old_title))) {
		m_project->undoStack()->push(cmd);
		refreshSchemes(kind, name);
	}
}

/**
	@brief ProjectAutoNumConfigPage::saveScheme
	Give the numbering shown the definition shown; with no numbering yet,
	create one under a name asked to the user. A conductor numbering applied
	becomes the one new conductors take.
*/
void ProjectAutoNumConfigPage::saveScheme(SchemeKind kind)
{
	if (m_project->isReadOnly()) {
		return;
	}
	const QString title = sawFor(kind)->contextComboBox()->currentText();
	const NumerotationContext wanted = sawFor(kind)->toNumContext();
	QString shown = title;
	AutoNumSchemeCommand *cmd = nullptr;
	if (!AutoNumSchemeCommand::contains(m_project, kind, title))
	{
		shown = askSchemeName(kind, tr("New numbering"), QString(), QString());
		if (shown.isEmpty()) {
			return;
		}
		cmd = AutoNumSchemeCommand::create(m_project, kind, shown, wanted, true);
	}
	else
	{
		cmd = AutoNumSchemeCommand::edit(m_project, kind, title, title, wanted, true);
	}
	if (cmd) {
		m_project->undoStack()->push(cmd);
	}
	refreshSchemes(kind, shown);
}

/**
	@brief ProjectAutoNumConfigPage::removeScheme
	Remove the numbering shown, unless a folio still follows it
*/
void ProjectAutoNumConfigPage::removeScheme(SchemeKind kind)
{
	const QString title = sawFor(kind)->contextComboBox()->currentText();
	if (m_project->isReadOnly() || !AutoNumSchemeCommand::contains(m_project, kind, title)) {
		return;
	}
	if (const int used = AutoNumSchemeCommand::usersOf(m_project, kind, title).size())
	{
		QMessageBox::information(
					this, tr("Delete numbering"),
					tr("%n sheets follow the “%1” numbering, so it cannot be deleted.\n"
					   "Give them another numbering first.", "", used)
					.arg(title));
		return;
	}
	if (auto *cmd = AutoNumSchemeCommand::remove(m_project, kind, title)) {
		m_project->undoStack()->push(cmd);
		refreshSchemes(kind, kind == SchemeKind::Conductor ? m_project->conductorCurrentAutoNum() : QString());
	}
}

/**
	@brief ProjectAutoNumConfigPage::changeToTab
	@param i index
	Change to Selected Tab
*/
void ProjectAutoNumConfigPage::changeToTab(int i)
{
	if (m_tab_widget && i >= 0 && i < m_tab_widget->count()) {
		m_tab_widget->setCurrentIndex(i);
	}
}
