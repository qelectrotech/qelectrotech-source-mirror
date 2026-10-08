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
#include "cablenumberingdialog.h"

#include "../autoNum/ui/selectautonumw.h"
#include "../cable/cablerenumber.h"
#include "../cable/editcablecommand.h"
#include "../qetproject.h"

#include <QAbstractButton>
#include <QDialogButtonBox>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QRadioButton>
#include <QSettings>
#include <QUndoStack>
#include <QVBoxLayout>

/**
	@brief CableNumberingDialog::CableNumberingDialog
	Build the window around the numbering rule of the project: the rule
	itself, the axis the cables are laid out along when the whole
	project is numbered again, and the button which does that numbering.
	@param project the project whose cables are numbered
	@param parent
*/
CableNumberingDialog::CableNumberingDialog(QETProject *project, QWidget *parent) :
	QDialog(parent),
	m_project(project)
{
	setWindowTitle(tr("Numérotation des câbles"));
		//The rule editor needs room to be read: this window opens with
		//the size it needs rather than with whatever its contents
		//happen to add up to, so nothing of the rule is cramped from
		//the first moment
	setMinimumSize(QSize(900, 620));

	auto *layout = new QVBoxLayout(this);

	auto *note = new QLabel(tr(
		"Cette fenêtre montre la règle de numérotation automatique des "
		"câbles de ce projet et la modifie. Une règle changée ici est "
		"celle que le projet garde : elle est écrite dans le projet quand "
		"vous validez."), this);
	note->setWordWrap(true);
	layout->addWidget(note);

	m_rule = new SelectAutonumW(3, this);
		//One rule only, so the row which picks between several named
		//numberings is hidden: what is on show is the project's rule
	m_rule->setSingleRuleMode(true);
	layout->addWidget(m_rule);

	auto *axis_group = new QGroupBox(tr("Priorité des axes"), this);
	auto *axis_layout = new QHBoxLayout(axis_group);
	m_axis_x = new QRadioButton(tr("Priorité à l'axe X (horizontal)"), axis_group);
	m_axis_y = new QRadioButton(tr("Priorité à l'axe Y (vertical)"), axis_group);
	axis_layout->addWidget(m_axis_x);
	axis_layout->addWidget(m_axis_y);
	layout->addWidget(axis_group);

	auto *axis_note = new QLabel(tr(
		"L'axe choisi décide l'ordre dans lequel les câbles sont "
		"renumérotés : X les numérote de gauche à droite, Y les numérote "
		"de haut en bas."), this);
	axis_note->setWordWrap(true);
	layout->addWidget(axis_note);

	auto *actions = new QHBoxLayout();
	m_renumber_pb = new QPushButton(tr("Renumérotter tous les câbles"), this);
	m_renumber_pb->setToolTip(tr(
		"Remet le numéro de chaque câble du projet d'après la règle, en "
		"partant du début : un numéro manquant après la suppression d'un "
		"câble se referme ainsi."));
	actions->addWidget(m_renumber_pb);
	actions->addStretch();

		//No confirm button: this window is there to number the cables
		//again and to show the rule which does it. What the window
		//holds is written by the editor's own Apply button, and by the
		//numbering button which writes it before numbering -- so a
		//second button only saving and closing would say the same
		//thing twice.
	m_box = new QDialogButtonBox(this);
	m_box->addButton(QDialogButtonBox::Cancel);
	m_box->button(QDialogButtonBox::Cancel)->setText(tr("Annuler"));
	actions->addWidget(m_box);
	layout->addLayout(actions);

	connect(m_box, &QDialogButtonBox::rejected, this, &QDialog::reject);
	connect(m_renumber_pb, &QPushButton::clicked, this, &CableNumberingDialog::renumber);
		//The editor's own buttons are wired the same way the project
		//properties wire them, so this window saves exactly what that
		//page would have saved
	connect(m_rule, &SelectAutonumW::applyPressed, this, &CableNumberingDialog::applyRule);
	connect(m_rule, &SelectAutonumW::removeClicked, this, &CableNumberingDialog::removeRule);
		//Filling a rule in lights the numbering button up as soon as
		//there is something to number with
	connect(m_rule, &SelectAutonumW::contextEdited, this,
			&CableNumberingDialog::updateEnabling);

	if (m_project)
	{
		const QString key = m_project->cableCurrentAutoNum();
		const bool has_rule = !key.isEmpty()
				&& !m_project->cableAutoNum(key).isEmpty();
		m_rule->setContext(has_rule ? m_project->cableAutoNum(key)
									: NumerotationContext());
			//The button which takes the rule away is only worth showing
			//when there is one, and only while the project may be
			//written to
		m_rule->setRuleRemovable(has_rule && !m_project->isReadOnly());

		m_axis_x->setChecked(m_project->cableXAxisFirst());
		m_axis_y->setChecked(!m_project->cableXAxisFirst());

		if (m_project->isReadOnly())
		{
			m_rule->setEnabled(false);
			m_axis_x->setEnabled(false);
			m_axis_y->setEnabled(false);
		}
	}
	updateEnabling();
}

