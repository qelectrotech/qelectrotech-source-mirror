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
#include "elementpropertieseditorwidget.h"

#include "../../qetapp.h"
#include "../../qetinformation.h"
#include "ui_elementpropertieseditorwidget.h"
#include "../../qetinformation.h"

#include <QItemDelegate>
#include <QComboBox>
#include <QSpinBox>
#include <QSignalBlocker>
#include <QTableWidgetItem>
#include <QHeaderView>
#include <QScrollBar>
#include <QWheelEvent>
#include <QTableWidget>
#include <QCheckBox>
#include <QGroupBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QClipboard>
#include <QApplication>
#include <QFontDialog>
#include <QFont>
#include <QLineEdit>
#include <QSplitter>
#include <QShortcut>
#include <QMenu>

/**
	@brief The EditorDelegate class
	This delegate is only use for disable the edition of the first
	column of the information tree widget
*/
class EditorDelegate : public QItemDelegate
{
	public:
		EditorDelegate(QObject *parent) :
			QItemDelegate(parent)
		{}

	QWidget* createEditor(QWidget *parent,
			      const QStyleOptionViewItem &option,
				  const QModelIndex &index) const override
	{
		if(index.column() == 1)
		{
			return QItemDelegate::createEditor(parent,
							   option,
							   index);
		}
		return nullptr;
	}
};

/**
 * @brief ElementPropertiesEditorWidget::ElementPropertiesEditorWidget
 * @param data
 * @param parent
 */
ElementPropertiesEditorWidget::ElementPropertiesEditorWidget(ElementData data, QWidget *parent) :
	QDialog(parent),
	ui(new Ui::ElementPropertiesEditorWidget),
	m_data(data)
{
	ui->setupUi(this);
	setUpInterface();
	upDateInterface();
}

/**
	@brief ElementPropertiesEditorWidget::~ElementPropertiesEditorWidget
	Default destructor
*/
ElementPropertiesEditorWidget::~ElementPropertiesEditorWidget()
{
	delete ui;
}

/**
	@brief ElementPropertiesEditorWidget::upDateInterface
	Update the interface with the current value
*/
void ElementPropertiesEditorWidget::upDateInterface()
{
	ui->m_base_type_cb->setCurrentIndex(
				ui->m_base_type_cb->findData(
					m_data.m_type));
	
	if (m_data.m_type == ElementData::Slave)
	{
		// If PLC Slave, select "Esclave PLC" in state combo (not in type combo)
		if (m_data.m_slave_type == ElementData::PLCSlave) {
			ui->m_state_cb->setCurrentIndex(
						ui->m_state_cb->findData(
							ElementData::PLCSlave));
		} else {
			ui->m_state_cb->setCurrentIndex(
						ui->m_state_cb->findData(
							m_data.m_slave_state));
		}
		ui->m_type_cb->setCurrentIndex (
					ui->m_type_cb->findData(
						m_data.m_slave_type));
		ui->m_number_ctc->setValue(m_data.m_contact_count);
	}
	else if (m_data.m_type == ElementData::Master) {
		ui->m_master_type_cb->setCurrentIndex(
			ui->m_master_type_cb->findData (
				m_data.m_master_type));

		// NEU: Checkbox und Zahlenbox für max_slaves einstellen
		if (m_data.m_max_slaves == -1) {
			ui->max_slaves_checkbox->setChecked(false);
			ui->max_slaves_spinbox->setEnabled(false);
		} else {
			ui->max_slaves_checkbox->setChecked(true);
			ui->max_slaves_spinbox->setEnabled(true);
			ui->max_slaves_spinbox->setValue(m_data.m_max_slaves);
		}

		// Slave contact groups checkbox
		ui->m_slave_groups_checkbox->setChecked(m_data.m_slave_contact_groups_enabled);
		ui->m_slave_groups_table->setEnabled(m_data.m_slave_contact_groups_enabled);
		if (m_data.m_slave_contact_groups_enabled) {
			populateSlaveGroupsTable();
		}

		// PLC configuration
		if (m_data.m_master_type == ElementData::PLC) {
			if (!m_plc_gb) {
				createPlcConfigWidgets();
			}
			m_plc_gb->setVisible(true);
			ui->max_slaves_checkbox->setVisible(false);
			ui->max_slaves_spinbox->setVisible(false);
			ui->m_slave_groups_checkbox->setVisible(false);
			ui->m_slave_groups_table->setVisible(false);
			populatePlcTable();
		} else {
			if (m_plc_gb)
				m_plc_gb->setVisible(false);
			ui->max_slaves_checkbox->setVisible(true);
			ui->max_slaves_spinbox->setVisible(true);
			ui->m_slave_groups_checkbox->setVisible(true);
			ui->m_slave_groups_table->setVisible(true);
		}
	} else if (m_data.m_type == ElementData::Terminal) {
		ui->m_terminal_type_cb->setCurrentIndex(
					ui->m_terminal_type_cb->findData(
						m_data.m_terminal_type));
		ui->m_terminal_func_cb->setCurrentIndex(
					ui->m_terminal_func_cb->findData(
						m_data.m_terminal_function));
	}

	on_m_base_type_cb_currentIndexChanged(ui->m_base_type_cb->currentIndex());
}

