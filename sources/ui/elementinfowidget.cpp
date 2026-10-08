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
#include "elementinfowidget.h"
#include "../qet.h"
#include <QCheckBox>
#include <QComboBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include "../autoNum/elementautonumschemecommand.h"
#include "../undocommand/freezeelementlabelcommand.h"
#include "../diagram.h"
#include "../materiallist/materiallist.h"
#include "../materiallist/materialselectiondialog.h"
#include "../qetapp.h"
#include "../qetgraphicsitem/element.h"
#include "../qetmessagebox.h"
#include "../dataBase/projectdatabase.h"
#include "../qetinformation.h"
#include "../qetproject.h"
#include "../ui/projectpropertiesdialog.h"
#include "../ui_elementinfowidget.h"
#include "../undocommand/changeelementinformationcommand.h"
#include "customelementinfopartwidget.h"
#include "elementinfopartwidget.h"

/**
	@brief ElementInfoWidget::ElementInfoWidget
	Constructor
	@param elmt element to edit information
	@param parent parent widget
*/
ElementInfoWidget::ElementInfoWidget(Element *elmt, QWidget *parent) :
	AbstractElementPropertiesEditorWidget(parent),
	ui(new Ui::ElementInfoWidget),
	m_first_activation (false)
{
	ui->setupUi(this);
	setElement(elmt);
}

/**
	@brief ElementInfoWidget::~ElementInfoWidget
	Destructor
*/
ElementInfoWidget::~ElementInfoWidget()
{
	qDeleteAll(m_eipw_list);
	qDeleteAll(m_custom_eipw_list);
	delete ui;
}

/**
	@brief ElementInfoWidget::setElement
	Set element to be the edited element
	@param element
*/
void ElementInfoWidget::setElement(Element *element)
{
	if (m_element == element) return;

	if (m_element)
		disconnect(m_element.data(), &Element::elementInfoChange, this, &ElementInfoWidget::elementInfoChange);

	m_element = element;
	updateUi();

	const auto formula_info_widget = infoPartWidgetForKey(QETInformation::ELMT_FORMULA);
	const auto label_info_widget = infoPartWidgetForKey(QETInformation::ELMT_LABEL);

	if (formula_info_widget && label_info_widget)
	{
		if (formula_info_widget->text().isEmpty())
			label_info_widget->setEnabled(true);
		else
			label_info_widget->setDisabled(true);

		connect(formula_info_widget, &ElementInfoPartWidget::textChanged, this, [label_info_widget](const QString text)
		{
			label_info_widget->setEnabled(text.isEmpty()? true : false);
		});
	}

	connect(m_element.data(), &Element::elementInfoChange, this, &ElementInfoWidget::elementInfoChange);
}

/**
	@brief ElementInfoWidget::apply
	Apply the new information with a new undo command (got with method associatedUndo)
	pushed to the stack of element project.
*/
void ElementInfoWidget::apply()
{
	if (auto undo = associatedUndo())
		m_element -> diagram() -> undoStack().push(undo);
}

/**
	@brief ElementInfoWidget::associatedUndo
	If the edited info is different of the actual element info,
	return a QUndoCommand with the change.
	If no change return nullptr;
	@return
*/
QUndoCommand* ElementInfoWidget::associatedUndo() const
{
	auto new_info = currentInfo();
	const auto old_info = m_element -> elementInformations();

	const bool was_frozen = m_element->isFreezeLabel();
	const bool want_frozen = m_freeze_cb ? m_freeze_cb->isChecked() : was_frozen;

		//A numbering chosen in the list is given to the element the way
		//a placed one gets it: its formula, and the next number of the
		//numbering, which moves on. That is a command of its own, first,
		//so the other edited fields are changed on top of its result.
	const QString chosen = chosenScheme();
	const bool assign_wanted = !chosen.isEmpty()
			&& chosen != followedScheme()
			&& m_element->diagram();
		//A number picked among the free ones, for an element which keeps its numbering
	const bool number_wanted = !assign_wanted && m_number_cb && m_number_row
			&& !m_number_row->isHidden() && m_number_cb->isEnabled()
			&& m_number_cb->currentData().toInt() > 0
			&& m_number_cb->currentData().toInt() != m_number_current
			&& m_element->diagram();

	if (!assign_wanted && !number_wanted && was_frozen == want_frozen)
	{
		if (old_info != new_info)
			return (new ChangeElementInformationCommand(m_element, old_info, new_info));
		return nullptr;
	}

	auto *macro = new QUndoCommand(
				QObject::tr("Modifier les informations de l'élément : %1")
				.arg(m_element->name()));
	DiagramContext base_info = old_info;
	bool state_frozen = was_frozen;

	if (assign_wanted || number_wanted)
	{
		RenumberElementsCommand *assign = assign_wanted
				? ElementAutoNumSchemeCommand::assign(
					  m_element->diagram()->project(), chosen,
					  {m_element.data()}, true, nullptr, macro)
				: ElementAutoNumSchemeCommand::assignNumber(
					  m_element->diagram()->project(), m_element.data(),
					  m_number_cb->currentData().toInt(), nullptr, macro);
		if (assign && !assign->changes().isEmpty())
		{
			const auto &change = assign->changes().first();
			base_info = change.new_infos;
			state_frozen = change.new_frozen;
			for (const QString &key : {QETInformation::ELMT_FORMULA,
									   QETInformation::ELMT_FORMULA_ID,
									   QETInformation::ELMT_LABEL}) {
				if (base_info.contains(key)) {
					new_info.addValue(key, base_info.value(key), key != QETInformation::ELMT_FORMULA_ID);
				}
			}
		}
	}

	if (base_info != new_info) {
		new ChangeElementInformationCommand(m_element, base_info, new_info, macro);
	}
	if (state_frozen != want_frozen) {
		new FreezeElementLabelCommand(m_element, state_frozen, want_frozen, macro);
	}

	if (macro->childCount() == 0) {
		delete macro;
		return nullptr;
	}
	return macro;
}

