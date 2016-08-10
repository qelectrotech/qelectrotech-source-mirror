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
#include "titleblockpropertieswidget.h"
#include "ui_titleblockpropertieswidget.h"
#include "templatescollection.h"
#include "qeticons.h"
#include "titleblocktemplate.h"
#include "qetapp.h"
#include <QMenu>

/**
 * @brief TitleBlockPropertiesWidget::TitleBlockPropertiesWidget
 * default constructor
 * @param titleblock properties to edit
 * @param current_date if true, display the radio button "current date"
 * @param parent parent widget
 */
TitleBlockPropertiesWidget::TitleBlockPropertiesWidget(const TitleBlockProperties &titleblock, bool current_date, QETProject *project, QWidget *parent) :
	QWidget(parent),
	ui(new Ui::TitleBlockPropertiesWidget)
{
	ui->setupUi(this);
    	initDialog(current_date, project);
    	setProperties(titleblock);
}

/**
 * @brief TitleBlockPropertiesWidget::TitleBlockPropertiesWidget
 * default constructor with tempalte list
 * @param tbt_collection template list
 * @param titleblock properties to edit
 * @param current_date if true, display the radio button "current date"
 * @param parent parent widget
 */
TitleBlockPropertiesWidget::TitleBlockPropertiesWidget(TitleBlockTemplatesCollection *tbt_collection, const TitleBlockProperties &titleblock, bool current_date, QETProject *project, QWidget *parent) :
	QWidget(parent),
	ui(new Ui::TitleBlockPropertiesWidget)
{
	ui->setupUi(this);
    initDialog(current_date,project);
	addCollection(tbt_collection);
	updateTemplateList();
	setProperties(titleblock);
}

/**
 * @brief TitleBlockPropertiesWidget::TitleBlockPropertiesWidget
 * Default constructor with several template collection
 * @param tbt_collection template list
 * @param titleblock properties to edit
 * @param current_date if true, display the radio button "current date"
 * @param parent parent widget
 */
TitleBlockPropertiesWidget::TitleBlockPropertiesWidget(QList<TitleBlockTemplatesCollection *> tbt_collection, const TitleBlockProperties &titleblock, bool current_date, QETProject *project, QWidget *parent) :
	QWidget(parent),
	ui(new Ui::TitleBlockPropertiesWidget)
{
	ui->setupUi(this);
	initDialog(current_date,project);
	foreach (TitleBlockTemplatesCollection *c, tbt_collection)
		addCollection(c);
	updateTemplateList();
	setProperties(titleblock);
}

/**
 * @brief TitleBlockPropertiesWidget::~TitleBlockPropertiesWidget
 * destructor
 */
TitleBlockPropertiesWidget::~TitleBlockPropertiesWidget()
{
	delete ui;
}

/**
 * @brief TitleBlockPropertiesWidget::setProperties
 * @param properties
 */
void TitleBlockPropertiesWidget::setProperties(const TitleBlockProperties &properties) {
	ui -> m_title_le  -> setText (properties.title);
	ui -> m_author_le -> setText (properties.author);
	ui -> m_file_le   -> setText (properties.filename);
	ui -> m_folio_le  -> setText (properties.folio);
	ui -> m_display_at_cb -> setCurrentIndex(properties.display_at == Qt::BottomEdge ? 0 : 1);
	ui->auto_page_cb->setCurrentText(properties.auto_page_num);

	//About date	
	ui -> m_date_now_pb -> setDisabled(true);
	ui -> m_date_edit   -> setDisabled(true);
	ui -> m_date_edit   -> setDate(QDate::currentDate());

	if (!ui -> m_current_date_rb -> isHidden()) {
		if(properties.useDate == TitleBlockProperties::CurrentDate)
			ui -> m_current_date_rb -> setChecked(true);
		else {
			if (properties.date.isNull())
				ui -> m_no_date_rb -> setChecked(true);
			else {
				ui -> m_fixed_date_rb -> setChecked(true);
				ui -> m_date_edit -> setDate(properties.date);
			}
		}
	}
	else {
		if (properties.useDate == TitleBlockProperties::CurrentDate)
			ui -> m_fixed_date_rb ->setChecked(true);
		else {
			if (properties.date.isNull())
				ui -> m_no_date_rb -> setChecked(true);
			else {
				ui -> m_fixed_date_rb -> setChecked(true);
				ui -> m_date_edit -> setDate(properties.date);
			}
		}
	} //About date

		//Set the current titleblock if any
	int index = 0;
	if (!properties.template_name.isEmpty())
	{
		index = getIndexFor(properties.template_name, properties.collection);
		if (index == -1) index = 0;
	}
	ui -> m_tbt_cb -> setCurrentIndex(index);

	m_dcw -> setContext(properties.context);
}

