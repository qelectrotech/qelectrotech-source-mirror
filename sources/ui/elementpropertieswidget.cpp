/*
	Copyright 2006-2015 The QElectroTech Team
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
#include "elementpropertieswidget.h"
#include "element.h"
#include "elementinfowidget.h"
#include "masterpropertieswidget.h"
#include "linksingleelementwidget.h"
#include "ghostelement.h"
#include "diagram.h"
#include "diagramposition.h"
#include "qeticons.h"

#include <QVBoxLayout>
#include <QLabel>
#include <QUndoStack>

/**
 * @brief ElementPropertiesWidget::ElementPropertiesWidget
 * Default constructor
 * @param elmt
 * @param parent
 */
ElementPropertiesWidget::ElementPropertiesWidget(Element *elmt, QWidget *parent) :
	PropertiesEditorWidget (parent),
	m_element (elmt),
	m_diagram (elmt->diagram()),
	m_tab (nullptr)
{
	buildGui();
}

/**
 * @brief ElementPropertiesWidget::apply
 * Apply the new properties by pushing an undo command
 * to the parent project's undo stack of element
 */
void ElementPropertiesWidget::apply()
{
	QList <QUndoCommand *> undo_list;

	foreach (PropertiesEditorWidget *pew, m_list_editor)
		if (QUndoCommand *undo = pew->associatedUndo())
			undo_list << undo;

	if (undo_list.isEmpty()) return;

	QString str;
	foreach(QUndoCommand *uc, undo_list) str += uc->text() += " ";

	QUndoStack &stack = m_element -> diagram() -> undoStack();
	stack.beginMacro(str);
	foreach(QUndoCommand *uc, undo_list) stack.push(uc);
	stack.endMacro();

	foreach(PropertiesEditorWidget *pew, m_list_editor)
		pew->updateUi();
}

/**
 * @brief ElementPropertiesWidget::reset
 * Reset the edited properties
 */
void ElementPropertiesWidget::reset() {
	foreach (PropertiesEditorWidget *pew, m_list_editor) pew->reset();
}

/**
 * @brief ElementPropertiesWidget::findInPanel
 * If m_element is a custom element, emit findElementRequired
 */
void ElementPropertiesWidget::findInPanel()
{
	if (CustomElement *custom_element = qobject_cast<CustomElement *>(m_element))
		emit findElementRequired(custom_element->location());
}

/**
 * @brief ElementPropertiesWidget::editElement
 * If m_element is a custom element, emit findElementRequired and editElementRequired
 */
void ElementPropertiesWidget::editElement()
{
	if (CustomElement *custom_element = qobject_cast<CustomElement *>(m_element))
	{
		emit findElementRequired(custom_element->location());
		emit editElementRequired(custom_element->location());
	}
}

/**
 * @brief ElementPropertiesWidget::build
 * Build the content of this widget
 */
void ElementPropertiesWidget::buildGui()
{
	m_tab = new QTabWidget(this);

		//Add tab according to the element
	switch (m_element -> linkType())
	{
		case Element::Simple:
			m_list_editor << new ElementInfoWidget(m_element, this);
			break;
		case Element::NextReport:
			m_list_editor << new LinkSingleElementWidget(m_element, this);
			break;
		case Element::PreviousReport:
			m_list_editor << new LinkSingleElementWidget(m_element, this);
			break;
		case Element::Master:
			m_list_editor << new MasterPropertiesWidget(m_element, this);
			m_list_editor << new ElementInfoWidget(m_element, this);
			break;
		case Element::Slave:
			m_list_editor << new LinkSingleElementWidget(m_element, this);
			break;
		case Element::Terminale:
			break;
		default:
			break;
	}

	foreach (PropertiesEditorWidget *pew, m_list_editor) m_tab->addTab(pew, pew->title());
	m_tab -> addTab(generalWidget(), tr("Général"));

	QVBoxLayout *main_layout = new QVBoxLayout(this);
	main_layout -> addWidget(m_tab);
	setLayout(main_layout);
}

/**
 * @brief ElementPropertiesWidget::generalWidget
 * @return build and return the "general" widget
 */
QWidget *ElementPropertiesWidget::generalWidget()
{
	CustomElement *custom_element = qobject_cast<CustomElement *>(m_element);
	GhostElement  *ghost_element  = qobject_cast<GhostElement  *>(m_element);

	// type de l'element
	QString description_string;
	if (ghost_element) {
		description_string += tr("Élément manquant");
	} else {
		description_string += tr("Élément");
	}
	description_string += "\n";

	// some element characteristic
	description_string += QString(tr("Nom : %1\n")).arg(m_element -> name());
	int folio_index = m_diagram -> folioIndex();
	if (folio_index != -1) {
		description_string += QString(tr("Folio : %1\n")).arg(folio_index + 1);
	}
	description_string += QString(tr("Position : %1\n")).arg(m_diagram -> convertPosition(m_element -> scenePos()).toString());
	description_string += QString(tr("Dimensions : %1*%2\n")).arg(m_element -> size().width()).arg(m_element -> size().height());
	description_string += QString(tr("Bornes : %1\n")).arg(m_element -> terminals().count());
	description_string += QString(tr("Champs de texte : %1\n")).arg(m_element -> texts().count());

	if (custom_element) {
		description_string += QString(tr("Emplacement : %1\n")).arg(custom_element -> location().toString());
	}

	// widget himself
	QWidget *general_widget = new QWidget (m_tab);
	QVBoxLayout *vlayout_ = new QVBoxLayout (general_widget);
	general_widget -> setLayout(vlayout_);

	//widget for the text
	QLabel *label = new QLabel (description_string, general_widget);
	label->setWordWrap(true);
	vlayout_->addWidget(label);

	//widget for the pixmap
//	QLabel *pix = new QLabel(general_widget);
//	pix->setPixmap(m_element->pixmap());
//	vlayout_->addWidget(pix, 0, Qt::AlignHCenter);
//	vlayout_ -> addStretch();

		//button widget
	QPushButton *find_in_panel = new QPushButton(QET::Icons::ZoomDraw, tr("Retrouver dans le panel"), general_widget);
	connect(find_in_panel, SIGNAL(clicked()), this, SLOT(findInPanel()));
	QPushButton *edit_element = new QPushButton(QET::Icons::ElementEdit, tr("Éditer l'élément"), general_widget);
	connect(edit_element, SIGNAL(clicked()), this, SLOT(editElement()));
	QHBoxLayout *hlayout_ = new QHBoxLayout;
	hlayout_->addWidget(find_in_panel);
	hlayout_->addWidget(edit_element);

	vlayout_->addLayout(hlayout_);
	return general_widget;
}
