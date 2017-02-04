/*
	Copyright 2006-2017 The QElectroTech Team
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
#include "QPropertyUndoCommand/qpropertyundocommand.h"
#include <QComboBox>
#include <QCheckBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>

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
	outline_color -> addItem(tr("Gris", "element part color"), CustomElementGraphicPart::GrayColor);
	outline_color -> addItem(tr("Marron", "element part color"), CustomElementGraphicPart::BrunColor);
	outline_color -> addItem(tr("Jaune", "element part color"), CustomElementGraphicPart::YellowColor);
	outline_color -> addItem(tr("Cyan", "element part color"), CustomElementGraphicPart::CyanColor);
	outline_color -> addItem(tr("Magenta", "element part color"), CustomElementGraphicPart::MagentaColor);
	outline_color -> addItem(tr("Gris clair", "element part color"), CustomElementGraphicPart::LightgrayColor);
	outline_color -> addItem(tr("Orange", "element part color"), CustomElementGraphicPart::OrangeColor);
	outline_color -> addItem(tr("Violet", "element part color"), CustomElementGraphicPart::PurpleColor);
	outline_color -> addItem(tr("Aucun", "element part color"), CustomElementGraphicPart::NoneColor);

	// style
	line_style = new QComboBox(this);
	line_style -> addItem(tr("Normal",       "element part line style"), CustomElementGraphicPart::NormalStyle);
	line_style -> addItem(tr("Tiret",        "element part line style"), CustomElementGraphicPart::DashedStyle);
	line_style -> addItem(tr("Pointillé", "element part line style"), CustomElementGraphicPart::DottedStyle);
	line_style -> addItem(tr("Traits et points", "element part line style"), CustomElementGraphicPart::DashdottedStyle);
	//normal_style -> setChecked(true);
	
	// epaisseur
	size_weight = new QComboBox(this);
	size_weight -> addItem(tr("Nulle", "element part weight"),  CustomElementGraphicPart::NoneWeight);
	size_weight -> addItem(tr("Fine", "element part weight"),  CustomElementGraphicPart::ThinWeight);
	size_weight -> addItem(tr("Normale", "element part weight"),  CustomElementGraphicPart::NormalWeight);
	size_weight -> addItem(tr("Forte", "element part weight"),  CustomElementGraphicPart::UltraWeight);
	size_weight -> addItem(tr("Élevé", "element part weight"),  CustomElementGraphicPart::BigWeight);

	// remplissage
	filling_color = new QComboBox (this);
	filling_color -> addItem(tr("Aucun", "element part filling"), CustomElementGraphicPart::NoneFilling);
	filling_color -> addItem(tr("Noir", "element part filling"), CustomElementGraphicPart::BlackFilling);
	filling_color -> addItem(tr("Blanc", "element part filling"), CustomElementGraphicPart::WhiteFilling);
	filling_color -> addItem(tr("Vert", "element part filling"), CustomElementGraphicPart::GreenFilling);
	filling_color -> addItem(tr("Rouge", "element part filling"), CustomElementGraphicPart::RedFilling);
	filling_color -> addItem(tr("Bleu", "element part filling"), CustomElementGraphicPart::BlueFilling);
	filling_color -> addItem(tr("Gris", "element part filling"), CustomElementGraphicPart::GrayFilling);
	filling_color -> addItem(tr("Marron", "element part filling"), CustomElementGraphicPart::BrunFilling);
	filling_color -> addItem(tr("Jaune", "element part filling"), CustomElementGraphicPart::YellowFilling);
	filling_color -> addItem(tr("Cyan", "element part filling"), CustomElementGraphicPart::CyanFilling);
	filling_color -> addItem(tr("Magenta", "element part filling"), CustomElementGraphicPart::MagentaFilling);
	filling_color -> addItem(tr("Gris clair", "element part filling"), CustomElementGraphicPart::LightgrayFilling);
	filling_color -> addItem(tr("Orange", "element part filling"), CustomElementGraphicPart::OrangeFilling);
	filling_color -> addItem(tr("Violet", "element part filling"), CustomElementGraphicPart::PurpleFilling);
	filling_color -> addItem(tr("Lignes Horizontales", "element part filling"), CustomElementGraphicPart::HorFilling);
	filling_color -> addItem(tr("Lignes Verticales", "element part filling"), CustomElementGraphicPart::VerFilling);
	filling_color -> addItem(tr("Hachures gauche", "element part filling"), CustomElementGraphicPart::BdiagFilling);
	filling_color -> addItem(tr("Hachures droite", "element part filling"), CustomElementGraphicPart::FdiagFilling);

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
	style_layout -> addWidget(new QLabel(tr("Épaisseur : ")), 0, Qt::AlignRight);
	style_layout -> addWidget(size_weight);
	main_layout -> addLayout(style_layout);

	main_layout -> addWidget(antialiasing);

	main_layout -> addSpacing(10);
	main_layout -> addWidget(new QLabel("<u>" + tr("Géométrie :") + "</u> "));
	main_layout -> addStretch();
	setLayout(main_layout);
}

/// Destructeur
StyleEditor::~StyleEditor() {
}

/// Update antialiasing with undo command
void StyleEditor::updatePartAntialiasing() {
	makeUndo(tr("style antialiasing"), "antialias", antialiasing -> isChecked());
}

/// Update color with undo command
void StyleEditor::updatePartColor() {
	makeUndo(tr("style couleur"),"color", outline_color->itemData(outline_color -> currentIndex()));
}

/// Update style with undo command
void StyleEditor::updatePartLineStyle() {
	makeUndo(tr("style ligne"), "line_style", line_style->itemData(line_style -> currentIndex()));
}

/// Update weight with undo command
void StyleEditor::updatePartLineWeight() {
	makeUndo(tr("style epaisseur"), "line_weight", size_weight->itemData(size_weight -> currentIndex()));
}

/// Update color filling with undo command
void StyleEditor::updatePartFilling() {
	makeUndo(tr("style remplissage"), "filling", filling_color->itemData(filling_color -> currentIndex()));
}

/**
 * @brief StyleEditor::updateForm
 * Update the edition form according to the value of edited part(s)
 */