/**
	@brief ElementInfoWidget::setLiveEdit
	@param live_edit true : enable the live edit mode, false disable
	@return always true;
*/
bool ElementInfoWidget::setLiveEdit(bool live_edit)
{
	if (m_live_edit == live_edit) return true;
	m_live_edit = live_edit;

	if (m_live_edit)
		enableLiveEdit();
	else
		disableLiveEdit();

	return true;
}

/**
	@brief ElementInfoWidget::event
	Reimplemented from QWidget::event
	Only give focus to the first line edit at first activation.
	After send the event to QWidget.
	@param event
	@return
*/
bool ElementInfoWidget::event(QEvent *event)
{
	if (m_first_activation)
	{
		if (event -> type() == QEvent::WindowActivate || event -> type() == QEvent::Show)
		{
			QTimer::singleShot(250, this, &ElementInfoWidget::firstActivated);
			m_first_activation = false;
		}
	}
	return(QWidget::event(event));
}

/**
	@brief ElementInfoWidget::enableLiveEdit
	Enable the live edit mode
*/
void ElementInfoWidget::enableLiveEdit()
{
	for (ElementInfoPartWidget *eipw : m_eipw_list)
		connect(eipw, &ElementInfoPartWidget::textChanged, this, &ElementInfoWidget::apply);
	connect(ui->m_auto_num_locked_cb, &QCheckBox::clicked, this, &ElementInfoWidget::apply);

	if (m_potential_isolating_cb) {
		connect(m_potential_isolating_cb, &QCheckBox::clicked, this, &ElementInfoWidget::apply);
	}
	if (m_exclude_from_bom_cb) {
		connect(m_exclude_from_bom_cb, &QCheckBox::clicked, this, &ElementInfoWidget::apply);
	}
	if (m_freeze_cb) {
		connect(m_freeze_cb, &QCheckBox::clicked, this, &ElementInfoWidget::apply);
	}
	if (m_number_cb) {
		connect(m_number_cb, QOverload<int>::of(&QComboBox::activated), this, &ElementInfoWidget::apply);
	}
}

/**
	@brief ElementInfoWidget::disableLiveEdit
	disable the live edit mode
*/
void ElementInfoWidget::disableLiveEdit()
{
	for (ElementInfoPartWidget *eipw : m_eipw_list)
		disconnect(eipw, &ElementInfoPartWidget::textChanged, this, &ElementInfoWidget::apply);
	disconnect(ui->m_auto_num_locked_cb, &QCheckBox::clicked, this, &ElementInfoWidget::apply);

	if (m_potential_isolating_cb) {
		disconnect(m_potential_isolating_cb, &QCheckBox::clicked, this, &ElementInfoWidget::apply);
	}
	if (m_freeze_cb) {
		disconnect(m_freeze_cb, &QCheckBox::clicked, this, &ElementInfoWidget::apply);
	}
	if (m_number_cb) {
		disconnect(m_number_cb, QOverload<int>::of(&QComboBox::activated), this, &ElementInfoWidget::apply);
	}
	if (m_exclude_from_bom_cb) {
		disconnect(m_exclude_from_bom_cb, &QCheckBox::clicked, this, &ElementInfoWidget::apply);
	}
}

