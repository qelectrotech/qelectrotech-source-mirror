/*
	Copyright 2006-2018 The QElectroTech Team
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
#include "searchandreplacewidget.h"
#include "ui_searchandreplacewidget.h"
#include "qetdiagrameditor.h"
#include "qetproject.h"
#include "diagram.h"
#include "qeticons.h"
#include "element.h"
#include "independenttextitem.h"
#include "conductor.h"
#include "replacefoliowidget.h"
#include "replaceelementdialog.h"
#include "qetapp.h"

#include <QSettings>

/**
 * @brief SearchAndReplaceWidget::SearchAndReplaceWidget
 * Constructor
 * @param parent
 */
SearchAndReplaceWidget::SearchAndReplaceWidget(QWidget *parent) :
	QWidget(parent),
	ui(new Ui::SearchAndReplaceWidget)
{
	ui->setupUi(this);
	setHideAdvanced(true);
	setUpTreeItems();
	
	connect(ui->m_search_le, &QLineEdit::textEdited, this, &SearchAndReplaceWidget::search);
}

/**
 * @brief SearchAndReplaceWidget::~SearchAndReplaceWidget
 * Destructor
 */
SearchAndReplaceWidget::~SearchAndReplaceWidget() {
	delete ui;
}

/**
 * @brief SearchAndReplaceWidget::event
 * Reimplemented to clear the the lines edit and hide
 * the advanced widgets, when this widget become hidden
 * @param event
 * @return 
 */
bool SearchAndReplaceWidget::event(QEvent *event)
{
	if (event->type() == QEvent::Hide)
	{
		clear();
		setHideAdvanced(true);
		if (m_highlighted_element)
		{
			m_highlighted_element.data()->setHighlighted(false);
			m_highlighted_element.clear();
		}
	}
	else if (event->type() == QEvent::Show)
	{
		ui->m_search_le->setFocus();
		fillItemsList();
		m_root_qtwi->setExpanded(true);
	}
	
	return QWidget::event(event);
}

/**
 * @brief SearchAndReplaceWidget::clear
 * Clear the content of the search and replace line edit
 * Clear all tree items in the tree widget (except the category items).
 */
void SearchAndReplaceWidget::clear()
{
	disconnect(ui->m_tree_widget, &QTreeWidget::itemChanged, this, &SearchAndReplaceWidget::itemChanged);
	
	qDeleteAll(m_diagram_hash.keys());
	m_diagram_hash.clear();
	
	qDeleteAll(m_element_hash.keys());
	m_element_hash.clear();
	
	qDeleteAll(m_text_hash.keys());
	m_text_hash.clear();
	
	qDeleteAll(m_conductor_hash.keys());
	m_conductor_hash.clear();
	
	for (QTreeWidgetItem *qtwi : m_category_qtwi)
		qtwi->setHidden(false);
	
	ui->m_tree_widget->collapseAll();
	ui->m_tree_widget->clearSelection();
	
	ui->m_search_le->clear();
	ui->m_replace_le->clear();
	updateNextPreviousButtons();
	ui->m_search_le->setPalette(QPalette());
}

/**
 * @brief SearchAndReplaceWidget::setEditor
 * Set the diagram editor of this widget
 * @param editor
 */
void SearchAndReplaceWidget::setEditor(QETDiagramEditor *editor) {
	m_editor = editor;
}

/**
 * @brief SearchAndReplaceWidget::setUpTreeItems
 * Set up the main tree widget items
 */
