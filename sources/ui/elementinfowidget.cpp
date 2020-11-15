/*
	Copyright 2006-2020 The QElectroTech Team
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
#include "elementinfowidget.h"
#include "ui_elementinfowidget.h"
#include "qetapp.h"
#include "changeelementinformationcommand.h"
#include "diagram.h"
#include "elementinfopartwidget.h"
#include "element.h"
#include "qetinformation.h"

/**
	@brief ElementInfoWidget::ElementInfoWidget
	Constructor
	@param elmt element to edit information
	@param parent parent widget
*/
ElementInfoWidget::ElementInfoWidget(Element *elmt, QWidget *parent) :
	AbstractElementPropertiesEditorWidget(parent),
	ui(new Ui::ElementInfoWidget),
	m_first_activation (false)
{
	ui->setupUi(this);
	buildInterface();
	setElement(elmt);
}

/**
	@brief ElementInfoWidget::~ElementInfoWidget
	Destructor
*/
ElementInfoWidget::~ElementInfoWidget()
{
	qDeleteAll(m_eipw_list);
	delete ui;
}

/**
	@brief ElementInfoWidget::setElement
	Set element to be the edited element
	@param element
*/
void ElementInfoWidget::setElement(Element *element)
{
	if (m_element == element) return;

	if (m_element)
		disconnect(m_element.data(), &Element::elementInfoChange, this, &ElementInfoWidget::elementInfoChange);

	m_element = element;
	updateUi();

	ElementInfoPartWidget *f = infoPartWidgetForKey("formula");
	ElementInfoPartWidget *l = infoPartWidgetForKey("label");

	if (f && l)
	{
		if (f->text().isEmpty())
			l->setEnabled(true);
		else
			l->setDisabled(true);

		connect(f, &ElementInfoPartWidget::textChanged, [l](const QString text)
		{
			l->setEnabled(text.isEmpty()? true : false);
		});
	}

	connect(m_element.data(), &Element::elementInfoChange, this, &ElementInfoWidget::elementInfoChange);
}

/**
	@brief ElementInfoWidget::apply
	Apply the new information with a new undo command (got with method associatedUndo)
	pushed to the stack of element project.
*/
void ElementInfoWidget::apply()
{
	if (QUndoCommand *undo = associatedUndo())
		m_element -> diagram() -> undoStack().push(undo);
}

/**
	@brief ElementInfoWidget::associatedUndo
	If the edited info is different of the actual element info,
	return a QUndoCommand with the change.
	If no change return nullptr;
	@return
*/
QUndoCommand* ElementInfoWidget::associatedUndo() const
{
	DiagramContext new_info = currentInfo();
	DiagramContext old_info = m_element -> elementInformations();

	if (old_info != new_info)
		return (new ChangeElementInformationCommand(m_element, old_info, new_info));

	return nullptr;
}

/**
	@brief ElementInfoWidget::setLiveEdit
	@param live_edit true : enable the live edit mode, false disable
	@return always true;
*/
bool ElementInfoWidget::setLiveEdit(bool live_edit)
{
	if (m_live_edit == live_edit) return true;
	m_live_edit = live_edit;

	if (m_live_edit)
		enableLiveEdit();
	else
		disableLiveEdit();

	return true;
}

/**
	@brief ElementInfoWidget::event
	Reimplemented from QWidget::event
	Only give focus to the first line edit at first activation.
	After send the event to QWidget.
	@param event
	@return
*/
bool ElementInfoWidget::event(QEvent *event)
{
	if (m_first_activation)
	{
		if (event -> type() == QEvent::WindowActivate || event -> type() == QEvent::Show)
		{
			QTimer::singleShot(250, this, SLOT(firstActivated()));
			m_first_activation = false;
		}
	}
	return(QWidget::event(event));
}

/**
	@brief ElementInfoWidget::enableLiveEdit
	Enable the live edit mode
*/
void ElementInfoWidget::enableLiveEdit()
{
	for (ElementInfoPartWidget *eipw : m_eipw_list)
		connect(eipw, &ElementInfoPartWidget::textChanged, this, &ElementInfoWidget::apply);
}

/**
	@brief ElementInfoWidget::disableLiveEdit
	disable the live edit mode
*/
void ElementInfoWidget::disableLiveEdit()
{
	for (ElementInfoPartWidget *eipw : m_eipw_list)
		disconnect(eipw, &ElementInfoPartWidget::textChanged, this, &ElementInfoWidget::apply);
}

/**
	@brief ElementInfoWidget::buildInterface
	Build the widget
*/
void ElementInfoWidget::buildInterface()
{
	for (auto str : QETInformation::elementInfoKeys())
	{
		ElementInfoPartWidget *eipw = new ElementInfoPartWidget(str, QETInformation::translatedInfoKey(str), this);
		ui->scroll_vlayout->addWidget(eipw);
		m_eipw_list << eipw;
	}
	ui->scroll_vlayout->addStretch();
}

/**
	@brief ElementInfoWidget::infoPartWidgetForKey
	@param key
	@return the ElementInfoPartWidget with key key,
	if not found return nullptr;
*/
ElementInfoPartWidget *ElementInfoWidget::infoPartWidgetForKey(const QString &key) const
{
	for (auto eipw : m_eipw_list)
	{
		if (eipw->key() == key)
			return eipw;
	}

	return nullptr;
}

/**
	@brief ElementInfoWidget::updateUi
	fill information fetch in m_element_info to the
	corresponding line edit
*/
void ElementInfoWidget::updateUi()
{
		//We disable live edit to avoid wrong undo when we fill the line edit with new text
	if (m_live_edit) disableLiveEdit();

	DiagramContext element_info = m_element->elementInformations();
	
	for (ElementInfoPartWidget *eipw : m_eipw_list) {
		eipw -> setText (element_info[eipw->key()].toString());
	}

	if (m_live_edit) {
		enableLiveEdit();
	}
}

/**
	@brief ElementInfoWidget::currentInfo
	@return the info currently edited
*/
DiagramContext ElementInfoWidget::currentInfo() const
{
	DiagramContext info_;

	for (ElementInfoPartWidget *eipw : m_eipw_list)
	{

			//add value only if they're something to store
		if (!eipw->text().isEmpty())
		{
			QString txt = eipw->text();
				//remove line feed and carriage return
			txt.remove("\r");
			txt.remove("\n");
			info_.addValue(eipw->key(), txt);
		}
	}

	return info_;
}

/**
	@brief ElementInfoWidget::firstActivated
	Slot activated when this widget is show.
	Set the focus to the first line edit provided by this widget
*/
void ElementInfoWidget::firstActivated()
{
	m_eipw_list.first() -> setFocusTolineEdit();
}

/**
	@brief ElementInfoWidget::elementInfoChange
	This slot is called when m_element::elementInformation change.
*/
void ElementInfoWidget::elementInfoChange()
{
	if(currentInfo() != m_element->elementInformations())
		updateUi();
}