/**
	@brief ElementPropertiesEditorWidget::setUpInterface
*/
void ElementPropertiesEditorWidget::setUpInterface()
{
		// Type combo box
	ui->m_base_type_cb->addItem (tr("Simple"),  ElementData::Simple);
	ui->m_base_type_cb->addItem (tr("Maître"),  ElementData::Master);
	ui->m_base_type_cb->addItem (tr("Esclave"), ElementData::Slave);
	ui->m_base_type_cb->addItem (tr("Renvoi de folio suivant"),   ElementData::NextReport);
	ui->m_base_type_cb->addItem (tr("Renvoi de folio précédent"), ElementData::PreviousReport);
	ui->m_base_type_cb->addItem (tr("Bornier"), ElementData::Terminal);
	ui->m_base_type_cb->addItem (tr("Vignette"), ElementData::Thumbnail);
	ui->m_base_type_cb->addItem (tr("Définition de conducteur"), ElementData::ConductorDefinition);

		// Slave option
	ui->m_state_cb->addItem(tr("Normalement ouvert"),       ElementData::NO);
	ui->m_state_cb->addItem(tr("Normalement fermé"),        ElementData::NC);
	ui->m_state_cb->addItem(tr("Inverseur"),                ElementData::SW);
	ui->m_state_cb->addItem(tr("Other"),                    ElementData::Other);
	ui->m_state_cb->addItem(tr("Esclave PLC"),              ElementData::PLCSlave);
	ui->m_type_cb->addItem(tr("Simple"),                    ElementData::SSimple);
	ui->m_type_cb->addItem(tr("Puissance"),                 ElementData::Power);
	ui->m_type_cb->addItem(tr("Temporisé travail"),         ElementData::DelayOn);
	ui->m_type_cb->addItem(tr("Temporisé repos"),           ElementData::DelayOff);
	ui->m_type_cb->addItem(tr("Temporisé travail & repos"), ElementData::delayOnOff);

		//Master option
	ui->m_master_type_cb->addItem(tr("Bobine"),               ElementData::Coil);
	ui->m_master_type_cb->addItem(tr("Organe de protection"), ElementData::Protection);
	ui->m_master_type_cb->addItem(tr("Commutateur / bouton"), ElementData::Commutator);
	ui->m_master_type_cb->addItem(tr("Module PLC"),           ElementData::PLC);
	ui->m_master_type_cb->setMinimumWidth(150);

		//Terminal option
	ui->m_terminal_type_cb->addItem(tr("Générique"),    ElementData::TTGeneric);
	ui->m_terminal_type_cb->addItem(tr("Fusible"),      ElementData::TTFuse);
	ui->m_terminal_type_cb->addItem(tr("Séctionnable"), ElementData::TTSectional);
	ui->m_terminal_type_cb->addItem(tr("Diode"),        ElementData::TTDiode);
	ui->m_terminal_type_cb->addItem(tr("Terre"),        ElementData::TTGround);

	ui->m_terminal_func_cb->addItem(tr("Générique"), ElementData::TFGeneric);
	ui->m_terminal_func_cb->addItem(tr("Phase"),     ElementData::TFPhase);
	ui->m_terminal_func_cb->addItem(tr("Neutre"),    ElementData::TFNeutral);

	//Disable the edition of the first column of the information tree
	//by this little workaround
	ui->m_tree->setItemDelegate(new EditorDelegate(this));

	// NEU: Checkbox mit der Zahlenbox verbinden (Aktivieren/Deaktivieren)
	connect(ui->max_slaves_checkbox, &QCheckBox::toggled, ui->max_slaves_spinbox, &QWidget::setEnabled);
	connect(ui->max_slaves_spinbox, QOverload<int>::of(&QSpinBox::valueChanged), [this](int) {
		if (ui->m_slave_groups_checkbox->isChecked()) {
			populateSlaveGroupsTable();
		}
	});

	// Connect master type combo box to show/hide PLC configuration
	connect(ui->m_master_type_cb, QOverload<int>::of(&QComboBox::currentIndexChanged), [this](int index) {
		auto master_type = ui->m_master_type_cb->itemData(index).value<ElementData::MasterType>();
		if (master_type == ElementData::PLC) {
			if (!m_plc_gb) {
				createPlcConfigWidgets();
			}
			m_plc_gb->setVisible(true);
			ui->max_slaves_checkbox->setVisible(false);
			ui->max_slaves_spinbox->setVisible(false);
			ui->m_slave_groups_checkbox->setVisible(false);
			ui->m_slave_groups_table->setVisible(false);
			populatePlcTable();
		} else {
			if (m_plc_gb)
				m_plc_gb->setVisible(false);
			ui->max_slaves_checkbox->setVisible(true);
			ui->max_slaves_spinbox->setVisible(true);
			ui->m_slave_groups_checkbox->setVisible(true);
			ui->m_slave_groups_table->setVisible(true);
		}
	});

	// When "Esclave PLC" is selected in state combo, disable type combo
	connect(ui->m_state_cb, QOverload<int>::of(&QComboBox::currentIndexChanged), [this](int index) {
		bool is_plc = (ui->m_state_cb->itemData(index) == ElementData::PLCSlave);
		ui->m_type_cb->setEnabled(!is_plc);
		if (is_plc) {
			ui->m_type_cb->setCurrentIndex(0);
		}
	});

	populateTree();
}

void ElementPropertiesEditorWidget::updateTree()
{
	auto type_ = ui->m_base_type_cb->currentData().value<ElementData::Type>();

	switch (type_) {
		case ElementData::Simple:
			ui->m_tree->setEnabled(true);
			break;
		case ElementData::Thumbnail:
			ui->m_tree->setEnabled(true);
			break;
		case ElementData::NextReport:
			ui->m_tree->setDisabled(true);
			break;
		case ElementData::PreviousReport:
			ui->m_tree->setDisabled(true);
			break;
		case ElementData::ConductorDefinition:
			ui->m_tree->setDisabled(true);
			break;
		case ElementData::Master:
			ui->m_tree->setEnabled(true);
			break;
		case ElementData::Slave:
			ui->m_tree->setDisabled(true);
			break;
		case ElementData::Terminal:
			ui->m_tree->setEnabled(true);
			break;
		default:
			ui->m_tree->setDisabled(true);
			break;
	}
}

/**
	@brief ElementPropertiesEditorWidget::populateTree
	Create QTreeWidgetItem of the tree widget and populate it
*/
void ElementPropertiesEditorWidget::populateTree()
{	
	const auto keys = QETInformation::elementEditorElementInfoKeys();

	// For PLC Slave: add PLC-specific info keys at the top
	if (m_data.m_type == ElementData::Slave
		&& m_data.m_slave_type == ElementData::PLCSlave)
	{
		QStringList plc_keys = {
			QETInformation::ELMT_PLC_TYPE,
			QETInformation::ELMT_PLC_ADDRESS,
			QETInformation::ELMT_PLC_FUNCTION,
			QETInformation::ELMT_PLC_COMMENT,
			QETInformation::ELMT_PLC_CROSSREF
		};
		for (const QString &key : plc_keys)
		{
			QTreeWidgetItem *qtwi = new QTreeWidgetItem(ui->m_tree);
			qtwi->setFlags(Qt::ItemIsEnabled | Qt::ItemIsEditable);
			qtwi->setData(0, Qt::DisplayRole,
						  QETInformation::translatedInfoKey(key));
			qtwi->setData(0, Qt::UserRole, key);
			qtwi->setText(1, m_data.m_informations.value(key).toString());
			qtwi->setForeground(0, QColor(0, 100, 180));
		}
	}

	for(const QString& key : keys)
	{
		QTreeWidgetItem *qtwi = new QTreeWidgetItem(ui->m_tree);
		qtwi->setFlags(Qt::ItemIsEnabled | Qt::ItemIsEditable);
		qtwi->setData(0, Qt::DisplayRole,
					  QETInformation::translatedInfoKey(key));
		qtwi->setData(0, Qt::UserRole, key);
		qtwi->setText(1, m_data.m_informations.value(key).toString());
		// Adjust column width
		ui->m_tree->resizeColumnToContents(0);
	}
}

