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
		QString title = elmt->diagram()->title();
		if (title.isEmpty()) title = tr("Sans titre");
		button_text += QString(tr("Folio\240 %1 (%2), position %3.")).arg(elmt->diagram()->folioIndex() + 1)
																		  .arg(title)
																		  .arg(elmt->diagram() -> convertPosition(elmt -> scenePos()).toString());

		//add the button himself
		QHBoxLayout *hl = new QHBoxLayout();
		QRadioButton *rb = new QRadioButton(button_text , this);
		QPushButton *pb = new QPushButton(QET::Icons::ZoomDraw,"", this);
		pb->setToolTip(tr("Voir l'\351l\351ment"));
		hl->addWidget(rb);
		hl->addStretch();
		hl->addWidget(pb);
		ui->scroll_layout_->addLayout(hl);

		//map the radio button signal
		connect(rb, SIGNAL(clicked()), sm_, SLOT(map()));
		sm_ -> setMapping(rb, map_id);
		//map the push button show diagram
		connect(pb, SIGNAL(clicked()), sm_show_, SLOT(map()));
		sm_show_->setMapping(pb, map_id);

		map_id++; //increase the map_id for next button.
	}
	ui->scroll_layout_->addStretch();
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