/**
	@brief ElementInfoWidget::buildInterface
	Build the widget
*/
void ElementInfoWidget::buildInterface()
{
	QStringList keys;
	if (m_element.data()->elementData().m_type == ElementData::Terminal) {
		keys = QETInformation::terminalElementInfoKeys();
	 } else {
		keys = QETInformation::elementInfoKeys();
	}

		//"exclude_from_bom" is part of elementInfoKeys() because the project
		//database builds the element_info table from that list, but it is not
		//a free-text property: it already has its own check box below. Without
		//this it also gets a generic edit row, and since translatedInfoKey()
		//has no entry for it that row carries no label at all - an anonymous
		//line that currentInfo() then fills with "true"/"false".
	keys.removeAll(QStringLiteral("exclude_from_bom"));

	for (auto str : keys)
	{
		ElementInfoPartWidget *eipw = new ElementInfoPartWidget(str, QETInformation::translatedInfoKey(str), this);
		ui->scroll_vlayout->addWidget(eipw);
		m_eipw_list << eipw;
	}

	setupMaterialButtons();
	setupSchemeRow();
	setupNumberRow();
	setupFreezeRow();

	m_add_custom_property_btn = new QPushButton(tr("Ajouter une propriété personnalisée"), this);
	connect(m_add_custom_property_btn, &QPushButton::clicked, this, [this]() { addCustomProperty(); });
	ui->scroll_vlayout->addWidget(m_add_custom_property_btn);

	ui->scroll_vlayout->addStretch();

	// Existing potential isolating checkbox
	m_potential_isolating_cb = new QCheckBox(tr("Séparation de potentiel"), this);
	m_potential_isolating_cb->setStyleSheet(QStringLiteral("margin: 5px; font-weight: bold;"));

	// English: Initialize and style the BOM exclusion checkbox
	m_exclude_from_bom_cb = new QCheckBox(tr("Exclure de la nomenclature"), this);
	m_exclude_from_bom_cb->setStyleSheet(QStringLiteral("margin: 5px; font-weight: bold;"));

	if (QVBoxLayout *mainLayout = qobject_cast<QVBoxLayout*>(this->layout())) {
		mainLayout->insertWidget(1, m_potential_isolating_cb);
		// English: Insert the new checkbox into the main vertical layout
		mainLayout->insertWidget(2, m_exclude_from_bom_cb);
	}

	// English: BOM exclusion applies to all elements, so it's always visible
	m_exclude_from_bom_cb->setVisible(true);

	// Show checkbox only if the element is a terminal
	if (m_element.data()->elementData().m_type == ElementData::Terminal) {
		ui->m_auto_num_locked_cb->setVisible(true);
		m_potential_isolating_cb->setVisible(true);
	} else {
		ui->m_auto_num_locked_cb->setVisible(false);
		m_potential_isolating_cb->setVisible(false);
	}
}
/**
	@brief ElementInfoWidget::predefinedKeys
	@return every key this widget already exposes a dedicated row for,
	whether through ElementInfoPartWidget (the ~40 ELMT_* keys) or one
	of the standalone checkboxes. Anything present in the element's
	informations but absent from this list is a user-defined custom
	property.
*/
QStringList ElementInfoWidget::predefinedKeys() const
{
	QStringList keys = (m_element.data()->elementData().m_type == ElementData::Terminal)
			? QETInformation::terminalElementInfoKeys()
			: QETInformation::elementInfoKeys();

	keys << QETInformation::ELMT_FORMULA_ID
		 << QStringLiteral("auto_num_locked")
		 << QStringLiteral("potential_isolating")
		 << QStringLiteral("exclude_from_bom");

	return keys;
}

/**
	@brief ElementInfoWidget::addCustomProperty
	Append a new user-defined key/value row to the widget.
	@param key initial key, left empty for a freshly added row
	@param value initial value
*/
void ElementInfoWidget::addCustomProperty(const QString &key, const QString &value)
{
	auto *widget = new CustomElementInfoPartWidget(key, value, this);

	const int insert_index = ui->scroll_vlayout->indexOf(m_add_custom_property_btn);
	ui->scroll_vlayout->insertWidget(insert_index >= 0 ? insert_index : ui->scroll_vlayout->count(), widget);
	m_custom_eipw_list << widget;

	connect(widget, &CustomElementInfoPartWidget::removeRequested, this, &ElementInfoWidget::removeCustomProperty);
	connect(widget, &CustomElementInfoPartWidget::changed, this, [this]() {
		if (m_live_edit) apply();
	});

	if (key.isEmpty()) {
		widget->setFocus();
	}
}

/**
	@brief ElementInfoWidget::removeCustomProperty
	Remove a user-defined key/value row.
	@param widget the row to remove
*/
void ElementInfoWidget::removeCustomProperty(CustomElementInfoPartWidget *widget)
{
	if (!m_custom_eipw_list.removeOne(widget))
		return;

	ui->scroll_vlayout->removeWidget(widget);
	widget->deleteLater();

	if (m_live_edit) apply();
}

/**
	@brief ElementInfoWidget::infoPartWidgetForKey
	@param key
	@return the ElementInfoPartWidget with key key,
	if not found return nullptr;
*/
ElementInfoPartWidget *ElementInfoWidget::infoPartWidgetForKey(const QString &key) const
{
	for (const auto &eipw : std::as_const(m_eipw_list))
	{
		if (eipw->key() == key)
			return eipw;
	}

	return nullptr;
}

/**
	@brief ElementInfoWidget::setupMaterialButtons
	Show the "..." button opening the material file on the description row
	of each block: one for the main article, one per auxiliary article
	which really has a description row.

	Only that row carries the button: the same window fills the whole
	block, so repeating it on every field of the block would just offer
	five ways to do the same thing.
*/
void ElementInfoWidget::setupMaterialButtons()
{
	auto register_button = [this](ElementInfoPartWidget *eipw, int block)
	{
		eipw->setMaterialButtonVisible(true);
		connect(eipw, &ElementInfoPartWidget::materialButtonClicked, this,
				[this, block]() { materialFromFile(block); });
	};

		//Main article. A terminal has no description row: its article
		//number is the one which identifies it, so the button sits there.
	if (ElementInfoPartWidget *eipw = infoPartWidgetForKey(QETInformation::ELMT_DESCRIPTION)) {
		register_button(eipw, 0);
	} else if (ElementInfoPartWidget *eipw = infoPartWidgetForKey(QETInformation::ELMT_DESIGNATION)) {
		register_button(eipw, 0);
	}

		//Auxiliary articles 1 to 4.
	const QStringList auxiliary_keys = {
		QETInformation::ELMT_DESCRIPTION_AUX1,
		QETInformation::ELMT_DESCRIPTION_AUX2,
		QETInformation::ELMT_DESCRIPTION_AUX3,
		QETInformation::ELMT_DESCRIPTION_AUX4
	};

	for (int block = 1; block <= 4 && block <= auxiliary_keys.size(); ++block)
	{
		if (ElementInfoPartWidget *eipw = infoPartWidgetForKey(auxiliary_keys.at(block - 1))) {
			register_button(eipw, block);
		}
	}
}

