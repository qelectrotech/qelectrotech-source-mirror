/*
	Copyright 2006-2026 The QElectroTech Team
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
#include "selectautonumw.h"

#include "../assignvariables.h"
#include "formulaautonumberingw.h"
#include "numparteditorw.h"
#include "ui_formulaautonumberingw.h"
#include "ui_selectautonumw.h"

#include "../../qeticons.h"

#include <QHBoxLayout>
#include <QMessageBox>
#include <QPushButton>
#include <QToolButton>

/**
	@brief SelectAutonumW::SelectAutonumW
	Constructor
	@param type : int m_edited_type
	@param parent : QWidget
*/
SelectAutonumW::SelectAutonumW(int type, QWidget *parent) :
	QWidget(parent),
	ui(new Ui::SelectAutonumW),
	m_edited_type(type)
{
	ui->setupUi(this);
	ui->m_comboBox->lineEdit()->setClearButtonEnabled(true);

		//The add button goes under the last part, at the left; each part
		//has its own remove and move buttons at its right. The one remove
		//button above the parts, which could only remove the last one, is
		//not needed any more.
	ui->remove_button->hide();
	ui->horizontalLayout->removeWidget(ui->add_button);
	m_add_row = new QWidget(this);
	auto *add_layout = new QHBoxLayout(m_add_row);
	add_layout->setContentsMargins(0, 0, 0, 0);
	ui->add_button->setParent(m_add_row);
	add_layout->addWidget(ui->add_button);
	add_layout->addStretch();
	ui->editor_layout->addWidget(m_add_row);
	ui->buttonBox->button(QDialogButtonBox::Reset)->setText(tr("Annuler"));
	ui->buttonBox->button(QDialogButtonBox::Reset)->setToolTip(
				tr("Revenir à la définition enregistrée"));

	if (m_edited_type == 0)
	{
		m_feaw = new FormulaAutonumberingW();
		m_feaw->ui->label->setHidden(true);
		ui->m_widget->layout()->addWidget(m_feaw);
	}
	else if (m_edited_type == 1)
	{
		m_fcaw = new FormulaAutonumberingW();
		m_fcaw->ui->label->setHidden(true);
		ui->m_widget->layout()->addWidget(m_fcaw);
	}
	setContext(NumerotationContext());
}

/**
	@brief SelectAutonumW::SelectAutonumW
	Constructor
	@param context : NumerotationContext
	@param type : int m_edited_type
	@param parent : QWidget
*/
SelectAutonumW::SelectAutonumW(const NumerotationContext &context,
			       int type,
			       QWidget *parent) :
	QWidget(parent),
	ui(new Ui::SelectAutonumW),
	m_edited_type(type)
{
	if (m_edited_type == 0)
	{
		m_feaw = new FormulaAutonumberingW();
		m_feaw->ui->label->setHidden(true);
		ui->m_widget->layout()->addWidget(m_feaw);
	}
	else if (m_edited_type == 1)
	{
		m_fcaw = new FormulaAutonumberingW();
		m_fcaw->ui->label->setHidden(true);
		ui->m_widget->layout()->addWidget(m_fcaw);
	}
	ui->setupUi(this);
	setContext(context);
}

/**
	@brief SelectAutonumW::~SelectAutonumW
	Destructor
*/
SelectAutonumW::~SelectAutonumW()
{
	delete ui;
}

/**
	@brief SelectAutonumW::setContext
	build the context of current diagram
	selected in the diagram_chooser QcomboBox
	@param context
*/
void SelectAutonumW::setContext(const NumerotationContext &context)
{
	m_context = context;

	clearPartRows();

	if (m_context.size() == 0) { //@context contain nothing, build a default numPartEditor
		insertPartRow(new NumPartEditorW(m_edited_type, this));
	}
	else {
		for (int i=0; i<m_context.size(); ++i) { //build with the content of @context
			insertPartRow(new NumPartEditorW(m_context, i, m_edited_type, this));
		}
	}

	updatePartButtons();
	applyEnable(false);
		//Everything the rule held before is read now, so nothing of it
		//is left to write: what is on show is what there is.
	m_dirty = false;
}

