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

#include "conductorpropertiesdialog.h"

#include "../QPropertyUndoCommand/qpropertyundocommand.h"
#include "../diagram.h"
#include "../qetgraphicsitem/conductor.h"
#include "conductorpropertieswidget.h"
#include "ui_conductorpropertiesdialog.h"

#include "../qet.h"
#include "../qetproject.h"
#include "../protectiveearthpattern.h"
#include "../protectiveearthpattern17.h"
#include "../qetgraphicsitem/terminal.h"
#include <QHeaderView>
#include <QLabel>
#include <QPointer>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QPushButton>
#include <QTreeWidget>
#include <QComboBox>
#include <QCheckBox>
#include <QFormLayout>

namespace {
// setProperties() normally recalculates label geometry. A line-pattern-only
// operation must preserve deliberate label positions/rotations on redo too.
class PePatternCommand17 final : public QUndoCommand {
    QPointer<Conductor> m_wire;
    ConductorProperties m_before,m_after;
    QPointF m_textPosition;
    qreal m_textRotation=0;
    void apply(const ConductorProperties &properties) {
        if (!m_wire) return;
        m_wire->setProperties(properties);
        if (auto *text=m_wire->textItem()) {
            text->QGraphicsItem::setPos(m_textPosition);
            text->setRotation(m_textRotation);
        }
    }
public:
    PePatternCommand17(Conductor *wire,const ConductorProperties &before,
                       const ConductorProperties &after,QUndoCommand *parent)
        : QUndoCommand(parent),m_wire(wire),m_before(before),m_after(after) {
        if (auto *text=wire->textItem()) {
            m_textPosition=text->pos();m_textRotation=text->rotation();
        }
    }
    void redo() override {apply(m_after);}
    void undo() override {apply(m_before);}
};


bool applyAppearance25(Conductor *source,const ConductorProperties &appearance,QWidget *parent){
 if(!source||!source->diagram()||source->diagram()->isReadOnly())return false;
 auto *project=source->diagram()->project();
 QDialog d(parent);d.setWindowTitle(QObject::tr("Apply conductor appearance"));auto *f=new QFormLayout(&d);
 auto *scope=new QComboBox;scope->addItems({QObject::tr("Selected conductors"),QObject::tr("Same potential"),QObject::tr("Current sheet"),QObject::tr("Entire project")});scope->setObjectName("appearanceScope25");scope->setCurrentIndex(2);f->addRow(QObject::tr("Apply to"),scope);
 auto *thickness=new QCheckBox(QObject::tr("Thickness"));thickness->setObjectName("appearanceThickness25");thickness->setChecked(true);f->addRow(thickness);
 auto *colour=new QCheckBox(QObject::tr("Colour"));colour->setObjectName("appearanceColour25");f->addRow(colour);
 auto *style=new QCheckBox(QObject::tr("Line style (including solid)"));style->setObjectName("appearanceStyle25");style->setChecked(true);f->addRow(style);
 auto *pattern=new QCheckBox(QObject::tr("Custom dash pattern"));pattern->setObjectName("appearancePattern25");pattern->setChecked(true);f->addRow(pattern);
 auto *pe=new QCheckBox(QObject::tr("PE conductors only"));pe->setObjectName("appearancePe25");f->addRow(pe);
 auto *buttons=new QDialogButtonBox(QDialogButtonBox::Apply|QDialogButtonBox::Cancel);f->addRow(buttons);
 QObject::connect(buttons->button(QDialogButtonBox::Apply),&QPushButton::clicked,&d,&QDialog::accept);
 QObject::connect(buttons,&QDialogButtonBox::rejected,&d,&QDialog::reject);
 if(d.exec()!=QDialog::Accepted)return false;
 QSet<Conductor*> targets;
 if(scope->currentIndex()==1){targets.insert(source);for(auto *c:source->relatedPotentialConductors())targets.insert(c);}
 else {auto sheets=scope->currentIndex()==3?project->diagrams():QList<Diagram*>{source->diagram()};for(auto *sheet:sheets)if(!sheet->isReadOnly())for(auto *item:sheet->items())if(auto *wire=dynamic_cast<Conductor*>(item))if(scope->currentIndex()!=0||wire->isSelected())targets.insert(wire);}
 if(pe->isChecked()){QSet<Conductor*> confirmed;for(auto t:ProtectiveEarthPattern::scanProject17(project,source,&appearance).targets)if(t.confirmed)confirmed.insert(t.wire);targets.intersect(confirmed);}
 auto *command=new QUndoCommand(QObject::tr("Apply conductor appearance"));
 for(auto *wire:targets){if(wire->diagram()->isReadOnly())continue;auto before=wire->properties(),after=before;
  if(thickness->isChecked())after.cond_size=appearance.cond_size;
  if(colour->isChecked()){after.color=appearance.color;after.m_color_2=appearance.m_color_2;after.m_bicolor=appearance.m_bicolor;}
  if(style->isChecked())after.style=appearance.style;
  if(pattern->isChecked()){after.m_dash_pattern=appearance.m_dash_pattern;after.m_dash_size=appearance.m_dash_size;}
  if(before!=after)new PePatternCommand17(wire,before,after,command);
 }
 if(!command->childCount()){delete command;QMessageBox::information(parent,QObject::tr("Conductor appearance"),QObject::tr("No matching conductors need a change."));return false;}
 source->diagram()->undoStack().push(command);return true;
}
bool applyPatternToProjectPe(Conductor *source, const ConductorProperties &pattern,
                             QWidget *parent)
{
    QPointer<QETProject> project = source && source->diagram()
            ? source->diagram()->project() : nullptr;
    if (!project || project->isReadOnly()) return false;
    if (pattern.style != Qt::DashLine && pattern.style != Qt::DashDotLine) {
        QMessageBox::information(parent, QObject::tr("PE conductor pattern"),
                QObject::tr("Choose Dashed or Dots and dashes in Appearance first."));
        return false;
    }

    const auto scan = ProtectiveEarthPattern::scanProject17(project, source, &pattern);

    QDialog review(parent);
    review.setWindowTitle(QObject::tr("Apply pattern to PE conductors — entire project"));
    review.resize(820, 440);
    auto *layout = new QVBoxLayout(&review);
    auto *description = new QLabel(QObject::tr(
        "Select PE conductors to update. Only line style and dash/gap lengths are copied.\n"
        "Wire numbers, cross-sections, colours, thickness and text positions are preserved.\n"
        "Nearby separate PE labels are offered unchecked: review and select them explicitly."), &review);
    description->setWordWrap(true);
    layout->addWidget(description);
    auto *list = new QTreeWidget(&review);
    list->setObjectName(QStringLiteral("projectPePatternTargets"));
    list->setHeaderLabels({QObject::tr("Apply"), QObject::tr("Sheet"),
                          QObject::tr("Conductor"), QObject::tr("Function"),
                          QObject::tr("Identified by")});
    list->setRootIsDecorated(false);
    list->setSelectionMode(QAbstractItemView::NoSelection);
    QList<QPointer<Conductor>> targets;
    QSet<Diagram *> sheets;
    for (const auto &target : scan.targets) {
        Conductor *wire = target.wire;
        const auto properties = wire->properties();
        const QString sheet = QStringLiteral("%1 — %2")
                .arg(wire->diagram()->folioIndex() + 1)
                .arg(wire->diagram()->border_and_titleblock.title());
        auto *item = new QTreeWidgetItem(list, {QString(), sheet, properties.text,
                properties.m_function, target.reason});
        item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsUserCheckable);
        item->setCheckState(0, target.confirmed ? Qt::Checked : Qt::Unchecked);
        targets << QPointer<Conductor>(wire);
        sheets.insert(wire->diagram());
    }
    for (int column = 0; column < 4; ++column) list->resizeColumnToContents(column);
    list->setColumnWidth(1, qMax(180, list->columnWidth(1)));
    list->setColumnWidth(2, qMax(100, list->columnWidth(2)));
    list->setColumnWidth(3, qMax(90, list->columnWidth(3)));
    layout->addWidget(list);
    auto *selection = new QHBoxLayout;
    auto *all = new QPushButton(QObject::tr("Select all"), &review);
    auto *none = new QPushButton(QObject::tr("Select none"), &review);
    auto *count = new QLabel(&review);
    selection->addWidget(all); selection->addWidget(none); selection->addStretch();
    selection->addWidget(count);
    layout->addLayout(selection);
    auto *hint = new QLabel(QObject::tr(
        "Detection uses PE in the conductor label, Function, Voltage/Protocol or terminal name, "
        "plus its connected network. Current source properties are included without closing this dialog. "
        "Visual-only PE text supplies review candidates, never automatic identification. "
        "If a wire is still missing, set its Function to PE and retry. "
        "Symbol drawing lines are edited separately in the symbol editor."), &review);
    hint->setWordWrap(true);
    layout->addWidget(hint);
    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Apply | QDialogButtonBox::Cancel, &review);
    layout->addWidget(buttons);
    auto updateCount = [=]() {
        int selected = 0;
        for (int i = 0; i < list->topLevelItemCount(); ++i)
            if (list->topLevelItem(i)->checkState(0) == Qt::Checked) ++selected;
        count->setText(QObject::tr("%1 selected / %2 candidates on %3 sheets; scanned %4 wires on %5 sheets")
                      .arg(selected).arg(targets.size()).arg(sheets.size())
                      .arg(scan.scannedWires).arg(scan.scannedSheets));
        buttons->button(QDialogButtonBox::Apply)->setEnabled(selected > 0);
    };
    QObject::connect(list, &QTreeWidget::itemChanged, &review, [=]() {updateCount();});
    QObject::connect(all, &QPushButton::clicked, &review, [=]() {
        for (int i = 0; i < list->topLevelItemCount(); ++i)
            list->topLevelItem(i)->setCheckState(0, Qt::Checked);
    });
    QObject::connect(none, &QPushButton::clicked, &review, [=]() {
        for (int i = 0; i < list->topLevelItemCount(); ++i)
            list->topLevelItem(i)->setCheckState(0, Qt::Unchecked);
    });
    QObject::connect(buttons->button(QDialogButtonBox::Apply), &QPushButton::clicked,
                     &review, &QDialog::accept);
    QObject::connect(buttons, &QDialogButtonBox::rejected, &review, &QDialog::reject);
    updateCount();
    if (review.exec() != QDialog::Accepted || !project || project->isReadOnly()) return false;

    auto *undo = new QUndoCommand(QObject::tr("Apply pattern to PE conductors in project"));
    for (int i = 0; i < targets.size(); ++i) {
        Conductor *wire = targets[i];
        if (!wire || wire->diagram()->isReadOnly()
                || list->topLevelItem(i)->checkState(0) != Qt::Checked) continue;
        const auto before = wire->properties();
        const auto after = ProtectiveEarthPattern::withPattern(before, pattern);
        if (before == after) continue;
        new PePatternCommand17(wire,before,after,undo);
    }
    if (undo->childCount()) project->undoStack()->push(undo);
    else delete undo;
    return true;
}
} // namespace

