/*
	Copyright 2006-2017 The QElectroTech Team
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
#include "ui_elementpropertieseditorwidget.h"
#include "qetapp.h"

#include <QItemDelegate>

/**
 * @brief The EditorDelegate class
 * This delegate is only use for disable the edition of the first
 * column of the information tree widget
 */
class EditorDelegate : public QItemDelegate
{
	public:
		EditorDelegate(QObject *parent) :
			QItemDelegate(parent)
		{}

	QWidget* createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const
	{
		if(index.column() == 1)
		{
			return QItemDelegate::createEditor(parent, option, index);
		}
		return nullptr;
	}
};

/**
 * @brief ElementPropertiesEditorWidget::ElementPropertiesEditorWidget
 * Default constructor
 * @param basic_type : QString of the drawed element
 * @param kind_info : DiagramContext to store kindInfo of drawed element
 * @param elmt_info : the information of element (label, manufacturer etc...]
 * @param parent : parent widget
 */
ElementPropertiesEditorWidget::ElementPropertiesEditorWidget(QString &basic_type, DiagramContext &kind_info, DiagramContext &elmt_info, QWidget *parent) :
	QDialog(parent),
	ui(new Ui::ElementPropertiesEditorWidget),
	m_basic_type(basic_type),
	m_kind_info (kind_info),
	m_elmt_info (elmt_info)
{
	ui->setupUi(this);
	setUpInterface();
	upDateInterface();
	qDebug() << "const";
}

/**
 * @brief ElementPropertiesEditorWidget::~ElementPropertiesEditorWidget
 * Default destructor
 */
ElementPropertiesEditorWidget::~ElementPropertiesEditorWidget()
{
	delete ui;
}

/**
 * @brief ElementPropertiesEditorWidget::upDateInterface
 * Update the interface with the curent value
 */
void ElementPropertiesEditorWidget::upDateInterface()
{
	ui->m_base_type_cb->setCurrentIndex(ui->m_base_type_cb->findData(QVariant(m_basic_type)));
	
	if (m_basic_type == "slave")
	{
		ui->m_state_cb->setCurrentIndex(ui->m_state_cb->findData(m_kind_info["state"].toString()));
		ui->m_type_cb->setCurrentIndex (ui->m_type_cb->findData(m_kind_info["type"].toString()));
		ui->m_number_ctc->setValue(m_kind_info["number"].toInt());
	}
	else if (m_basic_type == "master") {
		ui->m_master_type_cb->setCurrentIndex(ui->m_master_type_cb->findData (m_kind_info["type"]));
	}

	on_m_base_type_cb_currentIndexChanged(ui->m_base_type_cb->currentIndex());
}

/**
 * @brief ElementPropertiesEditorWidget::setUpInterface
 */
void ElementPropertiesEditorWidget::setUpInterface()
{
		// Type combo box
	ui->m_base_type_cb->addItem (tr("Simple"),					  QVariant("simple"));
	ui->m_base_type_cb->addItem (tr("Maître"),					  QVariant("master"));
	ui->m_base_type_cb->addItem (tr("Esclave"),					  QVariant("slave"));
	ui->m_base_type_cb->addItem (tr("Renvoi de folio suivant"),	  QVariant("next_report"));
	ui->m_base_type_cb->addItem (tr("Renvoi de folio précédent"), QVariant("previous_report"));
	ui->m_base_type_cb->addItem (tr("Bornier"),                   QVariant("terminal"));

		// Slave option
	ui->m_state_cb->addItem(tr("Normalement ouvert"),QVariant("NO"));
	ui->m_state_cb->addItem(tr("Normalement fermé"), QVariant("NC"));
	ui->m_state_cb->addItem(tr("Inverseur"),		 QVariant("SW"));
	ui->m_type_cb->addItem(tr("Simple"),			 QVariant("simple"));
	ui->m_type_cb->addItem(tr("Puissance"),			QVariant("power"));
	ui->m_type_cb->addItem(tr("Temporisé travail"), QVariant("delayOn"));
	ui->m_type_cb->addItem(tr("Temporisé repos"),	QVariant("delayOff"));

		//Master option
	ui->m_master_type_cb->addItem(tr("Bobine"),				  QVariant("coil"));
	ui->m_master_type_cb->addItem(tr("Organe de protection"), QVariant("protection"));
	ui->m_master_type_cb->addItem(tr("Commutateur / bouton"), QVariant("commutator"));
	
		//Disable the edition of the first column of the information tree
		//by this little workaround
	ui->m_tree->setItemDelegate(new EditorDelegate(this));
	ui->m_tree->header()->resizeSection(0, 150);
	populateTree();
}

