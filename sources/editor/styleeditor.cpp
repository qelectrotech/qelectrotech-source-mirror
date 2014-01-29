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
#include "styleeditor.h"
#include "customelementgraphicpart.h"

/**
	Constructeur
	@param editor L'editeur d'element concerne
	@param p La partie a editer
	@param parent le Widget parent
*/
StyleEditor::StyleEditor(QETElementEditor *editor, CustomElementGraphicPart *p, QWidget *parent) :
	ElementItemEditor(editor, parent),
	part(p)
{
	// couleur
	outline_color = new QComboBox(this);
	outline_color -> addItem(tr("Noir", "element part color"), CustomElementGraphicPart::BlackColor);
	outline_color -> addItem(tr("Blanc", "element part color"), CustomElementGraphicPart::WhiteColor);
	outline_color -> addItem(tr("Vert", "element part color"), CustomElementGraphicPart::GreenColor);
	outline_color -> addItem(tr("Rouge", "element part color"), CustomElementGraphicPart::RedColor);
	outline_color -> addItem(tr("Bleu", "element part color"), CustomElementGraphicPart::BlueColor);

	// style
	line_style = new QComboBox(this);
	line_style -> addItem(tr("Normal",       "element part line style"), CustomElementGraphicPart::NormalStyle);
	line_style -> addItem(tr("Tiret",        "element part line style"), CustomElementGraphicPart::DashedStyle);
	line_style -> addItem(tr("Pointill\351", "element part line style"), CustomElementGraphicPart::DottedStyle);
	line_style -> addItem(tr("Traits et points", "element part line style"), CustomElementGraphicPart::DashdottedStyle);
	//normal_style -> setChecked(true);
	
	// epaisseur
	size_weight = new QComboBox(this);
	size_weight -> addItem(tr("Nulle", "element part weight"),  CustomElementGraphicPart::NoneWeight);
	size_weight -> addItem(tr("Fine", "element part weight"),  CustomElementGraphicPart::ThinWeight);
	size_weight -> addItem(tr("Normale", "element part weight"),  CustomElementGraphicPart::NormalWeight);
	size_weight -> addItem(tr("Forte", "element part weight"),  CustomElementGraphicPart::UltraWeight);
	size_weight -> addItem(tr("\311lev\351", "element part weight"),  CustomElementGraphicPart::BigWeight);

	// remplissage
	filling_color = new QComboBox (this);
	filling_color -> addItem(tr("Aucun", "element part filling"), CustomElementGraphicPart::NoneFilling);
	filling_color -> addItem(tr("Noir", "element part filling"), CustomElementGraphicPart::BlackFilling);
	filling_color -> addItem(tr("Blanc", "element part filling"), CustomElementGraphicPart::WhiteFilling);
	filling_color -> addItem(tr("Vert", "element part filling"), CustomElementGraphicPart::GreenFilling);
	filling_color -> addItem(tr("Rouge", "element part filling"), CustomElementGraphicPart::RedFilling);
	filling_color -> addItem(tr("Bleu", "element part filling"), CustomElementGraphicPart::BlueFilling);

	// antialiasing
	antialiasing = new QCheckBox(tr("Antialiasing"));
	
	updateForm();
	
	main_layout = new QVBoxLayout();
	main_layout -> setMargin(0);
	
	main_layout -> addWidget(new QLabel("<u>" + tr("Apparence :") + "</u> "));
	
	QHBoxLayout *color_layout = new QHBoxLayout();
	color_layout -> addWidget(new QLabel(tr("Contour :")), 0, Qt::AlignRight);
	color_layout -> addWidget(outline_color);
	color_layout -> addSpacing(10);
	color_layout -> addWidget(new QLabel(tr("Remplissage :")), 0, Qt::AlignRight);
	color_layout -> addWidget(filling_color);
	main_layout -> addLayout(color_layout);
	
	QHBoxLayout *style_layout = new QHBoxLayout();
	style_layout -> addWidget(new QLabel(tr("Style : ")), 0, Qt::AlignRight);
	style_layout -> addWidget(line_style);
	style_layout -> addSpacing(10);
	style_layout -> addWidget(new QLabel(tr("\311paisseur : ")), 0, Qt::AlignRight);
	style_layout -> addWidget(size_weight);
	main_layout -> addLayout(style_layout);

	main_layout -> addWidget(antialiasing);

	main_layout -> addSpacing(10);
	main_layout -> addWidget(new QLabel("<u>" + tr("G\351om\351trie :") + "</u> "));
	main_layout -> addStretch();
	setLayout(main_layout);
}

/// Destructeur
StyleEditor::~StyleEditor() {
}

