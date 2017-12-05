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
#include "rotatetextscommand.h"
#include "diagramcontent.h"
#include "diagram.h"
#include "diagramtextitem.h"
#include "elementtextitemgroup.h"
#include "conductortextitem.h"
#include "qetapp.h"
#include "qtextorientationspinboxwidget.h"

/**
 * @brief RotateTextsCommand::RotateTextsCommand
 * @param diagram : Apply the rotation to the selected texts and group of texts
 * of diagram at construction time. 
 * @param parent : undo parent
 */
RotateTextsCommand::RotateTextsCommand(Diagram *diagram, QUndoCommand *parent) :
QUndoCommand(parent),
m_diagram(diagram)
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
			m_cond_texts.insert(cti, cti->wasRotateByUser());
		}
	}
	for(ElementTextItemGroup *etig : dc.selectedTextsGroup())
		groups_list << etig;
	
	if(texts_list.count() || groups_list.count())
	{
		openDialog();
		
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
#if QT_VERSION >= 0x050900
	else
		setObsolete(true);
#endif
	
}

void RotateTextsCommand::undo()
{
	if(m_diagram)
		m_diagram.data()->showMe();
	
	m_anim_group->setDirection(QAnimationGroup::Backward);
	m_anim_group->start();
	
	for(ConductorTextItem *cti : m_cond_texts.keys())
		cti->forceMovedByUser(m_cond_texts.value(cti));
}

void RotateTextsCommand::redo()
{
	if(m_diagram)
		m_diagram.data()->showMe();
	
	m_anim_group->setDirection(QAnimationGroup::Forward);
	m_anim_group->start();
	
	for(ConductorTextItem *cti : m_cond_texts.keys())
		cti->forceMovedByUser(true);
}

void RotateTextsCommand::openDialog()
{
		//Open the dialog
	QDialog ori_text_dialog;
	ori_text_dialog.setSizeGripEnabled(false);
#ifdef Q_OS_MAC
	ori_text_dialog.setWindowFlags(Qt::Sheet);
#endif
	ori_text_dialog.setWindowTitle(QObject::tr("Orienter les textes sélectionnés", "window title"));
	
	
	QTextOrientationSpinBoxWidget *ori_widget = QETApp::createTextOrientationSpinBoxWidget();
	ori_widget->setParent(&ori_text_dialog);
	ori_widget->spinBox()->selectAll();
	
	QDialogButtonBox buttons(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
	QObject::connect(&buttons, SIGNAL(accepted()), &ori_text_dialog, SLOT(accept()));
	QObject::connect(&buttons, SIGNAL(rejected()), &ori_text_dialog, SLOT(reject()));
	
	QVBoxLayout layout_v(&ori_text_dialog);
	layout_v.setSizeConstraint(QLayout::SetFixedSize);
	layout_v.addWidget(ori_widget);
	layout_v.addStretch();
	layout_v.addWidget(&buttons);
	
	if (ori_text_dialog.exec() == QDialog::Accepted)
		m_rotation = ori_widget->orientation();
#if QT_VERSION >= 0x050900
	else
		setObsolete(true);
#endif
}

void RotateTextsCommand::setupAnimation(QObject *target, const QByteArray &propertyName, const QVariant start, const QVariant end)
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
