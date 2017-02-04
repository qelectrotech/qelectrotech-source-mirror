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
#include "templatecellwidget.h"
#include "titleblockcell.h"
#include "nameslist.h"
#include "nameslistwidget.h"
#include "titleblocktemplate.h"
#include "templatecommands.h"
#include "qeticons.h"

/**
	Constructor
	@param parent Parent QWidget
*/
TitleBlockTemplateCellWidget::TitleBlockTemplateCellWidget(TitleBlockTemplate *parent_template, QWidget *parent) :
	QWidget(parent),
	read_only_(false)
{
	initWidgets();
	updateLogosComboBox(parent_template);
}

/**
	Destructor
*/
TitleBlockTemplateCellWidget::~TitleBlockTemplateCellWidget() {
}

/**
	Initialize layout and widgets.
*/
void TitleBlockTemplateCellWidget::initWidgets() {
	// type combo box: always displayed
	cell_type_label_ = new QLabel(tr("Type de cellule :"));
	cell_type_input_ = new QComboBox();
	cell_type_input_ -> addItem(tr("Vide"),  TitleBlockCell::EmptyCell);
	cell_type_input_ -> addItem(tr("Texte"), TitleBlockCell::TextCell);
	cell_type_input_ -> addItem(tr("Logo"),  TitleBlockCell::LogoCell);
	
	// name input: displayed for text and logo cells
	name_label_ = new QLabel(tr("Nom :"));
	name_input_ = new QLineEdit();
	
	// widgets specific to empty cells
    empty_label_ = new QLabel(tr("Attention : les bordures des cellules vides n'apparaissent pas lors du rendu final sur le folio."));
	
	// widgets specific to logo cells
	logo_label_ = new QLabel(tr("Logo"));
	logo_input_ = new QComboBox();
	logo_input_ -> addItem(tr("Aucun logo"));
	add_logo_input_ = new QPushButton(QET::Icons::InsertImage, tr("Gérer les logos"));
	
	// widgets specific to text cells
	label_checkbox_ = new QCheckBox(tr("Afficher un label :"));
	label_input_ = new QLineEdit();
	label_input_ -> setReadOnly(true);
	label_edit_ = new QPushButton(tr("Editer"));
	value_label_ = new QLabel(tr("Texte :"));
	value_input_ = new QLineEdit();
	value_input_ -> setReadOnly(true);
	value_edit_ = new QPushButton(tr("Editer"));
	align_label_ = new QLabel(tr("Alignement :"));
	horiz_align_label_ = new QLabel(tr("horizontal :"));
	horiz_align_input_ = new QComboBox();
	horiz_align_input_ -> addItem(tr("Gauche"),      Qt::AlignLeft);
	horiz_align_input_ -> addItem(tr("Centré"),   Qt::AlignHCenter);
	horiz_align_input_ -> addItem(tr("Droite"),      Qt::AlignRight);
	horiz_align_indexes_.insert(Qt::AlignLeft,    0);
	horiz_align_indexes_.insert(Qt::AlignHCenter, 1);
	horiz_align_indexes_.insert(Qt::AlignRight,   2);
	vert_align_label_= new QLabel(tr("vertical :"));
	vert_align_input_ = new QComboBox();
	vert_align_input_ -> addItem(tr("Haut"),   Qt::AlignTop);
	vert_align_input_ -> addItem(tr("Milieu"), Qt::AlignVCenter);
	vert_align_input_ -> addItem(tr("Bas"),    Qt::AlignBottom);
	vert_align_indexes_.insert(Qt::AlignTop,     0);
	vert_align_indexes_.insert(Qt::AlignVCenter, 1);
	vert_align_indexes_.insert(Qt::AlignBottom,  2);
	font_size_label_ = new QLabel(tr("Police :"));
	font_size_input_ = new QSpinBox();
	font_adjust_input_ = new QCheckBox(tr("Ajuster la taille de police si besoin"));
	
	// layout
	QHBoxLayout *label_edition = new QHBoxLayout();
	label_edition -> addWidget(label_input_);
	label_edition -> addWidget(label_edit_);
	
	QHBoxLayout *value_edition = new QHBoxLayout();
	value_edition -> addWidget(value_input_);
	value_edition -> addWidget(value_edit_);
	
	cell_editor_type_and_name_layout_ = new QHBoxLayout();
	cell_editor_type_and_name_layout_ -> addWidget(cell_type_label_);
	cell_editor_type_and_name_layout_ -> addWidget(cell_type_input_);
	cell_editor_type_and_name_layout_ -> addWidget(name_label_);
	cell_editor_type_and_name_layout_ -> addWidget(name_input_);
	cell_editor_type_and_name_layout_ -> addStretch(5);
	
	cell_editor_text_layout_ = new QGridLayout();
	cell_editor_text_layout_ -> addWidget(label_checkbox_,     2, 0);
	cell_editor_text_layout_ -> addLayout(label_edition,       2, 1);
	cell_editor_text_layout_ -> addWidget(value_label_,        3, 0);
	cell_editor_text_layout_ -> addLayout(value_edition,       3, 1);
	cell_editor_text_layout_ -> addWidget(align_label_,        1, 2, 1, 2, Qt::AlignHCenter);
	cell_editor_text_layout_ -> addWidget(horiz_align_label_,  2, 2);
	cell_editor_text_layout_ -> addWidget(horiz_align_input_,  2, 3);
	cell_editor_text_layout_ -> addWidget(vert_align_label_,   3, 2);
	cell_editor_text_layout_ -> addWidget(vert_align_input_,   3, 3);
	cell_editor_text_layout_ -> addWidget(font_size_label_,    4, 0);
	cell_editor_text_layout_ -> addWidget(font_size_input_,    4, 1);
	cell_editor_text_layout_ -> addWidget(font_adjust_input_,  4, 2, 1, 2, Qt::AlignLeft);
	cell_editor_text_layout_ -> addWidget(logo_label_,         5, 0);
	cell_editor_text_layout_ -> addWidget(logo_input_,         5, 1);
	cell_editor_text_layout_ -> addWidget(add_logo_input_,     5, 2);
	cell_editor_text_layout_ -> addWidget(empty_label_,        6, 0);
	cell_editor_text_layout_ -> setColumnStretch(4, 4000);
	cell_editor_layout_ = new QVBoxLayout();
	cell_editor_layout_ -> addLayout(cell_editor_type_and_name_layout_);
	cell_editor_layout_ -> addLayout(cell_editor_text_layout_);
	cell_editor_layout_ -> addStretch();
	setLayout(cell_editor_layout_);
	
	// trigger the logo manager
	connect(add_logo_input_, SIGNAL(released()), this, SIGNAL(logoEditionRequested()));
	
	// handle cell modifications
	connect(cell_type_input_,   SIGNAL(activated(int)),           this, SLOT(updateFormType(int)));
	connect(cell_type_input_,   SIGNAL(activated(int)),           this, SLOT(editType()));
	connect(name_input_,        SIGNAL(editingFinished()),        this, SLOT(editName()));
	connect(label_checkbox_,    SIGNAL(clicked(bool)),            this, SLOT(editLabelDisplayed()));
	connect(label_edit_,        SIGNAL(released()),               this, SLOT(editLabel()));
	connect(value_edit_,        SIGNAL(released()),               this, SLOT(editValue()));
	connect(horiz_align_input_, SIGNAL(activated(int)),           this, SLOT(editAlignment()));
	connect(vert_align_input_,  SIGNAL(activated(int)),           this, SLOT(editAlignment()));
	connect(font_size_input_,   SIGNAL(valueChanged(int)),        this, SLOT(editFontSize()));
	connect(font_adjust_input_, SIGNAL(clicked(bool)),            this, SLOT(editAdjust()));
	connect(logo_input_,        SIGNAL(activated(int)),           this, SLOT(editLogo()));
	
	updateFormType(TitleBlockCell::TextCell);
}