/**
	@brief ElementPropertiesEditorWidget::on_m_buttonBox_accepted
	Action on button accepted : the new information is set
*/
void ElementPropertiesEditorWidget::on_m_buttonBox_accepted()
{
	m_data.m_type = ui->m_base_type_cb->currentData().value<ElementData::Type>();

	if (m_data.m_type == ElementData::Slave)
	{
		auto state_val = ui->m_state_cb->currentData();
		if (state_val == ElementData::PLCSlave) {
			m_data.m_slave_type = ElementData::PLCSlave;
			m_data.m_slave_state = ElementData::Other;
		} else {
			m_data.m_slave_state = state_val.value<ElementData::SlaveState>();
			m_data.m_slave_type  = ui->m_type_cb->currentData().value<ElementData::SlaveType>();
		}
		m_data.m_contact_count = ui->m_number_ctc->value();
	}
		else if (m_data.m_type == ElementData::Master) {
		m_data.m_master_type = ui->m_master_type_cb->currentData().value<ElementData::MasterType>();

		//If the checkbox is checked, save the number; otherwise, -1 (infinity)
		if (ui->max_slaves_checkbox->isVisible() && ui->max_slaves_checkbox->isChecked()) {
			m_data.m_max_slaves = ui->max_slaves_spinbox->value();
		} else {
			m_data.m_max_slaves = -1;
		}

		if (m_data.m_master_type == ElementData::PLC) {
			readPlcTable();
		} else {
			readSlaveGroupsFromTable();
		}
	}
	else if (m_data.m_type == ElementData::Terminal)
	{
		m_data.m_terminal_type = ui->m_terminal_type_cb->currentData().value<ElementData::TerminalType>();
		m_data.m_terminal_function = ui->m_terminal_func_cb->currentData().value<ElementData::TerminalFunction>();
	}
	
	for (QTreeWidgetItem *qtwi : ui->m_tree->invisibleRootItem()->takeChildren())
	{
		QString txt = qtwi->text(1);
		//remove line feed and carriage return
		txt.remove("\r");
		txt.remove("\n");

		m_data.m_informations.addValue(qtwi->data(0, Qt::UserRole).toString(),
									   txt);
	}
	
	this->close();
}

/**
	@brief ElementPropertiesEditorWidget::on_m_base_type_cb_currentIndexChanged
	@param index : Action when combo-box base type index change
*/
void ElementPropertiesEditorWidget::on_m_base_type_cb_currentIndexChanged(int index)
{
	bool slave = false , master = false, terminal = false;

	auto type_ = ui->m_base_type_cb->itemData(index).value<ElementData::Type>();
	if (type_ == ElementData::Slave)
		slave  = true;
	else if (type_ == ElementData::Master)
		master = true;
	else if (type_ == ElementData::Terminal)
		terminal = true;

	ui->m_slave_gb->setVisible(slave);
	ui->m_master_gb->setVisible(master);
	ui->m_terminal_gb->setVisible(terminal);

	ui->tabWidget->setTabVisible(1,
								 (type_ == ElementData::Simple ||
								  type_ == ElementData::Master));

	updateTree();
}

/**
 * @brief ElementPropertiesEditorWidget::on_max_slaves_checkbox_toggled
 * When max_slaves checkbox is unchecked, also uncheck slave groups checkbox
 */
void ElementPropertiesEditorWidget::on_max_slaves_checkbox_toggled(bool checked)
{
	if (!checked && ui->m_slave_groups_checkbox->isChecked()) {
		ui->m_slave_groups_checkbox->setChecked(false);
	}
}

/**
 * @brief ElementPropertiesEditorWidget::on_m_slave_groups_checkbox_toggled
 * When slave groups checkbox is toggled, enable/disable the table
 * Also ensure max_slaves checkbox is checked when enabling groups
 */
void ElementPropertiesEditorWidget::on_m_slave_groups_checkbox_toggled(bool checked)
{
	ui->m_slave_groups_table->setEnabled(checked);

	if (checked && !ui->max_slaves_checkbox->isChecked()) {
		ui->max_slaves_checkbox->setChecked(true);
		if (!m_data.m_slave_contact_groups.isEmpty()) {
			ui->max_slaves_spinbox->setValue(m_data.m_slave_contact_groups.size());
		}
	}

	if (checked) {
		populateSlaveGroupsTable();
	}
}

/**
 * @brief ElementPropertiesEditorWidget::populateSlaveGroupsTable
 * Fill the slave contact groups table from m_data
 */
void ElementPropertiesEditorWidget::populateSlaveGroupsTable()
{
	QSignalBlocker blocker_table(ui->m_slave_groups_table);
	QSignalBlocker blocker_spinbox(ui->max_slaves_spinbox);
	ui->m_slave_groups_table->setRowCount(0);

	int row_count = ui->max_slaves_checkbox->isChecked()
		? ui->max_slaves_spinbox->value() : 0;

	// Adjust the groups list to match the spinbox value
	while (m_data.m_slave_contact_groups.size() < row_count) {
		ElementData::SlaveContactGroup group;
		group.type = ElementData::NO;
		group.subtype = ElementData::SSimple;
		group.contactCount = 1;
		group.terminalCount = 2;
		m_data.m_slave_contact_groups.append(group);
	}
	while (m_data.m_slave_contact_groups.size() > row_count) {
		m_data.m_slave_contact_groups.removeLast();
	}

	// Find max terminal count across all groups to determine T columns
	int max_tc = 0;
	for (const auto &g : m_data.m_slave_contact_groups) {
		max_tc = qMax(max_tc, g.terminalCount);
	}
	max_tc = qMax(max_tc, 2); // at least T1, T2

	// Set up 4 fixed columns + max_tc label columns
	int total_cols = 4 + max_tc;
	ui->m_slave_groups_table->setColumnCount(total_cols);

	// Set T column headers
	for (int t = 0; t < max_tc; ++t) {
		ui->m_slave_groups_table->setHorizontalHeaderItem(
			4 + t, new QTableWidgetItem(tr("T%1").arg(t + 1)));
	}

	// Set column widths for readability
	ui->m_slave_groups_table->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
	ui->m_slave_groups_table->horizontalHeader()->setMinimumSectionSize(60);
	ui->m_slave_groups_table->setColumnWidth(0, 180); // Type
	ui->m_slave_groups_table->setColumnWidth(1, 180); // Subtype
	ui->m_slave_groups_table->setColumnWidth(2, 80);  // Contacts
	ui->m_slave_groups_table->setColumnWidth(3, 80);  // Bornes

	ui->m_slave_groups_table->setRowCount(m_data.m_slave_contact_groups.size());

	for (int i = 0; i < m_data.m_slave_contact_groups.size(); ++i) {
		auto &group = m_data.m_slave_contact_groups[i];

		// Type column
		auto *type_cb = new QComboBox(ui->m_slave_groups_table);
		type_cb->addItem(tr("Normalement ouvert"), ElementData::NO);
		type_cb->addItem(tr("Normalement fermé"), ElementData::NC);
		type_cb->addItem(tr("Inverseur"), ElementData::SW);
		type_cb->addItem(tr("Autre"), ElementData::Other);
		type_cb->setCurrentIndex(type_cb->findData(group.type));
		ui->m_slave_groups_table->setCellWidget(i, 0, type_cb);

		// Subtype column
		auto *subtype_cb = new QComboBox(ui->m_slave_groups_table);
		subtype_cb->addItem(tr("Simple"), ElementData::SSimple);
		subtype_cb->addItem(tr("Puissance"), ElementData::Power);
		subtype_cb->addItem(tr("Temporisé travail"), ElementData::DelayOn);
		subtype_cb->addItem(tr("Temporisé repos"), ElementData::DelayOff);
		subtype_cb->addItem(tr("Temporisé travail & repos"), ElementData::delayOnOff);
		subtype_cb->setCurrentIndex(subtype_cb->findData(group.subtype));
		ui->m_slave_groups_table->setCellWidget(i, 1, subtype_cb);

		// Contact count
		auto *contact_ct = new QSpinBox(ui->m_slave_groups_table);
		contact_ct->setMinimum(1);
		contact_ct->setMaximum(20);
		contact_ct->setValue(group.contactCount);
		ui->m_slave_groups_table->setCellWidget(i, 2, contact_ct);

		// Terminal count
		auto *terminal_ct = new QSpinBox(ui->m_slave_groups_table);
		terminal_ct->setMinimum(1);
		terminal_ct->setMaximum(20);
		terminal_ct->setValue(group.terminalCount);
		ui->m_slave_groups_table->setCellWidget(i, 3, terminal_ct);

		// When terminal count changes, rebuild the table to update T columns
		connect(terminal_ct, QOverload<int>::of(&QSpinBox::valueChanged),
			this, [this, terminal_ct, i](int val) {
				if (i < m_data.m_slave_contact_groups.size()) {
					m_data.m_slave_contact_groups[i].terminalCount = val;
					readSlaveGroupsFromTable();
					populateSlaveGroupsTable();
				}
			});

		// Auto-generate labels if empty
		QStringList labels = group.labels;
		while (labels.size() < group.terminalCount) {
			labels << tr("T%1").arg(labels.size() + 1);
		}

		// Fill T1..TN columns
		for (int t = 0; t < max_tc; ++t) {
			auto *item = new QTableWidgetItem(
				t < labels.size() ? labels.at(t) : QString());
			if (t >= group.terminalCount) {
				item->setFlags(item->flags() & ~Qt::ItemIsEditable);
				item->setBackground(QBrush(QColor(240, 240, 240)));
			}
			ui->m_slave_groups_table->setItem(i, 4 + t, item);
		}

		// Store updated labels back
		group.labels = labels;
	}
}