/**
 * @brief TitleBlockPropertiesWidget::properties
 * @return the edited properties
 */
TitleBlockProperties TitleBlockPropertiesWidget::properties() const {
	TitleBlockProperties prop;
	prop.title    = ui -> m_title_le  -> text();
	prop.author   = ui -> m_author_le -> text();
	prop.filename = ui -> m_file_le   -> text();
	prop.folio    = ui -> m_folio_le  -> text();
	prop.display_at = ui -> m_display_at_cb -> currentIndex() == 0 ? Qt::BottomEdge : Qt::RightEdge;

	if (ui->m_no_date_rb->isChecked()) {
		prop.useDate = TitleBlockProperties::UseDateValue;
		prop.date = QDate();
	}
	else if (ui -> m_fixed_date_rb -> isChecked()) {
		prop.useDate = TitleBlockProperties::UseDateValue;
		prop.date = ui->m_date_edit->date();
	}
	else if (ui->m_current_date_rb->isVisible() && ui->m_current_date_rb->isChecked()) {
		prop.useDate = TitleBlockProperties::CurrentDate;
		prop.date = QDate::currentDate();
	}

	if (!currentTitleBlockTemplateName().isEmpty())
	{
		prop.template_name = currentTitleBlockTemplateName();
		prop.collection = m_map_index_to_collection_type.at(ui->m_tbt_cb->currentIndex());
	}

	prop.context = m_dcw -> context();

    prop.auto_page_num = ui->auto_page_cb->currentText();

	return prop;
}

/**
 * @brief TitleBlockPropertiesWidget::properties
 * @return return properties to enable folio autonum
 */
TitleBlockProperties TitleBlockPropertiesWidget::propertiesAutoNum(QString autoNum) const {
	TitleBlockProperties prop;
	prop.title    = ui -> m_title_le  -> text();
	prop.author   = ui -> m_author_le -> text();
	prop.filename = ui -> m_file_le   -> text();
	prop.folio    = "%autonum";
	prop.display_at = ui -> m_display_at_cb -> currentIndex() == 0 ? Qt::BottomEdge : Qt::RightEdge;

	if (ui->m_no_date_rb->isChecked()) {
		prop.useDate = TitleBlockProperties::UseDateValue;
		prop.date = QDate();
	}
	else if (ui -> m_fixed_date_rb -> isChecked()) {
		prop.useDate = TitleBlockProperties::UseDateValue;
		prop.date = ui->m_date_edit->date();
	}
	else if (ui->m_current_date_rb->isVisible() && ui->m_current_date_rb->isChecked()) {
		prop.useDate = TitleBlockProperties::CurrentDate;
		prop.date = QDate::currentDate();
	}

	if (!currentTitleBlockTemplateName().isEmpty())
	{
		prop.template_name = currentTitleBlockTemplateName();
		prop.collection = m_map_index_to_collection_type.at(ui->m_tbt_cb->currentIndex());
	}

	prop.context = m_dcw -> context();

	prop.auto_page_num = autoNum;

	return prop;
}