void SearchAndReplaceWidget::setUpTreeItems()
{
	m_root_qtwi = new QTreeWidgetItem(ui->m_tree_widget);
	m_root_qtwi->setIcon(0, QET::Icons::ProjectProperties);
	m_root_qtwi->setText(0, tr("Correspondance :"));
	m_root_qtwi->setCheckState(0, Qt::Checked);
	m_category_qtwi.append(m_root_qtwi);
	
	m_folio_qtwi = new QTreeWidgetItem(m_root_qtwi);
	m_folio_qtwi->setIcon(0, QET::Icons::Diagram);
	m_folio_qtwi->setText(0, tr("Folios"));
	m_folio_qtwi->setCheckState(0, Qt::Checked);
	m_category_qtwi.append(m_folio_qtwi);
	
	m_indi_text_qtwi = new QTreeWidgetItem(m_root_qtwi);
	m_indi_text_qtwi->setIcon(0, QET::Icons::PartText);
	m_indi_text_qtwi->setText(0, tr("Champs texte"));
	m_indi_text_qtwi->setCheckState(0, Qt::Checked);
	m_category_qtwi.append(m_indi_text_qtwi);
	
	m_elements_qtwi = new QTreeWidgetItem(m_root_qtwi);
	m_elements_qtwi->setIcon(0, QET::Icons::Element);
    m_elements_qtwi->setText(0, tr("Eléments"));
	m_elements_qtwi->setCheckState(0, Qt::Checked);
	m_category_qtwi.append(m_elements_qtwi);
	
	m_simple_elmt_qtwi = new QTreeWidgetItem(m_elements_qtwi);
    m_simple_elmt_qtwi->setText(0, tr("Eléments simple"));
	m_simple_elmt_qtwi->setIcon(0, QET::Icons::Element);
	m_simple_elmt_qtwi->setCheckState(0, Qt::Checked);
	m_qtwi_elmts.append(m_simple_elmt_qtwi);
	
	m_master_elmt_qtwi= new QTreeWidgetItem(m_elements_qtwi);
    m_master_elmt_qtwi->setText(0, tr("Eléments maître"));
	m_master_elmt_qtwi->setIcon(0, QET::Icons::ElementMaster);
	m_master_elmt_qtwi->setCheckState(0, Qt::Checked);
	m_qtwi_elmts.append(m_master_elmt_qtwi);
	
	m_slave_elmt_qtwi = new QTreeWidgetItem(m_elements_qtwi);
    m_slave_elmt_qtwi->setText(0, tr("Eléments esclave"));
	m_slave_elmt_qtwi->setIcon(0, QET::Icons::ElementSlave);
	m_slave_elmt_qtwi->setCheckState(0, Qt::Checked);
	m_qtwi_elmts.append(m_slave_elmt_qtwi);
	
	m_report_elmt_qtwi = new QTreeWidgetItem(m_elements_qtwi);
    m_report_elmt_qtwi->setText(0, tr("Eléments report de folio"));
	m_report_elmt_qtwi->setIcon(0, QET::Icons::FolioXrefComing);
	m_report_elmt_qtwi->setCheckState(0, Qt::Checked);
	m_qtwi_elmts.append(m_report_elmt_qtwi);
	
	m_terminal_elmt_qtwi = new QTreeWidgetItem(m_elements_qtwi);
    m_terminal_elmt_qtwi->setText(0, tr("Eléments bornier"));
	m_terminal_elmt_qtwi->setIcon(0, QET::Icons::ElementTerminal);
	m_terminal_elmt_qtwi->setCheckState(0, Qt::Checked);
	m_qtwi_elmts.append(m_terminal_elmt_qtwi);
	m_category_qtwi.append(m_qtwi_elmts);
	
	m_conductor_qtwi = new QTreeWidgetItem(m_root_qtwi);
	m_conductor_qtwi->setIcon(0, QET::Icons::Conductor);
	m_conductor_qtwi->setText(0, tr("Conducteurs"));
	m_conductor_qtwi->setCheckState(0, Qt::Checked);
	m_category_qtwi.append(m_conductor_qtwi);
	
	updateNextPreviousButtons();
}

/**
 * @brief SearchAndReplaceWidget::setHideAdvanced
 * Hide advanced widgets
 * @param hide
 */
void SearchAndReplaceWidget::setHideAdvanced(bool hide) const
{
	ui->m_advanced_pb      ->setChecked(!hide);
	ui->m_replace          ->setHidden(hide);
	ui->m_replace_le       ->setHidden(hide);
	ui->m_folio_pb         ->setHidden(hide);
	ui->m_element_pb       ->setHidden(hide);
	ui->m_conductor_pb     ->setHidden(hide);
	ui->m_tree_widget      ->setHidden(hide);
	ui->m_replace_pb       ->setHidden(hide);
	ui->m_replace_all_pb   ->setHidden(hide);
	ui->m_mode             ->setHidden(hide);
	ui->m_mode_cb          ->setHidden(hide);
	ui->m_case_sensitive_cb->setHidden(hide);
	QSize size = ui->m_v_spacer->sizeHint();
	ui->m_v_spacer->changeSize(size.width(), size.height(), QSizePolicy::Minimum, hide ? QSizePolicy::Expanding : QSizePolicy::Ignored);
}

