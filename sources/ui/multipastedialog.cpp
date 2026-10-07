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

#include "multipastedialog.h"
#include "../autoNum/elementautonumschemecommand.h"
#include "../qetproject.h"
#include "../conductorautonumerotation.h"
#include "../diagram.h"
#include "../diagramcommands.h"
#include "../undocommand/addgraphicsobjectcommand.h"
#include "../qetgraphicsitem/element.h"
#include "../qetgraphicsitem/conductor.h"
#include "../ui_multipastedialog.h"

#include "../qet.h"
#include <QHash>
#include <QSettings>

MultiPasteDialog::MultiPasteDialog(Diagram *diagram, QWidget *parent) :
	QDialog(parent),
	ui(new Ui::MultiPasteDialog),
	m_diagram(diagram)
{
	ui->setupUi(this);
	QET::trackDialogGeometry(this);

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
		m_diagram->undoStack().beginMacro(tr("Multi-paste"));

		QETProject *project = m_diagram->project();

			//The element numberings the copies follow, read now: the paste
			//erases the formulas of its elements, unless the preference
			//keeps them, and the numbering below is meant to work either way
		QList<QMap<QString, QVector<Element *>>> copy_schemes;
		for(const DiagramContent &dc : std::as_const(m_pasted_content_list))
		{
			copy_schemes << (ui->m_auto_num_cb->isChecked()
							 ? ElementAutoNumSchemeCommand::pastedSchemes(project, dc.m_elements)
							 : QMap<QString, QVector<Element *>>());
		}

		m_diagram->clearSelection();
			//The copies are numbered by this dialog, copy by copy, below
		auto *paste = new PasteDiagramCommand(m_diagram, m_pasted_content);
		paste->setAutoNumbering(false);
		m_diagram->undoStack().push(paste);

		for(int copy = 0 ; copy < m_pasted_content_list.size() ; ++copy)
		{
			const DiagramContent &dc = m_pasted_content_list.at(copy);
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
						m_diagram->undoStack().push(new AddGraphicsObjectCommand(conductor, m_diagram, QPointF()));

						//Autonum the new conductor, the undo command associated for this, have for parent undo_object
						ConductorAutoNumerotation can  (conductor, m_diagram);
						can.numerate();
						if (m_diagram->freezeNewConductors() || m_diagram->project()->isFreezeNewConductors()) {
							conductor->setFreezeLabel(true);
						}
					}
				}
			}

				//Number the elements of this copy with the numberings they
				//follow: the next numbers, in the order of the Renumber
				//button, the counters moving on, so that the next copy goes
				//on from there. The project's current numbering is not
				//changed, and labels which other elements keep are not given.
			if(!copy_schemes.at(copy).isEmpty())
			{
				auto *numbering = new QUndoCommand(tr("Number pasted elements"));
				ElementAutoNumSchemeCommand::numberPasted(project, copy_schemes.at(copy), numbering);
				if(numbering->childCount())
					m_diagram->undoStack().push(numbering);
				else
					delete numbering;
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
		m_diagram->undoStack().endMacro();
	}
}
