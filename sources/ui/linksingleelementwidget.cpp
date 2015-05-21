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
#include "linksingleelementwidget.h"
#include "ui_linksingleelementwidget.h"
#include "diagram.h"
#include "elementprovider.h"
#include "diagramcommands.h"
#include "elementselectorwidget.h"

/**
 * @brief LinkSingleElementWidget::LinkSingleElementWidget
 * Default constructor
 * @param elmt
 * the edited element
 * @param parent
 * the parent widget
 */
LinkSingleElementWidget::LinkSingleElementWidget(Element *elmt, QWidget *parent) :
	PropertiesEditorWidget(parent),
	ui(new Ui::LinkSingleElementWidget),
	m_element(nullptr),
	esw_(0),
	unlink_widget(0),
	unlink_(false),
	search_field(0)
{
	ui->setupUi(this);
	connect(ui->folio_combo_box, SIGNAL(currentIndexChanged(int)), this, SLOT(setNewList()));
	setElement(elmt);
}

/**
 * @brief LinkSingleElementWidget::~LinkSingleElementWidget
 * Default destructor
 */
LinkSingleElementWidget::~LinkSingleElementWidget() {
	delete ui;
}

/**
 * @brief LinkSingleElementWidget::setElement
 * Set element to be the edited element.
 * @param element
 */
void LinkSingleElementWidget::setElement(Element *element)
{
	if (m_element == element) return;
	if (m_element)
	{
		disconnect(m_element->diagram()->project(), &QETProject::diagramRemoved, this, &LinkSingleElementWidget::diagramWasRemovedFromProject);
		diagram_list.clear();
	}

	m_element = element;
	diagram_list << m_element->diagram()->project()->diagrams();

	if (m_element->linkType() & Element::Slave)
		filter_ = Element::Master;
	else if (m_element->linkType() & Element::AllReport)
		filter_ = m_element->linkType() == Element::NextReport? Element::PreviousReport : Element::NextReport;
	else
		filter_ = Element::Simple;

	connect(m_element->diagram()->project(), &QETProject::diagramRemoved, this, &LinkSingleElementWidget::diagramWasRemovedFromProject);

	buildInterface();
}

/**
 * @brief LinkSingleElementWidget::apply
 * Apply the new property of the edited element
 */
void LinkSingleElementWidget::apply()
{
	QUndoCommand *undo = associatedUndo();
	if (undo)
		m_element->diagram()->undoStack().push(undo);
}

/**
 * @brief LinkSingleElementWidget::associatedUndo
 * @return the undo command associated to the current edition
 * if there isn't change, return nulptr
 */
QUndoCommand *LinkSingleElementWidget::associatedUndo() const
{
	if (esw_->selectedElement())
		return new LinkElementsCommand(m_element, esw_->selectedElement());
	else if (unlink_)
		return new unlinkElementsCommand(m_element);

	return nullptr;
}

QString LinkSingleElementWidget::title() const
{
	if (m_element->linkType() == Element::AllReport)
		return tr("Report de folio");
	else
		return tr("Référence croisée (esclave)");
}

/**
 * @brief LinkSingleElementWidget::updateUi
 * Update the content of this widget
 */
void LinkSingleElementWidget::updateUi() {
	buildInterface();
}

/**
 * @brief LinkSingleElementWidget::buildList
 * Build the element list of this widget,
 * the list is fill with the element find in the
 * required folio (folio selected with the combo box)
 */
void LinkSingleElementWidget::buildList()
{
	if (!esw_)
	{
		esw_ = new ElementSelectorWidget(availableElements(), this);
		ui->content_layout->addWidget(esw_);
	}
	else
	{
		esw_->setList(availableElements());
	}
	buildSearchField();
}

/**
 * @brief LinkSingleElementWidget::buildLinkUnlinkButton
 * Build the button link or unlink according to the current edited
 * element, if is already linked with a master element or not
 */
void LinkSingleElementWidget::buildLinkUnlinkButton()
{
	if (m_element->isFree())
	{
		ui->button_linked->setDisabled(true);
		if (unlink_widget)
		{
			ui->main_layout->removeWidget(unlink_widget);
			delete unlink_widget; unlink_widget = nullptr;
		}
	}
	else if (!unlink_widget)
	{
		ui->button_linked->setEnabled(true);
		unlink_widget = new QWidget(this);
		QHBoxLayout *unlink_layout = new QHBoxLayout(unlink_widget);
		QLabel *lb = new QLabel(tr("Cet élément est déjà lié."), unlink_widget);
		QPushButton *pb = new QPushButton(tr("Délier"), unlink_widget);
		connect(pb, SIGNAL(clicked()), this, SLOT(unlinkClicked()));
		unlink_layout->addWidget(lb);
		unlink_layout->addStretch();
		unlink_layout->addWidget(pb);
		ui->main_layout->insertWidget(0, unlink_widget);
	}

}