/**
	Shows or hides various widgets depending on the selected cell type
*/
void TitleBlockTemplateCellWidget::updateFormType(int cell_type) {
	if (cell_type_input_ -> currentIndex() != cell_type) {
		cell_type_input_ -> setCurrentIndex(cell_type);
	}
	
	name_label_        -> setVisible(cell_type);
	name_input_        -> setVisible(cell_type);
	
	empty_label_       -> setVisible(cell_type == TitleBlockCell::EmptyCell);
	
	logo_label_        -> setVisible(cell_type == TitleBlockCell::LogoCell);
	logo_input_        -> setVisible(cell_type == TitleBlockCell::LogoCell);
	add_logo_input_    -> setVisible(cell_type == TitleBlockCell::LogoCell);
	
	label_checkbox_    -> setVisible(cell_type == TitleBlockCell::TextCell);
	label_input_       -> setVisible(cell_type == TitleBlockCell::TextCell);
	label_edit_        -> setVisible(cell_type == TitleBlockCell::TextCell);
	value_label_       -> setVisible(cell_type == TitleBlockCell::TextCell);
	value_input_       -> setVisible(cell_type == TitleBlockCell::TextCell);
	value_edit_        -> setVisible(cell_type == TitleBlockCell::TextCell);
	align_label_       -> setVisible(cell_type == TitleBlockCell::TextCell);
	horiz_align_label_ -> setVisible(cell_type == TitleBlockCell::TextCell);
	horiz_align_input_ -> setVisible(cell_type == TitleBlockCell::TextCell);
	vert_align_label_  -> setVisible(cell_type == TitleBlockCell::TextCell);
	vert_align_input_  -> setVisible(cell_type == TitleBlockCell::TextCell);
	font_size_label_   -> setVisible(cell_type == TitleBlockCell::TextCell);
	font_size_input_   -> setVisible(cell_type == TitleBlockCell::TextCell);
	font_adjust_input_ -> setVisible(cell_type == TitleBlockCell::TextCell);
}