bool ConductorPropertiesDialog::ApplyPatternToProject(Conductor *conductor, QWidget *parent)
{
    return conductor && applyPatternToProjectPe(conductor, conductor->properties(), parent);
}
/**
	@brief ConductorPropertiesDialog::ConductorPropertiesDialog
	Constructor
	@param conductor : conductor to edit properties
	@param parent : parent widget
*/
ConductorPropertiesDialog::ConductorPropertiesDialog(
		Conductor *conductor, QWidget *parent) :
	QDialog(parent),
	ui(new Ui::ConductorPropertiesDialog)
{
	ui->setupUi(this);
	QET::trackDialogGeometry(this);
	m_cpw = new ConductorPropertiesWidget(conductor->properties());
	m_cpw -> setHiddenOneTextPerFolio(true);
	m_cpw->setHiddenAvailableAutonum(true);
	if (conductor -> diagram() -> defaultConductorProperties.m_one_text_per_folio == true &&
		conductor -> relatedPotentialConductors().size()) {
		m_cpw->setDisabledShowText();
	}
	ui -> main_layout -> insertWidget(0, m_cpw);
    auto *appearance25=new QPushButton(tr("Apply conductor appearance..."),this);ui->main_layout->insertWidget(ui->main_layout->count()-1,appearance25);
    connect(appearance25,&QPushButton::clicked,this,[this,conductor]{if(applyAppearance25(conductor,properties(),this)){m_project_pattern_applied=true;accept();}});
	auto *project_pattern = new QPushButton(tr("Apply pattern to all PE in project…"), this);
	project_pattern->setObjectName(QStringLiteral("applyProjectPePattern"));
	project_pattern->setEnabled(conductor->diagram() && !conductor->diagram()->isReadOnly());
	ui->main_layout->insertWidget(ui->main_layout->count() - 1, project_pattern);
	connect(project_pattern, &QPushButton::clicked, this, [this, conductor]() {
		if (applyPatternToProjectPe(conductor, properties(), this)) {
			m_project_pattern_applied = true;
			accept();
		}
	});
}

