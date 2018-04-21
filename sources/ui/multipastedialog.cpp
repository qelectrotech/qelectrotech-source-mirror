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
	
	QPointF offset(ui->m_x_sb->value(), ui->m_y_sb->value());
	QPointF pos = m_origin+offset;
	
	for(int i=0 ; i<ui->m_copy_count->value() ; i++)
	{		
		DiagramContent dc;
		m_diagram->fromXml(m_document, pos, false, &dc);
		
		m_pasted_content += dc;
		pos += offset;
	}
	
	if(m_pasted_content.count())
		m_diagram->adjustSceneRect();
}

void MultiPasteDialog::on_m_button_box_accepted()
{
    if(m_pasted_content.count())
	{
		QSettings settings;
		bool erase_label = settings.value("diagramcommands/erase-label-on-copy", true).toBool();
			//Ensure when 'auto_num' is checked, the settings 'save_label' is to true.
			//Because in the class PasteDiagramCommand, if the settings 'save_label' is to false,
			//the function redo of PasteDiagramCommand, clear the formula and the label of the pasted element
			//and so the auto_num below do nothing (there is not a formula to compare)
		if(ui->m_auto_num_cb->isChecked())
			settings.setValue("diagramcommands/erase-label-on-copy", false);
		
		
		
		m_diagram->clearSelection();
		
		QUndoCommand *undo = new QUndoCommand(tr("Multi-collage"));
		new PasteDiagramCommand(m_diagram, m_pasted_content, undo);
		
			//Auto-connection
		if(ui->m_auto_connection_cb->isChecked())
		{
			for(Element *elmt : m_pasted_content.m_elements)
			{
				while (!elmt->AlignedFreeTerminals().isEmpty())
				{
					QPair <Terminal *, Terminal *> pair = elmt->AlignedFreeTerminals().takeFirst();
			
					Conductor *conductor = new Conductor(pair.first, pair.second);
					new AddItemCommand<Conductor *>(conductor, m_diagram, QPointF(), undo);
			
						//Autonum the new conductor, the undo command associated for this, have for parent undo_object
					ConductorAutoNumerotation can  (conductor, m_diagram, undo);
					can.numerate();
					if (m_diagram->freezeNewConductors() || m_diagram->project()->isFreezeNewConductors()) {
						conductor->setFreezeLabel(true);
					}
				}
			}
		}
		
		m_diagram->undoStack().push(undo);
		
			//Set up the label of element
			//Instead of use the current autonum of project,
			//we try to fetch the same formula of the pasted element, in the several autonum of the project
			//for apply the good formula for each elements
		if(ui->m_auto_num_cb->isChecked())
		{
			for(Element *elmt : m_pasted_content.m_elements)
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
		m_diagram->adjustSceneRect();
		m_accept = true;
		settings.setValue("diagramcommands/erase-label-on-copy", erase_label);
	}
}
