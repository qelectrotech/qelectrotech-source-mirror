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
#include "elementselectorwidget.h"
#include "ui_elementselectorwidget.h"
#include "diagramposition.h"
#include "qeticons.h"
#include "diagram.h"
#include "element.h"

/**
 * @brief ElementSelectorWidget::ElementSelectorWidget
 * Default constructor
 * @param elmt_list
 * List of element to be displayed by the selector
 * @param parent
 * Parent widget
 */
ElementSelectorWidget::ElementSelectorWidget(QList <Element *> elmt_list, QWidget *parent) :
	QWidget(parent),
	ui(new Ui::ElementSelectorWidget),
	elements_list(elmt_list),
	selected_element(0),
	showed_element(0)
{
	qSort(elements_list.begin(), elements_list.end(), comparPos);
	ui->setupUi(this);
	buildInterface();
}

/**
 * @brief ElementSelectorWidget::~ElementSelectorWidget
 * Default destructor
 */
ElementSelectorWidget::~ElementSelectorWidget()
{
	if (showed_element) showed_element->setHighlighted(false);
	delete ui;
}

/**
 * @brief ElementSelectorWidget::showElement
 * Show the element given by parametre
 * @param elmt
 */
void ElementSelectorWidget::showElement(Element *elmt) {
	if (showed_element) showed_element->setHighlighted(false);
	elmt->diagram()->showMe();
	elmt->setHighlighted(true);
	showed_element = elmt;
}

/**
 * @brief ElementSelectorWidget::clear
 * Clear the curent list and the widget
 */
void ElementSelectorWidget::clear() {
	elements_list.clear();
	string_filter.clear();
	if(showed_element) showed_element->setHighlighted(false);
	foreach(QWidget *w, content_list) {
		ui->scroll_layout_->removeWidget(w);
		delete w;
	}
	content_list.clear();
	delete sm_;
	delete sm_show_;
}

/**
 * @brief ElementSelectorWidget::setList
 * Set new list of elements
 * @param elmt_list the new elements list
 */
void ElementSelectorWidget::setList(QList<Element *> elmt_list) {
	clear();
	elements_list << elmt_list;
	qSort(elements_list.begin(), elements_list.end(), comparPos);
	buildInterface();
}

/**
 * @brief ElementSelectorWidget::buildInterface
 * Build interface of this widget (fill all available element)
 */
void ElementSelectorWidget::buildInterface() {
	//Setup the signal mapper
	int map_id = 0; //this int is used to map the signal
	sm_ = new QSignalMapper(this);
	connect(sm_, SIGNAL(mapped(int)), this, SLOT(setSelectedElement(int)));
	sm_show_ = new QSignalMapper(this);
	connect(sm_show_, SIGNAL(mapped(int)), this, SLOT(showElementFromList(int)));

	//Build the list
	foreach (Element *elmt, elements_list) {
		//label for the button
		QString button_text;

		//if element is master and have label, add label to the string
		//Also to comment
		if (elmt->linkType() & Element::Master) {
			DiagramContext dc = elmt -> elementInformations();

			if (!dc["label"].toString().isEmpty())
				button_text = dc["label"].toString() + " ";

			if (!dc["comment"].toString().isEmpty())
				button_text += dc["comment"].toString();

			if (!button_text.isEmpty())
				button_text += "\n";
		}

		QString title = elmt->diagram()->title();
		if (title.isEmpty()) title = tr("Sans titre");
		button_text += QString(tr("Folio\240 %1 (%2), position %3.")).arg(elmt->diagram()->folioIndex() + 1)
																		  .arg(title)
																		  .arg(elmt->diagram() -> convertPosition(elmt -> scenePos()).toString());

		//add the button himself
		QWidget *widget = new QWidget(this);
		QHBoxLayout *hl = new QHBoxLayout(widget);
		hl->setContentsMargins(0,0,0,0);
		QRadioButton *rb = new QRadioButton(button_text , widget);
		QPushButton *pb = new QPushButton(QET::Icons::ZoomDraw,"", widget);
		pb->setToolTip(tr("Voir l'\351l\351ment"));
		hl->addWidget(rb);
		hl->addStretch();
		hl->addWidget(pb);
		ui->scroll_layout_->insertWidget(map_id, widget);
		content_list << widget;
		//Add the string for filter this widget
		QString filter;
		foreach(QString str, elmt->elementInformations().keys()){
			filter += elmt->elementInformations()[str].toString();
		}
		string_filter << filter;

		//map the radio button signal
		connect(rb, SIGNAL(clicked()), sm_, SLOT(map()));
		sm_ -> setMapping(rb, map_id);
		//map the push button show diagram
		connect(pb, SIGNAL(clicked()), sm_show_, SLOT(map()));
		sm_show_->setMapping(pb, map_id);

		map_id++; //increase the map_id for next button.
	}
}

/**
 * @brief ElementSelectorWidget::showElementFromList
 * Show the element at the position i in @elements_list
 * @param i
 */
void ElementSelectorWidget::showElementFromList(const int i) {
	if (elements_list.size() >= i)
		showElement(elements_list.at(i));
}

/**
 * @brief ElementSelectorWidget::filter
 * Filter the content of the list.
 * Give an empty string remove all filter.
 * @param str string to filter
 */
void ElementSelectorWidget::filter(const QString &str) {
	if(str.isEmpty()) {
		foreach (QWidget *w, content_list) w->setHidden(false);
	}
	else {
		for (int i =0; i<string_filter.size(); i++) {
			if (string_filter.at(i).contains(str, Qt::CaseInsensitive))
				content_list.at(i)->setHidden(false);
			else
				content_list.at(i)->setHidden(true);
		}
	}
}