/**
	@brief SelectAutonumW::insertPartRow
	Show @p part as the last part of the definition, with the buttons which
	remove it and move it up or down at its right.
*/
void SelectAutonumW::insertPartRow(NumPartEditorW *part)
{
	connect(part, &NumPartEditorW::changed, this, [this]() {
		m_dirty = true;
		applyEnable();
		emit contextEdited();
	});

	PartRow r;
	r.part = part;
	r.row = new QWidget(this);
	auto *layout = new QHBoxLayout(r.row);
	layout->setContentsMargins(0, 0, 0, 0);
	layout->setSpacing(2);
	part->setParent(r.row);
	layout->addWidget(part, 1);

	const auto make_button = [&](const QIcon &icon, const QString &tip) {
		auto *button = new QToolButton(r.row);
		button->setIcon(icon);
		button->setToolTip(tip);
		button->setAutoRaise(true);
		layout->addWidget(button);
		return button;
	};
	r.up = make_button(QET::Icons::GoUp, tr("Monter cette variable"));
	r.down = make_button(QET::Icons::GoDown, tr("Descendre cette variable"));
	r.remove = make_button(QET::Icons::EditDelete, tr("Supprimer cette variable"));
	QWidget *row = r.row;
	connect(r.up, &QToolButton::clicked, this, [this, row]() { movePartRow(row, -1); });
	connect(r.down, &QToolButton::clicked, this, [this, row]() { movePartRow(row, +1); });
	connect(r.remove, &QToolButton::clicked, this, [this, row]() { removePartRow(row); });

	m_rows << r;
	num_part_list_ << part;
		//Before the add button, which stays under the last part
	ui->editor_layout->insertWidget(ui->editor_layout->count() - 1, r.row);
	updatePartButtons();
}

/// Take away every part of the definition shown
void SelectAutonumW::clearPartRows()
{
	for (const PartRow &r : std::as_const(m_rows)) {
		delete r.row;   //with the part, which it holds
	}
	m_rows.clear();
	num_part_list_.clear();
}

/**
	@brief SelectAutonumW::updatePartButtons
	The first part cannot move up, the last cannot move down, and the only
	one cannot be removed: those buttons are greyed out.
*/
void SelectAutonumW::updatePartButtons()
{
	for (int i = 0 ; i < m_rows.size() ; ++i) {
		m_rows.at(i).up->setEnabled(i > 0);
		m_rows.at(i).down->setEnabled(i < m_rows.size() - 1);
		m_rows.at(i).remove->setEnabled(m_rows.size() > 1);
	}
}

/// Remove the part shown in @p row; the definition keeps at least one
void SelectAutonumW::removePartRow(QWidget *row)
{
	if (m_rows.size() <= 1) {
		return;
	}
	for (int i = 0 ; i < m_rows.size() ; ++i) {
		if (m_rows.at(i).row == row) {
			const PartRow r = m_rows.takeAt(i);
			num_part_list_.removeAll(r.part);
			delete r.row;   //with the part, which it holds
			break;
		}
	}
	updatePartButtons();
	applyEnable();
}

/// Move the part shown in @p row up (-1) or down (+1)
void SelectAutonumW::movePartRow(QWidget *row, int step)
{
	for (int i = 0 ; i < m_rows.size() ; ++i) {
		if (m_rows.at(i).row != row) {
			continue;
		}
		const int to = i + step;
		if (to < 0 || to >= m_rows.size()) {
			return;
		}
		m_rows.swapItemsAt(i, to);
		num_part_list_.swapItemsAt(i, to);
		ui->editor_layout->removeWidget(row);
		ui->editor_layout->insertWidget(1 + to, row);   //after the header of the columns
		break;
	}
	updatePartButtons();
	applyEnable();
}

/**
	@brief SelectAutonumW::toNumContext
	@return the content to num_part_list to NumerotationContext
*/
NumerotationContext SelectAutonumW::toNumContext() const
{
	NumerotationContext nc;
	foreach (NumPartEditorW *npew, num_part_list_)
		nc << npew -> toNumContext();
	return nc;
}

/**
	@brief SelectAutonumW::on_add_button_clicked
 *	Action on add_button, add a NumPartEditor
*/
void SelectAutonumW::on_add_button_clicked()
{
	insertPartRow(new NumPartEditorW(m_edited_type, this));
	m_dirty = true;
	ui -> remove_button -> setEnabled(true);
	applyEnable();
	emit contextEdited();
}

