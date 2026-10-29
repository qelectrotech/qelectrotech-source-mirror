// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef RS18_ALIGN_TESTS_H
#define RS18_ALIGN_TESTS_H
#include "ui/textalignment18.h"
#include "qetapp.h"
#include "qetresult.h"
#include "qetgraphicsitem/conductor.h"
#include <QTemporaryDir>

template<class Check>
void runAlign18Tests(Check check,const QString &out,const QString &sourceDir,const QString &)
{
    using namespace QetTextAlignment18;
    const auto near=[](qreal a,qreal b){return qAbs(a-b)<0.001;};
    QETProject project;QETApp::registerProject(&project);auto *page=project.addNewDiagram();
    auto *a=new IndependentTextItem("Top reference\nsecond line");auto *b=new IndependentTextItem("Other description");auto *c=new IndependentTextItem("Locked text");
    page->addItem(a);page->addItem(b);page->addItem(c);
    a->setPos(105.3,55.7);b->setPos(305.2,165.1);c->setPos(410.4,210.6);
    a->setFont(QFont("DejaVu Sans",10));b->setFont(QFont("DejaVu Sans",15));c->setFlag(QGraphicsItem::ItemIsMovable,false);
    a->setSelected(true);b->setSelected(true);c->setSelected(true);
    check(selectedTexts(page).size()==3,"text alignment gathers explicitly selected sheet texts");
    const auto oldA=a->pos(),oldB=b->pos(),oldC=c->pos();int undoCount=page->undoStack().count();
    auto result=alignPositions(page,Right,ChosenObject,a);
    check(result.applied&&result.changed==1&&result.locked==1,"text alignment moves target and skips locked text");
    check(a->pos()==oldA&&c->pos()==oldC,"chosen reference and locked text stay stationary");
    check(near(geometry(a).right(),geometry(b).right()),"right alignment uses exact visible text edges without grid rounding");
    check(near(b->pos().y(),oldB.y()),"horizontal alignment does not change vertical position");
    check(page->undoStack().count()==undoCount+1,"multiple text alignment records one Undo entry");
    page->undoStack().undo();check(a->pos()==oldA&&b->pos()==oldB&&c->pos()==oldC,"text position Undo restores every original coordinate");
    page->undoStack().redo();check(near(geometry(a).right(),geometry(b).right()),"text position Redo restores exact alignment");
    page->undoStack().undo();
    result=alignPositions(page,HCenter,TopmostObject);
    check(result.applied&&a->pos()==oldA&&near(geometry(a).center().x(),geometry(b).center().x()),"topmost reference remains fixed when centring texts horizontally");
    page->undoStack().undo();
    c->setSelected(false);result=alignPositions(page,Left,BottommostObject);
    check(result.applied&&b->pos()==oldB&&near(geometry(a).left(),geometry(b).left()),"bottommost reference remains fixed for left alignment");
    page->undoStack().undo();
    result=alignPositions(page,Top,ChosenObject,a);
    check(result.applied&&near(geometry(a).top(),geometry(b).top())&&b->pos().x()==oldB.x(),"top alignment moves only vertical coordinates");
    page->undoStack().undo();
    const QRectF beforeBounds=geometry(a).united(geometry(b));result=alignPositions(page,HCenter,SelectionBounds);
    check(result.applied&&near(geometry(a).center().x(),beforeBounds.center().x())&&near(geometry(b).center().x(),beforeBounds.center().x()),"selection bounds centre is an explicit reference mode");
    page->undoStack().undo();
    a->setTextWidth(180);b->setTextWidth(200);const QString plainA=a->toPlainText(),htmlA=a->toHtml();
    const auto blockPosition=a->pos();result=alignParagraphs(page,Qt::AlignRight,{a,b,c});
    check(result.applied&&result.changed==2&&result.locked==1,"paragraph alignment formats selected texts and skips locked ones");
    check(a->toPlainText()==plainA&&a->pos()==blockPosition,"paragraph alignment preserves wording and block origin");
    bool right=true;for(auto block=a->document()->begin();block.isValid();block=block.next())right&=bool(block.blockFormat().alignment()&Qt::AlignRight);
    check(right,"paragraph alignment affects every line/paragraph in a multiline sheet text");
    check(a->textWidth()==180&&b->textWidth()==200,"paragraph alignment preserves explicit wrapping widths");
    page->undoStack().undo();check(a->toHtml()==htmlA&&a->pos()==blockPosition,"paragraph Undo restores source HTML and block position");
    page->undoStack().redo();
    QDomDocument doc;auto xml=a->toXml(doc);IndependentTextItem restored;restored.fromXml(xml);
    check(restored.toPlainText()==plainA&&bool(restored.document()->begin().blockFormat().alignment()&Qt::AlignRight),"sheet-text line alignment survives XML save/reopen");
    page->undoStack().undo();
    project.setReadOnly(true);result=alignPositions(page,Right,ChosenObject,a);
    check(!result.applied&&result.message.contains("editable")&&a->pos()==oldA&&b->pos()==oldB,"read-only projects reject text alignment without mutation");project.setReadOnly(false);

    // Existing source-language overrides must follow the new source alignment,
    // while translated wording/positions remain untouched by this source tool.
    const QString id=a->languageId16();
    const QVariantMap de{{"text","Oberer Bezug"},{"x",450.0},{"y",70.0},{"alignment",int(Qt::AlignLeft)}};
    QVariantMap en{{"alignment",int(Qt::AlignLeft)},{"x",a->pos().x()},{"y",a->pos().y()}};
    const QVariantMap record{{"source",plainA},{"enabled",true},{"baseX",a->pos().x()},{"baseY",a->pos().y()},{"variants",QVariantMap{{"en",en},{"de",de}}}};
    QVariantMap data{{"sourceLanguage","en"},{"activeLanguage","en"},{"languages",QStringList{"en","de"}},{"entries",QVariantMap{{id,record}}}};
    project.setLanguageData(data);result=alignParagraphs(page,Qt::AlignHCenter,{a});
    check(result.applied&&a->languageVariant16().value("alignment").toInt()==int(Qt::AlignHCenter),"source-language alignment override is updated with source paragraph formatting");
    check(project.languageData().value("entries").toMap().value(id).toMap().value("variants").toMap().value("de").toMap()==de,"paragraph alignment does not modify another language variant");
    page->undoStack().undo();check(project.languageData()==data,"paragraph Undo restores source language metadata too");
    a->setSelected(true);b->setSelected(true);result=alignPositions(page,Right,ChosenObject,b,{a});
    auto current=project.languageData().value("entries").toMap().value(id).toMap();
    check(result.applied&&near(current.value("baseX").toDouble(),a->pos().x()),"source position alignment updates existing base coordinates");
    check(near(current.value("variants").toMap().value("en").toMap().value("x").toDouble(),a->pos().x()),"source position alignment updates existing source-language coordinates");
    page->undoStack().undo();check(project.languageData()==data,"position Undo restores source language coordinates");
    auto translated=data;translated["activeLanguage"]="de";project.setLanguageData(translated);result=alignPositions(page,Left,ChosenObject,b,{a});
    check(!result.applied&&result.message.contains("source language")&&project.languageData()==translated,"translated drawing alignment is rejected with source-language guidance");project.setLanguageData({});

    // Exercise a real symbol text under rotation/mirroring without moving the
    // symbol or any conductor geometry.
    QTemporaryDir temp;const QString fixture=temp.filePath("alignment-symbols.qet");
    check(QFile::copy(sourceDir+"/tests/qttest/fixtures/ten_sequential_numbers.qet",fixture),"text alignment copies real embedded-symbol fixture");
    QETProject symbols(fixture);QETApp::registerProject(&symbols);check(symbols.state()==QETProject::Ok,"text alignment opens real symbol project");
    auto *symbolPage=symbols.diagrams().first();auto elements=symbolPage->elements();check(!elements.isEmpty(),"text alignment fixture has real symbols");auto *element=elements.first();
    auto *dynamic=new DynamicElementTextItem(element);element->addDynamicTextItem(dynamic);dynamic->setText("Symbol description\nsecond line");dynamic->setTextWidth(140);dynamic->setKeepVisualRotation(false);element->setRotation(37);element->setHorizontalMirror(true);dynamic->setRotation(23);dynamic->setPos(-160,-90);
    auto *reference=new IndependentTextItem("Sheet reference");symbolPage->addItem(reference);reference->setPos(320,210);dynamic->setSelected(true);reference->setSelected(true);
    const QPointF ownerPosition=element->pos();const qreal ownerRotation=element->rotation();const auto localBefore=dynamic->pos();QHash<Conductor*,QPainterPath> paths;for(auto *wire:symbolPage->conductors())paths[wire]=wire->path();
    result=alignPositions(symbolPage,Right,ChosenObject,reference,{dynamic});
    check(result.applied&&near(geometry(dynamic).right(),geometry(reference).right()),"rotated/mirrored-parent symbol text aligns in scene coordinates");
    check(element->pos()==ownerPosition&&element->rotation()==ownerRotation,"text alignment never moves or rotates owning symbol");
    bool wiresUnchanged=true;for(auto *wire:paths.keys())wiresUnchanged&=wire->path()==paths[wire];check(wiresUnchanged,"symbol-text alignment leaves conductor geometry unchanged");
    symbolPage->undoStack().undo();check(dynamic->pos()==localBefore,"symbol-text Undo restores parent-relative coordinates");
    const auto dynamicPlain=dynamic->toPlainText(),dynamicSource=dynamic->text();const auto paragraphOrigin=dynamic->pos();result=alignParagraphs(symbolPage,Qt::AlignRight,{dynamic});
    check(result.applied&&bool(dynamic->alignment()&Qt::AlignRight),"symbol text supports line alignment independently of block position");
    check(dynamic->toPlainText()==dynamicPlain&&dynamic->text()==dynamicSource&&dynamic->pos()==paragraphOrigin,"symbol line alignment preserves source wording and local origin");
    QDomDocument dynamicDoc;auto dynamicXml=dynamic->toXml(dynamicDoc);check(dynamicXml.attribute("Halignment")=="AlignRight","symbol paragraph alignment is serialized through existing alignment attributes");
    symbolPage->undoStack().undo();check(dynamic->pos()==paragraphOrigin&&!(dynamic->alignment()&Qt::AlignRight),"symbol paragraph Undo restores previous alignment");
    element->setMovable(false);result=alignPositions(symbolPage,Left,ChosenObject,reference,{dynamic});check(!result.applied&&result.locked==1,"locked owning symbol protects its text from alignment");element->setMovable(true);
    result=alignPositions(symbolPage,Top,ChosenObject,element,{reference});
    check(result.applied&&near(geometry(element).top(),geometry(reference).top())&&element->pos()==ownerPosition,"a selected symbol may be a fixed reference without being moved");symbolPage->undoStack().undo();
    dynamic->setSelected(true);reference->setSelected(true);bool dialogSeen=false;
    QTimer::singleShot(0,[&](){auto *dialog=qobject_cast<QDialog*>(QApplication::activeModalWidget());if(!dialog)return;
        auto *mode=dialog->findChild<QComboBox*>("textAlignment18Mode"),*ref=dialog->findChild<QComboBox*>("textAlignment18Reference");auto *tree=dialog->findChild<QTreeWidget*>("textAlignment18Texts");
        auto *positionButton=dialog->findChild<QPushButton*>("textAlignment18Positions"),*paragraphButton=dialog->findChild<QPushButton*>("textAlignment18ParagraphApply");
        dialogSeen=mode&&ref&&tree&&tree->topLevelItemCount()>=2&&positionButton&&paragraphButton;dialog->grab().save(out+"/text-alignment-dialog.png");dialog->reject();
    });show(symbolPage);
    check(dialogSeen,"text alignment dialog exposes reference, position and paragraph controls");
    QString error;check(symbols.writeContainerFile(out+"/Text_Alignment_1.8.qetz",&error),qPrintable(error));
    QETApp::unregisterProject(&symbols);QETApp::unregisterProject(&project);
}
#endif
