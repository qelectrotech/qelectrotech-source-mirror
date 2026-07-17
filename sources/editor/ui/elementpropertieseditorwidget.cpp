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
		ui->m_state_cb->setCurrentIndex(
					ui->m_state_cb->findData(
						m_data.m_slave_state));
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
	ui->m_type_cb->addItem(tr("Simple"),                    ElementData::SSimple);
	ui->m_type_cb->addItem(tr("Puissance"),                 ElementData::Power);
	ui->m_type_cb->addItem(tr("Temporisé travail"),         ElementData::DelayOn);
	ui->m_type_cb->addItem(tr("Temporisé repos"),           ElementData::DelayOff);
	ui->m_type_cb->addItem(tr("Temporisé travail & repos"), ElementData::delayOnOff);

		//Master option
	ui->m_master_type_cb->addItem(tr("Bobine"),               ElementData::Coil);
	ui->m_master_type_cb->addItem(tr("Organe de protection"), ElementData::Protection);
	ui->m_master_type_cb->addItem(tr("Commutateur / bouton"), ElementData::Commutator);

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
	connect(ui->max_slaves_checkbox, SIGNAL(toggled(bool)), ui->max_slaves_spinbox, SLOT(setEnabled(bool)));
	connect(ui->max_slaves_spinbox, QOverload<int>::of(&QSpinBox::valueChanged), [this](int) {
		if (ui->m_slave_groups_checkbox->isChecked()) {
			populateSlaveGroupsTable();
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
		m_data.m_slave_state = ui->m_state_cb->currentData().value<ElementData::SlaveState>();
		m_data.m_slave_type  = ui->m_type_cb->currentData().value<ElementData::SlaveType>();
		m_data.m_contact_count = ui->m_number_ctc->value();
	}
		else if (m_data.m_type == ElementData::Master) {
		m_data.m_master_type = ui->m_master_type_cb->currentData().value<ElementData::MasterType>();

		//If the checkbox is checked, save the number; otherwise, -1 (infinity)
		if (ui->max_slaves_checkbox->isChecked()) {
			m_data.m_max_slaves = ui->max_slaves_spinbox->value();
		} else {
			m_data.m_max_slaves = -1;
		}

		readSlaveGroupsFromTable();
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

#if QT_VERSION >= QT_VERSION_CHECK(5,15,0)
	ui->tabWidget->setTabVisible(1,
								 (type_ == ElementData::Simple ||
								  type_ == ElementData::Master));
#endif

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

	// If we have existing groups, use their count (up to max_slaves)
	int existing_groups = m_data.m_slave_contact_groups.size();

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
