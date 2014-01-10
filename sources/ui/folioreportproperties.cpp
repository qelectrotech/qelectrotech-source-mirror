#include "folioreportproperties.h"
#include "ui_folioreportproperties.h"

#include <diagramposition.h>
#include <elementprovider.h>
#include <qetgraphicsitem/elementtextitem.h>
#include <diagramcommands.h>
#include "qeticons.h"

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
	unlink = false;
	if(element_->isFree()) buildRadioList();
	else buildUnlinkButton();
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
void FolioReportProperties::buildRadioList() {
	sm_ = new QSignalMapper(this);
	connect(sm_, SIGNAL(mapped(int)), this, SLOT(linkToElement(int)));
	sm_show_ = new QSignalMapper(this);
	connect(sm_show_, SIGNAL(mapped(int)), this, SLOT(showDiagram(int)));

	//Research the invert report of @element_
	int rep = element_->linkType() == Element::NextReport? Element::PreviousReport : Element::NextReport;
	ElementProvider ep(element_->diagram()->project(), element_->diagram());
	QList <Element *> elmt_list = ep.freeElement(rep);
	qSort(elmt_list);
	qDebug() <<elmt_list;
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
			QHBoxLayout *hl = new QHBoxLayout(this);
			QRadioButton *rb = new QRadioButton(button_text , this);
			QPushButton *pb = new QPushButton(QET::Icons::ZoomDraw,"",this);
			hl->addWidget(rb);
			hl->addStretch();
			hl->addWidget(pb);
			ui->available_report_layout->addLayout(hl);
			element_list << elmt;
			qSort(element_list);
			qDebug() <<element_list;
			//map the radio button signal
			connect(rb, SIGNAL(clicked()), sm_, SLOT(map()));
			sm_ -> setMapping(rb, element_list.size()-1);
			//map the push button show diagram
			connect(pb, SIGNAL(clicked()), sm_show_, SLOT(map()));
			sm_show_->setMapping(pb, element_list.size()-1);
		}
	}
	ui->available_report_layout->addStretch();
}

/**
 * @brief FolioReportProperties::buildUnlinkButton
 *build button for ask user if want to unlink this element
 */
void FolioReportProperties::buildUnlinkButton() {
	unlink_widget = new QWidget(this);
	QHBoxLayout *unlink_layout = new QHBoxLayout(unlink_widget);
	QLabel *lb = new QLabel(tr("Ce report est d\351j\340 lier."), unlink_widget);
	QPushButton *pb = new QPushButton(tr("D\351lier"), unlink_widget);
	connect(pb, SIGNAL(clicked()), this, SLOT(unlinkClicked()));
	unlink_layout->addWidget(lb);
	unlink_layout->addStretch();
	unlink_layout->addWidget(pb);
	ui->v_main_layout->insertWidget(0, unlink_widget);
}

/**
 * @brief FolioReportProperties::Apply
 * Apply the new properties for this folio report
 */
void FolioReportProperties::Apply() {
	if (unlink && !element_to_link)
		element_->diagram()->undoStack().push(new unlinkElementsCommand(element_));
	else if (element_to_link)
		element_->diagram()->undoStack().push(new LinkElementsCommand(element_, element_to_link));
}

/**
 * @brief FolioReportProperties::unlinkClicked
 *this slot remove unlink_layout and call buildRadioList
 */
void FolioReportProperties::unlinkClicked() {
	ui->v_main_layout->removeWidget(unlink_widget);
	delete unlink_widget;
	unlink = true;
	buildRadioList();
}

/**
 * @brief FolioReportProperties::showDiagram
 * Show the wanted report element
 * @param i position of wanted element in element_list
 */
void FolioReportProperties::showDiagram(const int i) {
	Element *elmt = element_list.at(i);
	elmt->diagram()->showMe();
	foreach (QGraphicsItem *qgi, elmt->diagram()->selectedItems()) qgi->setSelected(false);
	elmt->setSelected(true);
}
