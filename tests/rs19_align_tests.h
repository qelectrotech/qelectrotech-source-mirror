// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef RS19_ALIGN_TESTS_H
#define RS19_ALIGN_TESTS_H
#include "ui/textalignment18.h"
#include "qetapp.h"
#include "qetresult.h"
#include "qetgraphicsitem/conductor.h"
#include <QTemporaryDir>

template<class Check>
void runAlign19Tests(Check check,const QString &out,const QString &sourceDir,const QString &realProject)
{
    using namespace QetTextAlignment18;
    const auto near=[](qreal a,qreal b){return qAbs(a-b)<0.001;};
    QTemporaryDir temporary;const QString fixture=temporary.filePath("normal-symbol-selection.qet");
    check(QFile::copy(sourceDir+"/tests/qttest/fixtures/ten_sequential_numbers.qet",fixture),"1.9 alignment prepares embedded-symbol fixture");
    QETProject project(fixture);QETApp::registerProject(&project);check(project.state()==QETProject::Ok,"1.9 alignment fixture opens");
    auto *page=project.diagrams().first();auto elements=page->elements();check(elements.size()>=2,"1.9 alignment has two real symbol owners");
    for(auto *element:elements)for(auto *text:element->dynamicTextItems())text->setVisible(false);
    auto *first=elements[0],*second=elements[1];
    auto make=[&](Element *owner,const QString &word,const QPointF &position){auto *text=new DynamicElementTextItem(owner);owner->addDynamicTextItem(text);text->setText(word);text->setFont(QFont("DejaVu Sans",10));text->setPos(position);text->setVisible(true);return text;};
    auto *company=make(first,"Rolf Schlicht GmbH",{0,-80});
    auto *street=make(first,"Grootkoppel 16",{47,-40});
    auto *city=make(second,"23858 Reinfeld",{100,10});
    auto *blank=make(first,"  ",{300,400});
    auto *hidden=make(second,"Hidden field",{20,20});hidden->setVisible(false);
    first->setPos(150,180);second->setPos(450,310);page->clearSelection();first->setSelected(true);second->setSelected(true);
    auto texts=selectedTexts(page);check(texts.size()==3&&texts.contains(company)&&texts.contains(street)&&texts.contains(city),"normal parent-symbol selection expands visible nonempty text fields");
    check(!texts.contains(blank)&&!texts.contains(hidden),"blank and hidden symbol texts are omitted from alignment list");
    company->setSelected(true);texts=selectedTexts(page);check(texts.size()==3,"selecting a symbol and its text does not duplicate a target");
    auto *group=first->addTextGroup("Address lines");group->blockAlignmentUpdate(true);check(first->addTextToGroup(street,group),"fixture groups a real symbol text");
    texts=selectedTexts(page);check(texts.count(street)==1&&texts.size()==3,"parent selection collects grouped text once");
    page->clearSelection();group->setSelected(true);check(selectedTexts(page).contains(street),"direct text-group selection expands its fields");
    page->clearSelection();first->setSelected(true);second->setSelected(true);

    auto *border=new QetShapeItem({-900,-600},{1800,1300},QetShapeItem::Rectangle);page->addItem(border);border->setSelected(true);
    auto *unchecked=new IndependentTextItem("Unchecked top text");page->addItem(unchecked);unchecked->setPos(-400,-500);unchecked->setSelected(true);
    const QList<DiagramTextItem*> checked{company,street,city};
    const auto companyBefore=company->pos(),streetBefore=street->pos(),cityBefore=city->pos();
    const auto ownerFirst=first->pos(),ownerSecond=second->pos(),borderBefore=border->pos(),uncheckedBefore=unchecked->pos();
    const auto languageBefore=project.languageData();const int undoBefore=page->undoStack().index();
    const auto pure=planPositions19(page,Right,TopmostObject,nullptr,checked);
    check(pure.anchor==company,"topmost automatic reference uses checked texts instead of selected border or unchecked text");
    check(pure.result.changed==2&&!pure.result.applied,"position plan describes two changes without applying");
    check(company->pos()==companyBefore&&street->pos()==streetBefore&&city->pos()==cityBefore&&page->undoStack().index()==undoBefore,"planning never changes source coordinates or Undo stack");
    {
        Preview19 preview;preview.show(page,pure);check(preview.count()>=3,"preview creates visible reference and target overlays");
        check(company->pos()==companyBefore&&street->pos()==streetBefore&&city->pos()==cityBefore&&project.languageData()==languageBefore,"preview leaves real source text and language metadata untouched");
        check(page->undoStack().index()==undoBefore,"preview creates no Undo operation");
        preview.clear();check(preview.count()==0,"cancelling preview removes every temporary overlay");
        check(street->pos()==streetBefore&&city->pos()==cityBefore,"preview cancellation needs no coordinate restoration");
    }
    auto applied=applyPlan19(page,pure);check(applied.applied&&applied.changed==2,"applying the preview moves only checked targets");
    check(near(geometry(company).right(),geometry(street).right())&&near(geometry(company).right(),geometry(city).right()),"right alignment matches the chosen checked reference edge");
    check(company->pos()==companyBefore&&street->pos().y()==streetBefore.y()&&city->pos().y()==cityBefore.y(),"horizontal alignment preserves reference and vertical spacing");
    check(first->pos()==ownerFirst&&second->pos()==ownerSecond&&border->pos()==borderBefore&&unchecked->pos()==uncheckedBefore,"alignment leaves owners, selected border and unchecked text stationary");
    check(page->undoStack().index()==undoBefore+1,"one applied multi-label alignment is one Undo step");
    page->undoStack().undo();check(street->pos()==streetBefore&&city->pos()==cityBefore,"Undo restores child and grouped text coordinates");
    page->undoStack().redo();check(near(geometry(company).right(),geometry(city).right()),"Redo restores the applied reference alignment");page->undoStack().undo();
    auto bounds=planPositions19(page,HCenter,SelectionBounds,nullptr,{company,city});
    check(bounds.referenceRect==geometry(company).united(geometry(city)),"selection bounds ignore selected border and unchecked texts");
    auto fixed=planPositions19(page,Left,ChosenObject,city,{company,street,city});
    check(fixed.anchor==city,"explicit fixed reference can be a checked text");applyPlan19(page,fixed);
    check(city->pos()==cityBefore&&near(geometry(company).left(),geometry(city).left()),"explicit reference text stays fixed while other texts align");page->undoStack().undo();
    auto area=pageDrawingArea19(page);check(area==page->border_and_titleblock.insideBorderRect()&&!area.isEmpty(),"page alignment uses QET drawing area inside the border");
    const auto wirePaths=[&](){QHash<Conductor*,QPainterPath> paths;for(auto *wire:page->conductors())paths[wire]=wire->path();return paths;}();
    auto centered=alignPositions(page,HCenter,PageDrawingArea,nullptr,{company,street,city});
    check(centered.applied&&near(geometry(company).center().x(),area.center().x())&&near(geometry(city).center().x(),area.center().x()),"page centring places each checked label on the drawing-area horizontal centre");
    check(company->pos().y()==companyBefore.y()&&city->pos().y()==cityBefore.y(),"horizontal page centring preserves original vertical positions");page->undoStack().undo();
    centered=alignPositions(page,VCenter,PageDrawingArea,nullptr,{company,city});
    check(centered.applied&&near(geometry(company).center().y(),area.center().y())&&near(geometry(city).center().y(),area.center().y()),"vertical page centring uses drawing-area vertical centre");
    check(company->pos().x()==companyBefore.x()&&city->pos().x()==cityBefore.x(),"vertical page centring preserves horizontal positions");page->undoStack().undo();
    bool unchanged=true;for(auto *wire:wirePaths.keys())unchanged&=wire->path()==wirePaths[wire];check(unchanged,"page alignment never changes conductor geometry");
    first->setMovable(false);const auto lockedPlan=planPositions19(page,Left,ChosenObject,city,{company,street,city});check(lockedPlan.result.locked==2&&lockedPlan.changes.isEmpty(),"locked parent protects all its checked text fields");first->setMovable(true);
    second->setRotation(31);second->setHorizontalMirror(true);city->setKeepVisualRotation(false);city->setRotation(17);
    const auto transformed=city->pos();auto rotated=alignPositions(page,Right,ChosenObject,company,{city});
    check(rotated.applied&&near(geometry(city).right(),geometry(company).right()),"parent-relative movement still handles rotation and mirroring");page->undoStack().undo();check(city->pos()==transformed,"rotated text Undo restores exact local coordinates");
    auto paragraph=planParagraphs19(page,Qt::AlignRight,{company,city});
    const auto companyAlignment=company->alignment();{Preview19 preview;preview.show(page,paragraph);check(company->alignment()==companyAlignment&&company->pos()==companyBefore,"paragraph preview leaves alignment and origin untouched");}
    auto formatted=applyPlan19(page,paragraph);check(formatted.applied&&bool(company->alignment()&Qt::AlignRight),"paragraph formatting has a separate explicit Apply");page->undoStack().undo();
    bool dialogSeen=false,dialogPristine=false;page->clearSelection();first->setSelected(true);second->setSelected(true);border->setSelected(true);
    const int indexBeforeDialog=page->undoStack().index();
    QTimer::singleShot(0,[&](){auto *dialog=qobject_cast<QDialog*>(QApplication::activeModalWidget());if(!dialog)return;
        auto *tree=dialog->findChild<QTreeWidget*>("textAlignment18Texts");auto *ref=dialog->findChild<QComboBox*>("textAlignment18Reference");
        auto *all=dialog->findChild<QPushButton*>("textAlignment19SelectAll"),*none=dialog->findChild<QPushButton*>("textAlignment19SelectNone");
        auto *preview=dialog->findChild<QPushButton*>("textAlignment19Preview");auto *apply=dialog->findChild<QPushButton*>("textAlignment18Positions");
        dialogSeen=tree&&tree->topLevelItemCount()==3&&ref&&ref->findData(PageDrawingArea)>=0&&all&&none&&preview&&apply;
        if(dialogSeen){none->click();check(!apply->isEnabled(),"Select none disables alignment Apply");all->click();ref->setCurrentIndex(ref->findData(PageDrawingArea));preview->click();dialog->grab().save(out+"/text-alignment-1.9.png");}
        dialogPristine=company->pos()==companyBefore&&city->pos()==transformed&&page->undoStack().index()==indexBeforeDialog;dialog->reject();
    });show(page);
    check(dialogSeen,"dialog exposes normal symbol texts, selection controls, page reference, preview and Apply");
    check(dialogPristine&&page->undoStack().index()==indexBeforeDialog,"closing alignment dialog without Apply preserves source and Undo history");
    int overlayCount=0;for(auto *item:page->items())if(item->data(0).toString()=="alignment19-preview")++overlayCount;check(overlayCount==0,"closing dialog removes all preview overlays");
    if(!realProject.isEmpty()) {
        const QString sample=temporary.filePath("provided-cover.qetz");check(QFile::copy(realProject,sample),"cover alignment uses a private copy of supplied project");
        QETProject actual(sample);QETApp::registerProject(&actual);check(actual.state()==QETProject::Ok,"provided cover project opens for regression");
        auto *cover=actual.diagrams().first();cover->clearSelection();QSet<Element*> owners;QStringList expected{"Rolf Schlicht GmbH","Grootkoppel 16","23858 Reinfeld"};
        for(auto *item:cover->items())if(auto *text=dynamic_cast<DynamicElementTextItem*>(item))for(const auto &word:expected)if(text->toPlainText().trimmed()==word)if(auto *e=owner(text))owners.insert(e);
        check(owners.size()>=2,"provided cover contains the address label symbols reported by user");for(auto *e:owners)e->setSelected(true);
        const auto gathered=selectedTexts(cover);int matches=0;for(auto *text:gathered)if(expected.contains(text->toPlainText().trimmed()))++matches;
        check(matches==3,"normal selection gathers all three reported cover address texts");QETApp::unregisterProject(&actual);
    }
    QETApp::unregisterProject(&project);
}
#endif
