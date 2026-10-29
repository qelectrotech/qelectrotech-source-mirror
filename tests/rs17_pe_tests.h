/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef RS17_PE_TESTS_H
#define RS17_PE_TESTS_H
#include "protectiveearthpattern17.h"
#include "qetapp.h"
#include "ui/conductorpropertiesdialog.h"
#include "qetgraphicsitem/independenttextitem.h"
#include <QtWidgets>
#include <QTemporaryDir>

template<class Check>
void runPe17Tests(Check check,const QString &out,const QString &sourceDir,
                  const QString &realProject)
{
    using namespace ProtectiveEarthPattern;
    QTemporaryDir temporary;
    const QString fixture=temporary.filePath("pe-fixture.qet");
    check(QFile::copy(sourceDir+"/tests/qttest/fixtures/ten_sequential_numbers.qet",fixture),"PE copy real embedded-symbol fixture");
    // The upstream contact fixture already contains three interconnections.
    // Remove only those wires in the temporary XML so the four independent
    // scenarios below cannot accidentally share a potential through them.
    QFile fixtureFile(fixture);check(fixtureFile.open(QIODevice::ReadOnly),"PE read fixture topology");
    QDomDocument fixtureXml;check(fixtureXml.setContent(fixtureFile.readAll()),"PE parse fixture topology");fixtureFile.close();
    auto fixturePage=fixtureXml.documentElement().firstChildElement("diagram");
    auto fixtureWires=fixturePage.firstChildElement("conductors");
    check(fixturePage.firstChildElement("elements").elementsByTagName("element").size()==4,"PE fixture contains four actual contact instances");
    while(!fixtureWires.firstChild().isNull())fixtureWires.removeChild(fixtureWires.firstChild());
    check(fixtureFile.open(QIODevice::WriteOnly|QIODevice::Truncate),"PE write isolated temporary fixture");
    fixtureFile.write(fixtureXml.toByteArray(2));fixtureFile.close();
    QETProject project(fixture);QETApp::registerProject(&project);
    check(project.state()==QETProject::Ok,"PE real application fixture opens");
    auto *page=project.diagrams().first();
    QList<Element *> elements;
    for(auto *item:page->items())if(auto *element=dynamic_cast<Element *>(item))elements<<element;
    std::sort(elements.begin(),elements.end(),[](Element *a,Element *b){return a->scenePos().x()<b->scenePos().x();});
    check(elements.size()==4&&page->conductors().isEmpty(),"PE fixture has four separate real contact symbols");
    QList<Conductor *> wires;
    for(int i=0;i<4;++i){
        const int pair=i%2,terminal=i/2;
        auto *wire=new Conductor(elements[pair*2]->terminals()[terminal],elements[pair*2+1]->terminals()[terminal]);
        page->addItem(wire);
        ConductorProperties properties;
        properties.text=QString::number(101+i);properties.m_function="Control";
        properties.m_wire_section="2.5";properties.color=QColor("#102030");properties.cond_size=1.7;
        properties.style=Qt::SolidLine;wire->setProperties(properties);
        wires<<wire;
    }
    auto initial=scanProject17(&project);
    check(initial.scannedSheets==1&&initial.scannedWires==4,"PE production scan sees real scene conductors");
    check(initial.targets.isEmpty(),"PE control wires are not classified by colour or style");
    auto marked=wires[0]->properties();marked.m_function="PE";wires[0]->setProperties(marked);
    auto pen=wires[1]->properties();pen.m_function="PEN";pen.text="PEN";wires[1]->setProperties(pen);
    auto strong=scanProject17(&project);
    check(strong.targets.size()==1&&strong.targets.first().wire==wires[0]&&strong.targets.first().confirmed,"PE stored exact function checked automatically; PEN excluded");
    const auto controlBefore=wires[3]->properties();
    auto draft=controlBefore;draft.m_function="PE";
    auto pending=scanProject17(&project,wires[3],&draft);
    check(pending.targets.size()==2,"PE draft Function identifies source without closing dialog");
    check(wires[3]->properties()==controlBefore,"PE draft detection does not modify stored source metadata");
    auto *label=new IndependentTextItem("PE");page->addItem(label);
    const QPointF midpoint=wires[2]->mapToScene(wires[2]->path().pointAtPercent(0.5));
    label->setPos(midpoint.x()-label->boundingRect().width()/2,midpoint.y()-label->boundingRect().height()-2);
    auto visual=scanProject17(&project);
    bool foundVisual=false;
    for(const auto &target:visual.targets)if(target.wire==wires[2])foundVisual=!target.confirmed&&target.reason.contains("confirm");
    check(foundVisual,"PE independent nearby exact PE text offers unchecked review candidate");
    label->setPlainText("PEN");
    check(scanProject17(&project).targets.size()==1,"PE nearby PEN text never creates a PE candidate");
    label->setPlainText("PE");
    const auto visualBefore=wires[2]->properties(),strongBefore=wires[0]->properties();
    wires[0]->textItem()->forceMovedByUser(true);
    wires[0]->textItem()->setPos(QPointF(150,440));
    wires[0]->textItem()->setRotation(25);
    const auto originalTextPosition=wires[0]->textItem()->pos();
    const int undoBefore=project.undoStack()->count();
    QString callbackError;
    auto runDialog=[&](bool apply){
        bool outerSeen=false,reviewSeen=false;
        QTimer::singleShot(0,[&](){
            auto *dialog=qobject_cast<ConductorPropertiesDialog *>(QApplication::activeModalWidget());
            if(!dialog){callbackError="PE outer properties dialog not shown";return;}
            outerSeen=true;
            auto *function=dialog->findChild<QLineEdit *>("m_function_le");
            auto *style=dialog->findChild<QComboBox *>("m_line_style_cb");
            if(!function||!style){callbackError="PE source property controls missing";dialog->reject();return;}
            function->setText("PE");
            for(int i=0;i<style->count();++i)if(style->itemData(i).value<QPen>().style()==Qt::DashDotLine)style->setCurrentIndex(i);
            dialog->findChild<QGroupBox *>("m_custom_dash_gb")->setChecked(true);
            dialog->findChild<QDoubleSpinBox *>("m_pattern_dash_sb")->setValue(9);
            QTimer::singleShot(0,[&,dialog](){
                auto *review=qobject_cast<QDialog *>(QApplication::activeModalWidget());
                if(!review||review==dialog){callbackError="PE review dialog not shown";dialog->reject();return;}
                reviewSeen=true;
                auto *list=review->findChild<QTreeWidget *>("projectPePatternTargets");
                int checked=0,unchecked=0;
                if(list)for(int i=0;i<list->topLevelItemCount();++i)
                    list->topLevelItem(i)->checkState(0)==Qt::Checked?++checked:++unchecked;
                if(!list||checked!=2||unchecked!=1){callbackError="PE review strong/draft/visual selection defaults wrong";review->reject();return;}
                if(apply){review->grab().save(out+"/pe-project-review.png");review->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Apply)->click();}
                else review->reject();
            });
            dialog->findChild<QPushButton *>("applyProjectPePattern")->click();
            if(!apply||!callbackError.isEmpty())dialog->reject();
        });
        ConductorPropertiesDialog::PropertiesDialog(wires[3]);
        check(callbackError.isEmpty(),qPrintable(callbackError));
        check(outerSeen&&reviewSeen,"PE actual nested properties/review dialogs exercised");
    };
    runDialog(false);
    check(project.undoStack()->count()==undoBefore&&wires[3]->properties()==controlBefore&&wires[0]->properties()==strongBefore,"PE cancel does not commit draft metadata, patterns or undo entries");
    runDialog(true);
    check(project.undoStack()->count()==undoBefore+1,"PE accepted project-wide pattern has one Undo");
    auto expectedPattern=wires[3]->properties();
    check(expectedPattern.style==Qt::DashDotLine&&expectedPattern.m_dash_pattern.enabled&&qFuzzyCompare(expectedPattern.m_dash_pattern.dash,9.0),"PE current unsaved appearance is applied");
    check(wires[3]->properties()==withPattern(controlBefore,expectedPattern),"PE only source pattern saved; pending classification not committed as unrelated metadata");
    check(wires[0]->properties()==withPattern(strongBefore,expectedPattern),"PE confirmed target preserves every non-pattern property");
    check(wires[0]->textItem()->pos()==originalTextPosition&&wires[0]->textItem()->rotation()==25,"PE pattern operation preserves manually placed label position and rotation");
    check(wires[2]->properties()==visualBefore&&wires[1]->properties()==pen,"PE unchecked visual candidate and PEN remain unchanged");
    project.undoStack()->undo();
    check(wires[3]->properties()==controlBefore&&wires[0]->properties()==strongBefore,"PE one Undo restores every accepted target");
    project.undoStack()->redo();
    check(wires[0]->properties()==withPattern(strongBefore,expectedPattern),"PE one Redo restores accepted target pattern");
    check(wires[0]->textItem()->pos()==originalTextPosition&&wires[0]->textItem()->rotation()==25,"PE Undo/Redo retains manual label geometry");
    project.setFilePath(temporary.filePath("pe-roundtrip.qetz"));
    check(project.write().isOk(),"PE production QETZ save succeeds");
    {
        const QString reopenPath=temporary.filePath("pe-reopened.qetz");
        check(QFile::copy(project.filePath(),reopenPath),"PE copy saved QETZ for independent reopen");
        QETProject reopened(reopenPath);QETApp::registerProject(&reopened);
        check(reopened.state()==QETProject::Ok,"PE production QETZ reopens");
        auto scan=scanProject17(&reopened);
        check(scan.scannedWires==4&&scan.targets.size()>=2,"PE QETZ scan retains metadata and visual candidates");
        QETApp::unregisterProject(&reopened);
    }
    QETApp::unregisterProject(&project);
    if(!realProject.isEmpty()){
        QETProject actual(realProject);QETApp::registerProject(&actual);
        check(actual.state()==QETProject::Ok,"PE actual RS NGX QETZ opens");
        const auto actualScan=scanProject17(&actual);
        check(actualScan.scannedSheets>=12&&actualScan.scannedWires>200,"PE actual QETZ scan covers all folios and conductors");
        check(!actualScan.targets.isEmpty(),"PE actual metadata-empty visually-labelled QETZ no longer yields zero candidates");
        bool allRequireConfirmation=true;
        for(const auto &target:actualScan.targets)allRequireConfirmation&=!target.confirmed;
        check(allRequireConfirmation,"PE actual visual-only project candidates require explicit confirmation");
        QFile report(out+"/pe-real-project.txt");
        if(report.open(QIODevice::WriteOnly|QIODevice::Text)){
            QTextStream stream(&report);stream<<"Scanned "<<actualScan.scannedWires<<" conductors on "<<actualScan.scannedSheets<<" sheets.\n";
            for(const auto &target:actualScan.targets)stream<<target.wire->uuid().toString()<<" | sheet "<<target.wire->diagram()->folioIndex()+1<<" | "<<target.reason<<" | checked="<<target.confirmed<<'\n';
        }
        QETApp::unregisterProject(&actual);
    }
}
#endif