/**
	Set the title block cell to be edited. The cell pointer is stored by this
	class; however, modifications made by the user are packaged as
	ModifyTitleBlockCellCommand objects and emitted through the
	cellModified() signal.
	@param cell Title block cell to be edited
*/
void TitleBlockTemplateCellWidget::edit(TitleBlockCell *cell) {
	if (!cell) return;
	edited_cell_ = cell;
	int type = cell -> type();
	updateFormType(type);
	
	name_input_        -> setText(cell -> value_name);
	label_checkbox_    -> setChecked(cell -> display_label);
	label_input_       -> setText(cell -> label.name());
	value_input_       -> setText(cell -> value.name());
	font_adjust_input_ -> setChecked(cell -> hadjust);
	horiz_align_input_ -> setCurrentIndex(horiz_align_indexes_[cell -> horizontalAlign()]);
	vert_align_input_  -> setCurrentIndex(vert_align_indexes_[cell -> verticalAlign()]);
	
	font_size_input_   -> blockSignals(true); // QSpinBox has no signal triggered for each non-programmatic change
	font_size_input_   -> setValue(TitleBlockTemplate::fontForCell(*cell).pointSize());
	font_size_input_   -> blockSignals(false);
	
	logo_input_        -> setCurrentIndex(logo_input_ -> findData(cell -> logo_reference));
}

/**
	Emit a type modification command.
	@see ModifyTitleBlockCellCommand
*/
void TitleBlockTemplateCellWidget::editType() {
	emitModification("type", cell_type_input_ -> itemData(cell_type_input_ -> currentIndex()));
}

/**
	Emit a name modification command.
	@see ModifyTitleBlockCellCommand
*/
void TitleBlockTemplateCellWidget::editName() {
	emitModification("name", name_input_ -> text());
}

/**
	Emit a modification command stating whether the label should be displayed or not.
	@see ModifyTitleBlockCellCommand
*/
void TitleBlockTemplateCellWidget::editLabelDisplayed() {
	emitModification("displaylabel", label_checkbox_ -> isChecked());
}

/**
	Emit a label modification command.
	@see ModifyTitleBlockCellCommand
*/
void TitleBlockTemplateCellWidget::editLabel() {
	if (!edited_cell_) return;
	editTranslatableValue(edited_cell_ -> label, "label", tr("Label de cette cellule"));
	label_input_ -> setText(edited_cell_ -> label.name());
}