/**
 * @brief LinkSingleElementWidget::buildSearchField
 * Build a line edit for search element by they information,
 * like label or information
 */
void LinkSingleElementWidget::buildSearchField()
{
		//If there isn't string to filter, we remove the search field
	if (esw_->filter().isEmpty()) {
		if (search_field) {
			ui -> header_layout -> removeWidget(search_field);
			delete search_field;
			search_field = nullptr;
		}
		return;
	}

	if(!search_field)
	{
		search_field = new QLineEdit(this);
		search_field -> setPlaceholderText(tr("Rechercher"));
		connect(search_field, SIGNAL(textChanged(QString)), esw_, SLOT(filtered(QString)));
		ui->header_layout->addWidget(search_field);
	}
	setUpCompleter();
}

/**
 * @brief LinkSingleElementWidget::availableElements
 * @return A QList with all available element
 * to be linked with the edited element.
 * This methode take care of the combo box "find in diagram"
 */
QList <Element *> LinkSingleElementWidget::availableElements()
{
	QList <Element *> elmt_list;
		//if element isn't free and unlink isn't pressed, return an empty list
	if (!m_element->isFree() && !unlink_) return elmt_list;

	int i = ui->folio_combo_box->currentIndex();
		//find in all diagram of this project
	if (i == 0)
	{
		ElementProvider ep(m_element->diagram()->project());
		if (filter_ & Element::AllReport)
			elmt_list = ep.freeElement(filter_);
		else
			elmt_list = ep.find(filter_);
	}
		//find in single diagram
	else
	{
		ElementProvider ep (diagram_list.at(i-1));
		if (filter_ & Element::AllReport)
			elmt_list = ep.freeElement(filter_);
		else
			elmt_list = ep.find(filter_);
	}
		//If element is linked, remove is parent from the list
	if(!m_element->isFree()) elmt_list.removeAll(m_element->linkedElements().first());

	return elmt_list;
}

/**
 * @brief LinkSingleElementWidget::setUpCompleter
 * Setup the completer of search_field
 */
void LinkSingleElementWidget::setUpCompleter()
{
	if (search_field)
	{
		search_field -> clear();
		delete search_field -> completer();

		QStringList filter = esw_->filter();
		filter.sort();
		QCompleter *comp = new QCompleter(filter, search_field);
		comp -> setCaseSensitivity(Qt::CaseInsensitive);
		search_field -> setCompleter(comp);
	}
}

/**
 * @brief LinkSingleElementWidget::buildInterface
 * Build the interface of this widget
 */
void LinkSingleElementWidget::buildInterface()
{
	ui->folio_combo_box->blockSignals(true);
	ui->folio_combo_box->clear();
	ui->folio_combo_box->addItem(tr("Tous"));

		//Fill the combo box for filter the result by folio
	foreach (Diagram *d, diagram_list)
	{
		QString title = d->title();
		if (title.isEmpty()) title = tr("Sans titre");
		title.prepend(QString::number(d->folioIndex() + 1) + " ");
		ui->folio_combo_box->addItem(title);
	}
	ui->folio_combo_box->blockSignals(false);

	unlink_ = false;
	buildList();
	buildLinkUnlinkButton();
	buildSearchField();
}

/**
 * @brief LinkSingleElementWidget::setNewList
 * Set the list according to the selected diagram in the combo_box
 */
void LinkSingleElementWidget::setNewList()
{
	esw_->setList(availableElements());
	buildSearchField();
}

/**
 * @brief LinkSingleElementWidget::unlinkClicked
 * Action when 'unlink' button is clicked
 */
void LinkSingleElementWidget::unlinkClicked()
{
	ui->main_layout->removeWidget(unlink_widget);
	delete unlink_widget; unlink_widget = nullptr;
	unlink_ = true;
	setNewList();
}

/**
 * @brief FolioReportProperties::on_button_this_clicked
 * Action when push button "this report" is clicked
 */
void LinkSingleElementWidget::on_button_this_clicked() {
	esw_->showElement(m_element);
}

/**
 * @brief FolioReportProperties::on_button_linked_clicked
 * Action when push button "linked report" is clicked
 */
void LinkSingleElementWidget::on_button_linked_clicked()
{
	if (m_element->isFree()) return;
	esw_->showElement(m_element->linkedElements().first());
}

/**
 * @brief LinkSingleElementWidget::diagramWasRemovedFromProject
 *  * This slot is called when a diagram is removed from the parent project of edited element
 * to update the content of this widget
 */
void LinkSingleElementWidget::diagramWasRemovedFromProject()
{
	diagram_list.clear();
	diagram_list << m_element->diagram()->project()->diagrams();
		//We use a timer because if the removed diagram contain the master element linked to the edited element
		//we must to wait for this elements be unlinked, else the list of available master isn't up to date
	QTimer::singleShot(10, this, SLOT(buildInterface()));
}