/**
 * @brief ElementPropertiesEditorWidget::readSlaveGroupsFromTable
 * Read the slave contact groups from the table back into m_data
 */
void ElementPropertiesEditorWidget::readSlaveGroupsFromTable()
{
	m_data.m_slave_contact_groups.clear();

	if (!ui->m_slave_groups_checkbox->isChecked()) {
		m_data.m_slave_contact_groups_enabled = false;
		return;
	}

	m_data.m_slave_contact_groups_enabled = true;

	for (int i = 0; i < ui->m_slave_groups_table->rowCount(); ++i) {
		ElementData::SlaveContactGroup group;

		auto *type_cb = qobject_cast<QComboBox *>(
			ui->m_slave_groups_table->cellWidget(i, 0));
		if (type_cb) {
			group.type = type_cb->currentData().value<ElementData::SlaveState>();
		}

		auto *subtype_cb = qobject_cast<QComboBox *>(
			ui->m_slave_groups_table->cellWidget(i, 1));
		if (subtype_cb) {
			group.subtype = subtype_cb->currentData().value<ElementData::SlaveType>();
		}

		auto *contact_ct = qobject_cast<QSpinBox *>(
			ui->m_slave_groups_table->cellWidget(i, 2));
		if (contact_ct) {
			group.contactCount = contact_ct->value();
		}

		auto *terminal_ct = qobject_cast<QSpinBox *>(
			ui->m_slave_groups_table->cellWidget(i, 3));
		if (terminal_ct) {
			group.terminalCount = terminal_ct->value();
		}

		// Read labels from T1..TN columns
		for (int t = 0; t < group.terminalCount; ++t) {
			int col = 4 + t;
			if (col < ui->m_slave_groups_table->columnCount()) {
				auto *item = ui->m_slave_groups_table->item(i, col);
				if (item && !item->text().isEmpty()) {
					group.labels.append(item->text());
				} else {
					group.labels << tr("T%1").arg(t + 1);
				}
			} else {
				group.labels << tr("T%1").arg(t + 1);
			}
		}

		m_data.m_slave_contact_groups.append(group);
	}
}

/**
 * @brief ElementPropertiesEditorWidget::createPlcConfigWidgets
 * Create the PLC configuration widgets programmatically and add them to m_master_gb
 */