TitleBlockTemplateLocation TitleBlockPropertiesWidget::currentTitleBlockLocation() const
{
	QET::QetCollection qc = m_map_index_to_collection_type.at(ui->m_tbt_cb->currentIndex());
	TitleBlockTemplatesCollection *collection = nullptr;
	foreach (TitleBlockTemplatesCollection *c, m_tbt_collection_list)
		if (c -> collection() == qc)
			collection = c;

	if (!collection)
		return TitleBlockTemplateLocation();

	return collection->location(currentTitleBlockTemplateName());
}

/**
 * @brief TitleBlockPropertiesWidget::setTitleBlockTemplatesVisible
 * if true, title block template combo box and menu button is visible
 */
void TitleBlockPropertiesWidget::setTitleBlockTemplatesVisible(const bool &visible) {
	ui -> m_tbt_label -> setVisible(visible);
	ui -> m_tbt_cb    -> setVisible(visible);
	ui -> m_tbt_pb    -> setVisible(visible);
}

/**
 * @brief TitleBlockPropertiesWidget::setReadOnly
 * if true, this widget is disable
 */
void TitleBlockPropertiesWidget::setReadOnly(const bool &ro) {
	ui->m_tbt_gb->setDisabled(ro);
}

/**
 * @brief TitleBlockPropertiesWidget::currentTitleBlockTemplateName
 * @return the current title block name
 */
QString TitleBlockPropertiesWidget::currentTitleBlockTemplateName() const {
	int index = ui -> m_tbt_cb -> currentIndex();
	if(index != -1)
		return (ui -> m_tbt_cb -> itemData(index).toString());
	return QString();
}

/**
 * @brief TitleBlockPropertiesWidget::addCollection
 * add a collection of title block available in the combo box
 * @param tbt_collection
 */
void TitleBlockPropertiesWidget::addCollection(TitleBlockTemplatesCollection *tbt_collection)
{
	if (!tbt_collection || m_tbt_collection_list.contains(tbt_collection)) return;
	m_tbt_collection_list << tbt_collection;
}

/**
 * @brief TitleBlockPropertiesWidget::initDialog
 * Init this dialog
 * @param current_date true for display current date radio button
 */
void TitleBlockPropertiesWidget::initDialog(const bool &current_date,  QETProject *project) {
	m_dcw = new DiagramContextWidget();
	ui -> m_tab2_vlayout -> addWidget(m_dcw);

	setTitleBlockTemplatesVisible(false);
	ui -> m_current_date_rb -> setVisible(current_date);

	m_tbt_edit = new QAction(tr("Éditer ce modèle", "menu entry"), this);
	m_tbt_duplicate = new QAction(tr("Dupliquer et editer ce modèle", "menu entry"), this);

	connect(m_tbt_edit, SIGNAL(triggered()), this, SLOT(editCurrentTitleBlockTemplate()));
	connect(m_tbt_duplicate, SIGNAL(triggered()), this, SLOT(duplicateCurrentTitleBlockTemplate()));

	m_tbt_menu = new QMenu(tr("Title block templates actions"));
	m_tbt_menu -> addAction(m_tbt_edit);
	m_tbt_menu -> addAction(m_tbt_duplicate);
	ui -> m_tbt_pb -> setMenu(m_tbt_menu);

	connect(ui->m_tbt_cb, SIGNAL(currentIndexChanged(int)), this, SLOT(changeCurrentTitleBlockTemplate(int)));

	if (project!= NULL){
		keys_2 = project -> folioAutoNum().keys();
		foreach (QString str, keys_2) { ui -> auto_page_cb -> addItem(str); }
		if (ui->auto_page_cb->currentText()==NULL)
			ui->auto_page_cb->addItem(tr("Créer un Folio Numérotation Auto"));
	}
	else{
		ui->auto_page_cb->hide();
		ui->m_edit_autofolionum_pb->hide();
		ui->label_9->hide();
	}

}

/**
 * @brief TitleBlockPropertiesWidget::getIndexFor
 * Find the index of the combo box for the title block @tbt_name available on the collection @collection
 * @param tbt_name : title block name
 * @param collection : title block collection
 * @return the index of the title block or -1 if no match
 */