/**
 * @brief SearchAndReplaceWidget::fillItemsList
 * Fill the tree
 */
void SearchAndReplaceWidget::fillItemsList()
{
	disconnect(ui->m_tree_widget, &QTreeWidget::itemChanged, this, &SearchAndReplaceWidget::itemChanged);
	
	qDeleteAll(m_element_hash.keys());
	m_element_hash.clear();
	
	QETProject *project_ = m_editor->currentProject();
	if (!project_)
	{
		ui->m_replace_all_pb->setDisabled(true);
		return;
	}
	ui->m_replace_all_pb->setEnabled(true);
	connect(project_, &QETProject::destroyed, this, &SearchAndReplaceWidget::on_m_reload_pb_clicked);

	
	DiagramContent dc;
	for (Diagram *diagram : project_->diagrams())
	{
		QString str;
		
		QSettings settings;
		if (settings.value("genericpanel/folio", true).toBool()) {
			str = diagram->border_and_titleblock.finalfolio();
		} else {
			str = QString::number(diagram->folioIndex());
		}
		
		str.append(" " + diagram->title());
		
		QTreeWidgetItem *qtwi = new QTreeWidgetItem(m_folio_qtwi);
		qtwi->setText(0, str);
		qtwi->setData(0, Qt::UserRole, searchTerms(diagram));
		qtwi->setCheckState(0, Qt::Checked);
		m_diagram_hash.insert(qtwi, QPointer<Diagram>(diagram));
		dc += DiagramContent(diagram, false);
	}
	
	for (Element *elmt : dc.m_elements)
		addElement(elmt);
	
		//Sort child of each "element type" tree item.
		//we hide, "element type" tree item, if they do not have children 
	for(QTreeWidgetItem *qtwi : m_qtwi_elmts)
	{
		qtwi->sortChildren(0, Qt::AscendingOrder);
		qtwi->setHidden(qtwi->childCount() ? false : true);
	}
	
	for (IndependentTextItem *iti : dc.m_text_fields)
	{
		QTreeWidgetItem *qtwi = new QTreeWidgetItem(m_indi_text_qtwi);
		qtwi->setText(0, iti->toPlainText());
		qtwi->setCheckState(0, Qt::Checked);
		qtwi->setData(0, Qt::UserRole, iti->toPlainText());
		m_text_hash.insert(qtwi, QPointer<IndependentTextItem>(iti));
	}
	m_indi_text_qtwi->sortChildren(0, Qt::AscendingOrder);

	for (Conductor *c : dc.m_potential_conductors)
	{
		QTreeWidgetItem *qtwi = new QTreeWidgetItem(m_conductor_qtwi);
		qtwi->setText(0, c->properties().text);
		qtwi->setCheckState(0, Qt::Checked);
		m_conductor_hash.insert(qtwi, QPointer<Conductor>(c));
	}
	m_conductor_qtwi->sortChildren(0, Qt::AscendingOrder);
	
	updateNextPreviousButtons();
	connect(ui->m_tree_widget, &QTreeWidget::itemChanged, this, &SearchAndReplaceWidget::itemChanged);
}

/**
 * @brief SearchAndReplaceWidget::addElement
 * Add a tree widget item for @element
 * @param element
 */
void SearchAndReplaceWidget::addElement(Element *element)
{
	QTreeWidgetItem *parent = m_elements_qtwi;
	switch (element->linkType()) {
		case Element::Simple:
			parent = m_simple_elmt_qtwi;
			break;
		case Element::NextReport:
			parent = m_report_elmt_qtwi;
			break;
		case Element::PreviousReport:
			parent = m_report_elmt_qtwi;
			break;
		case Element::Master:
			parent = m_master_elmt_qtwi;
			break;
		case Element::Slave:
			parent = m_slave_elmt_qtwi;
			break;
		case Element::Terminale:
			parent = m_terminal_elmt_qtwi;
			break;
		default:
			break;
	}
	QTreeWidgetItem *qtwi = new QTreeWidgetItem(parent);
	m_element_hash.insert(qtwi, QPointer<Element>(element));
	
	QString str;
	str += element->elementInformations().value("label").toString();
	if(!str.isEmpty())
		str += ("   ");
	str += element->elementInformations().value("comment").toString();
	if (str.isEmpty())
		str = tr("Inconnue");
	qtwi->setText(0, str);
	qtwi->setCheckState(0, Qt::Checked);
	qtwi->setData(0, Qt::UserRole, searchTerms(element));
}