void ElementPropertiesEditorWidget::createPlcConfigWidgets()
{
	// Create PLC group box
	m_plc_gb = new QGroupBox(tr("Configuration PLC"), ui->m_master_gb);
	auto *plc_layout = new QVBoxLayout(m_plc_gb);

	// Toolbar
	auto *toolbar = new QHBoxLayout();
	auto *add_btn = new QPushButton(tr("+"), m_plc_gb);
	auto *remove_btn = new QPushButton(tr("-"), m_plc_gb);

	add_btn->setMaximumWidth(30);
	remove_btn->setMaximumWidth(30);

	toolbar->addStretch();
	toolbar->addWidget(add_btn);
	toolbar->addWidget(remove_btn);
	plc_layout->addLayout(toolbar);

	// Tables side by side: IO table (left) + Terminal table (right)
	// Both share a single vertical scrollbar on the right
	auto *tables_container = new QWidget(m_plc_gb);
	auto *tables_layout = new QHBoxLayout(tables_container);
	tables_layout->setContentsMargins(0, 0, 0, 0);

	auto *splitter = new QSplitter(Qt::Horizontal, tables_container);

	// IO Table
	m_plc_table = new QTableWidget(splitter);
	m_plc_table->setColumnCount(5);
	m_plc_table->setHorizontalHeaderLabels({
		tr("Type"), tr("Adresse"), tr("Fonction"),
		tr("Commentaire"), tr("Réf. croisée")
	});
	m_plc_table->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
	m_plc_table->horizontalHeader()->setSectionsMovable(true);
	m_plc_table->horizontalHeader()->setSectionsClickable(true);
	m_plc_table->horizontalHeader()->resizeSection(0, 120);
	m_plc_table->horizontalHeader()->resizeSection(1, 100);
	m_plc_table->horizontalHeader()->resizeSection(2, 150);
	m_plc_table->horizontalHeader()->resizeSection(3, 150);
	m_plc_table->horizontalHeader()->resizeSection(4, 100);
	m_plc_table->setSelectionBehavior(QAbstractItemView::SelectRows);
	m_plc_table->setSelectionMode(QAbstractItemView::ExtendedSelection);
	m_plc_table->setMinimumHeight(200);
	m_plc_table->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

	// Terminal table (per IO: Nb + T1-T4)
	m_plc_terminal_table = new QTableWidget(splitter);
	m_plc_terminal_table->setColumnCount(2);
	m_plc_terminal_table->setHorizontalHeaderLabels({
		tr("Nb."), tr("T1")
	});
	m_plc_terminal_table->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
	m_plc_terminal_table->horizontalHeader()->resizeSection(0, 50);
	m_plc_terminal_table->horizontalHeader()->resizeSection(1, 80);
	m_plc_terminal_table->setSelectionBehavior(QAbstractItemView::SelectItems);
	m_plc_terminal_table->setSelectionMode(QAbstractItemView::ExtendedSelection);
	m_plc_terminal_table->setMinimumHeight(200);
	m_plc_terminal_table->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

	// Shared vertical scrollbar
	m_plc_shared_scrollbar = new QScrollBar(Qt::Vertical, tables_container);
	m_plc_shared_scrollbar->setMinimum(0);

	splitter->addWidget(m_plc_table);
	splitter->addWidget(m_plc_terminal_table);
	splitter->setStretchFactor(0, 3);
	splitter->setStretchFactor(1, 1);

	tables_layout->addWidget(splitter);
	tables_layout->addWidget(m_plc_shared_scrollbar);

	// Bidirectional sync between tables and shared scrollbar
	// Use flag to prevent infinite loops
	connect(m_plc_shared_scrollbar, &QScrollBar::valueChanged,
		this, [this](int value) {
			if (m_plc_scroll_sync) return;
			m_plc_scroll_sync = true;
			m_plc_table->verticalScrollBar()->setValue(value);
			m_plc_terminal_table->verticalScrollBar()->setValue(value);
			m_plc_scroll_sync = false;
		});
	connect(m_plc_table->verticalScrollBar(), &QScrollBar::valueChanged,
		this, [this](int value) {
			if (m_plc_scroll_sync) return;
			m_plc_scroll_sync = true;
			m_plc_shared_scrollbar->setValue(value);
			m_plc_terminal_table->verticalScrollBar()->setValue(value);
			m_plc_scroll_sync = false;
		});
	connect(m_plc_terminal_table->verticalScrollBar(), &QScrollBar::valueChanged,
		this, [this](int value) {
			if (m_plc_scroll_sync) return;
			m_plc_scroll_sync = true;
			m_plc_shared_scrollbar->setValue(value);
			m_plc_table->verticalScrollBar()->setValue(value);
			m_plc_scroll_sync = false;
		});

	// Update shared scrollbar range from both tables
	auto syncRange = [this]() {
		int max = qMax(m_plc_table->verticalScrollBar()->maximum(),
				   m_plc_terminal_table->verticalScrollBar()->maximum());
		m_plc_shared_scrollbar->blockSignals(true);
		m_plc_shared_scrollbar->setMaximum(max);
		m_plc_shared_scrollbar->blockSignals(false);
	};
	connect(m_plc_table->verticalScrollBar(), &QScrollBar::rangeChanged,
		this, syncRange);
	connect(m_plc_terminal_table->verticalScrollBar(), &QScrollBar::rangeChanged,
		this, syncRange);

	plc_layout->addWidget(tables_container);

	// Font settings
	auto *font_layout = new QHBoxLayout();

	m_plc_header_font_btn = new QPushButton(tr("Police des en-têtes"), m_plc_gb);
	m_plc_header_font_btn->setToolTip(tr("Configurer la police des en-têtes de colonnes"));
	connect(m_plc_header_font_btn, &QPushButton::clicked, this, &ElementPropertiesEditorWidget::plcSelectHeaderFont);
	font_layout->addWidget(m_plc_header_font_btn);

	m_plc_cell_font_btn = new QPushButton(tr("Police du texte"), m_plc_gb);
	m_plc_cell_font_btn->setToolTip(tr("Configurer la police du texte dans les cellules"));
	connect(m_plc_cell_font_btn, &QPushButton::clicked, this, &ElementPropertiesEditorWidget::plcSelectCellFont);
	font_layout->addWidget(m_plc_cell_font_btn);

	m_plc_show_headers_cb = new QCheckBox(tr("Afficher les en-têtes sur la feuille"), m_plc_gb);
	m_plc_show_headers_cb->setToolTip(tr("Afficher ou masquer les en-têtes de colonnes du tableau PLC sur la feuille"));
	m_plc_show_headers_cb->setChecked(true);
	font_layout->addWidget(m_plc_show_headers_cb);

	font_layout->addStretch();
	plc_layout->addLayout(font_layout);

	// Display settings
	auto *settings_layout = new QGridLayout();

	for (int i = 0; i < 4; ++i) {
		m_plc_break_checkboxes[i] = new QCheckBox(
			tr("Saut %1 après:").arg(i + 1), m_plc_gb);
		settings_layout->addWidget(m_plc_break_checkboxes[i], i, 0);

		m_plc_break_spinboxes[i] = new QSpinBox(m_plc_gb);
		m_plc_break_spinboxes[i]->setMinimum(0);
		m_plc_break_spinboxes[i]->setMaximum(128);
		m_plc_break_spinboxes[i]->setSpecialValueText(tr("Aucun"));
		m_plc_break_spinboxes[i]->setEnabled(false);
		settings_layout->addWidget(m_plc_break_spinboxes[i], i, 1);

		connect(m_plc_break_checkboxes[i], &QCheckBox::toggled,
			m_plc_break_spinboxes[i], &QSpinBox::setEnabled);
	}

	settings_layout->addWidget(new QLabel(tr("H. ligne:"), m_plc_gb), 4, 0);
	m_plc_row_height_spinbox = new QSpinBox(m_plc_gb);
	m_plc_row_height_spinbox->setMinimum(4);
	m_plc_row_height_spinbox->setMaximum(30);
	m_plc_row_height_spinbox->setValue(8);
	m_plc_row_height_spinbox->setSuffix(tr(" mm"));
	settings_layout->addWidget(m_plc_row_height_spinbox, 4, 1);

	settings_layout->setColumnStretch(2, 1);
	plc_layout->addLayout(settings_layout);

	// Column name, visibility and width
	auto *col_layout = new QHBoxLayout();
	QStringList col_names = {tr("Type"), tr("Adresse"), tr("Fonction"),
							 tr("Commentaire"), tr("Réf.")};

	for (int i = 0; i < 5; ++i) {
		auto *col_widget = new QVBoxLayout();

		auto *le = new QLineEdit(m_plc_gb);
		le->setPlaceholderText(col_names.at(i));
		le->setToolTip(tr("Nom personnalisé de la colonne (vide = par défaut)"));
		m_plc_col_name_edits.append(le);
		col_widget->addWidget(le);

		auto *cb = new QCheckBox(tr("Visible"), m_plc_gb);
		cb->setChecked(true);
		m_plc_col_visibility_checkboxes.append(cb);
		col_widget->addWidget(cb);

		auto *sb = new QSpinBox(m_plc_gb);
		sb->setMinimum(10);
		sb->setMaximum(500);
		sb->setValue(40);
		sb->setSuffix(tr(" mm"));
		m_plc_col_width_spinboxes.append(sb);
		col_widget->addWidget(sb);

		col_layout->addLayout(col_widget);
	}
	plc_layout->addLayout(col_layout);

	// Add to master group box - row 4, full width
	auto *gl = qobject_cast<QGridLayout*>(ui->m_master_gb->layout());
	gl->addWidget(m_plc_gb, 4, 0, 1, 2);

	// Connect signals
	connect(add_btn, &QPushButton::clicked, this, &ElementPropertiesEditorWidget::plcAddRow);
	connect(remove_btn, &QPushButton::clicked, this, &ElementPropertiesEditorWidget::plcRemoveRow);

	// Ctrl+V shortcut for paste
	auto *paste_shortcut = new QShortcut(QKeySequence::Paste, m_plc_table);
	connect(paste_shortcut, &QShortcut::activated, this, &ElementPropertiesEditorWidget::plcPasteFromClipboard);

	// Context menu for the PLC table
	m_plc_table->setContextMenuPolicy(Qt::CustomContextMenu);
	connect(m_plc_table, &QTableWidget::customContextMenuRequested, this, [this](const QPoint &pos) {
		QMenu menu;
		menu.addAction(tr("Coller depuis le presse-papiers"), this, &ElementPropertiesEditorWidget::plcPasteFromClipboard);
		menu.exec(m_plc_table->mapToGlobal(pos));
	});
}

