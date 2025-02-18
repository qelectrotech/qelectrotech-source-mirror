/*
	Copyright 2006-2025 The QElectroTech Team
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
	ui->m_tree->header()->resizeSection(0, 150);
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
