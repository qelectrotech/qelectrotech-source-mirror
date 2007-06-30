#include "styleeditor.h"
#include "customelementgraphicpart.h"

StyleEditor::StyleEditor(CustomElementGraphicPart *p, QWidget *parent) : QWidget(parent), part(p) {
	// couleur
	color = new QButtonGroup(this);
	color -> addButton(black_color = new QRadioButton(tr("Noir")),  CustomElementGraphicPart::BlackColor);
	color -> addButton(white_color = new QRadioButton(tr("Blanc")), CustomElementGraphicPart::WhiteColor);
	connect(color, SIGNAL(buttonClicked(int)), this, SLOT(updatePart()));
	
	// style
	style = new QButtonGroup(this);
	style -> addButton(normal_style = new QRadioButton(tr("Normal")),       CustomElementGraphicPart::NormalStyle);
	style -> addButton(dashed_style = new QRadioButton(tr("Pointill\351")), CustomElementGraphicPart::DashedStyle);
	style -> button(part -> lineStyle()) -> setChecked(true);
	connect(style, SIGNAL(buttonClicked(int)), this, SLOT(updatePart()));
	
	// epaisseur
	weight = new QButtonGroup(this);
	weight -> addButton(none_weight   = new QRadioButton(tr("Nulle")),   CustomElementGraphicPart::NoneWeight);
	weight -> addButton(thin_weight   = new QRadioButton(tr("Fine")),    CustomElementGraphicPart::ThinWeight);
	weight -> addButton(normal_weight = new QRadioButton(tr("Normale")), CustomElementGraphicPart::NormalWeight);
	connect(weight, SIGNAL(buttonClicked(int)), this, SLOT(updatePart()));
	
	// remplissage
	filling = new QButtonGroup(this);
	filling -> addButton(no_filling    = new QRadioButton(tr("Aucun")), CustomElementGraphicPart::NoneFilling );
	filling -> addButton(black_filling = new QRadioButton(tr("Noir")),  CustomElementGraphicPart::BlackFilling);
	filling -> addButton(white_filling = new QRadioButton(tr("Blanc")), CustomElementGraphicPart::WhiteFilling);
	connect(filling, SIGNAL(buttonClicked(int)), this, SLOT(updatePart()));
	
	// antialiasing
	antialiasing = new QCheckBox(tr("Antialiasing"));
	connect(antialiasing, SIGNAL(stateChanged(int)), this, SLOT(updatePart()));
	
	updateForm();
	
	main_layout = new QVBoxLayout();
	main_layout -> addWidget(antialiasing);
	
	main_layout -> addWidget(new QLabel("<u>" + tr("Trait :") + "</u> "));
	
	QHBoxLayout *color_layout = new QHBoxLayout();
	color_layout -> addWidget(new QLabel(tr("Couleur : ")));
	color_layout -> addWidget(black_color);
	color_layout -> addWidget(white_color);
	color_layout -> addStretch();
	main_layout -> addItem(color_layout);
	
	QHBoxLayout *style_layout = new QHBoxLayout();
	style_layout -> addWidget(new QLabel(tr("Style : ")));
	style_layout -> addWidget(normal_style);
	style_layout -> addWidget(dashed_style);
	style_layout -> addStretch();
	main_layout -> addItem(style_layout);
	
	QHBoxLayout *weight_layout = new QHBoxLayout();
	weight_layout -> addWidget(new QLabel(tr("\311paisseur : ")));
	weight_layout -> addWidget(none_weight);
	weight_layout -> addWidget(thin_weight);
	weight_layout -> addWidget(normal_weight);
	weight_layout -> addStretch();
	main_layout -> addItem(weight_layout);
	
	main_layout -> addWidget(new QLabel("<u>" + tr("Remplissage :") + "</u> "));
	
	QHBoxLayout *filling_layout = new QHBoxLayout();
	filling_layout -> addWidget(no_filling);
	filling_layout -> addWidget(black_filling);
	filling_layout -> addWidget(white_filling);
	filling_layout -> addStretch();
	main_layout -> addItem(filling_layout);
	
	main_layout -> addStretch();
	
	setLayout(main_layout);
}

StyleEditor::~StyleEditor() {
	qDebug() << "~StyleEditor()";
}

void StyleEditor::updatePart() {
	// applique l'antialiasing
	part -> setAntialiased(antialiasing -> isChecked());
	
	// applique la couleur
	part -> setColor(static_cast<CEGP::Color>(color -> checkedId()));
	
	// applique le style
	part -> setLineStyle(static_cast<CEGP::LineStyle>(style -> checkedId()));
	
	// applique l'epaisseur
	part -> setLineWeight(static_cast<CEGP::LineWeight>(weight -> checkedId()));
	
	// applique le remplissage
	part -> setFilling(static_cast<CEGP::Filling>(filling -> checkedId()));
}

void StyleEditor::updateForm() {
	// lit l'antialiasing : deconnexion du slot pour eviter l'appel a updatePart()
	disconnect(antialiasing, SIGNAL(stateChanged(int)), this, SLOT(updatePart()));
	antialiasing -> setChecked(part -> antialiased());
	connect(antialiasing, SIGNAL(stateChanged(int)), this, SLOT(updatePart()));
	
	// lit la couleur
	color -> button(part -> color()) -> setChecked(true);
	
	// lit le style
	style -> button(part -> lineStyle()) -> setChecked(true);
	
	// lit l'epaisseur
	weight -> button(part -> lineWeight()) -> setChecked(true);
	
	// lit le remplissage
	filling -> button(part -> filling()) -> setChecked(true);
}

void StyleEditor::appendWidget(QWidget *w) {
	main_layout -> insertWidget(7, w);
}
