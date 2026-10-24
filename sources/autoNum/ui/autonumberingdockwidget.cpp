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
#include "../../qetgraphicsitem/element.h"
#include "../../qeticons.h"
#include "../elementautonumschemecommand.h"
#include "counterwarning.h"
#include "renumberpreviewdialog.h"
#include "../../qetapp.h"
#include "../../shortcutmanager.h"
#include "../../titleblockproperties.h"
#include "../../ui/projectpropertiesdialog.h"
#include "../numerotationcontext.h"
#include "../numerotationcontextcommands.h"
#include "ui_autonumberingdockwidget.h"
#include "../../undocommand/changetitleblockcommand.h"

#include <QComboBox>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QSignalBlocker>
#include <QSpinBox>

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

		//The element row gets a button to the left of its reset button
		//which gives the numbering shown to the selected elements
	QPushButton *reset = ui->m_element_reset_start_pb;
	ui->gridLayout->removeWidget(reset);
	auto *actions = new QWidget(this);
	auto *actions_layout = new QHBoxLayout(actions);
	actions_layout->setContentsMargins(0, 0, 0, 0);
	actions_layout->setSpacing(2);
	m_element_apply_pb = new QPushButton(QET::Icons::DialogOk, QString(), actions);
	m_element_apply_pb->setObjectName(QStringLiteral("m_element_apply_pb"));
	m_element_apply_pb->setMaximumWidth(24);
	m_element_apply_pb->setToolTip(tr("Apply this numbering to the selected elements"));
	reset->setParent(actions);
	actions_layout->addWidget(m_element_apply_pb);
	actions_layout->addWidget(reset);
	ui->gridLayout->addWidget(actions, 3, 2);
	connect(m_element_apply_pb, &QPushButton::clicked,
			this, &AutoNumberingDockWidget::applyElementSchemeToSelection);
	connect(ui->m_element_cb, qOverload<int>(&QComboBox::currentIndexChanged),
			this, &AutoNumberingDockWidget::updateApplyEnabled);
	m_element_apply_pb->setEnabled(false);

	this->setDisabled(true);
}

/**
	@brief AutoNumberingDockWidget::applyElementSchemeToSelection
	Give the element numbering shown in the list to the elements selected
	in the current folio: each takes its formula and the next number, in
	folio and position order, and the counter moves on.

	What would change is shown first. An element which holds another
	numbering, or whose label is frozen, is left as it is, unless the user
	asks to replace them too (a frozen one is then given back). One undo
	step.
*/
void AutoNumberingDockWidget::applyElementSchemeToSelection()
{
	if (!m_project || m_project->isReadOnly()) {
		return;
	}
	const QString caption = tr("Apply numbering");
	const QString title = ui->m_element_cb->currentText();
	DiagramView *view = m_project_view ? m_project_view->currentDiagram() : nullptr;
	Diagram *diagram = view ? view->diagram() : nullptr;
	if (!diagram || !m_project->elementAutoNum().contains(title)) {
		QMessageBox::information(this, caption,
								 tr("Choose an element numbering from the project."));
		return;
	}

	QVector<Element *> selected;
	const auto items = diagram->selectedItems();
	for (QGraphicsItem *item : items) {
		if (auto *el = qgraphicsitem_cast<Element *>(item)) {
			selected << el;
		}
	}
	if (selected.isEmpty()) {
		QMessageBox::information(this, caption,
								 tr("First select elements in the sheet."));
		return;
	}

	const auto plan = ElementAutoNumSchemeCommand::assignPlan(m_project, title, selected);
	QVector<RenumberPreviewDialog::LeftAlone> left;
	for (Element *el : plan.frozen) {
		left.append({el, tr("(frozen)")});
	}
	for (Element *el : plan.otherFormula) {
		left.append({el, tr("(other numbering)")});
	}
	const int replaceable = static_cast<int>(left.size());

	auto *cmd = ElementAutoNumSchemeCommand::assign(m_project, title, selected, false);
	if (!cmd && !replaceable) {
		QMessageBox::information(
					this, caption,
					tr("Nothing to apply: the selected elements already follow the “%1” "
					   "numbering, or take their name from another element.").arg(title));
		return;
	}

	const QString intro = cmd
			? tr("%n elements will receive the “%1” numbering, in sheet and position order.", "", static_cast<int>(plan.todo.size())).arg(title)
			: tr("No element can receive the “%1” numbering without replacing what it "
				 "already has.").arg(title);
	const auto answer = RenumberPreviewDialog::ask(
				this, caption, intro,
				cmd ? cmd->changes() : QVector<RenumberElementsCommand::ElementChange>(),
				left,
				replaceable
				? tr("Also replace the %n elements left as they are", "", replaceable)
				: QString());

	if (answer == RenumberPreviewDialog::Answer::Cancel) {
		delete cmd;
		return;
	}
	int applied = cmd ? static_cast<int>(cmd->changes().size()) : 0;
	if (answer == RenumberPreviewDialog::Answer::GoAndReplace) {
		delete cmd;
		cmd = ElementAutoNumSchemeCommand::assign(m_project, title, selected, true);
		applied = cmd ? static_cast<int>(cmd->changes().size()) : 0;
	}
	if (cmd) {
		m_project->undoStack()->push(cmd);
	}

	const int not_given = static_cast<int>(selected.size()) - applied;
	if (not_given > 0) {
		QMessageBox::information(
					this, caption,
					tr("%n elements numbered.", "", applied)
					+ QLatin1Char('\n')
					+ tr("%n selected elements were left as they are.", "", not_given));
	}
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
	ui->m_conductor_value_le->clear();
	ui->m_element_value_le->clear();
	ui->m_folio_value_le->clear();
	ui->m_conductor_next_le->clear();
	ui->m_element_next_le->clear();
	ui->m_folio_next_le->clear();
}

