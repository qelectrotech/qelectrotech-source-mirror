/*
	Copyright 2006-2014 The QElectroTech Team
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
#include "closediagramsdialog.h"
#include "diagram.h"
#include "qeticons.h"

/**
	Construct a dialog showing \a diagrams.
	@param parent Parent QWidget
*/
CloseDiagramsDialog::CloseDiagramsDialog(const QList<Diagram *> &diagrams, QWidget *parent) :
	QDialog(parent),
	diagrams_list_(diagrams),
	answer_(-1)
{
	initWidgets();
	initLayout();
}

/**
	Destructor
*/
CloseDiagramsDialog::~CloseDiagramsDialog() {
}

/**
	@return the user answer once the dialog has been executed.
*/
int CloseDiagramsDialog::answer() const {
	return(answer_);
}

/**
	@return what the user wants to do with \a diagram
	@see CloseDiagramsDialog::Actions
*/
int CloseDiagramsDialog::actionForDiagram(Diagram *diagram) {
	if (QCheckBox *checkbox = getCheckBoxForDiagram(diagram)) {
		if (!diagram -> undoStack().isClean()) {
			return(checkbox -> isChecked() ? Save : DoNotSave);
		} else if (!diagram -> wasWritten()) {
			return(checkbox -> isChecked() ? Save : Remove);
		}
	}
	return(Unknown);
}

/**
	@return the list of diagrams for which users have chosen the \a action
	action.
*/
QList<Diagram *> CloseDiagramsDialog::diagramsByAction(Actions action) {
	QList<Diagram *> diagrams;
	foreach (Diagram *diagram, diagrams_list_) {
		if (actionForDiagram(diagram) == action) {
			diagrams << diagram;
		}
	}
	return(diagrams);
}

/**
	Initialize widgets.
*/
void CloseDiagramsDialog::initWidgets() {
	setWindowTitle(tr("Fermer un projet", "window title"));
	
	connect(&show_mapper_, SIGNAL(mapped(int)), this, SLOT(requireShowDiagram(int)));
	
	// label when diagrams were modified
	informative_label1_ = new QLabel(
		tr(
			"Les sch\351mas ci-dessous contiennent des modifications non "
			"enregistr\351es. Faut-il les sauvegarder ?",
			"informative label"
		)
	);
	informative_label1_ -> setWordWrap(true);
	
	// label when no diagrams were modified
	informative_label2_ = new QLabel(tr("Voulez-vous enregistrer le projet ?", "informative label"));
	informative_label2_ -> setWordWrap(true);
	
	// header labels
	QLabel *state_label = new QLabel(tr("\311tat", "column header"));
	QLabel *title_label = new QLabel(tr("Sch\351ma", "column header"));
	
	// header checkbox in order to check/uncheck all diagrams
	QString action_label_text = tr("Action", "column header");
	QLabel *action_label = new QLabel(action_label_text);
	all_checkbox_ = new QCheckBox(action_label_text);
	all_checkbox_ -> setToolTip(tr("Cocher ou d\351cocher toutes les cases \340 cocher", "checbox tooltip"));
	all_checkbox_ -> setChecked(true);
	connect(all_checkbox_, SIGNAL(stateChanged(int)), this, SLOT(topCheckBoxChangedState(int)));
	QWidget *header_widget = diagrams_list_.count() > 1 ? static_cast<QWidget *>(all_checkbox_) : static_cast<QWidget *>(action_label);
	Qt::Alignment header_alignment = diagrams_list_.count() > 1 ? Qt::AlignLeft : Qt::AlignCenter;
	
	// spacers inserted in the header row
	QSpacerItem *spacer1 = new QSpacerItem(10, 10, QSizePolicy::Expanding, QSizePolicy::Minimum);
	QSpacerItem *spacer2 = new QSpacerItem(25, 10, QSizePolicy::Preferred, QSizePolicy::Minimum);
	
	buttons_ = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Discard | QDialogButtonBox::Cancel);
	connect(buttons_, SIGNAL(clicked(QAbstractButton *)), this, SLOT(storeAnswer(QAbstractButton *)));
	
	// widget layout
	diagrams_list_layout_ = new QGridLayout();
	diagrams_list_layout_ -> addWidget(title_label, 0, 1, 1, 1, Qt::AlignCenter);
	diagrams_list_layout_ -> addItem(spacer1, 0, 2);
	diagrams_list_layout_ -> addWidget(state_label, 0, 3, 1, 1, Qt::AlignCenter);
	diagrams_list_layout_ -> addItem(spacer2, 0, 4);
	diagrams_list_layout_ -> addWidget(header_widget, 0, 5, 1, 1, header_alignment);
	
	// widget
	diagrams_list_widget_ = new QWidget();
	diagrams_list_widget_ -> setLayout(diagrams_list_layout_);
	diagrams_list_widget_ -> setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);
	
	// scroll area
	diagrams_list_area_ = new QScrollArea();
	diagrams_list_area_ -> setWidgetResizable(true);
	diagrams_list_area_ -> setFrameStyle(QFrame::Plain | QFrame::NoFrame);
	
	loadDiagramsList();
	diagrams_list_area_ -> setWidget(diagrams_list_widget_);
}