/**
	@brief SelectAutonumW::on_remove_button_clicked
 *	Action on remove button, remove the last NumPartEditor
*/
void SelectAutonumW::on_remove_button_clicked()
{
	//remove if @num_part_list contains more than one item
	if (num_part_list_.size() > 1) {
		m_dirty = true;
		removePartRow(m_rows.last().row);
		if (num_part_list_.size() == 1) {
			ui -> remove_button -> setDisabled(true);
		}
	}
	applyEnable();
	emit contextEdited();
}

/**
	@brief SelectAutonumW::formula
	@return autonumbering widget formula
*/
QString SelectAutonumW::formula()
{
	if (m_edited_type == 0)
		return m_feaw->formula();
	else if (m_edited_type == 1)
		return m_fcaw->formula();
	else
		return "";
}

QComboBox *SelectAutonumW::contextComboBox() const
{
	return ui->m_comboBox;
}

/**
	@brief SelectAutonumW::setSingleRuleMode
	@param single true to show this editor as the one and only rule of
	its kind.

	The row which lists the available numberings -- its label, its combo
	box and the button which deletes one -- is hidden, so there is
	nothing to name a rule with and nothing to choose between: the rule
	being edited is all there is. The combo box itself stays where it is
	and keeps holding the rule's name, so everything which reads or
	writes the rule through contextComboBox() carries on untouched; he
	just cannot see it.
*/
void SelectAutonumW::setSingleRuleMode(bool single)
{
	m_single_rule = single;
	ui->label->setVisible(!single);
	ui->m_comboBox->setVisible(!single);
	ui->m_remove_pb->setVisible(!single);

		//A single rule has no list to be deleted from, so it needs a
		//button of its own to get out of the numbering again. It is
		//only shown when there is really a rule to take away -- see
		//setRuleRemovable.
	if (single && !m_rule_remove_pb) {
		m_rule_remove_pb = new QPushButton(tr("Supprimer la règle"), this);
		m_rule_remove_pb->setToolTip(tr(
			"Supprimer la règle de numérotation des câbles : les câbles "
			"sont à nouveau numérotés comme avant, et la question d'en "
			"définir une revient au prochain dessin."));
		m_rule_remove_pb->setObjectName(QStringLiteral("m_rule_remove_pb"));
		if (ui->m_definition_groupe && ui->m_definition_groupe->layout()) {
			ui->m_definition_groupe->layout()->addWidget(m_rule_remove_pb);
		}
		connect(m_rule_remove_pb, &QPushButton::clicked,
				this, &SelectAutonumW::removeClicked);
	}
	if (m_rule_remove_pb) {
		m_rule_remove_pb->setVisible(m_single_rule && m_rule_removable);
	}
}

/**
	@brief SelectAutonumW::setRuleRemovable
	Whether the button which takes the whole rule away is worth showing:
	it has nothing to do while this editor holds no rule at all, and it
	would only be a button which does nothing.
	@param can_remove true when a rule is there to be taken away
*/
void SelectAutonumW::setRuleRemovable(bool can_remove)
{
	m_rule_removable = can_remove;
	if (m_rule_remove_pb) {
		m_rule_remove_pb->setVisible(m_single_rule && can_remove);
	}
}

/**
	@brief SelectAutonumW::isValid
	@return true when there is at least one variable in the editor and
	every one of them holds what it needs. An editor which has only just
	been opened holds an empty rule, and an empty rule is not one.
*/
bool SelectAutonumW::isValid()
{
	if (num_part_list_.isEmpty()) return false;
	for (NumPartEditorW *npe : num_part_list_) {
		if (!npe || !npe->isValid()) return false;
	}
	return true;
}