/**
	@brief AutoNumberingDockWidget::rowFor
	@return the combo/value/increase/next widgets that make up category's row.
*/
AutoNumberingDockWidget::Row AutoNumberingDockWidget::rowFor(AutoNumCategory category) const
{
	switch (category) {
		case AutoNumCategory::Conductor:
			return {ui->m_conductor_cb, ui->m_conductor_value_le,
				ui->m_conductor_increase_sb, ui->m_conductor_next_le};
		case AutoNumCategory::Element:
			return {ui->m_element_cb, ui->m_element_value_le,
				ui->m_element_increase_sb, ui->m_element_next_le};
		case AutoNumCategory::Folio:
			return {ui->m_folio_cb, ui->m_folio_value_le,
				ui->m_folio_increase_sb, ui->m_folio_next_le};
	}
	return {nullptr, nullptr, nullptr, nullptr};
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
		disconnect(m_project, &QETProject::conductorAutoNumChanged, this, &AutoNumberingDockWidget::conductorAutoNumChanged);
		disconnect(m_project, &QETProject::conductorAutoNumRemoved, this, &AutoNumberingDockWidget::conductorAutoNumChanged);
		disconnect(m_project, &QETProject::conductorAutoNumAdded, this, &AutoNumberingDockWidget::conductorAutoNumChanged);
		disconnect(m_project_view, &ProjectView::diagramActivated, this, &AutoNumberingDockWidget::setConductorActive);
		disconnect(m_project_view, &ProjectView::diagramActivated, this, &AutoNumberingDockWidget::followSelectionOf);
		followSelectionOf(nullptr);

		//Element Signals
		disconnect(m_project, &QETProject::elementAutoNumRemoved, this, &AutoNumberingDockWidget::elementAutoNumChanged);
		disconnect(m_project, &QETProject::elementAutoNumAdded, this, &AutoNumberingDockWidget::elementAutoNumChanged);
	
			//Folio Signals
		disconnect(m_project, &QETProject::folioAutoNumRemoved, this, qOverload<>(&AutoNumberingDockWidget::folioAutoNumChanged));
		disconnect(m_project, &QETProject::folioAutoNumAdded, this, qOverload<>(&AutoNumberingDockWidget::folioAutoNumChanged));
		disconnect(m_project, &QETProject::defaultTitleBlockPropertiesChanged, this, &AutoNumberingDockWidget::setActive);
	
			//Conductor, Element and Folio Signals
		disconnect(m_project, &QETProject::autoNumContextUpdated,
			   this, &AutoNumberingDockWidget::refreshValueFields);
		disconnect(m_project, &QETProject::destroyed,
			   this, &AutoNumberingDockWidget::projectClosed);
	}
	
	m_project = project;
	m_project_view = projectview;
	this->setEnabled(true);

	connect(m_project, &QETProject::conductorAutoNumChanged, this, &AutoNumberingDockWidget::conductorAutoNumChanged);
	connect(m_project, &QETProject::conductorAutoNumRemoved, this, &AutoNumberingDockWidget::conductorAutoNumChanged);
	connect(m_project, &QETProject::conductorAutoNumAdded, this, &AutoNumberingDockWidget::conductorAutoNumChanged);
	connect(m_project_view, &ProjectView::diagramActivated, this, &AutoNumberingDockWidget::setConductorActive);
	connect(m_project_view, &ProjectView::diagramActivated, this, &AutoNumberingDockWidget::followSelectionOf);
	followSelectionOf(m_project_view->currentDiagram());

	//Element Signals
	connect(m_project, &QETProject::elementAutoNumRemoved, this, &AutoNumberingDockWidget::elementAutoNumChanged);
	connect(m_project, &QETProject::elementAutoNumAdded, this, &AutoNumberingDockWidget::elementAutoNumChanged);

		//Folio Signals
	connect(m_project, &QETProject::folioAutoNumRemoved, this, qOverload<>(&AutoNumberingDockWidget::folioAutoNumChanged));
	connect(m_project, &QETProject::folioAutoNumAdded, this, qOverload<>(&AutoNumberingDockWidget::folioAutoNumChanged));
	connect(m_project, &QETProject::defaultTitleBlockPropertiesChanged, this, &AutoNumberingDockWidget::setActive);

		//Conductor, Element and Folio Signals
	connect(m_project, &QETProject::autoNumContextUpdated,
		this, &AutoNumberingDockWidget::refreshValueFields);
	connect(m_project, &QETProject::destroyed,
		this, &AutoNumberingDockWidget::projectClosed);

		//Set Combobox Context
	setContext();
	
	ShortcutManager::instance().registerAction(ui->m_configure_pb, "autonum.configure",
						    tr("Auto Numbering"), Qt::CTRL | Qt::SHIFT | Qt::Key_P);
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

		//The combo boxes have just been repopulated, so the value fields next
		//to them are showing whatever the previous project left there.
	refreshRow(AutoNumCategory::Conductor);
	refreshRow(AutoNumCategory::Element);
	refreshRow(AutoNumCategory::Folio);

	this->setActive();
}