/**
	Emit a value modification command.
	@see ModifyTitleBlockCellCommand
*/
void TitleBlockTemplateCellWidget::editValue() {
	if (!edited_cell_) return;
	editTranslatableValue(edited_cell_ -> value, "value", tr("Valeur de cette cellule"));
	value_input_ -> setText(edited_cell_ -> value.name());
}

/**
	Emit an alignment modification command.
	@see ModifyTitleBlockCellCommand
*/
void TitleBlockTemplateCellWidget::editAlignment() {
	emitModification("alignment", alignment());
}

/**
	Emit a font size modification command.
	@see ModifyTitleBlockCellCommand
*/
void TitleBlockTemplateCellWidget::editFontSize() {
	emitModification("fontsize", font_size_input_ -> value());
}

/**
	Emit a modification command stating whether the text should be adjusted if needed.
	@see ModifyTitleBlockCellCommand
*/
void TitleBlockTemplateCellWidget::editAdjust() {
	emitModification("horizontal_adjust", font_adjust_input_ -> isChecked());
}

/**
	Emit a logo modification command.
	@see ModifyTitleBlockCellCommand
*/
void TitleBlockTemplateCellWidget::editLogo() {
	emitModification("logo", logo_input_ -> currentText());
}

/**
	Updates the list of available logos
	@param parent_template The title block template which contains the currently edited cell
*/
void TitleBlockTemplateCellWidget::updateLogosComboBox(const TitleBlockTemplate *parent_template) {
	// saves the current value before erasing all entries
	QVariant current_value = logo_input_ -> itemData(logo_input_ -> currentIndex());
	logo_input_ -> clear();
	
	// default choice (the parent template may have no logo yet)
	logo_input_ -> addItem(
		tr("Aucun logo", "text displayed in the combo box when a template has no logo"),
		QVariant(QString(""))
	);
	logo_input_ -> setCurrentIndex(0);
	
	if (!parent_template) return;
	for (QString logo: parent_template -> logos()) {
		logo_input_ -> addItem(logo, QVariant(logo));
	}
	int current_value_index = logo_input_ -> findData(current_value);
	if (current_value_index != -1) {
		logo_input_ -> setCurrentIndex(current_value_index);
	}
}

/**
	@param read_only whether this edition widget should be read only
*/
void TitleBlockTemplateCellWidget::setReadOnly(bool read_only) {
	if (read_only_ == read_only) return;
	read_only_ = read_only;
	
	cell_type_input_ -> setEnabled(!read_only_);
	logo_input_ -> setEnabled(!read_only_);
	name_input_ -> setReadOnly(read_only_);
	label_checkbox_ -> setEnabled(!read_only_);
	label_edit_ -> setEnabled(!read_only_);
	value_edit_ -> setEnabled(!read_only_);
	horiz_align_input_ -> setEnabled(!read_only_);
	vert_align_input_ -> setEnabled(!read_only_);
	font_size_input_ -> setReadOnly(read_only_);
	font_adjust_input_ -> setEnabled(!read_only_);
}

/**
	Emit a horizontal alignment modification command.
	@see ModifyTitleBlockCellCommand
*/
int TitleBlockTemplateCellWidget::horizontalAlignment() const {
	return(horiz_align_indexes_.key(horiz_align_input_ -> currentIndex()));
}

/**
	Emit a vertical alignment modification command.
	@see ModifyTitleBlockCellCommand
*/
int TitleBlockTemplateCellWidget::verticalAlignment() const {
	return(vert_align_indexes_.key(vert_align_input_ -> currentIndex()));
}

/**
	@return the currently selected alignment.
*/
int TitleBlockTemplateCellWidget::alignment() const {
	return(horizontalAlignment() | verticalAlignment());
}

/**
	@return whether this edition widget is read only
*/
bool TitleBlockTemplateCellWidget::isReadOnly() const {
	return(read_only_);
}