/**
	@brief CableNumberingDialog::applyRule
	What the editor's own Apply button does: write the rule into the
	project and leave the window open, the way that button behaves on
	the numbering pages of the settings.
*/
void CableNumberingDialog::applyRule()
{
	save();
	updateEnabling();
}

/**
	@brief CableNumberingDialog::save
	Write the rule and the axis into the project. Nothing is written
	when nothing was changed, so confirming a window which was only
	ever looked at leaves the project exactly as it was.
	@return true when the window holds nothing which could not be written
*/
bool CableNumberingDialog::save()
{
	if (!m_project || m_project->isReadOnly()) return false;

	bool changed = false;

	if (m_rule->isModified())
	{
		if (!m_rule->isValid())
		{
			QMessageBox::warning(this, tr("Numérotation des câbles"),
								 tr("La règle n'est pas encore complète : "
									"il reste un champ vide dedans."));
			return false;
		}

		const QString title = QETProject::cableAutoNumRuleName();
		m_project->addCableAutoNum(title, m_rule->toNumContext());
		m_project->setCurrentCableAutoNum(title);
			//What was edited is written now, so there is nothing of it
			//left for a second save to write again
		m_rule->setContext(m_rule->toNumContext());
		m_rule->setRuleRemovable(true);
		changed = true;
	}

	const bool axis_first = m_axis_x->isChecked();
	if (m_project->cableXAxisFirst() != axis_first) changed = true;
	m_project->setCableXAxisFirst(axis_first);

	if (changed) m_project->setModified(true);
	return true;
}

/**
	@brief CableNumberingDialog::removeRule
	Take the cable numbering rule out of the project. The project then
	numbers its cables by no rule at all -- they are called W until one
	is defined again -- and the cable tool asks once more whether one
	should be defined.
*/
void CableNumberingDialog::removeRule()
{
	if (!m_project || m_project->isReadOnly()) return;

	const QString title = m_project->cableCurrentAutoNum();
	if (title.isEmpty()) return;

	m_project->removeCableAutoNum(title);
	m_rule->setContext(NumerotationContext());
		//No rule left, so the button which takes one away goes too. And
		//having just taken the rule away means numbering cables by hand
		//again: the question of defining one comes back the next time a
		//cable is drawn, so it is not held back any more.
	m_rule->setRuleRemovable(false);
	QSettings settings;
	settings.setValue(QStringLiteral("cable-management/ask_numbering_rule"),
					  true);
	m_project->setModified(true);
	updateEnabling();
}

/**
	@brief CableNumberingDialog::updateEnabling
	Renumbering needs something to number with: a rule filled in well
	enough to be one, and a project which may be written to.
*/
void CableNumberingDialog::updateEnabling()
{
	if (!m_project || m_project->isReadOnly() || !m_rule) {
		m_renumber_pb->setEnabled(false);
		return;
	}

	const QString key = m_project->cableCurrentAutoNum();
	const bool saved_rule = !key.isEmpty()
			&& !m_project->cableAutoNum(key).isEmpty();
		//A rule which is still being filled in counts as soon as it is
		//written, and the numbering writes it together with the numbers
		//it hands out
	const bool drawn_rule = m_rule->isModified() && m_rule->isValid()
			&& !m_rule->toNumContext().isEmpty();

	m_renumber_pb->setEnabled((saved_rule || drawn_rule)
							  && m_rule->isValid()
							  && !m_rule->toNumContext().isEmpty());
}