/**
	@brief AutoNumberingDockWidget::followSelectionOf
	Enable the apply button only while elements are selected in the folio
	shown: it gives a numbering to the selected elements, and with none
	selected there is nothing for it to do.
	@param view : the folio now shown, nullptr for none
*/
void AutoNumberingDockWidget::followSelectionOf(DiagramView *view)
{
	Diagram *diagram = view ? view->diagram() : nullptr;
	if (m_selection_diagram) {
		disconnect(m_selection_diagram, &QGraphicsScene::selectionChanged,
				   this, &AutoNumberingDockWidget::updateApplyEnabled);
	}
	m_selection_diagram = diagram;
	if (m_selection_diagram) {
		connect(m_selection_diagram, &QGraphicsScene::selectionChanged,
				this, &AutoNumberingDockWidget::updateApplyEnabled);
	}
	updateApplyEnabled();
}

/**
	@brief AutoNumberingDockWidget::updateApplyEnabled
	The apply button needs a numbering to give and elements selected.
*/
void AutoNumberingDockWidget::updateApplyEnabled()
{
	if (!m_element_apply_pb) {
		return;
	}
	bool elements_selected = false;
	if (m_selection_diagram) {
		const auto items = m_selection_diagram->selectedItems();
		for (QGraphicsItem *item : items) {
			if (qgraphicsitem_cast<Element *>(item)) {
				elements_selected = true;
				break;
			}
		}
	}
	m_element_apply_pb->setEnabled(
				elements_selected && m_project && !m_project->isReadOnly()
				&& ui->m_element_cb->count() > 0);
	m_element_apply_pb->setToolTip(
				elements_selected
				? tr("Apply this numbering to the selected elements")
				: tr("First select elements in the sheet"));
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
	refreshRow(AutoNumCategory::Conductor);
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
	refreshRow(AutoNumCategory::Element);
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

	if (m_project_view && m_project_view->currentDiagram()) {
		Diagram *diagram = m_project_view->currentDiagram()->diagram();
		TitleBlockProperties old_properties = diagram->border_and_titleblock.exportTitleBlock();
		TitleBlockProperties new_properties = old_properties;
		new_properties.auto_page_num = ip.auto_page_num;
		new_properties.folio = ip.folio;
		if (new_properties != old_properties)
			diagram->undoStack().push(new ChangeTitleBlockCommand(diagram, old_properties, new_properties));
	}
	refreshRow(AutoNumCategory::Folio);
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
	resetAutoNum(ui->m_conductor_cb, AutoNumCategory::Conductor);
}

