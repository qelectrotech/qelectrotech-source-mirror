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
#include "ghostelement.h"
#include "qeticons.h"
#include "diagramposition.h"
#include "diagram.h"
#include "elementinfowidget.h"
#include "masterpropertieswidget.h"
#include "linksingleelementwidget.h"

/**
 * @brief elementpropertieswidget::elementpropertieswidget
 * default constructor
 * @param elmt
 * @param parent
 */
elementpropertieswidget::elementpropertieswidget(Element *elmt, QWidget *parent) :
	QDialog(parent),
	element_ (elmt),
	diagram_ (elmt->diagram())
{
	eiw_ = 0;
	mpw_ = 0;
	lsew_ = 0;
	buildInterface();
}

/**
 * @brief elementpropertieswidget::generalWidget
 * build the widget for the tab général
 * @return
 */
QWidget* elementpropertieswidget::generalWidget() {
	CustomElement *custom_element = qobject_cast<CustomElement *>(element_);
	GhostElement  *ghost_element  = qobject_cast<GhostElement  *>(element_);

	// type de l'element
	QString description_string;
	if (ghost_element) {
		description_string += tr("Élément manquant");
	} else {
		description_string += tr("Élément");
	}
	description_string += "\n";

	// some element characteristic
	description_string += QString(tr("Nom : %1\n")).arg(element_ -> name());
	int folio_index = diagram_ -> folioIndex();
	if (folio_index != -1) {
		description_string += QString(tr("Folio : %1\n")).arg(folio_index + 1);
	}
	description_string += QString(tr("Position : %1\n")).arg(diagram_ -> convertPosition(element_ -> scenePos()).toString());
	description_string += QString(tr("Dimensions : %1\327%2\n")).arg(element_ -> size().width()).arg(element_ -> size().height());
	description_string += QString(tr("Bornes : %1\n")).arg(element_ -> terminals().count());
	description_string += QString(tr("Champs de texte : %1\n")).arg(element_ -> texts().count());

	if (custom_element) {
		description_string += QString(tr("Emplacement : %1\n")).arg(custom_element -> location().toString());
	}

	// widget himself
	QWidget *general_widget = new QWidget (tab_);
	QVBoxLayout *vlayout_ = new QVBoxLayout (general_widget);
	general_widget -> setLayout(vlayout_);

	//widget for the text
	vlayout_->addWidget(new QLabel (description_string, general_widget));

	//widget for the pixmap
	QLabel *pix = new QLabel(general_widget);
	pix->setPixmap(element_->pixmap());
	vlayout_->addWidget(pix, 0, Qt::AlignHCenter);
	vlayout_ -> addStretch();

	//button widget
	find_in_panel = new QPushButton(QET::Icons::ZoomDraw, tr("Retrouver dans le panel"), general_widget);
	connect(find_in_panel, SIGNAL(clicked()), this, SLOT(findInPanel()));
	edit_element = new QPushButton(QET::Icons::ElementEdit, tr("Éditer l'élément"), general_widget);
	connect(edit_element, SIGNAL(clicked()), this, SLOT(editElement()));
	QHBoxLayout *hlayout_ = new QHBoxLayout;
	hlayout_->addWidget(find_in_panel);
	hlayout_->addWidget(edit_element);

	vlayout_->addLayout(hlayout_);
	return general_widget;
}

/**
 * @brief elementpropertieswidget::buildInterface
 *build the interface of this dialog, the main tab can have
 *different tab according to the edited element
 */
void elementpropertieswidget::buildInterface() {

	setWindowTitle(tr("Propriétés de l'élément"));
	tab_ = new QTabWidget(this);

	//Add tab according to the element
	switch (element_ -> linkType()) {
		case Element::Simple:
			eiw_ = new ElementInfoWidget(element_, this);
			tab_ -> addTab(eiw_, tr("Information"));
			break;
		case Element::NextReport:
			lsew_ = new LinkSingleElementWidget(element_, this);
			tab_ -> addTab(lsew_, tr("Report de folio"));
			break;
		case Element::PreviousReport:
			lsew_ = new LinkSingleElementWidget(element_, this);
			tab_ -> addTab(lsew_, tr("Report de folio"));
			break;
		case Element::Master:
			mpw_ = new MasterPropertiesWidget(element_, this);
			tab_ -> addTab(mpw_, tr("Référence croisée (maitre)"));
			eiw_ = new ElementInfoWidget(element_, this);
			tab_ -> addTab(eiw_, tr("Informations"));
			break;
		case Element::Slave:
			lsew_ = new LinkSingleElementWidget(element_, this);
			tab_ -> addTab(lsew_, tr("Référence croisée (esclave)"));
			break;
		case Element::Terminale:
			break;
		default:
			break;
	}

	tab_ -> addTab(generalWidget(), tr("Général"));

	dbb = new QDialogButtonBox(QDialogButtonBox::Apply | QDialogButtonBox::Cancel | QDialogButtonBox::Reset,
							   Qt::Horizontal, this);
	connect(dbb, SIGNAL(clicked(QAbstractButton*)), this, SLOT(standardButtonClicked(QAbstractButton*)));

	QVBoxLayout *main_layout = new QVBoxLayout(this);
	main_layout -> addWidget(tab_);
	main_layout -> addWidget(dbb);
	setLayout(main_layout);
}

/**
 * @brief elementpropertieswidget::standardButtonClicked
 * apply action when click in the dialog standard button box
 * @param button
 * the cliked button
 */
void elementpropertieswidget::standardButtonClicked(QAbstractButton *button) {
	int  answer = dbb -> buttonRole(button);
	bool accept = false;

	switch (answer) {
		case QDialogButtonBox::ResetRole:
			if (mpw_) mpw_->reset();
			break;
		case QDialogButtonBox::ApplyRole:
			accept = true;
			break;
		case QDialogButtonBox::RejectRole:
			this -> reject();
			break;
		default:
			this -> reject();
			break;
	}

	if (accept) {
		QUndoCommand *a = nullptr;
		QUndoCommand *b = nullptr;

		if (eiw_) a = eiw_ -> associatedUndo();
		if (mpw_) b = mpw_ -> associatedUndo();
		if (lsew_) lsew_ -> apply();

		 //If two undo, we push it in a macro
		if (a && b) {
			QUndoStack &stack = element_ -> diagram() -> undoStack();
			stack.beginMacro(a -> text() + " + " + b -> text());
			stack.push(a);
			stack.push(b);
			stack.endMacro();
		}
		else {
			if (a) element_ -> diagram() -> undoStack().push(a);
			if (b) element_ -> diagram() -> undoStack().push(b);
		}

		this -> accept();
	}
}

/**
 * @brief elementpropertieswidget::findInPanel
 * Slot
 */
void elementpropertieswidget::findInPanel() {
	if (CustomElement *custom_element = qobject_cast<CustomElement *>(element_)) {
		emit findElementRequired(custom_element->location());
	}
	reject();
}

/**
 * @brief elementpropertieswidget::editElement
 * Slot
 */
void elementpropertieswidget::editElement() {
	if (CustomElement *custom_element = qobject_cast<CustomElement *>(element_)) {
		emit findElementRequired(custom_element->location());
		emit editElementRequired(custom_element->location());
	}
	reject();
}
