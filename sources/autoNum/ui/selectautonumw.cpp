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
#include "../numerotationcontextcommands.h"
#include "formulaautonumberingw.h"
#include "numparteditorw.h"
#include "ui_formulaautonumberingw.h"
#include "ui_selectautonumw.h"

#include <QMessageBox>
#include <QPushButton>

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

	qDeleteAll(num_part_list_);
	num_part_list_.clear();

	if (m_context.size() == 0) { //@context contain nothing, build a default numPartEditor
		on_add_button_clicked();
	}
	else {
		for (int i=0; i<m_context.size(); ++i) { //build with the content of @context
			NumPartEditorW *part= new NumPartEditorW(m_context, i, m_edited_type, this);
			connect(part, &NumPartEditorW::changed, this, [this]() {
				m_dirty = true;
				applyEnable();
				emit contextEdited();
			});
			num_part_list_ << part;
			ui -> editor_layout -> addWidget(part);
		}
	}

	num_part_list_.size() == 1 ?
				ui -> remove_button -> setDisabled(true):
				ui -> remove_button -> setEnabled (true);

	applyEnable(false);
		//Everything the rule held before is read now, so nothing of it
		//is left to write: what is on show is what there is.
	m_dirty = false;
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
	applyEnable(false);
	m_dirty = true;
	NumPartEditorW *part = new NumPartEditorW(m_edited_type, this);
	connect(part, &NumPartEditorW::changed, this, [this]() {
		m_dirty = true;
		applyEnable();
		emit contextEdited();
	});
	num_part_list_ << part;
	ui -> editor_layout -> addWidget(part);
	ui -> remove_button -> setEnabled(true);
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
		NumPartEditorW *part = num_part_list_.takeLast();
		// deliberately not disconnecting as not possible to resolve with lambda and will happen automatically when the object "part" is destroyed.
		delete part;
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

/**
	@brief SelectAutonumW::on_m_next_pb_clicked
	Increase NumerotationContext
*/
void SelectAutonumW::on_m_next_pb_clicked()
{
	NumerotationContextCommands ncc (toNumContext());
	setContext(ncc.next());
	applyEnable(true);
}

/**
	@brief SelectAutonumW::on_m_previous_pb_clicked
	Decrease NumerotationContext
*/
void SelectAutonumW::on_m_previous_pb_clicked()
{
	NumerotationContextCommands ncc (toNumContext());
	setContext(ncc.previous());
	applyEnable(true);
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
