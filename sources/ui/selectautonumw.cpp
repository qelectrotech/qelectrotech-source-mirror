#include "selectautonumw.h"
#include "ui_selectautonumw.h"
#include "diagram.h"

/**
 * Constructor
 */
SelectAutonumW::SelectAutonumW(const QList <Diagram *> &diagrams, QWidget *parent) :
	QWidget(parent),
	ui(new Ui::SelectAutonumW),
	diagram_list(diagrams)
{
	ui->setupUi(this);

	initDiagramChooser();
	setCurrentContext();
}

/**
 * Destructor
 */
SelectAutonumW::~SelectAutonumW()
{
	delete ui;
}

/**
 * @brief SelectAutonumW::setDiagramChooser
 * build the content of QComboBox @diagram_chooser.
 */
void SelectAutonumW::initDiagramChooser() {
	for (int i=0; i<diagram_list.size(); ++i) {
		QString diagram_title = diagram_list.at(i) -> title();
		if (diagram_title.isEmpty()) diagram_title = (tr("Sch\351ma sans titre"));
		ui -> diagram_chooser -> addItem(diagram_title);
	}
}

/**
 * @brief SelectAutonumW::setCurrentContext
 * build the context of current diagram selected in the @diagram_chooser QcomboBox
 */
void SelectAutonumW::setCurrentContext() {
	NumerotationContext nc = diagram_list.at(ui->diagram_chooser->currentIndex()) -> getNumerotation(Diagram::Conductors);

	if (nc.size() == 0) { //@nc contain nothing, build a default numPartEditor
		on_add_button_clicked();
		applyEnable(false);
		return;
	}
	for (int i=0; i<nc.size(); ++i) { //build with the content of @nc
		NumPartEditorW *part= new NumPartEditorW(nc, i, this);
		connect (part, SIGNAL(changed()), this, SLOT(applyEnable()));
		num_part_list_ << part;
		ui -> editor_layout -> addWidget(part);
	}
	applyEnable(false);
}

/**
 * @brief SelectAutonumW::toNumContext
 * @return the content to @num_part_list to NumerotationContext
 */
NumerotationContext SelectAutonumW::toNumContext() const {
	NumerotationContext nc;
	foreach (NumPartEditorW *npew, num_part_list_) nc << npew -> toNumContext();
	return nc;
}

/**
 * @brief SelectAutonumW::on_add_button_clicked
 *	Action on add_button, add a @NumPartEditor
 */
void SelectAutonumW::on_add_button_clicked() {
	applyEnable(false);
	NumPartEditorW *part = new NumPartEditorW(this);
	connect (part, SIGNAL(changed()), this, SLOT(applyEnable()));
	num_part_list_ << part;
	ui -> editor_layout -> addWidget(part);
}

/**
 * @brief SelectAutonumW::on_remove_button_clicked
 *	Action on remove button, remove the last @NumPartEditor
 */
void SelectAutonumW::on_remove_button_clicked() {
	//remove if @num_part_list contains more than one item
	if (num_part_list_.size() > 1) {
		NumPartEditorW *part = num_part_list_.takeLast();
		disconnect(part, SIGNAL(changed()), this, SLOT(applyEnable()));
		delete part;
	}
	applyEnable();
}

/**
 * @brief SelectAutonumW::on_diagram_chooser_activated
 * Action on diagram_chooser
 */
void SelectAutonumW::on_diagram_chooser_activated() {
	foreach(NumPartEditorW *npew, num_part_list_) delete npew;
	num_part_list_.clear();
	setCurrentContext();
}

/**
 * @brief SelectAutonumW::on_buttonBox_clicked
 * Action on @buttonBox clicked
 */
void SelectAutonumW::on_buttonBox_clicked(QAbstractButton *button) {
	//transform button to int
	int answer = ui -> buttonBox -> buttonRole(button);

	switch (answer) {
		//reset the displayed context to default context of @diagram_chooser.
		case QDialogButtonBox::ResetRole:
			on_diagram_chooser_activated();
			applyEnable(false);
			break;
		//apply the context in the diagram displayed by @diagram_chooser.
		case QDialogButtonBox::ApplyRole:
			NumerotationContext nc = toNumContext();
			diagram_list.at(ui -> diagram_chooser -> currentIndex()) -> setNumerotation(Diagram::Conductors, nc);
			applyEnable(false);
			break;
	};
}

/**
 * @brief SelectAutonumW::applyEnable
 * enable/disable the apply button
 */
void SelectAutonumW::applyEnable(bool b) {
	if (b){
		bool valid= true;
		foreach (NumPartEditorW *npe, num_part_list_) if (!npe -> isValid()) valid= false;
		ui -> buttonBox -> button(QDialogButtonBox::Apply) -> setEnabled(valid);
	}
	else
		ui -> buttonBox -> button(QDialogButtonBox::Apply) -> setEnabled(b);
}
