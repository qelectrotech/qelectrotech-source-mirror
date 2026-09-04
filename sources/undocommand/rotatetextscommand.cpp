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
#include "rotatetextscommand.h"

#include "../diagram.h"
#include "../diagramcontent.h"
#include "../qetapp.h"
#include "../qetgraphicsitem/conductortextitem.h"
#include "../qetgraphicsitem/diagramtextitem.h"
#include "../qetgraphicsitem/elementtextitemgroup.h"
#include "../qtextorientationspinboxwidget.h"

/**
	@brief RotateTextsCommand::hasSelectedTexts
	@param diagram
	@return true if @p diagram has at least one selected text or text group.
	Lets a caller decide whether to ask the user for an angle at all, keeping
	the previous behaviour where no dialog appeared for an empty selection.
*/
bool RotateTextsCommand::hasSelectedTexts(Diagram *diagram)
{
	if(!diagram)
		return false;

	DiagramContent dc(diagram);
	return (!dc.selectedTexts().isEmpty() || !dc.selectedTextsGroup().isEmpty());
}

/**
	@brief RotateTextsCommand::RotateTextsCommand
	@param diagram : Apply the rotation to the selected texts and group of texts
	of diagram at construction time.
	@param rotation : the angle to apply, in degrees. Obtain it from
	askRotation() for interactive use; pass it directly from a test or script.
	@param parent : undo parent
*/
RotateTextsCommand::RotateTextsCommand(Diagram *diagram, qreal rotation, QUndoCommand *parent) :
QUndoCommand(parent),
m_diagram(diagram),
m_rotation(rotation)
{
	DiagramContent dc(m_diagram);
	QList <DiagramTextItem *> texts_list;
	QList <ElementTextItemGroup *> groups_list;
	
	for(DiagramTextItem *dti : dc.selectedTexts())
	{
		texts_list << dti;
		if(dti->type() == ConductorTextItem::Type)
		{
			ConductorTextItem *cti = static_cast<ConductorTextItem *>(dti);
			m_cond_texts.insert(cti, cti->wasRotatedByUser());
		}
	}
	for(ElementTextItemGroup *etig : dc.selectedTextsGroup())
		groups_list << etig;
	
	if(texts_list.count() || groups_list.count())
	{
		QString text;
		if(texts_list.count())
			text.append(QObject::tr("Pivoter %1 textes").arg(texts_list.count()));
		if(groups_list.count())
		{
			if(text.isEmpty())
				text.append(QObject::tr("Pivoter"));
			else
				text.append(QObject::tr(" et"));
			
			text.append(QObject::tr(" %1 groupes de textes").arg(groups_list.count()));
		}
		if(!text.isNull())
			setText(text);
		
		for(DiagramTextItem *dti : texts_list)
			setupAnimation(dti, "rotation", dti->rotation(), m_rotation);
		for(ElementTextItemGroup *grp : groups_list)
			setupAnimation(grp, "rotation", grp->rotation(), m_rotation);
	}
	else
		setObsolete(true);
	
}

void RotateTextsCommand::undo()
{
	if(m_diagram)
		m_diagram.data()->showMe();
	
		//Nothing was selected at construction time: there is no animation to
		//run. QUndoStack::push() calls redo() before it discards an obsolete
		//command, so this has to be survivable rather than assumed away.
	if(!m_anim_group)
		return;

	m_anim_group->setDirection(QAnimationGroup::Backward);
	m_anim_group->start();

	for(ConductorTextItem *cti : m_cond_texts.keys())
		cti->forceRotateByUser(m_cond_texts.value(cti));
}

void RotateTextsCommand::redo()
{
	if(m_diagram)
		m_diagram.data()->showMe();
	
	if(!m_anim_group)
		return;

	m_anim_group->setDirection(QAnimationGroup::Forward);
	m_anim_group->start();

	for(ConductorTextItem *cti : m_cond_texts.keys())
		cti->forceRotateByUser(true);
}

/**
	@brief RotateTextsCommand::askRotation
	Ask the user for an orientation.
	@param rotation : set to the chosen angle when the dialog is accepted,
	left untouched otherwise.
	@return true if the user accepted, false if they cancelled.

	Deliberately static and separate from the command: a QUndoCommand that
	blocks on a modal in its constructor cannot be built by a test, a script,
	or any headless caller.
*/
bool RotateTextsCommand::askRotation(qreal &rotation)
{
		//Open the dialog
	QDialog ori_text_dialog;
	ori_text_dialog.setSizeGripEnabled(false);
#ifdef Q_OS_MACOS
	ori_text_dialog.setWindowFlags(Qt::Sheet);
#endif
	ori_text_dialog.setWindowTitle(QObject::tr("Orienter les textes sélectionnés", "window title"));
	
	
	QTextOrientationSpinBoxWidget *ori_widget = QETApp::createTextOrientationSpinBoxWidget();
	ori_widget->setParent(&ori_text_dialog);
	ori_widget->spinBox()->selectAll();
	
	QDialogButtonBox buttons(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
	QObject::connect(&buttons, &QDialogButtonBox::accepted, &ori_text_dialog, &QDialog::accept);
	QObject::connect(&buttons, &QDialogButtonBox::rejected, &ori_text_dialog, &QDialog::reject);
	
	QVBoxLayout layout_v(&ori_text_dialog);
	layout_v.setSizeConstraint(QLayout::SetFixedSize);
	layout_v.addWidget(ori_widget);
	layout_v.addStretch();
	layout_v.addWidget(&buttons);
	
	if (ori_text_dialog.exec() != QDialog::Accepted)
		return false;

	rotation = ori_widget->orientation();
	return true;
}

void RotateTextsCommand::setupAnimation(QObject *target, const QByteArray &propertyName, const QVariant& start, const QVariant& end)
{
	if(m_anim_group == nullptr)
		m_anim_group = new QParallelAnimationGroup();
	
	QPropertyAnimation *animation = new QPropertyAnimation(target, propertyName);
	animation->setDuration(300);
	animation->setStartValue(start);
	animation->setEndValue(end);
	animation->setEasingCurve(QEasingCurve::OutQuad);
	m_anim_group->addAnimation(animation);
}
