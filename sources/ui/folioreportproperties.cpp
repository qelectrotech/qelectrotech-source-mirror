#include "folioreportproperties.h"
#include "ui_folioreportproperties.h"

#include <diagramposition.h>
#include <elementprovider.h>

FolioReportProperties::FolioReportProperties(Element *elmt, QWidget *parent) :
	QWidget(parent),
	element_(elmt),
	ui(new Ui::FolioReportProperties)
{
	ui->setupUi(this);

	ElementProvider ep(element_->diagram()->project(), element_->diagram());
	QList <Element *> elmt_list = ep.FreeElement(Element::report);

	foreach (Element *elmt, elmt_list) {
		if (elmt != element_) {
			QString button_text;
			button_text += elmt->name();
			button_text += QString(tr(" Folio\240: %1, ")).arg(elmt->diagram()->folioIndex() + 1);
			button_text += QString(tr("Position\240: %1")).arg(elmt->diagram() -> convertPosition(elmt -> scenePos()).toString());

			QRadioButton *rb = new QRadioButton(button_text , this);
			ui->available_report_layout->addWidget(rb);
		}
	}
	ui->available_report_layout->addStretch();
}

FolioReportProperties::~FolioReportProperties()
{
	delete ui;
}