/**
 * @brief ElementPropertiesEditorWidget::populatePlcTable
 * Fill the PLC table from m_data
 */
void ElementPropertiesEditorWidget::populatePlcTable()
{
	if (!m_plc_table)
		return;

	m_plc_table->setRowCount(0);
	if (m_plc_terminal_table)
		m_plc_terminal_table->setRowCount(0);

	const auto &plc_data = m_data.m_plc_master_data;
	m_plc_table->setRowCount(plc_data.ios.size());
	if (m_plc_terminal_table)
		m_plc_terminal_table->setRowCount(plc_data.ios.size());

	for (int row = 0; row < plc_data.ios.size(); ++row) {
		const auto &io = plc_data.ios.at(row);

		// --- IO Table ---
		auto *type_cb = new QComboBox(m_plc_table);
		QStringList plc_types = ElementData::plcIOTypeList();
		for (int t = 0; t < plc_types.size(); ++t) {
			type_cb->addItem(plc_types.at(t), t);
		}
		type_cb->setCurrentIndex(static_cast<int>(io.type));
		m_plc_table->setCellWidget(row, 0, type_cb);

		m_plc_table->setItem(row, 1, new QTableWidgetItem(io.address));
		m_plc_table->setItem(row, 2, new QTableWidgetItem(io.functionText));
		m_plc_table->setItem(row, 3, new QTableWidgetItem(io.comment));

		auto *crossref_item = new QTableWidgetItem(io.crossRef);
		crossref_item->setFlags(crossref_item->flags() & ~Qt::ItemIsEditable);
		m_plc_table->setItem(row, 4, crossref_item);

		// --- Terminal Table ---
		if (m_plc_terminal_table) {
			int tc = qMax(1, io.terminalCount);

			// Ensure enough columns
			while (m_plc_terminal_table->columnCount() < tc + 1)
				m_plc_terminal_table->insertColumn(m_plc_terminal_table->columnCount());

			auto *tc_sb = new QSpinBox(m_plc_terminal_table);
			tc_sb->setMinimum(1);
			tc_sb->setMaximum(4);
			tc_sb->setValue(tc);
			m_plc_terminal_table->setCellWidget(row, 0, tc_sb);
			connect(tc_sb, QOverload<int>::of(&QSpinBox::valueChanged),
				this, [this, row](int val) { plcTerminalCountChanged(row, val); });

			for (int i = 0; i < m_plc_terminal_table->columnCount() - 1; ++i) {
				QString val = (i < io.terminals.size()) ? io.terminals.at(i) : QString();
				auto *item = new QTableWidgetItem(val);
				if (i >= tc) {
					item->setFlags(item->flags() & ~Qt::ItemIsEditable);
					item->setBackground(QColor(230, 230, 230));
				}
				m_plc_terminal_table->setItem(row, i + 1, item);
			}
		}
	}

	// Load display settings
	for (int i = 0; i < 4; ++i) {
		int val = plc_data.breakPositions.value(i, 0);
		bool enabled = (val > 0);
		m_plc_break_checkboxes[i]->setChecked(enabled);
		m_plc_break_spinboxes[i]->setValue(enabled ? val : 0);
	}
	m_plc_row_height_spinbox->setValue(static_cast<int>(plc_data.rowHeight));

	// Load fonts
	m_plc_header_font = plc_data.headerFont;
	m_plc_cell_font = plc_data.cellFont;
	if (m_plc_header_font.family().isEmpty()) {
		m_plc_header_font = QETApp::diagramTextsFont();
		m_plc_header_font.setBold(true);
		m_plc_header_font.setPointSize(8);
	}
	if (m_plc_cell_font.family().isEmpty()) {
		m_plc_cell_font = QETApp::diagramTextsFont();
		m_plc_cell_font.setPointSize(8);
	}
	m_plc_header_font_btn->setText(tr("Police des en-têtes: %1 %2pt")
		.arg(m_plc_header_font.family()).arg(m_plc_header_font.pointSize()));
	m_plc_cell_font_btn->setText(tr("Police du texte: %1 %2pt")
		.arg(m_plc_cell_font.family()).arg(m_plc_cell_font.pointSize()));
	m_plc_show_headers_cb->setChecked(plc_data.showHeaders);

	// UI columns 0-4 map to data model indices 0,1,2,3,4
	const int ui_to_data[] = {0, 1, 2, 3, 4};

	for (int i = 0; i < 5; ++i) {
		int di = ui_to_data[i];
		m_plc_col_visibility_checkboxes.at(i)->setChecked(
			plc_data.colVisible.value(di, true));
		m_plc_col_width_spinboxes.at(i)->setValue(
			static_cast<int>(plc_data.colWidths.value(di, 40)));

		if (i < plc_data.columnNames.size())
			m_plc_col_name_edits.at(i)->setText(plc_data.columnNames.at(i));
		else
			m_plc_col_name_edits.at(i)->clear();
	}

	// Restore column visual order (translate data model index → table col index)
	QHeaderView *hdr = m_plc_table->horizontalHeader();
	if (!plc_data.columnOrder.isEmpty()) {
		const int data_to_table[] = {0, 1, 2, 3, 4};
		for (int visual = 0; visual < plc_data.columnOrder.size(); ++visual) {
			int data_idx = plc_data.columnOrder.at(visual);
			if (data_idx < 0 || data_idx > 4) continue;
			int logical = data_to_table[data_idx];
			if (logical < 0 || logical >= 5) continue;
			if (hdr->visualIndex(logical) != visual)
				hdr->moveSection(hdr->visualIndex(logical), visual);
		}
	}

	// Ensure scrollbars stay hidden and sync shared scrollbar range
	if (m_plc_table) {
		m_plc_table->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
		m_plc_table->verticalScrollBar()->setVisible(false);
	}
	if (m_plc_terminal_table) {
		m_plc_terminal_table->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
		m_plc_terminal_table->verticalScrollBar()->setVisible(false);
	}
	if (m_plc_shared_scrollbar && m_plc_table) {
		int max = qMax(m_plc_table->verticalScrollBar()->maximum(),
				   m_plc_terminal_table->verticalScrollBar()->maximum());
		m_plc_shared_scrollbar->setMaximum(max);
	}
}