/**
	@brief ElementInfoWidget::materialFromFile
	Open the material file, let the user pick an article, and write it
	into the block the button belongs to.
	@param block 0 for the main article, 1 to 4 for an auxiliary article
*/
void ElementInfoWidget::materialFromFile(int block)
{
		//Nothing configured yet: the standard location is used, the same
		//one the preferences offer to create, so that the catalogue can be
		//reached from here without asking for a file to be saved first.
	QString path = MaterialList::configuredPath();
	if (path.isEmpty())
	{
		path = MaterialList::defaultPath();
		MaterialList::setConfiguredPath(path);
	}

		//A missing or empty file has no entry to pick: offer to write the
		//header line instead of opening an empty window.
	if (MaterialList::isEmptyFile(path))
	{
		const auto answer = QET::QetMessageBox::question(
			this,
			tr("Liste de matériaux absente"),
			tr("Aucun fichier de liste de matériaux n'existe à cet emplacement :\n"
			   "%1\n\nLe créer ?", "message asking to create the material file").arg(path),
			QMessageBox::Yes | QMessageBox::No,
			QMessageBox::Yes);

		if (answer != QMessageBox::Yes) {
			return;
		}

		QString error;
		if (!MaterialList::createFile(path, &error))
		{
			QET::QetMessageBox::critical(this,
										 tr("Création impossible"),
										 tr("Impossible de créer le fichier :\n%1\n%2")
											.arg(path, error));
			return;
		}
	}

		//The catalogue opens on the whole list : the search field starts
		//empty, whatever the block already holds.
	MaterialSelectionDialog dialog(path, this);
	if (dialog.exec() == QDialog::Accepted) {
		applyMaterialRecord(dialog.selectedRecord(), block);
	}
}

/**
	@brief ElementInfoWidget::applyMaterialRecord
	Write a catalogue entry into the fields of one block.

	Only the fields of that block are touched: applied to an auxiliary
	article, the entry never reaches the main article.

	An empty cell of a column describing the article (MaterialList::
	isArticleBound) clears the field: keeping the manufacturer of the
	article picked before would describe a part which does not exist.
	An empty cell of any other column, and any column the file does not
	hold at all, leaves the field alone, so picking an entry describing
	only the order reference never wipes a comment.
	@param record the entry taken from the material file
	@param block 0 for the main article, 1 to 4 for an auxiliary article
*/
void ElementInfoWidget::applyMaterialRecord(const MaterialRecord &record, int block)
{
	if (record.values.isEmpty()) {
		return;
	}

		//Fill the fields without letting the live edit push one undo
		//command per line edit: the whole entry must be undoable at once.
	const bool live_edit = m_live_edit;
	if (live_edit) {
		disableLiveEdit();
	}

	for (const QString &column : MaterialList::columnsForBlock(block))
	{
			//A column the file does not hold says nothing about the
			//article: the field of the element stays as it is.
		if (!record.values.contains(column)) {
			continue;
		}

		const QString value = record.value(column);

			//An empty cell only matters for the columns describing the
			//article itself, where the previous value belongs to another
			//part. For the others (function, comment, quantity...) an
			//empty cell means the file has nothing to say about them.
		if (value.isEmpty() && !MaterialList::isArticleBound(column)) {
			continue;
		}

		const QString key = MaterialList::elementInfoKey(column, block);
		if (key.isEmpty() || !QETInformation::elementInfoKeys().contains(key)) {
			continue;
		}

		if (ElementInfoPartWidget *eipw = infoPartWidgetForKey(key)) {
			eipw->setText(value);
		}
	}

	if (live_edit) {
		enableLiveEdit();
	}

		//Outside of the live edit the fields only carry the article: it
		//is the properties window which applies them when the user presses
		//"Apply" (ElementPropertiesWidget::apply() takes the undo command
		//from these very fields). Applying right away would push the change
		//on the undo stack before he has decided anything, and "Cancel"
		//would give the fields back but never the element.
	if (live_edit) {
		apply();
	}
}