/**
	Met a jour le style de la partie a partir des donnees du formulaire
*/
void StyleEditor::updatePart() {
	if (!part) return;
	// applique l'antialiasing
	part -> setAntialiased(antialiasing -> isChecked());
	
	// applique la couleur
	part -> setColor(static_cast<CEGP::Color>(outline_color -> currentIndex()));
	
	// applique le style
	part -> setLineStyle(static_cast<CEGP::LineStyle>(line_style -> currentIndex()));
	
	// applique l'epaisseur
	part -> setLineWeight(static_cast<CEGP::LineWeight>(size_weight -> currentIndex()));

	// applique le remplissage
	part -> setFilling(static_cast<CEGP::Filling>(filling_color -> currentIndex()));
}

/// Met a jour l'antialiasing et cree un objet d'annulation
void StyleEditor::updatePartAntialiasing()   { addChangePartCommand(tr("style antialiasing"), part, "antialias",   antialiasing -> isChecked()); }
/// Met a jour la couleur du trait et cree un objet d'annulation
void StyleEditor::updatePartColor()          { addChangePartCommand(tr("style couleur"),      part, "color",       outline_color -> currentIndex());}
/// Met a jour le style du trait et cree un objet d'annulation
void StyleEditor::updatePartLineStyle()      { addChangePartCommand(tr("style ligne"),        part, "line-style",  line_style -> currentIndex());}
/// Met a jour l'epaisseur du trait et cree un objet d'annulation
void StyleEditor::updatePartLineWeight()     { addChangePartCommand(tr("style epaisseur"),    part, "line-weight", size_weight -> currentIndex());}
/// Met a jour la couleur de fond et cree un objet d'annulation
void StyleEditor::updatePartFilling()        { addChangePartCommand(tr("style remplissage"),  part, "filling",     filling_color -> currentIndex());}

/**
	Met a jour le formulaire d'edition
*/
void StyleEditor::updateForm() {
	if (!part) return;
	activeConnections(false);
	// lit l'antialiasing
	antialiasing -> setChecked(part -> antialiased());
	
	// lit la couleur
	outline_color -> setCurrentIndex(part -> color());
	
	// lit le style
	line_style -> setCurrentIndex(part -> lineStyle());
	
	// lit l'epaisseur
	size_weight -> setCurrentIndex(part -> lineWeight());

	// lit le remplissage
	filling_color -> setCurrentIndex(part -> filling());
	activeConnections(true);
}

/**
	Permet de specifier a cet editeur quelle primitive il doit editer. A noter
	qu'un editeur peut accepter ou refuser d'editer une primitive.
	L'editeur de ligne acceptera d'editer la primitive new_part s'il s'agit d'un
	objet de la classe CustomElementGraphicPart.
	@param new_part Nouvelle primitive a editer
	@return true si l'editeur a accepter d'editer la primitive, false sinon
*/
bool StyleEditor::setPart(CustomElementPart *new_part) {
	if (!new_part) {
		part = 0;
		return(true);
	}
	if (CustomElementGraphicPart *part_graphic = dynamic_cast<CustomElementGraphicPart *>(new_part)) {
		part = part_graphic;
		updateForm();
		return(true);
	} else {
		return(false);
	}
}

/**
	@return la primitive actuellement editee, ou 0 si ce widget n'en edite pas
*/
CustomElementPart *StyleEditor::currentPart() const {
	return(part);
}

/**
	Active ou desactive les connexionx signaux/slots entre les widgets internes.
	@param active true pour activer les connexions, false pour les desactiver
*/
void StyleEditor::activeConnections(bool active) {
	if (active) {
		connect (outline_color, SIGNAL(activated(int)), this, SLOT(updatePartColor()));
		connect(line_style,        SIGNAL(activated(int)), this, SLOT(updatePartLineStyle()));
		connect(size_weight,       SIGNAL(activated(int)), this, SLOT(updatePartLineWeight()));
		connect(filling_color, SIGNAL(activated(int)), this, SLOT(updatePartFilling()));
		connect(antialiasing, SIGNAL(stateChanged(int)),  this, SLOT(updatePartAntialiasing()));
	} else {
		disconnect(outline_color, SIGNAL(activated(int)), this, SLOT(updatePartColor()));
		disconnect(line_style,        SIGNAL(activated(int)), this, SLOT(updatePartLineStyle()));
		disconnect(size_weight,       SIGNAL(activated(int)), this, SLOT(updatePartLineWeight()));
		disconnect(filling_color, SIGNAL(activated(int)), this, SLOT(updatePartFilling()));
		disconnect(antialiasing, SIGNAL(stateChanged(int)),  this, SLOT(updatePartAntialiasing()));
	}
}