/**
 * @brief SearchAndReplaceWidget::search
 * Start the search
 */
void SearchAndReplaceWidget::search()
{
	QString str = ui->m_search_le->text();
	if(str.isEmpty())
	{
		for (QTreeWidgetItemIterator it(m_root_qtwi) ; *it ; ++it) {
			(*it)->setHidden(false);
		}
		
		for (QTreeWidgetItem *item : m_category_qtwi) {
			item->setExpanded(false);
		}
		m_root_qtwi->setExpanded(true);
		
		ui->m_tree_widget->setCurrentItem(m_root_qtwi);
		ui->m_search_le->setPalette(QPalette());
	}
	else
	{
		for (QTreeWidgetItemIterator it(m_root_qtwi) ; *it ; ++it) {
			(*it)->setHidden(true);
		}
		
		bool match = false;
		Qt::MatchFlags flags;
			
			//Search on the text displayed at column 0 of each item
		if (ui->m_mode_cb->currentIndex() == 0) {
			flags = Qt::MatchContains | Qt::MatchRecursive; //Contain string
		} else {
			flags = Qt::MatchFixedString | Qt::MatchRecursive; //entire word
		}
		if (ui->m_case_sensitive_cb->isChecked()) {
			flags = flags | Qt::MatchCaseSensitive;
		}
		for (QTreeWidgetItem *qtwi : ui->m_tree_widget->findItems(str, flags))
		{
			match = true;
			qtwi->setHidden(false);
			setVisibleAllParents(qtwi);
		}
		
			//Extended search,
			//on each string stored in column 0 with role : UserRole
		QList<QTreeWidgetItem *> qtwi_list;
		qtwi_list.append(m_diagram_hash.keys());
		qtwi_list.append(m_element_hash.keys());
		qtwi_list.append(m_text_hash.keys());
		for (QTreeWidgetItem *qtwi : qtwi_list)
		{
			QStringList list = qtwi->data(0, Qt::UserRole).toStringList();
			
			if(ui->m_mode_cb->currentIndex() == 0)
			{
					//Contain string
				list = list.filter(str, ui->m_case_sensitive_cb->isChecked()? Qt::CaseSensitive : Qt::CaseInsensitive);
			}
			else
			{
					//entire word
				QRegularExpression rx("\\b" + str + "\\b");
				if (!ui->m_case_sensitive_cb->isChecked()) {
					rx.setPatternOptions(QRegularExpression::CaseInsensitiveOption);
				}
				list = list.filter(rx);
			}
			if (!list.isEmpty())
			{
				match = true;
				qtwi->setHidden(false);
				setVisibleAllParents(qtwi);
			}
		}
		
		QPalette background = ui->m_search_le->palette();
		background.setColor(QPalette::Base, match ? QColor("#E0FFF0") : QColor("#FFE0EF"));
		ui->m_search_le->setPalette(background);
		
			//Go to the first occurence
		ui->m_tree_widget->setCurrentItem(m_root_qtwi);
		on_m_next_pb_clicked();
	}
}

/**
 * @brief SearchAndReplaceWidget::setVisibleAllParents
 * Set visible all parents of @item until the invisible root item
 * @param item
 * @param expend_parent
 */
void SearchAndReplaceWidget::setVisibleAllParents(QTreeWidgetItem *item, bool expend_parent)
{
	if (item->parent())
	{
		QTreeWidgetItem *parent = item->parent();
		parent->setHidden(false);
		setVisibleAllParents(parent);
		parent->setExpanded(expend_parent);
	}
}

/**
 * @brief SearchAndReplaceWidget::nextItem
 * @param item : find the next item from @item, if @item is nullptr, start the search for the root of the tree
 * @param flags
 * @return the next item according to flag or nullptr if there is not a next item
 */