int TitleBlockPropertiesWidget::getIndexFor(const QString &tbt_name, const QET::QetCollection collection) const
{
	for (int i = 0; i<ui->m_tbt_cb->count(); i++) {
		if (ui->m_tbt_cb->itemData(i).toString() == tbt_name)
			if (m_map_index_to_collection_type.at(i) == collection)
				return i;
	}
	return -1;
}

void TitleBlockPropertiesWidget::editCurrentTitleBlockTemplate() {
	QETApp::instance()->openTitleBlockTemplate(currentTitleBlockLocation(), false);
}

void TitleBlockPropertiesWidget::duplicateCurrentTitleBlockTemplate() {
	QETApp::instance()->openTitleBlockTemplate(currentTitleBlockLocation(), true);
}

/**
 * @brief TitleBlockPropertiesWidget::updateTemplateList
 * Update the title block template list available in the combo box
 */
void TitleBlockPropertiesWidget::updateTemplateList()
{
	ui -> m_tbt_cb ->clear();

	if (m_tbt_collection_list.isEmpty())
	{
		setTitleBlockTemplatesVisible(false);
		return;
	}
	setTitleBlockTemplatesVisible(true);

		//Add the default title block
	m_map_index_to_collection_type.clear();
	m_map_index_to_collection_type.append(QET::QetCollection::Common);
	ui -> m_tbt_cb -> addItem(QET::Icons::QETLogo, tr("Modèle par défaut"));

		//Add every title block stored in m_tbt_collection_list
	foreach (TitleBlockTemplatesCollection *tbt_c, m_tbt_collection_list)
	{
		QIcon icon;
		QET::QetCollection qc = tbt_c -> collection();
		if (qc == QET::QetCollection::Common)
			icon = QET::Icons::QETLogo;
		else if (qc == QET::QetCollection::Custom)
			icon = QET::Icons::Home;
		else if (qc == QET::QetCollection::Embedded)
			icon = QET::Icons::TitleBlock;

		foreach(QString tbt_name, tbt_c -> templates())
		{
			m_map_index_to_collection_type.append(qc);
			ui -> m_tbt_cb -> addItem(icon, tbt_name, tbt_name);
		}
	}
}

/**
 * @brief TitleBlockPropertiesWidget::changeCurrentTitleBlockTemplate
 * Load the additionnal field of title block "text"
 */
void TitleBlockPropertiesWidget::changeCurrentTitleBlockTemplate(int index)
{
	m_dcw -> clear();

	QET::QetCollection qc = m_map_index_to_collection_type.at(index);
	TitleBlockTemplatesCollection *collection = nullptr;
	foreach (TitleBlockTemplatesCollection *c, m_tbt_collection_list)
		if (c -> collection() == qc)
			collection = c;

	if (!collection) return;

		// get template
	TitleBlockTemplate *tpl = collection -> getTemplate(ui -> m_tbt_cb -> currentText());
	if(tpl != 0) {
			// get all template fields
		QStringList fields = tpl -> listOfVariables();
			// set fields to additional_fields_ widget
		DiagramContext templateContext;
		for(int i =0; i<fields.count(); i++)
			templateContext.addValue(fields.at(i), "");
		m_dcw -> setContext(templateContext);
	}
}

/**
 * @brief TitleBlockPropertiesWidget::on_m_date_now_pb_clicked
 * Set the date to current date
 */
void TitleBlockPropertiesWidget::on_m_date_now_pb_clicked() {
	ui -> m_date_edit -> setDate(QDate::currentDate());
}

/**
 * @brief TitleBlockPropertiesWidget::on_m_edit_autofolionum_pb_clicked
 * Open Auto Folio Num dialog
 */
void TitleBlockPropertiesWidget::on_m_edit_autofolionum_pb_clicked() {
    emit openAutoNumFolioEditor(ui->auto_page_cb->currentText());
	if (ui->auto_page_cb->currentText()!=tr("Créer un Folio Numérotation Auto"))
    {
		//still to implement: load current auto folio num settings
    }
}
