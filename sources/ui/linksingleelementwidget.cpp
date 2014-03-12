/*
	Copyright 2006-2014 The QElectroTech Team
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
#include "element.h"
#include "elementprovider.h"
#include "diagramcommands.h"

/**
 * @brief LinkSingleElementWidget::LinkSingleElementWidget
 * Default constructor
 * @param elmt
 * the edited element
 * @param parent
 * the parent widget
 */
LinkSingleElementWidget::LinkSingleElementWidget(Element *elmt, QWidget *parent) :
	QWidget(parent),
	ui(new Ui::LinkSingleElementWidget),
	element_(elmt),
	esw_(0),
	diagram_list(element_->diagram()->project()->diagrams()),
	unlink_widget(0),
	unlink_(false)
{
	ui->setupUi(this);

	if (elmt->linkType() & Element::Slave)
		filter_ = Element::Master;
	else if (elmt->linkType() & Element::AllReport)
		filter_ = elmt->linkType() == Element::NextReport? Element::PreviousReport : Element::NextReport;
	else
		filter_ = Element::Simple;

	buildInterface();
	connect(ui->folio_combo_box, SIGNAL(currentIndexChanged(int)), this, SLOT(reBuildList()));
}

/**
 * @brief LinkSingleElementWidget::~LinkSingleElementWidget
 * Default destructor
 */
LinkSingleElementWidget::~LinkSingleElementWidget()
{
	delete ui;
}

/**
 * @brief LinkSingleElementWidget::apply
 * Apply the new property of the edited element
 */
void LinkSingleElementWidget::apply() {
	if (esw_->selectedElement())
		element_->diagram()->undoStack().push(new LinkElementsCommand(element_, esw_->selectedElement()));
	else if (unlink_)
		element_->diagram()->undoStack().push(new unlinkElementsCommand(element_));
}

/**
 * @brief LinkSingleElementWidget::buildInterface
 * Build the interface of this widget
 */
void LinkSingleElementWidget::buildInterface() {
	ui->folio_combo_box->addItem(tr("Tous"));

	//Fill the combo box for filter the result by folio
	foreach (Diagram *d, diagram_list) {
		QString title = d->title();
		if (title.isEmpty()) title = tr("Sans titre");
		title.prepend(QString::number(d->folioIndex() + 1) + " ");
		ui->folio_combo_box->addItem(title);
	}

	//Element is free build list
	if(element_->isFree()) {
		buildList();
		ui->button_linked->setDisabled(true);
	}
	//Element isn't free build an empty list and add 'unlink' button
	else {
		buildUnlinkButton();
		QList <Element *> elmt_list;
		esw_ = new ElementSelectorWidget(elmt_list, this);
		ui->content_layout->addWidget(esw_);
	}
}

/**
 * @brief LinkSingleElementWidget::buildList
 * Build the element list of this widget,
 * the list is fill with the element find in the
 * required folio (folio selected with the combo box)
 */
void LinkSingleElementWidget::buildList() {
	QList <Element *> elmt_list;
	int i = ui->folio_combo_box->currentIndex();

	//find in all diagram of this project
	if (i == 0) {
		ElementProvider ep(element_->diagram()->project());
		if (filter_ & Element::AllReport)
			elmt_list = ep.freeElement(filter_);
		else
			elmt_list = ep.find(filter_);
	}
	//find in single diagram
	else {
		ElementProvider ep (diagram_list.at(i-1));
		if (filter_ & Element::AllReport)
			elmt_list = ep.freeElement(filter_);
		else
			elmt_list = ep.find(filter_);
	}

	//If element is linked, remove is parent from the list
	if(!element_->isFree()) elmt_list.removeAll(element_->linkedElements().first());

	esw_ = new ElementSelectorWidget(elmt_list, this);
	ui->content_layout->addWidget(esw_);
}

/**
 * @brief LinkSingleElementWidget::buildUnlinkButton
 * Build a widget with button 'unlink' if the edited
 * element is already linked with a master element
 */
void LinkSingleElementWidget::buildUnlinkButton() {
	unlink_widget = new QWidget(this);
	QHBoxLayout *unlink_layout = new QHBoxLayout(unlink_widget);
	QLabel *lb = new QLabel(tr("Cet \351l\351ment est d\351j\340 li\351."), unlink_widget);
	QPushButton *pb = new QPushButton(tr("D\351lier"), unlink_widget);
	connect(pb, SIGNAL(clicked()), this, SLOT(unlinkClicked()));
	unlink_layout->addWidget(lb);
	unlink_layout->addStretch();
	unlink_layout->addWidget(pb);
	ui->main_layout->insertWidget(0, unlink_widget);
}

/**
 * @brief LinkSingleElementWidget::reBuildList
 * Rebuild the list of element
 */
void LinkSingleElementWidget::reBuildList() {
	if (element_->isFree() || unlink_) {
		ui->content_layout->removeWidget(esw_);
		delete esw_;
		buildList();
	}
}

/**
 * @brief LinkSingleElementWidget::unlinkClicked
 * Action when 'unlink' button is clicked
 */
void LinkSingleElementWidget::unlinkClicked() {
	ui->main_layout->removeWidget(unlink_widget);
	delete unlink_widget;
	unlink_widget = 0;
	unlink_ = true;
	reBuildList();
}

/**
 * @brief FolioReportProperties::on_button_this_clicked
 * Action when push button "this report" is clicked
 */
void LinkSingleElementWidget::on_button_this_clicked() {
	esw_->showElement(element_);
}

/**
 * @brief FolioReportProperties::on_button_linked_clicked
 * Action when push button "linked report" is clicked
 */
void LinkSingleElementWidget::on_button_linked_clicked() {
	if (element_->isFree()) return;
	esw_->showElement(element_->linkedElements().first());
}