QTreeWidgetItem *SearchAndReplaceWidget::nextItem(QTreeWidgetItem *item, QTreeWidgetItemIterator::IteratorFlag flags) const
{
	QTreeWidgetItem *qtwi = item;
	if (!item) {
		qtwi = ui->m_tree_widget->currentItem();
	}
	
	if (!qtwi) {
		qtwi = m_root_qtwi;
	}
	
	QTreeWidgetItemIterator it(qtwi, flags);

	++it;
	QTreeWidgetItem *next_ = *it;
	if (next_) {
		return next_;
	}
	else {
		return nullptr;
	}
}

/**
 * @brief SearchAndReplaceWidget::previousItem
 * @param item : find the previous item from @item, if @item is nullptr, start the search for the root of the tree
 * @param flags
 * @return the previous item according to flag or nullptr if there is not a previous item
 */
QTreeWidgetItem *SearchAndReplaceWidget::previousItem(QTreeWidgetItem *item, QTreeWidgetItemIterator::IteratorFlag flags) const
{
	QTreeWidgetItem *qtwi = item;
	if (!item) {
		qtwi = ui->m_tree_widget->currentItem();
	}
	
	if (!qtwi) {
		qtwi = m_root_qtwi;
	}
	
	QTreeWidgetItemIterator it(qtwi, flags);
	
	--it;
	QTreeWidgetItem *previous_ = *it;
	if (previous_) {
		return previous_;
	}
	else {
		return nullptr;
	}
}

/**
 * @brief SearchAndReplaceWidget::updateNextPreviousButtons
 * According to the current item, if there is a next or a previous item,
 * we enable/disable the buttons next/previous item. 
 */
void SearchAndReplaceWidget::updateNextPreviousButtons()
{
	QTreeWidgetItem *item_ = ui->m_tree_widget->currentItem();
	if (!item_)
	{
		ui->m_next_pb->setEnabled(true);
		ui->m_previous_pb->setDisabled(true);
		return;
	}
	
	QTreeWidgetItem *next_ = item_;
	do
	{
		next_ = nextItem(next_, QTreeWidgetItemIterator::NotHidden);
		if (!next_)
		{
			ui->m_next_pb->setDisabled(true);
			break;
		}
		else if (!m_category_qtwi.contains(next_))
		{
				 ui->m_next_pb->setEnabled(true);
				 break;
		}
	} while (m_category_qtwi.contains(next_));
	
	QTreeWidgetItem *previous_ = item_;
	do
	{
		previous_ = previousItem(previous_, QTreeWidgetItemIterator::NotHidden);
		if (!previous_)
		{
			ui->m_previous_pb->setDisabled(true);
			break;
		}
		else if (!m_category_qtwi.contains(previous_))
		{
				 ui->m_previous_pb->setEnabled(true);
				 break;
		}
	} while (m_category_qtwi.contains(previous_));
}

/**
 * @brief SearchAndReplaceWidget::itemChanged
 * Reimplemented from QTreeWidget.
 * Use to update the check state of items.
 * @param item
 * @param column
 */
void SearchAndReplaceWidget::itemChanged(QTreeWidgetItem *item, int column)
{
	Q_UNUSED(column);
	ui->m_tree_widget->blockSignals(true);
	
	setChildCheckState(item, item->checkState(0));
	updateParentCheckState(item);
	
	ui->m_tree_widget->blockSignals(false);
}

/**
 * @brief SearchAndReplaceWidget::setChildCheckState
 * @param item : Parent of the items to be evaluated
 * @param check : check state
 * @param deep : if true, we evaluate every subchilds.
 */
void SearchAndReplaceWidget::setChildCheckState(QTreeWidgetItem *item, Qt::CheckState check, bool deep)
{
	for (int i=0 ; i<item->childCount() ; ++i)
	{
		item->child(i)->setCheckState(0, check);
		if (deep && item->child(i)->childCount()) {
			setChildCheckState(item->child(i), check, deep);
		}
	}
}

/**
 * @brief SearchAndReplaceWidget::updateParentCheckState
 * @param item : a child item of the parent to be evaluated.
 * @param all_parents : if true, we evaluate every parents, until the root item.
 */