/**
 * @brief ElementPropertiesEditorWidget::readPlcTable
 * Read the PLC data from the table back into m_data
 */
void ElementPropertiesEditorWidget::readPlcTable()
{
	if (!m_plc_table)
		return;

	ElementData::PlcMasterData plc_data;

	for (int row = 0; row < m_plc_table->rowCount(); ++row) {
		ElementData::PlcIO io;

		auto *type_cb = qobject_cast<QComboBox *>(m_plc_table->cellWidget(row, 0));
		if (type_cb)
			io.type = static_cast<ElementData::PlcIOType>(type_cb->currentData().toInt());

		auto *addr_item = m_plc_table->item(row, 1);
		if (addr_item)
			io.address = addr_item->text();

		auto *func_item = m_plc_table->item(row, 2);
		if (func_item)
			io.functionText = func_item->text();

		auto *comment_item = m_plc_table->item(row, 3);
		if (comment_item)
			io.comment = comment_item->text();

		auto *crossref_item = m_plc_table->item(row, 4);
		if (crossref_item)
			io.crossRef = crossref_item->text();

		// Read terminal data from terminal table
		if (m_plc_terminal_table && row < m_plc_terminal_table->rowCount()) {
			auto *tc_sb = qobject_cast<QSpinBox *>(m_plc_terminal_table->cellWidget(row, 0));
			if (tc_sb) {
				io.terminalCount = tc_sb->value();
				for (int i = 0; i < io.terminalCount && i < 4; ++i) {
					auto *term_item = m_plc_terminal_table->item(row, i + 1);
					io.terminals.append(term_item ? term_item->text() : QString());
				}
			}
		}

		plc_data.ios.append(io);
	}

	plc_data.rowHeight = m_plc_row_height_spinbox->value();

	// Save break positions
	for (int i = 0; i < 4; ++i) {
		if (m_plc_break_checkboxes[i]->isChecked())
			plc_data.breakPositions.append(m_plc_break_spinboxes[i]->value());
		else
			plc_data.breakPositions.append(0);
	}

	// UI columns 0-4 map to data model indices 0,1,2,3,4
	const int ui_to_data[] = {0, 1, 2, 3, 4};

	for (int i = 0; i < 5; ++i) {
		int di = ui_to_data[i];
		plc_data.colVisible[di] = m_plc_col_visibility_checkboxes.at(i)->isChecked();
		plc_data.colWidths[di] = m_plc_col_width_spinboxes.at(i)->value();

		QString name = m_plc_col_name_edits.at(i)->text().trimmed();
		if (!name.isEmpty())
			plc_data.columnNames.append(name);
		else
			plc_data.columnNames.append(QString());
	}

	// Save current column visual order (translate table col index → data model index)
	QHeaderView *hdr = m_plc_table->horizontalHeader();
	const int table_to_data[] = {0, 1, 2, 3, 4};
	for (int visual = 0; visual < 5; ++visual) {
		int table_col = hdr->logicalIndex(visual);
		plc_data.columnOrder.append(table_to_data[table_col]);
	}

	plc_data.headerFont = m_plc_header_font;
	plc_data.cellFont = m_plc_cell_font;
	plc_data.showHeaders = m_plc_show_headers_cb->isChecked();

	m_data.setPlcMasterData(plc_data);
}

/**
 * @brief ElementPropertiesEditorWidget::plcAddRow
 * Add a new empty row to the PLC IO table
 */
void ElementPropertiesEditorWidget::plcAddRow()
{
	if (!m_plc_table)
		return;

	int row = m_plc_table->rowCount();
	m_plc_table->insertRow(row);

	// Type combo
	auto *type_cb = new QComboBox(m_plc_table);
	QStringList plc_types = ElementData::plcIOTypeList();
	for (int t = 0; t < plc_types.size(); ++t) {
		type_cb->addItem(plc_types.at(t), t);
	}
	m_plc_table->setCellWidget(row, 0, type_cb);

	m_plc_table->setItem(row, 1, new QTableWidgetItem());
	m_plc_table->setItem(row, 2, new QTableWidgetItem());
	m_plc_table->setItem(row, 3, new QTableWidgetItem());

	auto *crossref_item = new QTableWidgetItem();
	crossref_item->setFlags(crossref_item->flags() & ~Qt::ItemIsEditable);
	m_plc_table->setItem(row, 4, crossref_item);

	// Terminal table row
	if (m_plc_terminal_table) {
		m_plc_terminal_table->insertRow(row);

		// Ensure enough columns (at least Nb + T1)
		while (m_plc_terminal_table->columnCount() < 2)
			m_plc_terminal_table->insertColumn(m_plc_terminal_table->columnCount());

		auto *tc_sb = new QSpinBox(m_plc_terminal_table);
		tc_sb->setMinimum(1);
		tc_sb->setMaximum(4);
		tc_sb->setValue(1);
		m_plc_terminal_table->setCellWidget(row, 0, tc_sb);
		connect(tc_sb, QOverload<int>::of(&QSpinBox::valueChanged),
			this, [this, row](int val) { plcTerminalCountChanged(row, val); });

		// T1 editable, rest gray
		auto *t1_item = new QTableWidgetItem();
		m_plc_terminal_table->setItem(row, 1, t1_item);

		for (int i = 2; i < m_plc_terminal_table->columnCount(); ++i) {
			auto *item = new QTableWidgetItem();
			item->setFlags(item->flags() & ~Qt::ItemIsEditable);
			item->setBackground(QColor(230, 230, 230));
			m_plc_terminal_table->setItem(row, i, item);
		}
	}
}

/**
 * @brief ElementPropertiesEditorWidget::plcTerminalCountChanged
 * Enable/disable T1-T4 cells and grow columns based on terminal count spinner for a row
 */
