#include "elementpropertieswidget.h"
#include <qetgraphicsitem/ghostelement.h>
#include <qeticons.h>
#include <diagramposition.h>

/**
 * @brief elementpropertieswidget::elementpropertieswidget
 * default constructor
 * @param elmt
 * @param parent
 */
elementpropertieswidget::elementpropertieswidget(Element *elmt, QWidget *parent) :
	QDialog(parent),
	element_ (elmt),
	diagram_ (elmt->diagram())
{
	frp_ = 0;
	buildInterface();
}

/**
 * @brief elementpropertieswidget::generalWidget
 * build the widget for the tab général
 * @return
 */
QWidget* elementpropertieswidget::generalWidget() {
	CustomElement *custom_element = qobject_cast<CustomElement *>(element_);
	GhostElement  *ghost_element  = qobject_cast<GhostElement  *>(element_);

	// type de l'element
	QString description_string;
	if (ghost_element) {
		description_string += tr("\311l\351ment manquant");
	} else {
		description_string += tr("\311l\351ment");
	}
	description_string += "\n";

	// some element characteristic
	description_string += QString(tr("Nom\240: %1\n")).arg(element_ -> name());
	int folio_index = diagram_ -> folioIndex();
	if (folio_index != -1) {
		description_string += QString(tr("Folio\240: %1\n")).arg(folio_index + 1);
	}
	description_string += QString(tr("Position\240: %1\n")).arg(diagram_ -> convertPosition(element_ -> scenePos()).toString());
	description_string += QString(tr("Dimensions\240: %1\327%2\n")).arg(element_ -> size().width()).arg(element_ -> size().height());
	description_string += QString(tr("Bornes\240: %1\n")).arg(element_ -> terminals().count());
	description_string += QString(tr("Champs de texte\240: %1\n")).arg(element_ -> texts().count());

	if (custom_element) {
		description_string += QString(tr("Emplacement\240: %1\n")).arg(custom_element -> location().toString());
	}

	// widget himself
	QWidget *general_widget = new QWidget (tab_);
	QVBoxLayout *vlayout_ = new QVBoxLayout (general_widget);
	general_widget -> setLayout(vlayout_);

	//widget for the text
	vlayout_->addWidget(new QLabel (description_string, general_widget));

	//widget for the pixmap
	QLabel *pix = new QLabel(general_widget);
	pix->setPixmap(element_->pixmap());
	vlayout_->addWidget(pix, 0, Qt::AlignHCenter);
	vlayout_ -> addStretch();

	//button widget
	find_in_panel = new QPushButton(QET::Icons::ZoomDraw, tr("Retrouver dans le panel"), general_widget);
	connect(find_in_panel, SIGNAL(clicked()), this, SLOT(findInPanel()));
	edit_element = new QPushButton(QET::Icons::ElementEdit, tr("\311diter l'\351l\351ment"), general_widget);
	connect(edit_element, SIGNAL(clicked()), this, SLOT(editElement()));
	QHBoxLayout *hlayout_ = new QHBoxLayout;
	hlayout_->addWidget(find_in_panel);
	hlayout_->addWidget(edit_element);

	vlayout_->addLayout(hlayout_);
	return general_widget;
}

/**
 * @brief elementpropertieswidget::buildInterface
 *build the interface of this dialog, the main tab can have
 *different tab according to the edited element
 */
void elementpropertieswidget::buildInterface() {

	setWindowTitle(tr("Propri\351t\351s de l'\351l\351ment"));
	tab_ = new QTabWidget(this);
	tab_ -> addTab(generalWidget(), tr("G\351n\351ral"));

	//Add tab according to the element
	switch (element_ -> linkType()) {
		case Element::Simple:
			break;
		case Element::NextReport:
			frp_ = new FolioReportProperties(element_, this);
			tab_ -> addTab(frp_, tr("Report de folio"));
			break;
		case Element::PreviousReport:
			frp_ = new FolioReportProperties(element_, this);
			tab_ -> addTab(frp_, tr("Report de folio"));
			break;
		case Element::Master:
			break;
		case Element::SlaveNC:
			break;
		case Element::SlaveNO:
			break;
		case Element::Bornier:
			break;
		default:
			break;
	}

	dbb = new QDialogButtonBox(QDialogButtonBox::Apply | QDialogButtonBox::Cancel | QDialogButtonBox::Reset,
							   Qt::Horizontal, this);
	connect(dbb, SIGNAL(clicked(QAbstractButton*)), this, SLOT(standardButtonClicked(QAbstractButton*)));

	QVBoxLayout *main_layout = new QVBoxLayout(this);
	main_layout -> addWidget(tab_);
	main_layout -> addWidget(dbb);
	setLayout(main_layout);
}

/**
 * @brief elementpropertieswidget::standardButtonClicked
 * apply action when click in the dialog standard button box
 * @param button
 * the cliked button
 */
void elementpropertieswidget::standardButtonClicked(QAbstractButton *button) {
	int answer = dbb -> buttonRole(button);

	switch (answer) {
		case QDialogButtonBox::ResetRole:
			break;
		case QDialogButtonBox::ApplyRole:
			if (frp_) frp_->Apply();
			this->accept();
		case QDialogButtonBox::RejectRole:
			this->reject();
		default:
			this->reject();
	}
}

/**
 * @brief elementpropertieswidget::findInPanel
 * Slot
 */
void elementpropertieswidget::findInPanel() {
	if (CustomElement *custom_element = qobject_cast<CustomElement *>(element_)) {
		emit findElementRequired(custom_element->location());
	}
	reject();
}

/**
 * @brief elementpropertieswidget::editElement
 * Slot
 */
void elementpropertieswidget::editElement() {
	if (CustomElement *custom_element = qobject_cast<CustomElement *>(element_)) {
		emit findElementRequired(custom_element->location());
		emit editElementRequired(custom_element->location());
	}
	reject();
}