/**
	Initialize layout.
*/
void CloseDiagramsDialog::initLayout() {
	if (diagrams_list_.count()) {
		setMinimumSize(650, 340);
	}
	
	QVBoxLayout *vlayout0 = new QVBoxLayout();
	vlayout0 -> addWidget(informative_label1_);
	vlayout0 -> addWidget(informative_label2_);
	vlayout0 -> addWidget(diagrams_list_area_);
	vlayout0 -> addWidget(buttons_);
	setLayout(vlayout0);
}

/**
	Create a visual list of all modified diagrams.
*/
void CloseDiagramsDialog::loadDiagramsList() {
	if (diagrams_list_.count()) {
		int row_id = 1;
		foreach (Diagram *diagram, diagrams_list_) {
			addDiagram(diagram, row_id);
			++ row_id;
		}
		informative_label2_ -> setVisible(false);
	} else {
		informative_label1_ -> setVisible(false);
		diagrams_list_area_ -> setVisible(false);
	}
}

/**
	Add \a diagram to the \a row_id row of the visual list.
*/
void CloseDiagramsDialog::addDiagram(Diagram *diagram, int row_id) {
	QLabel *diagram_title = new QLabel(diagramTitle(diagram));
	QPushButton *diagram_show = new QPushButton(QET::Icons::ZoomOriginal, "");
	diagram_show -> setToolTip(tr("Afficher ce sch\351ma", "button tooltip"));
	show_mapper_.setMapping(diagram_show, row_id - 1);
	connect(diagram_show, SIGNAL(released()), &show_mapper_, SLOT(map()));
	QLabel *diagram_status = new QLabel(diagramStatus(diagram));
	QCheckBox *diagram_checkbox = new QCheckBox(diagramAction(diagram));
	diagram_checkbox -> setChecked(true);
	connect(diagram_checkbox, SIGNAL(stateChanged(int)), this, SLOT(lambdaCheckBoxChangedState(int)));
	
	diagrams_list_layout_ -> addWidget(diagram_show,       row_id, 0, 1, 1, Qt::AlignCenter);
	diagrams_list_layout_ -> addWidget(diagram_title,      row_id, 1, 1, 1, Qt::AlignCenter);
	diagrams_list_layout_ -> addWidget(diagram_status,     row_id, 3, 1, 1, Qt::AlignCenter);
	diagrams_list_layout_ -> addWidget(diagram_checkbox,   row_id, 5, 1, 1, Qt::AlignLeft);
}

