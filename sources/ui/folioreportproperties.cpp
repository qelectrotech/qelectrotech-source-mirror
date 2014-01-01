#include "folioreportproperties.h"
#include "ui_folioreportproperties.h"

#include <diagramposition.h>
#include <elementprovider.h>
#include <qetgraphicsitem/elementtextitem.h>

/**
 * @brief FolioReportProperties::FolioReportProperties : Construcor
 * @param elmt : The edited element
 * @param parent : Parent widget
 */
FolioReportProperties::FolioReportProperties(Element *elmt, QWidget *parent) :
	QWidget(parent),
	element_(elmt),
	element_to_link(0),
	ui(new Ui::FolioReportProperties)
{
	ui->setupUi(this);

	sm_ = new QSignalMapper(this);
	connect(sm_, SIGNAL(mapped(int)) , this, SLOT(linkToElement(int)));

	BuildRadioList();
}

/**
 * @brief FolioReportProperties::~FolioReportProperties : Destructor
 */
FolioReportProperties::~FolioReportProperties()
{
	delete ui;
}

/**
 * @brief FolioReportProperties::BuildRadioList : build the radio list for each available folio report
 */
void FolioReportProperties::BuildRadioList() {
	ElementProvider ep(element_->diagram()->project(), element_->diagram());
	QList <Element *> elmt_list = ep.freeElement(Element::Report);

	foreach (Element *elmt, elmt_list) {
		if (elmt != element_) {
			//label for the button
			QString button_text;
			QString title = elmt->diagram()->title();
			if (title.isEmpty()) title = tr("Sans titre");
			button_text += QString(tr("Folio\240 %1 (%2), position %3.")).arg(elmt->diagram()->folioIndex() + 1)
																		  .arg(title)
																		  .arg(elmt->diagram() -> convertPosition(elmt -> scenePos()).toString());

			//button himself
			QRadioButton *rb = new QRadioButton(button_text , this);
			ui->available_report_layout->addWidget(rb);
			element_list << elmt;
			//map the radio button signal
			connect(rb, SIGNAL(clicked()), sm_, SLOT(map()));
			sm_ -> setMapping(rb, element_list.size()-1);
		}
	}
	ui->available_report_layout->addStretch();
}

/**
 * @brief FolioReportProperties::Apply
 * Apply the new properties for this folio report
 */
void FolioReportProperties::Apply() {
	if (element_to_link) element_to_link->linkToElement(element_);
}