/**
	@brief ElementInfoWidget::updateSuggestions
	Offer, for each information, the values already used by the other
	elements of the project (supplier, manufacturer...), so they can be
	picked instead of typed again.
	The values come from the project database rather than from the
	diagrams, which already holds them in the element_info table.
*/
void ElementInfoWidget::updateSuggestions()
{
	Diagram *diagram = m_element ? m_element->diagram() : nullptr;
	QETProject *project = diagram ? diagram->project() : nullptr;
	if (!project || !project->dataBase()) {
		return;
	}

		//Only a column of element_info can be queried. The key is checked
		//against that list rather than trusted, because it becomes part of
		//the SQL text.
	const QStringList columns = QETInformation::elementInfoKeys();
	const QString uuid = m_element->uuid().toString();

	for (ElementInfoPartWidget *eipw : m_eipw_list)
	{
		const QString key = eipw->key();
			//A label identifies one element, suggesting the others is noise
		if (key == QETInformation::ELMT_LABEL || !columns.contains(key)) {
			continue;
		}

			//"Schneider" and "schneider" are offered once, spelled the way
			//most elements spell it: SQLite takes the bare column v from
			//the row that holds MAX(n).
		QStringList values;
		auto query = project->dataBase()->newQuery(QStringLiteral(
			"SELECT v, MAX(n) FROM ("
				"SELECT \"%1\" AS v, COUNT(*) AS n FROM element_info "
				"WHERE \"%1\" IS NOT NULL AND \"%1\" != '' "
				"AND element_uuid != '%2' "
				"GROUP BY \"%1\") "
			"GROUP BY v COLLATE NOCASE "
			"ORDER BY v COLLATE NOCASE").arg(key, uuid));
		while (query.next()) {
			values << query.value(0).toString();
		}
		eipw->setSuggestions(values);
	}
}

/**
	@brief ElementInfoWidget::updateUi
	fill information fetch in m_element_info to the
	corresponding line edit
*/
void ElementInfoWidget::updateUi()
{
	if (!m_ui_builded) {
		buildInterface();
		m_ui_builded = true;
	}
		//We disable live edit to avoid wrong undo when we fill the line edit with new text
	if (m_live_edit) disableLiveEdit();

	const auto element_info{m_element->elementInformations()};
	
	for (ElementInfoPartWidget *eipw : m_eipw_list) {
		eipw -> setText (element_info[eipw->key()].toString());
	}
	refreshSchemeRow();
	if (m_freeze_cb) {
		m_freeze_cb->setChecked(m_element->isFreezeLabel());
		updateFreezeRow();
	}
	refreshNumberRow();
	updateSuggestions();

	// Rebuild the custom-property rows to match whatever
	// user-defined keys this element currently carries.
	while (!m_custom_eipw_list.isEmpty()) {
		CustomElementInfoPartWidget *w = m_custom_eipw_list.takeLast();
		ui->scroll_vlayout->removeWidget(w);
		delete w;
	}
	const auto known_keys = predefinedKeys();
	for (const QString &key : element_info.keys()) {
		if (!known_keys.contains(key)) {
			addCustomProperty(key, element_info[key].toString());
		}
	}

	// Load the lock status for auto numbering
	if (m_element->elementData().m_type == ElementData::Terminal) {
		QString lock_value = element_info.value(QStringLiteral("auto_num_locked")).toString();
		ui->m_auto_num_locked_cb->setChecked(QET::infoFlagIsTrue(lock_value));

		// English: Load the potential isolating status from the element information mapping
		if (m_potential_isolating_cb) {
			QString isolating_value = element_info.value(QStringLiteral("potential_isolating")).toString();
			m_potential_isolating_cb->setChecked(QET::infoFlagIsTrue(isolating_value));
		}
	}
	// English: Load the BOM exclusion status from the element information mapping
	if (m_exclude_from_bom_cb) {
		QString exclude_bom_value = element_info.value(QStringLiteral("exclude_from_bom")).toString();
		m_exclude_from_bom_cb->setChecked(QET::infoFlagIsTrue(exclude_bom_value));
	}

	if (m_live_edit) {
		enableLiveEdit();
	}
}