/**
	@brief SelectAutonumW::on_buttonBox_clicked
	Action on button clicked
	@param button
*/
void SelectAutonumW::on_buttonBox_clicked(QAbstractButton *button)
{
	//transform button to int
	int answer = ui -> buttonBox -> buttonRole(button);
	switch (answer) {
			//Reset the current context
		case QDialogButtonBox::ResetRole:
			setContext(m_context);
			break;
			//help dialog
		case QDialogButtonBox::HelpRole:
			if (m_edited_type == 2)
			{
				QMessageBox::information (
							this,
							tr("Folio Autonumérotation",
							   "title window"),
							tr("C'est ici que vous pouvez définir la manière dont seront numérotés les nouveaux folios.\n"
							   "-Une numérotation est composée d'une variable minimum.\n"
							   "-Vous pouvez ajouter ou supprimer une variable de numérotation par le biais des boutons - et +.\n"
							   "-Une variable de numérotation comprend : un type, une valeur et une incrémentation.\n"

							   "\n-les types \"Chiffre 1\", \"Chiffre 01\" et \"Chiffre 001\", représentent un type numérique défini dans le champ \"Valeur\", "
							   "qui s'incrémente à chaque nouveau folio de la valeur du champ \"Incrémentation\".\n"
							   "-\"Chiffre 01\" et \"Chiffre 001\", sont respectivement représentés sur le schéma par deux et trois digits minimum.\n"
							   "Si le chiffre défini dans le champ Valeur possède moins de digits que le type choisi,"
							   "celui-ci sera précédé par un ou deux 0 afin de respecter son type.\n"

							   "\n-Le type \"Texte\", représente un texte fixe.\nLe champ \"Incrémentation\" n'est pas utilisé.\n",
							   "help dialog about the folio autonumerotation"
							   ));
				break;
			}
			else if (m_edited_type == 1)
			{
				QMessageBox::information (
							this,
							tr("Conducteur Autonumérotation",
							   "title window"),
							tr("C'est ici que vous pouvez définir la manière dont seront numérotés les nouveaux conducteurs.\n"
							   "-Une numérotation est composée d'une variable minimum.\n"
							   "-Vous pouvez ajouter ou supprimer une variable de numérotation par le biais des boutons - et +.\n"
							   "-Une variable de numérotation comprend : un type, une valeur et une incrémentation.\n"

							   "\n-les types \"Chiffre 1\", \"Chiffre 01\" et \"Chiffre 001\", représentent un type numérique défini dans le champ \"Valeur\", "
							   "qui s'incrémente à chaque nouveau conducteur de la valeur du champ \"Incrémentation\".\n"
							   "-\"Chiffre 01\" et \"Chiffre 001\", sont respectivement représentés sur le schéma par deux et trois digits minimum.\n"
							   "Si le chiffre défini dans le champ Valeur possède moins de digits que le type choisi,"
							   "celui-ci sera précédé par un ou deux 0 afin de respecter son type.\n"

							   "\n-Le type \"Texte\", représente un texte fixe.\nLe champ \"Incrémentation\" n'est pas utilisé.\n"

							   "\n-Le type \"N° folio\" représente le n° du folio en cours.\nLes autres champs ne sont pas utilisés.\n"

							   "\n-Le type \"Folio\" représente le nom du folio en cours.\nLes autres champs ne sont pas utilisés.",
							   "help dialog about the conductor autonumerotation"
							   ));
				break;
			}
			else if (m_edited_type == 3)
			{
				QMessageBox::information (
							this,
							tr("Câble Autonumérotation",
							   "title window"),
							tr("C'est ici que vous pouvez définir la manière dont seront numérotés les nouveaux câbles.\n"
							   "-Une numérotation est composée d'une variable minimum.\n"
							   "-Vous pouvez ajouter ou supprimer une variable de numérotation par le biais des boutons - et +.\n"
							   "-Une variable de numérotation comprend : un type, une valeur et une incrémentation.\n"

							   "\n-les types \"Chiffre 1\", \"Chiffre 01\" et \"Chiffre 001\", représentent un type numérique défini dans le champ \"Valeur\", "
							   "qui s'incrémente à chaque nouveau câble de la valeur du champ \"Incrémentation\".\n"
							   "-\"Chiffre 01\" et \"Chiffre 001\", sont respectivement représentés sur le schéma par deux et trois digits minimum.\n"
							   "Si le chiffre défini dans le champ Valeur possède moins de digits que le type choisi,"
							   "celui-ci sera précédé par un ou deux 0 afin de respecter son type.\n"

							   "\n-Le type \"Texte\", représente un texte fixe.\nLe champ \"Incrémentation\" n'est pas utilisé.\n"

							   "\n-Le type \"N° folio\" représente le n° du folio en cours.\nLes autres champs ne sont pas utilisés.\n"

							   "\n-Le type \"Folio\" représente le nom du folio en cours.\nLes autres champs ne sont pas utilisés.",
							   "help dialog about the cable autonumerotation"
							   ));
				break;
			}
			else
			{
				QMessageBox::information (
							this,
							tr("Element Autonumérotation",
							   "title window"),
							tr("C'est ici que vous pouvez définir la manière dont seront numérotés les nouveaux elements.\n"
							   "-Une numérotation est composée d'une variable minimum.\n"
							   "-Vous pouvez ajouter ou supprimer une variable de numérotation par le biais des boutons - et +.\n"
							   "-Une variable de numérotation comprend : un type, une valeur et une incrémentation.\n"

							   "\n-les types \"Chiffre 1\", \"Chiffre 01\" et \"Chiffre 001\", représentent un type numérique défini dans le champ \"Valeur\", "
							   "qui s'incrémente à chaque nouveau conducteur de la valeur du champ \"Incrémentation\".\n"
							   "-\"Chiffre 01\" et \"Chiffre 001\", sont respectivement représentés sur le schéma par deux et trois digits minimum.\n"
							   "Si le chiffre défini dans le champ Valeur possède moins de digits que le type choisi,"
							   "celui-ci sera précédé par un ou deux 0 afin de respecter son type.\n"

							   "\n-Le type \"Texte\", représente un texte fixe.\nLe champ \"Incrémentation\" n'est pas utilisé.\n"

							   "\n-Le type \"N° folio\" représente le n° du folio en cours.\nLes autres champs ne sont pas utilisés.\n"

							   "\n-Le type \"Folio\" représente le nom du folio en cours.\nLes autres champs ne sont pas utilisés.",
							   "help dialog about the element autonumerotation"
							   ));
				break;
			}
			//apply the context in the diagram displayed by @diagram_chooser.
		case QDialogButtonBox::ApplyRole:
			applyEnable(false);
			emit applyPressed();
				//Whatever was edited is written now, so there is
				//nothing of it left for a later save to write again.
			m_dirty = false;
			break;
	};
}