void SearchAndReplaceWidget::updateParentCheckState(QTreeWidgetItem *item, bool all_parents)
{
	QTreeWidgetItem *parent = item->parent();
	
	if (!parent) {
		parent = item;
	}
	
	int check=0,
		partially=0;
			
	for (int i=0 ; i<parent->childCount() ; ++i)
	{
		switch (parent->child(i)->checkState(0)) {
			case Qt::Checked:
				++check;
				break;
			case Qt::PartiallyChecked:
				++partially;
				break;
			default:
				break;
		}
	}

	if (check == parent->childCount()) {
		parent->setCheckState(0, Qt::Checked);
	}
	else if (partially || check) {
		parent->setCheckState(0, Qt::PartiallyChecked);
	}
	else {
		parent->setCheckState(0, Qt::Unchecked);
	}
	
	if (all_parents && item->parent()) {
		updateParentCheckState(parent, all_parents);
	}
}

/**
 * @brief SearchAndReplaceWidget::activateNextChecked
 * Activate the next checked (and visible) item
 */
void SearchAndReplaceWidget::activateNextChecked()
{
		//Next button is disabled, so there is not a next item.
	if (!ui->m_next_pb->isEnabled())
		return;
	
	do {
		on_m_next_pb_clicked();
	} while ((ui->m_tree_widget->currentItem()->checkState(0) != Qt::Checked) &&
			 ui->m_next_pb->isEnabled());
}

/**
 * @brief SearchAndReplaceWidget::searchTerms
 * @param diagram
 * @return All QStrings use as terms for search.
 */
QStringList SearchAndReplaceWidget::searchTerms(Diagram *diagram) const
{
	QStringList list;
	TitleBlockProperties prop = diagram->border_and_titleblock.exportTitleBlock();
	list.append(prop.title);
	list.append(prop.author);
	list.append(prop.filename);
    list.append(prop.plant);
	list.append(prop.locmach);
	list.append(prop.indexrev);
	list.append(prop.folio);
	list.append(prop.date.toString());
	for (QString key : prop.context.keys()) {
		list.append(prop.context.value(key).toString());
	}
	
	return list;
}

/**
 * @brief SearchAndReplaceWidget::searchTerms
 * @param element
 * @return All QString use as terms for search
 */
QStringList SearchAndReplaceWidget::searchTerms(Element *element) const
{
	QStringList list;
	DiagramContext context = element->elementInformations();
	for (QString key : QETApp::elementInfoKeys())
	{
		QString str = context.value(key).toString();
		if (!str.isEmpty()) {
			list.append(str);
		}
	}
	
	return list;
}

void SearchAndReplaceWidget::on_m_quit_button_clicked() {
    this->setHidden(true);
}

void SearchAndReplaceWidget::on_m_advanced_pb_toggled(bool checked) {
    setHideAdvanced(!checked);
}

void SearchAndReplaceWidget::on_m_tree_widget_itemDoubleClicked(QTreeWidgetItem *item, int column)
{
    Q_UNUSED(column);
	
	if (m_diagram_hash.keys().contains(item))
	{
		QPointer<Diagram> diagram = m_diagram_hash.value(item);
		if(diagram) {
			diagram.data()->showMe();
		}
	}
	else if (m_element_hash.keys().contains(item))
	{
		QPointer<Element> elmt = m_element_hash.value(item);
		if (elmt) {
			elmt.data()->diagram()->showMe();
		}
	}
	else if (m_text_hash.keys().contains(item))
	{
		QPointer<IndependentTextItem> text = m_text_hash.value(item);
		if (text) {
			text.data()->diagram()->showMe();
		}
	}
	else if (m_conductor_hash.keys().contains(item))
	{
		QPointer<Conductor> cond = m_conductor_hash.value(item);
		if (cond) {
			cond.data()->diagram()->showMe();
		}
	}
}

void SearchAndReplaceWidget::on_m_reload_pb_clicked()
{
	clear();
	if (m_highlighted_element)
	{
		m_highlighted_element.data()->setHighlighted(false);
		m_highlighted_element.clear();
	}
	if (m_last_selected)
	{
		m_last_selected.data()->setSelected(false);
		m_last_selected.clear();
	}
	
	ui->m_search_le->setFocus();
	fillItemsList();
	m_root_qtwi->setExpanded(true);
}