/**
	Allow the user to edit a translatable string (e.g. value or label).
	If the user modified the string, this method emits a
	ModifyTitleBlockCellCommand object through the cellModified() signal.
	@param names Translatable string to be edited
	@param attribute Name of the edited cell attribute
	@param title Title of the dialog window
*/
void TitleBlockTemplateCellWidget::editTranslatableValue(NamesList &names, const QString &attribute, const QString &title) const {
	NamesListWidget *names_widget = new NamesListWidget();
	names_widget -> setNames(names);
	QDialogButtonBox * buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
	
	QLabel *information = new QLabel(labelValueInformationString());
	information -> setTextFormat(Qt::RichText);
	information -> setWordWrap(true);
	
	QLabel *def_var_label = new QLabel(defaultVariablesString());
	def_var_label -> setTextFormat(Qt::RichText);
	
	QVBoxLayout *editor_layout = new QVBoxLayout();
	editor_layout -> addWidget(information);
	editor_layout -> addWidget(names_widget);
	editor_layout -> addWidget(def_var_label);
	editor_layout -> addWidget(buttons);
	
	QDialog edit_dialog;
	edit_dialog.setWindowTitle(title);
	connect(buttons, SIGNAL(rejected()), &edit_dialog, SLOT(reject()));
	connect(buttons, SIGNAL(accepted()), &edit_dialog, SLOT(accept()));
	edit_dialog.setLayout(editor_layout);
	if (edit_dialog.exec() == QDialog::Accepted) {
		emitModification(attribute, qVariantFromValue(names_widget -> names()));
		
	}
}

/**
	Create a ModifyTitleBlockCellCommand object to change \a attribute to \a new_value.
	This object is then emitted through the cellModified() signal.
	@see ModifyTitleBlockCellCommand
	@param attribute Modified cell attribute
	@param new_value New value for the modified cell attribute
*/
void TitleBlockTemplateCellWidget::emitModification(const QString &attribute, const QVariant &new_value) const {
	if (!edited_cell_) return;
	
	// avoid creating a QUndoCommand object when no modification was actually done
	if (edited_cell_ -> attribute(attribute) == new_value) return;
	
	ModifyTitleBlockCellCommand *command = new ModifyTitleBlockCellCommand(edited_cell_);
	command -> addModification(attribute, new_value);
	command -> setText(
		tr("Édition d'une cellule : %1", "label of and undo command when editing a cell")
		.arg(TitleBlockCell::attributeName(attribute))
	);
	emit(cellModified(command));
}

/**
	@return a string describing the various variables provided by default by
	the application.
*/
QString TitleBlockTemplateCellWidget::defaultVariablesString() const {
	QString def_var_string = tr(
		"Par défaut, les variables suivantes sont disponibles :"
		"<ul>"
		"<li>%{author} : auteur du folio</li>"
		"<li>%{date} : date du folio</li>"
		"<li>%{title} : titre du folio</li>"
		"<li>%{filename} : nom de fichier du projet</li>"
		"<li>%{machine} : nom du groupe fonctionnel du projet</li>"
		"<li>%{locmach} : nom de la localisation dans le groupe fonctionnel du projet</li>"
		"<li>%{indexrev} : indice de révision du folio</li>"
		"<li>%{version} : version du logiciel</li>"
		"<li>%{folio} : indications relatives au folio</li>"
		"<li>%{folio-id} : position du folio dans le projet</li>"
		"<li>%{folio-total} : nombre total de folios dans le projet</li>"
		"</ul>"
	);
	return(def_var_string);
}

/**
	@return a string describing what the user may enter as cell label / value.
*/
QString TitleBlockTemplateCellWidget::labelValueInformationString() const {
	QString lab_val_inf_string = tr(
		"Chaque cellule d'un cartouche affiche une valeur, optionnellement "
		"précédée d'un label. Tous deux peuvent être traduits en "
		"plusieurs langues."
		"<br/>"
		"Comme ce que vous éditez actuellement est un "
		"<em>modèle</em> de cartouche, ne saisissez pas directement des "
		"données brutes : insérez plutôt des variables sous la forme "
		"%{nom-de-variable}, qui seront ensuite remplacées par les valeurs "
		"adéquates sur le folio."
	);
	return(lab_val_inf_string);
}