/**
	@brief ConductorPropertiesDialog::~ConductorPropertiesDialog
*/
bool ConductorPropertiesDialog::ApplyAppearance(Conductor *conductor,QWidget *parent){return conductor&&applyAppearance25(conductor,conductor->properties(),parent);}

ConductorPropertiesDialog::~ConductorPropertiesDialog()
{
	delete ui;
}

/**
	@brief ConductorPropertiesDialog::PropertiesDialog
	Static method for open and apply properties.
	@param conductor : conductor to edit properties
	@param parent : parent widget
*/
void ConductorPropertiesDialog::PropertiesDialog(Conductor *conductor,
						 QWidget *parent)
{
	ConductorPropertiesDialog cpd (conductor, parent);

	if (cpd.exec() == QDialog::Rejected || cpd.m_project_pattern_applied
			|| (cpd.properties() == conductor->properties() && !cpd.applyAll())) return;

	QVariant old_value, new_value;
	old_value.setValue(conductor->properties());
	new_value.setValue(cpd.properties());

	QPropertyUndoCommand *undo = new QPropertyUndoCommand(conductor, "properties", old_value, new_value);
	undo->setText(tr("Edit conductor properties", "undo caption"));

	if (!conductor->relatedPotentialConductors().isEmpty() && cpd.applyAll())
	{
		undo->setText(tr("Edit the properties of several conductors", "undo caption"));

		foreach (Conductor *potential_conductor, conductor->relatedPotentialConductors())
		{
			old_value.setValue(potential_conductor->properties());
			new QPropertyUndoCommand (potential_conductor, "properties", old_value, new_value, undo);
		}
	}

		//A function separated terminal element (element information
		//potential_isolating) keeps the two sides of the terminal apart:
		//every property but one stays local to the side it was edited on.
		//The voltage / protocol still crosses, because it describes the
		//electrical node and not one side of it.
	if (cpd.applyAll())
	{
			//Only an edit that really changed the voltage / protocol passes
			//it on, so the comparison is the edit itself -- what the dialog
			//was filled with, against what the user left in it -- and never
			//the far wire's own value, which differs from it just as often
			//as not (a 24 V side against a 0 V one, say) and would then be
			//overwritten by every edit, voltage included.
		const ConductorProperties shown = conductor->properties();
		foreach (Conductor *separated_conductor, conductor->functionSeparatedConductors())
		{
			ConductorProperties separated_properties = separated_conductor->properties();
			separated_properties.applyTensionProtocolChange(
						shown,
						cpd.properties());
			if (separated_properties == separated_conductor->properties())
				continue;

			QVariant separated_old_value, separated_new_value;
			separated_old_value.setValue(separated_conductor->properties());
			separated_new_value.setValue(separated_properties);
			new QPropertyUndoCommand (separated_conductor, "properties",
						  separated_old_value, separated_new_value, undo);
		}
	}

	conductor->diagram()->undoStack().push(undo);
}

/**
	@brief ConductorPropertiesDialog::properties
	@return the edited properties
*/
ConductorProperties ConductorPropertiesDialog::properties() const
{
	return m_cpw -> properties();
}

/**
	@brief ConductorPropertiesDialog::applyAll
	@return
	true -> must apply properties to all conductors at the same potential
	false -> must apply properties only for the edited conductor
*/
bool ConductorPropertiesDialog::applyAll() const
{
	return ui -> m_apply_all_cb -> isChecked();
}