void SearchAndReplaceWidget::on_m_tree_widget_currentItemChanged(QTreeWidgetItem *current, QTreeWidgetItem *previous)
{
	Q_UNUSED(previous);
	
	if(m_highlighted_element) {
		m_highlighted_element.data()->setHighlighted(false);
	}
	if (m_last_selected) {
		m_last_selected.data()->setSelected(false);
	}
	
	if (m_element_hash.keys().contains(current))
	{
		QPointer<Element> elmt = m_element_hash.value(current);
		if (elmt)
		{
			m_highlighted_element = elmt;
			elmt.data()->setHighlighted(true);
		}
	}
	else if (m_text_hash.keys().contains(current))
	{
		QPointer<IndependentTextItem> text = m_text_hash.value(current);
		if (text) 
		{
			text.data()->setSelected(true);
			m_last_selected = text;
		}
	}
	else if (m_conductor_hash.keys().contains(current))
	{
		QPointer<Conductor> cond = m_conductor_hash.value(current);
		if (cond)
		{
			cond.data()->setSelected(true);
			m_last_selected = cond;
		}
	}
	
	updateNextPreviousButtons();
	if (!current) {
		return;
	}
	
	if (current->checkState(0) == Qt::Checked && !m_category_qtwi.contains(current)) {
		ui->m_replace_pb->setEnabled(true);
	} else {
		ui->m_replace_pb->setDisabled(true);
	}
}

void SearchAndReplaceWidget::on_m_next_pb_clicked()
{
	QTreeWidgetItem *item = nullptr;
	
	do
	{
		item = nextItem(nullptr, QTreeWidgetItemIterator::NotHidden);
		if (item) {
			ui->m_tree_widget->setCurrentItem(item);
		} else {
			return;
		}
	} while (m_category_qtwi.contains(item));
	
	ui->m_tree_widget->setCurrentItem(item);
	ui->m_tree_widget->scrollToItem(item);
	on_m_tree_widget_itemDoubleClicked(item, 0);
}

void SearchAndReplaceWidget::on_m_previous_pb_clicked()
{	
	QTreeWidgetItem *item = nullptr;
	
	do
	{
		item = previousItem(nullptr, QTreeWidgetItemIterator::NotHidden);
		if (item) {
			ui->m_tree_widget->setCurrentItem(item);
		}
		else
		{
				//There is not a previous selected item, so the current item is m_root_qtwi but we know he must not be selected
				//we user click on the button 'next item'.
				//So we select the first selectable item by calling on_m_next_pb_clicked
			on_m_next_pb_clicked();
			return;
		}
	} while (m_category_qtwi.contains(item));
	
	ui->m_tree_widget->setCurrentItem(item);
	ui->m_tree_widget->scrollToItem(item);
	on_m_tree_widget_itemDoubleClicked(item, 0);
}

void SearchAndReplaceWidget::on_m_folio_pb_clicked()
{
	ReplaceFolioDialog *dialog = new ReplaceFolioDialog(this);
	dialog->setTitleBlockProperties(m_worker.m_titleblock_properties);
	
	int result = dialog->exec();
	if (result == QDialogButtonBox::AcceptRole)
	{
		QString text = ui->m_folio_pb->text();
        if (!text.endsWith(tr(" [édité]"))) {
            text.append(tr(" [édité]"));
		}
		ui->m_folio_pb->setText(text);
		m_worker.m_titleblock_properties = dialog->titleBlockProperties();
	}
	else if (result == QDialogButtonBox::ResetRole)
	{
		QString text = ui->m_folio_pb->text();
        if (text.endsWith(tr(" [édité]"))) {
            text.remove(tr(" [édité]"));
		}
		ui->m_folio_pb->setText(text);
		m_worker.m_titleblock_properties = TitleBlockProperties();
	}
}

/**
 * @brief SearchAndReplaceWidget::on_m_replace_pb_clicked
 * Replace the current selection
 */