void AutoNumberingDockWidget::on_m_element_reset_start_pb_clicked()
{
	resetAutoNum(ui->m_element_cb, AutoNumCategory::Element);
}

void AutoNumberingDockWidget::on_m_folio_reset_start_pb_clicked()
{
	resetAutoNum(ui->m_folio_cb, AutoNumCategory::Folio);
}

void AutoNumberingDockWidget::on_m_conductor_value_le_editingFinished()
{
	applyValueField(ui->m_conductor_cb, ui->m_conductor_value_le, AutoNumCategory::Conductor);
}

void AutoNumberingDockWidget::on_m_element_value_le_editingFinished()
{
	applyValueField(ui->m_element_cb, ui->m_element_value_le, AutoNumCategory::Element);
}

void AutoNumberingDockWidget::on_m_folio_value_le_editingFinished()
{
	applyValueField(ui->m_folio_cb, ui->m_folio_value_le, AutoNumCategory::Folio);
}

void AutoNumberingDockWidget::on_m_conductor_increase_sb_valueChanged(int)
{
	applyIncreaseField(ui->m_conductor_cb, ui->m_conductor_increase_sb, AutoNumCategory::Conductor);
}

void AutoNumberingDockWidget::on_m_element_increase_sb_valueChanged(int)
{
	applyIncreaseField(ui->m_element_cb, ui->m_element_increase_sb, AutoNumCategory::Element);
}

void AutoNumberingDockWidget::on_m_folio_increase_sb_valueChanged(int)
{
	applyIncreaseField(ui->m_folio_cb, ui->m_folio_increase_sb, AutoNumCategory::Folio);
}

/**
	@brief AutoNumberingDockWidget::contextFor
	@return the numerotation context named by combo_box, for category
*/
NumerotationContext AutoNumberingDockWidget::contextFor(QComboBox *combo_box, AutoNumCategory category) const
{
	const QString key = combo_box->currentText();
	switch (category) {
		case AutoNumCategory::Conductor: return m_project->conductorAutoNum(key);
		case AutoNumCategory::Element:   return m_project->elementAutoNum(key);
		case AutoNumCategory::Folio:     return m_project->folioAutoNum(key);
	}
	return NumerotationContext();
}

/**
	@brief AutoNumberingDockWidget::storeContext
	Write context back under the name selected in combo_box and flag the
	project as modified -- without that last step the change is not saved
	and the user is never asked to save it on close.
*/
void AutoNumberingDockWidget::storeContext(QComboBox *combo_box, AutoNumCategory category, const NumerotationContext &context)
{
	const QString key = combo_box->currentText();
	switch (category) {
		case AutoNumCategory::Conductor: m_project->addConductorAutoNum(key, context); break;
		case AutoNumCategory::Element:   m_project->addElementAutoNum(key, context);   break;
		case AutoNumCategory::Folio:     m_project->addFolioAutoNum(key, context);     break;
	}
	m_project->setModified(true);
}

/**
	@brief AutoNumberingDockWidget::counterIndex
	@return the index of the part the value field shows: the last one that
	actually progresses, i.e. the least significant digit of the number.
	-1 when the context has no progressing part at all.
*/
int AutoNumberingDockWidget::counterIndex(const NumerotationContext &context)
{
	for (int i = context.size() - 1 ; i >= 0 ; --i)
	{
		const QString type = context.itemAt(i).at(0);
		if (type == QLatin1String("unit")
				|| type == QLatin1String("ten")
				|| type == QLatin1String("hundred")
				|| type == QLatin1String("unitfolio")
				|| type == QLatin1String("tenfolio")
				|| type == QLatin1String("hundredfolio")
				|| type == QLatin1String("wrap")
				|| type == QLatin1String("alpha"))
			return i;
	}
	return -1;
}