void ElementPropertiesEditorWidget::updateTree()
{
	QString type = ui->m_base_type_cb->itemData(ui->m_base_type_cb->currentIndex()).toString();
	
	if (type == "master")
		ui->m_tree->setEnabled(true);
	else if (type == "slave")
		ui->m_tree->setDisabled(true);
	else if (type == "simple")
		ui->m_tree->setEnabled(true);
	else if (type == "next_report")
		ui->m_tree->setDisabled(true);
	else if (type == "previous_report")
		ui->m_tree->setDisabled(true);
	else if (type == "terminal")
		ui->m_tree->setEnabled(true);
}

/**
 * @brief ElementPropertiesEditorWidget::populateTree
 * Create QTreeWidgetItem of the tree widget and populate it
 */
void ElementPropertiesEditorWidget::populateTree()
{
	QStringList keys{"label", "group-function", "comment", "description", "designation", "manufacturer", "manufacturer-reference", "provider", "quantity", "unity", "machine-manufacturer-reference"};
	
	for(const QString& key : keys)
	{
		QTreeWidgetItem *qtwi = new QTreeWidgetItem(ui->m_tree);
		qtwi->setFlags(Qt::ItemIsEnabled | Qt::ItemIsEditable);
		qtwi->setData(0, Qt::DisplayRole, QETApp::elementTranslatedInfoKey(key));
		qtwi->setData(0, Qt::UserRole, key);
		qtwi->setText(1, m_elmt_info.value(key).toString());
	}
}

/**
 * @brief ElementPropertiesEditorWidget::on_m_buttonBox_accepted
 * Action on button accepted : the new information is set
 */
void ElementPropertiesEditorWidget::on_m_buttonBox_accepted()
{
	m_basic_type = ui -> m_base_type_cb -> itemData(ui -> m_base_type_cb -> currentIndex()).toString();
	if (m_basic_type == "slave") {
		m_kind_info.addValue("state",  ui -> m_state_cb -> itemData(ui -> m_state_cb -> currentIndex()));
		m_kind_info.addValue("type",	ui -> m_type_cb  -> itemData(ui -> m_type_cb  -> currentIndex()));
		m_kind_info.addValue("number", QVariant(ui -> m_number_ctc -> value()));
	}
	else if(m_basic_type == "master") {
		m_kind_info.addValue("type", ui -> m_master_type_cb -> itemData(ui -> m_master_type_cb -> currentIndex()));
	}
	
	for (QTreeWidgetItem *qtwi : ui->m_tree->invisibleRootItem()->takeChildren()) {
		m_elmt_info.addValue(qtwi->data(0, Qt::UserRole).toString(), qtwi->text(1));
	}
	
	this->close();
}

/**
 * @brief ElementPropertiesEditorWidget::on_m_base_type_cb_currentIndexChanged
 * @param index : Action when combo-box base type index change
 */
void ElementPropertiesEditorWidget::on_m_base_type_cb_currentIndexChanged(int index)
{
	bool slave = false , master = false;

	if		(ui->m_base_type_cb->itemData(index).toString() == "slave")  slave  = true;
	else if (ui->m_base_type_cb->itemData(index).toString() == "master") master = true;

	ui->m_slave_gb->setVisible(slave);
	ui->m_master_gb->setVisible(master);
	
	updateTree();
}