void SearchAndReplaceWidget::on_m_replace_pb_clicked()
{
	QTreeWidgetItem *qtwi = ui->m_tree_widget->currentItem();	
	if(!qtwi) {
		return;
	}
	if (!m_category_qtwi.contains(qtwi) && qtwi->checkState(0) == Qt::Checked)
	{
        if (ui->m_folio_pb->text().endsWith(tr(" [édité]")) &&
			m_diagram_hash.keys().contains(qtwi))
		{
			QPointer<Diagram> d = m_diagram_hash.value(qtwi);
			if (d) {
				m_worker.replaceDiagram(d.data());
			}
		}
        else if (ui->m_element_pb->text().endsWith(tr(" [édité]")) &&
				 m_element_hash.keys().contains(qtwi))
		{
			QPointer<Element> e = m_element_hash.value(qtwi);
			if (e) {
				m_worker.replaceElement(e.data());
			}
		}
		else if (!ui->m_replace_le->text().isEmpty() &&
				 m_text_hash.keys().contains(qtwi))
		{
			m_worker.m_indi_text = ui->m_replace_le->text();
			QPointer<IndependentTextItem> t = m_text_hash.value(qtwi);
			if (t) {
				m_worker.replaceIndiText(t.data());
			}
			
		}
	}
	activateNextChecked();
	ui->m_replace_pb->setEnabled(ui->m_next_pb->isEnabled());
}

/**
 * @brief SearchAndReplaceWidget::on_m_replace_all_pb_clicked
 * Replace all checked item
 */
void SearchAndReplaceWidget::on_m_replace_all_pb_clicked()
{
		//Replace folio
    if (ui->m_folio_pb->text().endsWith(tr(" [édité]")))
	{
		QList <Diagram *> diagram_list;
		for (QTreeWidgetItem *qtwi : m_diagram_hash.keys())
		{
			if (!qtwi->isHidden() && qtwi->checkState(0) == Qt::Checked)
			{
				QPointer <Diagram> p = m_diagram_hash.value(qtwi);
				if (p) {
					diagram_list.append(p.data());
				}
			}
		}
		m_worker.replaceDiagram(diagram_list);
	}
		//Replace text
    if (ui->m_element_pb->text().endsWith(tr(" [édité]")))
	{
		QList <Element *> element_list;
		for (QTreeWidgetItem *qtwi : m_element_hash.keys())
		{
			if (!qtwi->isHidden() && qtwi->checkState(0) == Qt::Checked)
			{
				QPointer <Element> p = m_element_hash.value(qtwi);
				if (p) {
					element_list.append(p.data());
				}
			}
		}
		m_worker.replaceElement(element_list);
	}
		//Replace indi text
	if (!ui->m_replace_le->text().isEmpty())
	{
		QList <IndependentTextItem*> text_list;
		for(QTreeWidgetItem *qtwi : m_text_hash.keys())
		{
			if (!qtwi->isHidden() && qtwi->checkState(0) == Qt::Checked)
			{
				QPointer<IndependentTextItem> t = m_text_hash.value(qtwi);
				if (t) {
					text_list.append(t.data());
				}
			}
		}
		m_worker.m_indi_text = ui->m_replace_le->text();
		m_worker.replaceIndiText(text_list );
	}
	
		//Change was made, we reload the panel
		//and search again to keep up to date the tree widget
		//and the match item of search
	QString txt = ui->m_search_le->text();
	on_m_reload_pb_clicked();
	ui->m_search_le->setText(txt);
	search();
}

void SearchAndReplaceWidget::on_m_element_pb_clicked()
{
	ReplaceElementDialog *dialog = new ReplaceElementDialog(m_worker.m_element_context, this);
	
	int result = dialog->exec();
	if (result == QDialogButtonBox::AcceptRole)
	{
		QString text = ui->m_element_pb->text();
        if (!text.endsWith(tr(" [édité]"))) {
            text.append(tr(" [édité]"));
		}
		ui->m_element_pb->setText(text);
		m_worker.m_element_context = dialog->context();
	}
	else if (result == QDialogButtonBox::ResetRole)
	{
		QString text = ui->m_element_pb->text();
        if (text.endsWith(tr(" [édité]"))) {
            text.remove(tr(" [édité]"));
		}
		ui->m_element_pb->setText(text);
		m_worker.m_element_context = DiagramContext();
	}
}

/**
 * @brief SearchAndReplaceWidget::on_m_mode_cb_currentIndexChanged
 * Update the search when user change mode.
 * @param index
 */
void SearchAndReplaceWidget::on_m_mode_cb_currentIndexChanged(int index)
{
	Q_UNUSED(index);	
	search();
}

/**
 * @brief SearchAndReplaceWidget::on_m_case_sensitive_cb_stateChanged
 * Update the search when change the case sensitive
 * @param arg1
 */
void SearchAndReplaceWidget::on_m_case_sensitive_cb_stateChanged(int arg1)
{
    Q_UNUSED(arg1);
	search();
}