void ElementPropertiesEditorWidget::plcTerminalCountChanged(int row, int count)
{
	Q_UNUSED(row)
	Q_UNUSED(count)
	if (!m_plc_terminal_table)
		return;

	// Find max terminal count across all rows
	int max_tc = 1;
	for (int r = 0; r < m_plc_terminal_table->rowCount(); ++r) {
		auto *sb = qobject_cast<QSpinBox *>(m_plc_terminal_table->cellWidget(r, 0));
		if (sb && sb->value() > max_tc)
			max_tc = sb->value();
	}

	// Ensure enough columns
	int needed = max_tc + 1; // +1 for Nb column
	while (m_plc_terminal_table->columnCount() < needed)
		m_plc_terminal_table->insertColumn(m_plc_terminal_table->columnCount());

	// Rebuild headers
	QStringList th;
	th << tr("Nb.");
	for (int i = 1; i < m_plc_terminal_table->columnCount(); ++i)
		th << tr("T%1").arg(i);
	m_plc_terminal_table->setHorizontalHeaderLabels(th);

	// Enable/disable cells per row
	for (int r = 0; r < m_plc_terminal_table->rowCount(); ++r) {
		auto *sb = qobject_cast<QSpinBox *>(m_plc_terminal_table->cellWidget(r, 0));
		int tc = sb ? sb->value() : 1;
		for (int c = 1; c < m_plc_terminal_table->columnCount(); ++c) {
			auto *item = m_plc_terminal_table->item(r, c);
			if (!item) {
				item = new QTableWidgetItem();
				m_plc_terminal_table->setItem(r, c, item);
			}
			if (c <= tc) {
				item->setFlags(item->flags() | Qt::ItemIsEditable);
				item->setBackground(Qt::NoBrush);
			} else {
				item->setFlags(item->flags() & ~Qt::ItemIsEditable);
				item->setText(QString());
				item->setBackground(QColor(230, 230, 230));
			}
		}
	}
}

/**
 * @brief ElementPropertiesEditorWidget::plcRemoveRow
 * Remove selected rows from the PLC IO table
 */
void ElementPropertiesEditorWidget::plcRemoveRow()
{
	if (!m_plc_table)
		return;

	QModelIndexList selected = m_plc_table->selectionModel()->selectedRows();
	if (selected.isEmpty())
		return;

	// Remove from bottom to top
	std::sort(selected.begin(), selected.end(),
		[](const QModelIndex &a, const QModelIndex &b) { return a.row() > b.row(); });

	for (const QModelIndex &idx : selected) {
		m_plc_table->removeRow(idx.row());
		if (m_plc_terminal_table && idx.row() < m_plc_terminal_table->rowCount())
			m_plc_terminal_table->removeRow(idx.row());
	}
}

/**
 * @brief ElementPropertiesEditorWidget::plcPasteFromClipboard
 * Paste IO data from clipboard (tab-separated, e.g. from Excel)
 */
void ElementPropertiesEditorWidget::plcPasteFromClipboard()
{
	if (!m_plc_table)
		return;

	QString clipboard_text = QApplication::clipboard()->text();
	if (clipboard_text.isEmpty())
		return;

	QStringList lines = clipboard_text.split('\n', Qt::SkipEmptyParts);
	if (lines.isEmpty())
		return;

	bool has_tabs = false;
	for (const QString &line : lines) {
		if (line.contains('\t')) {
			has_tabs = true;
			break;
		}
	}

	if (!has_tabs) {
		// Vertical paste: values go down the same column
		int target_col = m_plc_table->currentColumn();
		if (target_col < 0) target_col = 0;
		int target_row = m_plc_table->currentRow();
		if (target_row < 0) target_row = 0;
		int max_rows = m_plc_table->rowCount();

		for (int i = 0; i < lines.size(); ++i) {
			int row = target_row + i;
			if (row >= max_rows) break;
			plcSetCellFromValue(row, target_col, lines.at(i).trimmed());
		}
	} else {
		// Horizontal paste: each line is a separate IO row
		int target_row = m_plc_table->currentRow();
		if (target_row < 0) target_row = 0;
		int max_rows = m_plc_table->rowCount();

		for (int i = 0; i < lines.size(); ++i) {
			int row = target_row + i;
			if (row >= max_rows) break;
			QStringList cells = lines.at(i).split('\t');
			for (int c = 0; c < cells.size(); ++c) {
				if (c > 4) break;
				plcSetCellFromValue(row, c, cells.at(c).trimmed());
			}
		}
	}
}

/**
 * @brief ElementPropertiesEditorWidget::plcSetCellFromValue
 * Set a single table cell value, respecting the column widget type.
 */
void ElementPropertiesEditorWidget::plcSetCellFromValue(int row, int col, const QString &val)
{
	if (!m_plc_table || row < 0 || col < 0 || col > 4)
		return;

	if (col == 0) {
		auto *type_cb = qobject_cast<QComboBox*>(m_plc_table->cellWidget(row, col));
		if (!type_cb)
			return;
		if (!val.isEmpty()) {
			QStringList plc_types = ElementData::plcIOTypeList();
			for (int t = 0; t < plc_types.size(); ++t) {
				if (plc_types.at(t).compare(val, Qt::CaseInsensitive) == 0) {
					type_cb->setCurrentIndex(t);
					return;
				}
			}
		}
	}
	else if (col == 4) {
		// CrossRef - read-only
		auto *item = new QTableWidgetItem(val);
		item->setFlags(item->flags() & ~Qt::ItemIsEditable);
		m_plc_table->setItem(row, col, item);
	}
	else {
		// Text columns: Address, Function, Comment
		m_plc_table->setItem(row, col, new QTableWidgetItem(val));
	}
}

/**
 * @brief ElementPropertiesEditorWidget::plcSelectHeaderFont
 * Open font dialog to select column header font
 */
void ElementPropertiesEditorWidget::plcSelectHeaderFont()
{
	bool ok;
	QFont font = QFontDialog::getFont(&ok, m_plc_header_font, this,
		tr("Police des en-têtes de colonnes"));
	if (ok) {
		m_plc_header_font = font;
		m_plc_header_font_btn->setText(tr("Police des en-têtes: %1 %2pt")
			.arg(font.family()).arg(font.pointSize()));
	}
}

/**
 * @brief ElementPropertiesEditorWidget::plcSelectCellFont
 * Open font dialog to select cell text font
 */
void ElementPropertiesEditorWidget::plcSelectCellFont()
{
	bool ok;
	QFont font = QFontDialog::getFont(&ok, m_plc_cell_font, this,
		tr("Police du texte des cellules"));
	if (ok) {
		m_plc_cell_font = font;
		m_plc_cell_font_btn->setText(tr("Police du texte: %1 %2pt")
			.arg(font.family()).arg(font.pointSize()));
	}
}
