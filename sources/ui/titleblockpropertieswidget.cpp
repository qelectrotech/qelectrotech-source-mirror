/*
	Copyright 2006-2015 The QElectroTech Team
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
#include <QMenu>

/**
 * @brief TitleBlockPropertiesWidget::TitleBlockPropertiesWidget
 * default constructor
 * @param titleblock properties to edit
 * @param current_date if true, display the radio button "current date"
 * @param parent parent widget
 */
TitleBlockPropertiesWidget::TitleBlockPropertiesWidget(const TitleBlockProperties &titleblock, bool current_date, QWidget *parent) :
	QWidget(parent),
	ui(new Ui::TitleBlockPropertiesWidget),
	m_tbt_collection(nullptr)
{
	ui->setupUi(this);
	initDialog(current_date);
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
TitleBlockPropertiesWidget::TitleBlockPropertiesWidget(TitleBlockTemplatesCollection *tbt_collection, const TitleBlockProperties &titleblock, bool current_date, QWidget *parent) :
	QWidget(parent),
	ui(new Ui::TitleBlockPropertiesWidget),
	m_tbt_collection(nullptr)
{
	ui->setupUi(this);
	initDialog(current_date);
	setTitleBlockTemplatesCollection(tbt_collection);
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

	if (!properties.template_name.isEmpty()) {
		int matching_index = ui -> m_tbt_cb -> findData (properties.template_name);
		if (matching_index != -1)
			ui -> m_tbt_cb -> setCurrentIndex(matching_index);
	}

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
	}

	prop.context = m_dcw -> context();

	return prop;
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
 * @brief TitleBlockPropertiesWidget::setCurrentTitleBlockTemplateName
 * set the current title block "name", if "name" doesn't match, this method do nothing
 * @param name
 */
void TitleBlockPropertiesWidget::setCurrentTitleBlockTemplateName (const QString &name) {
	int index = ui -> m_tbt_cb -> findData(name);
	if (index != -1)
		ui -> m_tbt_cb -> setCurrentIndex(index);
}

/**
 * @brief TitleBlockPropertiesWidget::setTitleBlockTemplatesCollection
 * Set the collection of title block
 * @param tbt_collection
 */
void TitleBlockPropertiesWidget::setTitleBlockTemplatesCollection(TitleBlockTemplatesCollection *tbt_collection) {
	if (!tbt_collection) return;
	setTitleBlockTemplatesVisible(true);
	if (m_tbt_collection && tbt_collection != m_tbt_collection) {
		// forget any connection with the previous collection
		disconnect(m_tbt_collection, 0, this, 0);
	}

	m_tbt_collection = tbt_collection;
	updateTemplateList();
	connect(m_tbt_collection, SIGNAL(changed(TitleBlockTemplatesCollection*,QString)), this, SLOT(updateTemplateList()));
}

/**
 * @brief TitleBlockPropertiesWidget::setTitleBlockTemplatesList
 * Set the list of title block used.
 * Fill the combo box of title box with each name of title block.
 * @param tbt
 */
void TitleBlockPropertiesWidget::setTitleBlockTemplatesList(const QStringList &tbt) {
	ui -> m_tbt_cb ->clear();
	ui -> m_tbt_cb -> addItem(QET::Icons::TitleBlock, tr("Modèle par défaut"));
	foreach (QString tbt_name, tbt) {
		ui -> m_tbt_cb -> addItem(QET::Icons::TitleBlock, tbt_name, tbt_name);
	}
}

/**
 * @brief TitleBlockPropertiesWidget::initDialog
 * Init this dialog
 * @param current_date true for display current date radio button
 */
void TitleBlockPropertiesWidget::initDialog(const bool &current_date) {
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

	connect(ui->m_tbt_cb, SIGNAL(currentIndexChanged(QString)), this, SLOT(changeCurrentTitleBlockTemplate(QString)));
}

void TitleBlockPropertiesWidget::editCurrentTitleBlockTemplate() {
	emit(editTitleBlockTemplate(currentTitleBlockTemplateName(), false));
}

void TitleBlockPropertiesWidget::duplicateCurrentTitleBlockTemplate() {
	emit(editTitleBlockTemplate(currentTitleBlockTemplateName(), true));
}

/**
 * @brief TitleBlockPropertiesWidget::updateTemplateList
 * Update the title block template list
 */
void TitleBlockPropertiesWidget::updateTemplateList() {
	if (!m_tbt_collection) return;

	QString current_template_name = currentTitleBlockTemplateName();
	setTitleBlockTemplatesList(m_tbt_collection -> templates());
	setCurrentTitleBlockTemplateName(current_template_name);
}

/**
 * @brief TitleBlockPropertiesWidget::changeCurrentTitleBlockTemplate
 * Load the additionnal field of title block "text"
 */
void TitleBlockPropertiesWidget::changeCurrentTitleBlockTemplate(QString name) {
	// delete all entry
	m_dcw -> clear();
	// get template
	TitleBlockTemplate *tpl = m_tbt_collection -> getTemplate( name );
	if(tpl != 0) {
		// get all template fields
		QStringList fields = tpl -> listOfVariables();
		// set fields to additional_fields_ widget
		DiagramContext templateContext;
		for(int i =0; i<fields.count(); i++)
			templateContext.addValue(fields.at(i), "");
		m_dcw->setContext(templateContext);
	}
}

/**
 * @brief TitleBlockPropertiesWidget::on_m_date_now_pb_clicked
 * Set the date to current date
 */
void TitleBlockPropertiesWidget::on_m_date_now_pb_clicked() {
	ui -> m_date_edit -> setDate(QDate::currentDate());
}