void StyleEditor::updateForm()
{
	if (!part && m_part_list.isEmpty()) return;
	activeConnections(false);

	if (part)
	{
		antialiasing -> setChecked(part -> antialiased());
		outline_color -> setCurrentIndex(part -> color());
		line_style    -> setCurrentIndex(part -> lineStyle());
		size_weight   -> setCurrentIndex(part -> lineWeight());
		filling_color -> setCurrentIndex(part -> filling());
	}
	else if (m_part_list.size())
	{
		CustomElementGraphicPart *first_part = m_part_list.first();
		antialiasing -> setChecked(first_part -> antialiased());
		outline_color -> setCurrentIndex(first_part -> color());
		line_style    -> setCurrentIndex(first_part -> lineStyle());
		size_weight   -> setCurrentIndex(first_part -> lineWeight());
		filling_color -> setCurrentIndex(first_part -> filling());

		for (CustomElementGraphicPart *cegp: m_part_list)
		{
			if (first_part -> antialiased() != cegp -> antialiased()) antialiasing -> setChecked(false);
			if (first_part -> color()       != cegp -> color())      outline_color -> setCurrentIndex(-1);
			if (first_part -> lineStyle()   != cegp -> lineStyle())  line_style    -> setCurrentIndex(-1);
			if (first_part -> lineWeight()  != cegp -> lineWeight()) size_weight   -> setCurrentIndex(-1);
			if (first_part -> filling()     != cegp -> filling())    filling_color -> setCurrentIndex(-1);
		}
	}

	activeConnections(true);
}

/**
 * @brief StyleEditor::setPart
 * Set the part to edit by this editor.
 * Note : editor can accept or refuse to edit a part
 * @param new_part : part to edit
 * @return  true if editor accept to edit this CustomElementPart otherwise false
 */
bool StyleEditor::setPart(CustomElementPart *new_part) {
	m_part_list.clear();

	if (!new_part)
	{
		part = nullptr;
		return(true);
	}

	if (CustomElementGraphicPart *part_graphic = dynamic_cast<CustomElementGraphicPart *>(new_part))
	{
		part = part_graphic;
		updateForm();
		return(true);
	}

	return(false);
}

/**
 * @brief StyleEditor::setParts
 * Set several parts to edit by this editor.
 * Note : editor can accept or refuse to edit several parts.
 * @param part_list
 * @return true if every customeElementPart stored in part_list can
 * be edited by this part editor, otherwise return false
 * (see StyleEditor::isStyleEditable)
 */
bool StyleEditor::setParts(QList<CustomElementPart *> part_list)
{
	if (part_list.isEmpty()) return false;
	if (part_list.size() == 1) return setPart(part_list.first());

	part = nullptr;
	m_part_list.clear();
	m_cep_list.clear();

	if (!isStyleEditable(part_list)) return false;

	for (CustomElementPart *cep: part_list)
	{
		if (CustomElementGraphicPart *cegp = dynamic_cast<CustomElementGraphicPart *>(cep))
			m_part_list << cegp;
		else
			return false;
	}

	for (CustomElementGraphicPart *cegp: m_part_list)
		m_cep_list << cegp;

	updateForm();
	return true;
}

/**
	@return la primitive actuellement editee, ou 0 si ce widget n'en edite pas
*/
CustomElementPart *StyleEditor::currentPart() const {
	return(part);
}

/**
 * @brief StyleEditor::isStyleEditable
 * @param cep_list
 * @return true if all of the content of cep_list can be edited by style editor, else return false.
 */
bool StyleEditor::isStyleEditable(QList<CustomElementPart *> cep_list)
{
	QStringList str;
	str << "arc" << "ellipse" << "line" << "polygon" << "rect";

	for (CustomElementPart *cep: cep_list)
		if (!str.contains(cep -> xmlName()))
			return false;

	return true;
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

void StyleEditor::makeUndo(const QString &undo_text, const char *property_name, const QVariant &new_value)
{
	QPropertyUndoCommand *undo = nullptr;
	if (part && (new_value != part->property(property_name)))
	{
		undo = new QPropertyUndoCommand(part, property_name, part->property(property_name), new_value);
		undo->setText(undo_text);
		undoStack().push(undo);
		return;
	}
	else if (!m_part_list.isEmpty())
	{
		for (CustomElementGraphicPart *cegp: m_part_list)
		{
			if (!undo)
			{
				undo = new QPropertyUndoCommand(cegp, property_name, cegp->property(property_name), new_value);
				undo->setText(undo_text);
			}
			else
				new QPropertyUndoCommand(cegp, property_name, cegp->property(property_name), new_value, undo);
		}
		undoStack().push(undo);
	}
}
