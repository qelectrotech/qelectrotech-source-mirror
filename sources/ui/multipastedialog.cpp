/*
	Copyright 2006-2018 The QElectroTech team
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
#include "multipastedialog.h"
#include "ui_multipastedialog.h"
#include "diagram.h"
#include "diagramcommands.h"
#include "element.h"
#include "conductorautonumerotation.h"
#include <QSettings>

MultiPasteDialog::MultiPasteDialog(Diagram *diagram, QWidget *parent) :
	QDialog(parent),
	ui(new Ui::MultiPasteDialog),
	m_diagram(diagram)
{
	ui->setupUi(this);
	
	connect(ui->m_x_sb, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, &MultiPasteDialog::updatePreview);
	connect(ui->m_y_sb, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, &MultiPasteDialog::updatePreview);
	connect(ui->m_copy_count, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, &MultiPasteDialog::updatePreview);
	
	QRectF br;
	for (QGraphicsItem *item : m_diagram->selectedItems())
		br = br.united(item->mapToScene(item->boundingRect()).boundingRect());
	m_origin = br.topLeft();
	
	m_document = m_diagram->toXml(false);
	updatePreview();
}

MultiPasteDialog::~MultiPasteDialog()
{
	if(m_accept == false)
	{
		for(QGraphicsItem *item : m_pasted_content.items())
		{
			if(item->scene() && item->scene() == m_diagram)
			{
				m_diagram->removeItem(item);
				delete item;
			}
		}
	}
	
	delete ui;
}

void MultiPasteDialog::updatePreview()
{
		//First of all we remove all precedent items added from the previous preview.
	for(QGraphicsItem *item : m_pasted_content.items())
	{
		if(item->scene() && item->scene() == m_diagram)
		{
			m_diagram->removeItem(item);
			delete item;
		}
	}
	m_pasted_content.clear();
	m_pasted_content_list.clear();
	
	QPointF offset(ui->m_x_sb->value(), ui->m_y_sb->value());
	QPointF pos = m_origin+offset;
	
	for(int i=0 ; i<ui->m_copy_count->value() ; i++)
	{		
		DiagramContent dc;
		m_diagram->fromXml(m_document, pos, false, &dc);
		
		m_pasted_content += dc;
		m_pasted_content_list << dc;
		pos += offset;
	}
	
	if(m_pasted_content.count())
		m_diagram->adjustSceneRect();
}

void MultiPasteDialog::on_m_button_box_accepted()
{
    if(m_pasted_content.count())
	{
		m_diagram->undoStack().beginMacro(tr("Multi-collage"));
		
		QSettings settings;
		bool erase_label = settings.value("diagramcommands/erase-label-on-copy", true).toBool();
			//Ensure when 'auto_num' is checked, the settings 'save_label' is to true.
			//Because in the class PasteDiagramCommand, if the settings 'save_label' is to false,
			//the function redo of PasteDiagramCommand, clear the formula and the label of the pasted element
			//and so the auto_num below do nothing (there is not a formula to compare)
		if(ui->m_auto_num_cb->isChecked())
			settings.setValue("diagramcommands/erase-label-on-copy", false);
		
		
		
		m_diagram->clearSelection();
		m_diagram->undoStack().push(new PasteDiagramCommand(m_diagram, m_pasted_content));
		
		for(DiagramContent dc : m_pasted_content_list)
		{
			QList<Element *> pasted_elements = dc.m_elements;
				//Sort the list element by there pos (top -> bottom)
			std::sort(pasted_elements.begin(), pasted_elements.end(), [](Element *a, Element *b){return (a->pos().y() < b->pos().y());});
				
				//Auto-connection
			if(ui->m_auto_connection_cb->isChecked())
			{
				for(Element *elmt : pasted_elements)
				{
					while (!elmt->AlignedFreeTerminals().isEmpty())
					{
						QPair <Terminal *, Terminal *> pair = elmt->AlignedFreeTerminals().takeFirst();
						
						Conductor *conductor = new Conductor(pair.first, pair.second);
						m_diagram->undoStack().push(new AddItemCommand<Conductor *>(conductor, m_diagram, QPointF()));
						
						//Autonum the new conductor, the undo command associated for this, have for parent undo_object
						ConductorAutoNumerotation can  (conductor, m_diagram);
						can.numerate();
						if (m_diagram->freezeNewConductors() || m_diagram->project()->isFreezeNewConductors()) {
							conductor->setFreezeLabel(true);
						}
					}
				}
			}
			
				//Set up the label of element
				//Instead of use the current autonum of project,
				//we try to fetch the same formula of the pasted element, in the several autonum of the project
				//for apply the good formula for each elements
			if(ui->m_auto_num_cb->isChecked())
			{
				for(Element *elmt : pasted_elements)
				{
					QString formula = elmt->elementInformations()["formula"].toString();
					if(!formula.isEmpty())
					{
						QHash <QString, NumerotationContext> autonums = m_diagram->project()->elementAutoNum();
						QHashIterator<QString, NumerotationContext> hash_iterator(autonums);
						
						while(hash_iterator.hasNext())
						{
							hash_iterator.next();
							if(autonum::numerotationContextToFormula(hash_iterator.value()) == formula)
							{
								m_diagram->project()->setCurrrentElementAutonum(hash_iterator.key());
								elmt->setUpFormula();
							}
						}
					}
				}
			}
				//Like elements, we compare formula of pasted conductor with the autonums available in the project.
			if(ui->m_auto_num_cond_cb->isChecked())
			{		
					//This list is to ensure we not numerate twice the same conductor
				QList<Conductor *> numerated;
					//Start with the element at top
				for(Element *elmt : pasted_elements)
				{
					for(Conductor *c : elmt->conductors())
					{
						if(numerated.contains(c))
							continue;
						else
							numerated << c;
						QString formula = c->properties().m_formula;
						if(!formula.isEmpty())
						{
							QHash <QString, NumerotationContext> autonums = m_diagram->project()->conductorAutoNum();
							QHashIterator <QString, NumerotationContext> hash_iterator(autonums);
							
							while (hash_iterator.hasNext())
							{
								hash_iterator.next();
								if(autonum::numerotationContextToFormula(hash_iterator.value()) == formula)
								{
									m_diagram->project()->setCurrentConductorAutoNum(hash_iterator.key());
									c->rSequenceNum().clear();
									ConductorAutoNumerotation can(c, m_diagram);
									can.numerate();
									if (m_diagram->freezeNewConductors() || m_diagram->project()->isFreezeNewConductors())
									{
										c->setFreezeLabel(true);
									}
								}
							}
						}
					}
				}
			}
		}
		
		m_diagram->adjustSceneRect();
		m_accept = true;
		settings.setValue("diagramcommands/erase-label-on-copy", erase_label);
		m_diagram->undoStack().endMacro();
	}
}