/**
	@brief ElementInfoWidget::currentInfo
	@return the info currently edited
*/
DiagramContext ElementInfoWidget::currentInfo() const
{
	DiagramContext info_;

	for (const auto &eipw : std::as_const(m_eipw_list))
	{
		if (!eipw->hasAcceptableInput())
			continue;

		//add value only if they're something to store
		if (!eipw->text().isEmpty())
		{
			QString txt{eipw->text()};
			//remove line feed and carriage return
			txt.remove(QStringLiteral("\r"));
			txt.remove(QStringLiteral("\n"));
			info_.addValue(eipw->key(), txt);
		}
	}

	for (const auto &custom : std::as_const(m_custom_eipw_list))
	{
		if (custom->hasValidKey() && !custom->value().isEmpty())
		{
			QString txt{custom->value()};
			txt.remove(QStringLiteral("\r"));
			txt.remove(QStringLiteral("\n"));
			info_.addValue(custom->key(), txt);
		}
	}

	// Save the auto numbering lock status
	if (m_element->elementData().m_type == ElementData::Terminal) {
		info_.addValue(QStringLiteral("auto_num_locked"), ui->m_auto_num_locked_cb->isChecked() ? QStringLiteral("true") : QStringLiteral("false"));

		if (m_potential_isolating_cb) {
			info_.addValue(QStringLiteral("potential_isolating"), m_potential_isolating_cb->isChecked() ? QStringLiteral("true") : QStringLiteral("false"));
		}
	}

	if (m_exclude_from_bom_cb) {
		info_.addValue(QStringLiteral("exclude_from_bom"), m_exclude_from_bom_cb->isChecked() ? QStringLiteral("true") : QStringLiteral("false"));
	}

		//The element keeps following its numbering scheme as long as its
		//formula is left as it is; a formula edited by hand is its own.
	const DiagramContext &elmt_info = m_element->elementInformations();
	if (elmt_info.contains(QETInformation::ELMT_FORMULA_ID)
			&& !info_.value(QETInformation::ELMT_FORMULA).toString().isEmpty()
			&& info_.value(QETInformation::ELMT_FORMULA) == elmt_info.value(QETInformation::ELMT_FORMULA)) {
		info_.addValue(QETInformation::ELMT_FORMULA_ID,
					   elmt_info.value(QETInformation::ELMT_FORMULA_ID), false);
	}
	return info_;
}
/**
	@brief ElementInfoWidget::setupSchemeRow
	Add above the formula the list of the numberings of the project, the
	formula of the element being the one of the numbering picked there:
	no formula to type, a numbering to choose, and a button to open the
	numberings without leaving this window.
*/
void ElementInfoWidget::setupSchemeRow()
{
	ElementInfoPartWidget *formula = infoPartWidgetForKey(QETInformation::ELMT_FORMULA);
	if (!formula || !m_element || !m_element->diagram()) {
		return;
	}

	m_scheme_row = new QWidget(this);
	auto *layout = new QHBoxLayout(m_scheme_row);
	layout->setContentsMargins(0, 0, 0, 0);
	layout->addWidget(new QLabel(tr("Numérotation automatique"), m_scheme_row));

	m_scheme_cb = new QComboBox(m_scheme_row);
	m_scheme_cb->setObjectName(QStringLiteral("m_scheme_cb"));
	m_scheme_cb->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
	layout->addWidget(m_scheme_cb, 1);

	auto *open = new QPushButton(tr("…"), m_scheme_row);
	open->setToolTip(tr("Ouvrir les numérotations d'éléments du projet"));
	open->setMaximumWidth(32);
	layout->addWidget(open);

	ui->scroll_vlayout->insertWidget(ui->scroll_vlayout->indexOf(formula), m_scheme_row);

		//The formula is shown, not typed: it comes from the numbering
	formula->setDisabled(true);

	connect(m_scheme_cb, QOverload<int>::of(&QComboBox::activated),
			this, &ElementInfoWidget::schemeChosen);
	connect(m_scheme_cb, QOverload<int>::of(&QComboBox::activated),
			this, &ElementInfoWidget::updateNumberRow);
	connect(open, &QPushButton::clicked, this, &ElementInfoWidget::openSchemePage);
}

/**
	@brief ElementInfoWidget::setupFreezeRow
	Add below the label a check box which freezes it: a frozen label is
	not touched by the numbering of elements, and its number is not given
	to another element.
*/
void ElementInfoWidget::setupFreezeRow()
{
	ElementInfoPartWidget *label = infoPartWidgetForKey(QETInformation::ELMT_LABEL);
	if (!label || !m_element) {
		return;
	}
	m_freeze_cb = new QCheckBox(tr("Figer le nom"), this);
	m_freeze_cb->setToolTip(tr("Un nom figé n'est pas changé par la numérotation automatique, "
							   "et son numéro n'est pas donné à un autre élément."));
	ui->scroll_vlayout->insertWidget(ui->scroll_vlayout->indexOf(label) + 1, m_freeze_cb);
	connect(m_freeze_cb, &QCheckBox::toggled, this, &ElementInfoWidget::updateNumberRow);
}

/**
	@brief ElementInfoWidget::updateFreezeRow
	Freezing means something for a label a formula gives: the box can be
	ticked when there is one, and always unticked, so that a frozen
	element can be given back.
*/
void ElementInfoWidget::updateFreezeRow()
{
	ElementInfoPartWidget *formula = infoPartWidgetForKey(QETInformation::ELMT_FORMULA);
	if (!m_freeze_cb || !formula) {
		return;
	}
	m_freeze_cb->setEnabled(!formula->text().isEmpty() || m_freeze_cb->isChecked());
	updateNumberRow();
}

/**
	@brief ElementInfoWidget::setupNumberRow
	Add under the numbering a list of the numbers the element may be given
	by hand: its own, then the free ones, each with the label it would give.
	For an element which follows a numbering whose numbers are one sequence.
*/
void ElementInfoWidget::setupNumberRow()
{
	if (!m_scheme_row || !m_element || !m_element->diagram()) {
		return;
	}
	m_number_row = new QWidget(this);
	auto *layout = new QHBoxLayout(m_number_row);
	layout->setContentsMargins(0, 0, 0, 0);
	layout->addWidget(new QLabel(tr("Numéro"), m_number_row));
	m_number_cb = new QComboBox(m_number_row);
	m_number_cb->setObjectName(QStringLiteral("m_number_cb"));
	m_number_cb->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
	m_number_cb->setToolTip(tr("Seuls les numéros libres sont proposés : un élément qui doit garder son "
							   "numéro réel peut le retrouver si personne ne l'a."));
	layout->addWidget(m_number_cb, 1);
	ui->scroll_vlayout->insertWidget(ui->scroll_vlayout->indexOf(m_scheme_row) + 1, m_number_row);
	m_number_row->hide();
	connect(m_number_cb, QOverload<int>::of(&QComboBox::activated), this, &ElementInfoWidget::numberChosen);
}