/**
	@brief CableNumberingDialog::askAboutHandWritten
	Some cables carry a name he typed in himself rather than one the
	rule handed out. Taking such a name away is his to allow: yes and
	they are numbered like all the others, no and they keep their name
	while the rest is numbered around them, and cancelling leaves every
	cable exactly where it stands.
	@param count how many such names the project holds
	@return 1 to number them too, 0 to leave them alone, -1 for nothing
*/
int CableNumberingDialog::askAboutHandWritten(int count)
{
	if (count <= 0) return 1;

	QMessageBox box(QMessageBox::Question, tr("Numérotation des câbles"),
					tr("%n nom(s) de câble(s) de ce projet ont été saisis à "
					   "la main.\n\n"
					   "Les renuméroter aussi ?", "", count),
					QMessageBox::NoButton, this);
	QPushButton *take = box.addButton(
		tr("Oui", "Les noms saisis à la main sont remplacés eux aussi."),
		QMessageBox::YesRole);
	QPushButton *leave = box.addButton(
		tr("Non", "Les noms saisis à la main sont laissés tels quels."),
		QMessageBox::NoRole);
	QPushButton *cancel = box.addButton(
		tr("Annuler", "Ne renumérote rien du tout."),
		QMessageBox::RejectRole);
		//Keeping his own names is the safe answer under his finger
	box.setDefaultButton(leave);
	box.setEscapeButton(cancel);
	box.exec();

	const QAbstractButton *clicked = box.clickedButton();
	if (clicked == take) return 1;
	if (clicked == leave) return 0;
	return -1;
}

/**
	@brief CableNumberingDialog::renumber
	Number every cable of the project again with the rule on show,
	from the beginning of that rule and in the order the cables stand
	along the chosen axis. One single undo step takes it all back, and
	the window closes once the numbering has been written so that he
	can see what became of his cables.
*/
void CableNumberingDialog::renumber()
{
	if (!m_project || m_project->isReadOnly()) return;

		//A rule which is not complete yet is refused before anything is
		//worked out at all. What the window holds is written only together
		//with the numbering itself, so every way out coming before that
		//point -- a rule which does not fit, him saying no, nothing to
		//number -- leaves the project exactly as it stands.
	const NumerotationContext rule = m_rule->toNumContext();
	if (!m_rule->isValid() || rule.isEmpty())
	{
		QMessageBox::warning(this, tr("Numérotation des câbles"),
							 tr("La règle n'est pas encore complète."));
		return;
	}
	const bool axis_first = m_axis_x->isChecked();

		//Whether he lets his own names be taken away or not, a project
		//with nothing to number with says so before he is asked
		//anything: the rule has to be one first
	CableRenumberPlan plan = CableRenumber::plan(m_project, rule, axis_first,
												 true);
	if (!plan.ok())
	{
		QMessageBox::warning(this, tr("Numérotation des câbles"), plan.error);
		return;
	}

	const int answer = askAboutHandWritten(CableRenumber::byHandCount(m_project));
	if (answer < 0) return;

		//Leaving his own names alone means the numbering runs around
		//them, which can hand out a number somebody else keeps: that
		//is worked out again here and refused if it does not fit
	if (answer == 0)
	{
		plan = CableRenumber::plan(m_project, rule, axis_first, false);
		if (!plan.ok())
		{
			QMessageBox::warning(this, tr("Numérotation des câbles"), plan.error);
			return;
		}
	}
	if (plan.cables.isEmpty())
	{
		QMessageBox::information(this, tr("Numérotation des câbles"),
								 tr("Aucun câble à renuméroter dans ce projet."));
		return;
	}

		//Every cable already carries the number the rule says: there is
		//nothing to write, and an undo step which undoes nothing would
		//only be in the way
	bool changed = false;
	for (int i = 0; i < plan.cables.size(); ++i)
	{
		if (plan.cables.at(i)->designation() != plan.designations.at(i)
			|| plan.cables.at(i)->designationByHand()) {
			changed = true;
			break;
		}
	}
	if (!changed)
	{
		QMessageBox::information(this, tr("Numérotation des câbles"),
								 tr("Tous les câbles portent déjà le numéro "
									"que la règle leur donne."));
		return;
	}

		//The rule on show is written now, right with the numbering it
		//belongs to, and the numbering then leaves the counters of that
		//rule where the last number was taken from: both go into the
		//project with the names, as one single act which undo takes
		//back together. Nothing was written before this point, which is
		//why he may still back out of here without the project having
		//been touched.
	if (!save()) return;

	const QString num_key = QETProject::cableAutoNumRuleName();
	if (QUndoStack *stack = m_project->undoStack()) {
		stack->push(new RenumberCablesCommand(
			plan.cables, plan.designations, m_project, num_key,
			m_project->cableAutoNum(num_key), plan.after));
	}
	accept();
}
