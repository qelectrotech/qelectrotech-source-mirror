#include "multipastedialog.h"
#include "ui_multipastedialog.h"
#include "diagram.h"
#include "diagramcommands.h"

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
		m_diagram->clearSelection();
		m_diagram->undoStack().push(new PasteDiagramCommand(m_diagram, m_pasted_content));
		m_diagram->adjustSceneRect();
		m_accept = true;
	}
}