/**
	@brief AutoNumberingDockWidget::refreshValueFields
	Re-read all three value fields from the project. Called whenever a
	numerotation context's values change, which includes every element or
	conductor that consumes the next number -- without this the field only
	caught up when the user re-picked a rule from the combo box, because
	the combo's activated() signal fires on user interaction alone.
*/
void AutoNumberingDockWidget::refreshValueFields()
{
		//Leave alone a row the user is typing in: numbering an element
		//refreshes all three rows, and overwriting a half-typed value or
		//increment under the cursor is worse than showing it a moment out
		//of date. Only this automatic path skips; an explicit refresh after
		//a reset or an edit still writes, so the row always ends up
		//canonical. The next-value preview has no such guard: it is
		//read-only, so there is nothing a refresh could clobber.
	for (AutoNumCategory category : {AutoNumCategory::Conductor,
					 AutoNumCategory::Element,
					 AutoNumCategory::Folio})
	{
		const Row row = rowFor(category);
		if (!row.value->hasFocus() && !row.increase->hasFocus())
			refreshRow(category);
	}
}

/**
	@brief AutoNumberingDockWidget::refreshValueField
	Show the current value of the selected context's counter, so the field
	always reflects where the numbering has actually got to.
*/
void AutoNumberingDockWidget::refreshValueField(QComboBox *combo_box, QLineEdit *line_edit, AutoNumCategory category)
{
	if (!m_project || combo_box->currentText().isEmpty())
	{
		line_edit->clear();
		line_edit->setEnabled(false);
		return;
	}

	const NumerotationContext context = contextFor(combo_box, category);
	const int index = counterIndex(context);
	line_edit->setEnabled(index >= 0);
	line_edit->setText(index >= 0 ? context.itemAt(index).at(1) : QString());
}

/**
	@brief AutoNumberingDockWidget::applyValueField
	Write the value typed in line_edit to the counter it displays. An empty
	field is treated as "no change" rather than as an empty value, so
	clearing the box by accident cannot wipe the counter.
*/
void AutoNumberingDockWidget::applyValueField(QComboBox *combo_box, QLineEdit *line_edit, AutoNumCategory category)
{
	if (!m_project || combo_box->currentText().isEmpty())
		return;

	NumerotationContext context = contextFor(combo_box, category);
	const int index = counterIndex(context);
	if (index < 0)
		return;

	const QString typed = line_edit->text();
	if (typed.isEmpty() || typed == context.itemAt(index).at(1))
	{
		refreshRow(category);
		return;
	}

	context.replaceValue(index, typed);
		//An element counter put back on numbers in use: ask first
	if (category == AutoNumCategory::Element
			&& !CounterWarning::confirm(this, m_project, combo_box->currentText(), context)) {
		refreshRow(category);
		return;
	}
	storeContext(combo_box, category, context);
	refreshRow(category);
}

/**
	@brief AutoNumberingDockWidget::refreshIncreaseField
	Show the counter's current step size (bug #331: previously only
	reachable from the full configuration dialog, via "Configurer").
*/
void AutoNumberingDockWidget::refreshIncreaseField(QComboBox *combo_box, QSpinBox *increase_sb, AutoNumCategory category)
{
		//QSpinBox::setValue() emits valueChanged() even when called
		//programmatically. Without blocking it, this refresh would
		//immediately re-trigger on_..._increase_sb_valueChanged() ->
		//applyIncreaseField() -> storeContext() -> the project's
		//autoNumContextUpdated signal -> refreshValueFields() -> back here.
	const QSignalBlocker blocker(increase_sb);
	if (!m_project || combo_box->currentText().isEmpty())
	{
		increase_sb->setEnabled(false);
		increase_sb->setValue(increase_sb->minimum());
		return;
	}

	const NumerotationContext context = contextFor(combo_box, category);
	const int index = counterIndex(context);
	increase_sb->setEnabled(index >= 0);
	increase_sb->setValue(index >= 0 ? context.itemAt(index).at(2).toInt()
					 : increase_sb->minimum());
}