/**
	@brief ElementInfoWidget::refreshNumberRow
	Fill the list of numbers: the element's own, then the free ones. Hidden
	when the element follows no numbering, or one whose numbers are not one
	sequence.
*/
void ElementInfoWidget::refreshNumberRow()
{
	if (!m_number_cb || !m_element || !m_element->diagram()) {
		return;
	}
	QETProject *project = m_element->diagram()->project();
	const QString title = followedScheme();
	const auto support = title.isEmpty()
			? ElementAutoNumSchemeCommand::NumberSupport()
			: ElementAutoNumSchemeCommand::numberSupport(project->elementAutoNum().value(title));

	const QSignalBlocker blocker(m_number_cb);
	m_number_cb->clear();
	m_number_current = -1;
	if (!support.supported) {
		m_number_row->hide();
		return;
	}
	if (const auto own = ElementAutoNumSchemeCommand::numberOf(support, m_element)) {
		m_number_current = *own;
		m_number_cb->addItem(tr("%1  (actuel)").arg(*own), *own);
	} else {
		m_number_cb->addItem(tr("— (numéro inconnu)"), 0);
	}
	for (int n : ElementAutoNumSchemeCommand::freeNumbers(project, title, m_element)) {
		if (n == m_number_current) continue;
		m_number_cb->addItem(tr("%1  →  %2").arg(n).arg(
								 ElementAutoNumSchemeCommand::labelForNumber(project, title, m_element, n)), n);
	}
	m_number_cb->setCurrentIndex(0);
	m_number_row->show();
	updateNumberRow();
}

/**
	@brief ElementInfoWidget::updateNumberRow
	A number is chosen only for an element which keeps its numbering and
	whose name is not frozen.
*/
void ElementInfoWidget::updateNumberRow()
{
	if (!m_number_cb || !m_number_row || m_number_row->isHidden()) {
		return;
	}
	const bool keeps = chosenScheme() == followedScheme() && !chosenScheme().isEmpty();
	const bool frozen = m_freeze_cb ? m_freeze_cb->isChecked() : m_element->isFreezeLabel();
	m_number_cb->setEnabled(keeps && !frozen);
	m_number_cb->setToolTip(
				frozen ? tr("Le nom est figé : dégelez-le pour changer son numéro.")
				: !keeps ? tr("Le numéro se choisit quand l'élément garde sa numérotation.")
				: tr("Seuls les numéros libres sont proposés : un élément qui doit garder son "
					 "numéro réel peut le retrouver si personne ne l'a."));
}

/**
	@brief ElementInfoWidget::numberChosen
	Show the label the number picked gives, in the field of the label.
*/
void ElementInfoWidget::numberChosen()
{
	ElementInfoPartWidget *label = infoPartWidgetForKey(QETInformation::ELMT_LABEL);
	if (!label || !m_element || !m_element->diagram()) {
		return;
	}
	const int number = m_number_cb->currentData().toInt();
	if (number <= 0 || number == m_number_current) {
		label->setText(m_element->elementInformations().value(QETInformation::ELMT_LABEL).toString());
		return;
	}
	label->setText(ElementAutoNumSchemeCommand::labelForNumber(
					   m_element->diagram()->project(), followedScheme(), m_element, number));
}

/**
	@brief ElementInfoWidget::followedScheme
	@return the title of the numbering the element follows, empty if none
*/
QString ElementInfoWidget::followedScheme() const
{
	if (!m_element || !m_element->diagram()) {
		return QString();
	}
	return m_element->diagram()->project()->elementAutoNumTitle(
				QUuid(m_element->elementInformations()
					  .value(QETInformation::ELMT_FORMULA_ID).toString()));
}

/**
	@brief ElementInfoWidget::chosenScheme
	@return the title of the numbering picked in the list, empty if the
	element is to follow none
*/
QString ElementInfoWidget::chosenScheme() const
{
	const QString data = m_scheme_cb ? m_scheme_cb->currentData().toString() : QString();
	return data == QLatin1String("\x01") ? QString() : data;
}

/**
	@brief ElementInfoWidget::refreshSchemeRow
	Fill the list with the numberings of the project and show the one the
	element follows. An element with a formula of its own, followed from
	no numbering, gets an entry for it, so that it is neither hidden nor
	lost by looking at it.
*/
void ElementInfoWidget::refreshSchemeRow()
{
	if (!m_scheme_cb || !m_element || !m_element->diagram()) {
		return;
	}
	QETProject *project = m_element->diagram()->project();

	const QSignalBlocker blocker(m_scheme_cb);
	m_scheme_cb->clear();
	m_scheme_cb->addItem(tr("Aucune (nom saisi à la main)"), QString());

	QStringList titles(project->elementAutoNum().keys());
	titles.sort(Qt::CaseInsensitive);
	for (const QString &title : titles) {
		m_scheme_cb->addItem(title, title);
	}

	const QString followed = followedScheme();
	if (!followed.isEmpty()) {
		m_scheme_cb->setCurrentIndex(m_scheme_cb->findData(followed));
		m_scheme_index = m_scheme_cb->currentIndex();
		return;
	}

	const QString formula = m_element->elementInformations()
			.value(QETInformation::ELMT_FORMULA).toString();
	if (!formula.isEmpty()) {
			//Same data as "none" would clash: the entry is for display only
		m_scheme_cb->insertItem(1, tr("Formule propre : %1").arg(formula), QStringLiteral("\x01"));
		m_scheme_cb->setCurrentIndex(1);
	}
	m_scheme_index = m_scheme_cb->currentIndex();
}