/**
	@brief SelectAutonumW::applyEnable
	enable/disable the apply button
*/
void SelectAutonumW::applyEnable(bool b)
{
		//Apply and Cancel both stand for a change: grey until there is one
	ui->buttonBox->button(QDialogButtonBox::Reset)->setEnabled(b);
	if (b){
		bool valid= true;
		foreach (NumPartEditorW *npe, num_part_list_)
			if (!npe -> isValid())
				valid= false;

		ui->buttonBox->button(QDialogButtonBox::Apply)
				->setEnabled(valid);
	}
	else {
		ui->buttonBox->button(QDialogButtonBox::Apply)
				->setEnabled(b);
	}
	if (m_edited_type == 0)
		contextToFormula();
	if (m_edited_type == 1)
		contextToFormula();
}

/**
	@brief SelectAutonumW::contextToFormula
	Apply formula to ElementAutonumbering Widget
*/
void SelectAutonumW::contextToFormula()
{
	FormulaAutonumberingW* m_faw = nullptr;
	if (m_edited_type == 0)
		m_faw = m_feaw;
	else if (m_edited_type == 1)
		m_faw = m_fcaw;

	if (m_faw)
	{
		m_faw->clearContext();
		m_faw->setContext(autonum::numerotationContextToFormula(
					  toNumContext()));
	}
}

void SelectAutonumW::on_m_comboBox_currentTextChanged(const QString &arg1)
{
	Q_UNUSED(arg1);
	applyEnable(true);
}

void SelectAutonumW::on_m_remove_pb_clicked()
{
	emit removeClicked();
}

/**
	@brief SelectAutonumW::setExplicitNaming
	Make the list of numberings a plain choice: no typing a name into it
	(an easy way to create a numbering by accident, or one with an empty
	name), but a button to create a numbering and one to rename the shown
	one, which emit newClicked() and renameClicked() for the owner to ask
	for the name.
*/
void SelectAutonumW::setExplicitNaming()
{
	ui->m_comboBox->setEditable(false);
	auto *row = ui->horizontalLayout_2;
	const int index = row->indexOf(ui->m_remove_pb);

	auto *new_pb = new QPushButton(QET::Icons::Add, QString(), this);
	new_pb->setToolTip(tr("Nouvelle numérotation…"));
	connect(new_pb, &QPushButton::clicked, this, &SelectAutonumW::newClicked);

	auto *rename_pb = new QPushButton(QET::Icons::EditRename, QString(), this);
	rename_pb->setToolTip(tr("Renommer la numérotation…"));
	connect(rename_pb, &QPushButton::clicked, this, &SelectAutonumW::renameClicked);

	row->insertWidget(index, rename_pb);
	row->insertWidget(index, new_pb);
}