/**
	@return the action checkbox for \a diagram, or 0 if no adequate checkbox could be found.
*/
QCheckBox *CloseDiagramsDialog::getCheckBoxForDiagram(Diagram *diagram) {
	int diagram_index = diagrams_list_.indexOf(diagram);
	if (diagram_index == -1) return(0);
	
	// We add 1 because there is one row dedicated to column headers;
	// 4 is the column containing checkboxes, see initWidgets().
	QLayoutItem *item = diagrams_list_layout_ -> itemAtPosition(diagram_index + 1, 5);
	if (!item) return(0);
	
	QWidget *widget = item -> widget();
	if (!widget) return(0);
	
	return(static_cast<QCheckBox *>(widget));
}

/**
	@return the title for \a diagram
*/
QString CloseDiagramsDialog::diagramTitle(Diagram *diagram) {
	if (!diagram -> title().isEmpty()) {
		return(diagram -> title());
	}
	return(tr("Sch\351ma sans titre", "fallback diagram title"));
}

/**
	@return a string describing the status of \a diagram
*/
QString CloseDiagramsDialog::diagramStatus(Diagram *diagram) {
	if (!diagram) return(QString());
	if (!diagram -> undoStack().isClean()) {
		return(tr("Modifi\351", "diagram status"));
	} else if (!diagram -> wasWritten()) {
		return(tr("Ajout\351, non modifi\351", "diagram status"));
	} else {
		return(QString());
	}
}

/**
	@return a string describing the effect of saving \a diagram (e.g. "Save" or
	"Keep").
*/
QString CloseDiagramsDialog::diagramAction(Diagram *diagram) {
	if (!diagram) return(QString());
	if (!diagram -> undoStack().isClean()) {
		return(tr("Enregistrer", "diagram action"));
	} else if (!diagram -> wasWritten()) {
		return(tr("Conserver", "diagram action"));
	} else {
		return(QString());
	}
}

/**
	Adjust the state of the header checkbox when a diagram checkbox was
	switched to \a new_state.
*/
void CloseDiagramsDialog::lambdaCheckBoxChangedState(int new_state) {
	bool state = new_state;
	
	bool all_same_state = true;
	foreach (Diagram *diagram, diagrams_list_) {
		if (QCheckBox *checkbox = getCheckBoxForDiagram(diagram)) {
			if (checkbox -> isChecked() != state) {
				all_same_state = false;
				break;
			}
		}
	}
	
	all_checkbox_ -> blockSignals(true);
	if (all_same_state) {
		all_checkbox_ -> setTristate(false);
		all_checkbox_ -> setChecked(state);
	} else {
		all_checkbox_ -> setTristate(true);
		all_checkbox_ -> setCheckState(Qt::PartiallyChecked);
	}
	all_checkbox_ -> blockSignals(false);
	all_checkbox_ -> update();
}

/**
	Adjust diagram checkboxes when the header checkbox was switched to \a
	new_state.
*/
void CloseDiagramsDialog::topCheckBoxChangedState(int new_state) {
	setCheckedAll((bool)new_state);
}

/**
	Set all diagram checkboxes to the checked (true) or unchecked (false)
	state.
*/
void CloseDiagramsDialog::setCheckedAll(bool checked) {
	foreach (Diagram *diagram, diagrams_list_) {
		if (QCheckBox *checkbox = getCheckBoxForDiagram(diagram)) {
			if (checkbox -> isChecked() != checked) {
				checkbox -> blockSignals(true);
				checkbox -> setChecked(checked);
				checkbox -> blockSignals(false);
			}
		}
	}
}

/**
	Find the diagram at \a diagram_index and emts the showDiagram() signal with
	it.
*/
void CloseDiagramsDialog::requireShowDiagram(int diagram_index) {
	Diagram *diagram = diagrams_list_.value(diagram_index);
	if (!diagram) return;
	emit(showDiagram(diagram));
}

/**
	Store the user answer when the dialog is validated or rejected.
*/
void CloseDiagramsDialog::storeAnswer(QAbstractButton *button) {
	answer_ = buttons_ -> buttonRole(button);
	accept();
}