/**
	@brief AutoNumberingDockWidget::applyIncreaseField
	Write the spin box's step size to the counter it displays (bug #331).
*/
void AutoNumberingDockWidget::applyIncreaseField(QComboBox *combo_box, QSpinBox *increase_sb, AutoNumCategory category)
{
	if (!m_project || combo_box->currentText().isEmpty())
		return;

	NumerotationContext context = contextFor(combo_box, category);
	const int index = counterIndex(context);
	if (index < 0)
		return;

	if (increase_sb->value() == context.itemAt(index).at(2).toInt())
		return;

	context.replaceIncrease(index, increase_sb->value());
	storeContext(combo_box, category, context);
	refreshRow(category);
}

/**
	@brief AutoNumberingDockWidget::refreshNextField
	Show what this counter will read after one more step (bug #331: "visualiser
	la prochaine numérotation qui sera appliquée"). Advances a copy of the
	whole context through NumerotationContextCommands -- the same engine the
	Suivant button in the full configuration dialog uses to step a context --
	so wrap-and-carry into this part from a following part, or out of it into
	a preceding one, comes out identical to what will actually happen when the
	number is next consumed.
*/
void AutoNumberingDockWidget::refreshNextField(QComboBox *combo_box, QLineEdit *next_edit, AutoNumCategory category)
{
	if (!m_project || combo_box->currentText().isEmpty())
	{
		next_edit->clear();
		next_edit->setEnabled(false);
		return;
	}

	const NumerotationContext context = contextFor(combo_box, category);
	const int index = counterIndex(context);
	if (index < 0)
	{
		next_edit->clear();
		next_edit->setEnabled(false);
		return;
	}

	Diagram *diagram = (m_project_view && m_project_view->currentDiagram())
				    ? m_project_view->currentDiagram()->diagram()
				    : nullptr;
	NumerotationContextCommands ncc(context, diagram);
	const NumerotationContext next_context = ncc.next();

	next_edit->setEnabled(true);
	next_edit->setText(NumerotationContext::formatValue(next_context.itemAt(index)));
}

/**
	@brief AutoNumberingDockWidget::refreshRow
	Refresh a category's value, increment and next-value preview together --
	every call site that used to refresh just the value field needs the
	other two kept in step with it as well.
*/
void AutoNumberingDockWidget::refreshRow(AutoNumCategory category)
{
	const Row row = rowFor(category);
	refreshValueField(row.combo, row.value, category);
	refreshIncreaseField(row.combo, row.increase, category);
	refreshNextField(row.combo, row.next, category);
}

/**
	@brief AutoNumberingDockWidget::resetAutoNum
	Reset the numerotation context currently selected in combo_box back to
	a per-type starting value, then write it back. Does nothing if no
	context is selected.

	Only parts that actually progress are touched: folio-anchored numeric
	types go back to their own stored initialvalue, plain numeric types go
	back to "1", a wrap part goes back to "0" because a modulo counter
	cycles over [0, modulus) -- a PLC card addressed %IX0.0..%IX0.31 starts
	at 0, not 1 -- and alpha goes back to "a". Non-incrementing types
	(string, plant, locmach, idfolio, folio, elementline, elementcolumn,
	elementprefix) are left alone: there is no "start" for them distinct
	from the fixed or contextual value the user configured.
*/
void AutoNumberingDockWidget::resetAutoNum(QComboBox *combo_box, AutoNumCategory category)
{
	if (!m_project || combo_box->currentText().isEmpty())
		return;

	NumerotationContext context = contextFor(combo_box, category);

	for (int i = 0 ; i < context.size() ; ++i)
	{
		const QStringList item = context.itemAt(i);
		const QString &type = item.at(0);
		if (type == QLatin1String("unitfolio")
				|| type == QLatin1String("tenfolio")
				|| type == QLatin1String("hundredfolio"))
			context.replaceValue(i, item.size() > 3 ? item.at(3) : QStringLiteral("1"));
		else if (type == QLatin1String("unit")
				|| type == QLatin1String("ten")
				|| type == QLatin1String("hundred"))
			context.replaceValue(i, QStringLiteral("1"));
		else if (type == QLatin1String("wrap"))
			context.replaceValue(i, QStringLiteral("0"));
		else if (type == QLatin1String("alpha"))
			context.replaceValue(i, QStringLiteral("a"));
	}

		//Back to the first number, over numbers in use: ask first
	if (category == AutoNumCategory::Element
			&& !CounterWarning::confirm(this, m_project, combo_box->currentText(), context)) {
		return;
	}

	storeContext(combo_box, category, context);
	refreshRow(category);
}