/**
	@brief ElementInfoWidget::schemeChosen
	The user picked an entry of the list.

	The formula shown is the one of the numbering picked; the label is
	typed only when there is no formula. Leaving a numbering, or the
	element's own formula, for "none" also empties the label: what stays
	would pass for a number the numbering gave, and could be given again
	to another element. Going back to what the element has at present
	gives its label back.

	An element whose label is frozen is changed only if the user agrees.
*/
void ElementInfoWidget::schemeChosen()
{
	ElementInfoPartWidget *formula = infoPartWidgetForKey(QETInformation::ELMT_FORMULA);
	ElementInfoPartWidget *label = infoPartWidgetForKey(QETInformation::ELMT_LABEL);
	if (!formula || !label || !m_element || !m_element->diagram()) {
		return;
	}

	const DiagramContext info = m_element->elementInformations();
	const QString own_formula = info.value(QETInformation::ELMT_FORMULA).toString();
	const bool own_entry = m_scheme_cb->currentData().toString() == QLatin1String("\x01");
	const QString chosen = chosenScheme();
	const bool unchanged = own_entry
			|| (!chosen.isEmpty() && chosen == followedScheme())
			|| (chosen.isEmpty() && own_formula.isEmpty());

	const bool frozen_now = m_freeze_cb ? m_freeze_cb->isChecked() : m_element->isFreezeLabel();
	if (!unchanged && frozen_now)
	{
		const auto answer = QET::QetMessageBox::question(
					this,
					tr("Nom figé"),
					tr("Le nom de cet élément est figé.\n"
					   "Changer sa numérotation le remplacera ou l'effacera.\n\nContinuer ?"),
					QMessageBox::Yes | QMessageBox::No,
					QMessageBox::No);
		if (answer != QMessageBox::Yes) {
			m_scheme_cb->setCurrentIndex(m_scheme_index);
			return;
		}
	}
	m_scheme_index = m_scheme_cb->currentIndex();
		//The user agreed: the element is no longer protected
	if (!unchanged && m_freeze_cb) {
		m_freeze_cb->setChecked(false);
	}

		//Setting the fields one by one must not apply each step
	const bool live = m_live_edit;
	if (live) disableLiveEdit();

	if (unchanged) {
		formula->setText(own_formula);
		label->setText(info.value(QETInformation::ELMT_LABEL).toString());
	} else if (chosen.isEmpty()) {
		formula->setText(QString());
		if (!own_formula.isEmpty()) {
			label->setText(QString());
		}
	} else {
		formula->setText(m_element->diagram()->project()->elementAutoNumFormula(chosen));
	}
	updateFreezeRow();

	if (live) {
		enableLiveEdit();
		apply();
	}
}

/**
	@brief ElementInfoWidget::openSchemePage
	Open the numberings of the project, on the one of elements, then show
	again the list, which may have changed.
*/
void ElementInfoWidget::openSchemePage()
{
	if (!m_element || !m_element->diagram()) {
		return;
	}
	const QString before = chosenScheme();

	ProjectPropertiesDialog ppd(m_element->diagram()->project(), this);
	ppd.setCurrentPage(ProjectPropertiesDialog::Autonum);
	ppd.changeToElement();
	ppd.exec();

	refreshSchemeRow();
	const int index = m_scheme_cb->findData(before);
	if (index >= 0) {
		m_scheme_cb->setCurrentIndex(index);
	}
}

/**
	@brief ElementInfoWidget::firstActivated
	Slot activated when this widget is show.
	Set the focus to the first line edit provided by this widget
*/
void ElementInfoWidget::firstActivated()
{
	m_eipw_list.first() -> setFocusTolineEdit();
}

/**
	@brief ElementInfoWidget::elementInfoChange
	This slot is called when m_element::elementInformation change.
*/
void ElementInfoWidget::elementInfoChange()
{
	auto elmt_info = m_element->elementInformations();
	auto current_info = currentInfo();

		//If both info have a formula, we remove the label
		//value before compare the equality, because the
		//label of the information returned by the element
		//can be different of the current label because
		//updated by the element to reflect the actual
		//displayed label according the current formula.
	if (current_info.contains(QETInformation::ELMT_FORMULA) &&
		elmt_info.contains(QETInformation::ELMT_FORMULA))
	{
		elmt_info.remove(QETInformation::ELMT_LABEL);
		current_info.remove((QETInformation::ELMT_LABEL ));
	}

	if(current_info != elmt_info)
		updateUi();
}
