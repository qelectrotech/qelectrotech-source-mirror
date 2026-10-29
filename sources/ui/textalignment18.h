// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef QET_TEXTALIGNMENT18_H
#define QET_TEXTALIGNMENT18_H
#include <QtWidgets>
#include <QAbstractTextDocumentLayout>
#include <QTextBlock>
#include <QTextLayout>
#include <algorithm>
#include "../diagram.h"
#include "../qetproject.h"
#include "../qetgraphicsitem/diagramtextitem.h"
#include "../qetgraphicsitem/independenttextitem.h"
#include "../qetgraphicsitem/dynamicelementtextitem.h"
#include "../qetgraphicsitem/diagramimageitem.h"
#include "../qetgraphicsitem/qetshapeitem.h"
#include "../qetgraphicsitem/element.h"
#include "../qetgraphicsitem/elementtextitemgroup.h"

namespace QetTextAlignment18 {
enum Mode { Left, HCenter, Right, Top, VCenter, Bottom };
enum Reference { ChosenObject, TopmostObject, BottommostObject, SelectionBounds,
                 LeftmostObject, RightmostObject, PageDrawingArea };
struct Result {
    int considered=0,changed=0,locked=0;
    bool applied=false;
    QString message;
};
inline bool supported(DiagramTextItem *text) {
    return dynamic_cast<IndependentTextItem*>(text)||dynamic_cast<DynamicElementTextItem*>(text);
}
inline Element *owner(DiagramTextItem *text) {
    for(auto *p=text?text->parentItem():nullptr;p;p=p->parentItem())
        if(auto *e=dynamic_cast<Element*>(p))return e;
    return nullptr;
}
inline bool locked(DiagramTextItem *text) {
    if(!text||!(text->flags()&QGraphicsItem::ItemIsMovable)||text->property("lockPosition").toBool())return true;
    if(auto *element=owner(text))if(!element->isMovable())return true;
    return text->languageVariant16().value("locked").toBool();
}
inline QRectF geometry(QGraphicsItem *item) {
    if(auto *text=dynamic_cast<DiagramTextItem*>(item)) {
        QRectF written=text->writtenRect();
        if(const auto *document=text->languageDocument16()) {
            QRectF lines;auto *layout=document->documentLayout();
            for(auto block=document->begin();block.isValid();block=block.next())if(const auto *textLayout=block.layout()) {
                const auto origin=layout->blockBoundingRect(block).topLeft();
                for(int line=0;line<textLayout->lineCount();++line) {
                    const auto rect=textLayout->lineAt(line).naturalTextRect().translated(origin);
                    if(rect.width()>0)lines=lines.united(rect);
                }
            }
            if(!lines.isEmpty())written=lines.adjusted(-1,-1,1,1).intersected(text->boundingRect());
        }
        return text->mapRectToScene(written);
    }
    if(auto *element=dynamic_cast<Element*>(item))return element->mapRectToScene(element->drawingRect());
    if(auto *shape=dynamic_cast<QetShapeItem*>(item))return shape->sceneOutlineRect();
    if(auto *image=dynamic_cast<DiagramImageItem*>(item))return image->mapRectToScene(image->imageRect());
    return item?item->sceneBoundingRect():QRectF();
}
inline QString description(QGraphicsItem *item) {
    QString kind,text;
    if(auto *t=dynamic_cast<DiagramTextItem*>(item)) {
        kind=owner(t)?QObject::tr("Symbol text"):QObject::tr("Sheet text");text=t->toPlainText();
    }else if(auto *e=dynamic_cast<Element*>(item)) {
        kind=QObject::tr("Symbol");text=e->elementInformations().value("label").toString();
    }else if(dynamic_cast<QetShapeItem*>(item))kind=QObject::tr("Shape");
    else if(dynamic_cast<DiagramImageItem*>(item))kind=QObject::tr("Picture");
    text.replace('\n'," / ");if(text.size()>90)text=text.left(87)+"...";
    const auto r=geometry(item);
    return QObject::tr("%1: %2 (%3, %4)").arg(kind,text).arg(r.left(),0,'f',1).arg(r.top(),0,'f',1);
}
// Normal symbol selection is enough: collect the editable text descendants,
// including fields in text groups. Resize handles alone do not distinguish a
// selected field from its selected owner, so users should not need to do so.
inline QList<DiagramTextItem*> selectedTexts(Diagram *diagram) {
    QList<DiagramTextItem*> texts;if(!diagram)return texts;
    QSet<DiagramTextItem*> seen;
    auto add=[&](DiagramTextItem *text){
        if(text&&supported(text)&&text->isVisible()&&!text->toPlainText().trimmed().isEmpty()&&!seen.contains(text)) {
            seen.insert(text);texts<<text;
        }
    };
    for(auto *item:diagram->selectedItems()) {
        if(auto *text=dynamic_cast<DiagramTextItem*>(item))add(text);
        if(auto *element=dynamic_cast<Element*>(item)) {
            for(auto *text:element->dynamicTextItems())add(text);
            for(auto *group:element->textGroups())for(auto *text:group->texts())add(text);
        }else if(auto *group=dynamic_cast<ElementTextItemGroup*>(item))for(auto *text:group->texts())add(text);
    }
    std::sort(texts.begin(),texts.end(),[](auto *a,auto *b){auto x=geometry(a),y=geometry(b);
        if(x.top()!=y.top())return x.top()<y.top();if(x.left()!=y.left())return x.left()<y.left();
        return a->languageId16()<b->languageId16();});
    return texts;
}
// These objects are offered only as explicitly chosen fixed references.
// Automatic extremes/bounds are calculated from checked text targets instead.
inline QList<QGraphicsItem*> referenceObjects(Diagram *diagram) {
    QList<QGraphicsItem*> result;if(!diagram)return result;
    QSet<QGraphicsItem*> seen;
    auto add=[&](QGraphicsItem *item){if(item&&item->isVisible()&&!seen.contains(item)){seen.insert(item);result<<item;}};
    for(auto *text:selectedTexts(diagram))add(text);
    for(auto *item:diagram->selectedItems()) {
        auto *text=dynamic_cast<DiagramTextItem*>(item);
        if((text&&supported(text))||dynamic_cast<Element*>(item)||dynamic_cast<QetShapeItem*>(item)||dynamic_cast<DiagramImageItem*>(item))add(item);
    }
    std::sort(result.begin(),result.end(),[](auto *a,auto *b){auto x=geometry(a),y=geometry(b);
        if(x.top()!=y.top())return x.top()<y.top();if(x.left()!=y.left())return x.left()<y.left();return description(a)<description(b);});
    return result;
}
inline QRectF pageDrawingArea19(Diagram *diagram) {
    // QET defines this rectangle inside the folio border, excluding coordinate
    // headers and the title block (unlike sceneRect, which includes loose items).
    return diagram?diagram->border_and_titleblock.insideBorderRect():QRectF();
}
inline bool sourceEditingAllowed(Diagram *diagram,QString *error=nullptr) {
    if(!diagram||diagram->isReadOnly()) {if(error)*error=QObject::tr("Choose an editable sheet.");return false;}
    const auto data=diagram->project()->languageData();
    if(data.value("activeLanguage",data.value("sourceLanguage","en"))!=data.value("sourceLanguage","en")) {
        if(error)*error=QObject::tr("This tool edits source text. Switch the drawing to its source language before aligning texts.");return false;
    }
    return true;
}
inline QPointF parentPositionForSceneOffset(DiagramTextItem *text,const QPointF &offset) {
    if(auto *parent=text->parentItem())return text->pos()+parent->mapFromScene(offset)-parent->mapFromScene(QPointF());
    return text->pos()+offset;
}
struct State {
    QPointer<DiagramTextItem> text;
    QPointF before,after;
    Qt::Alignment beforeAlignment,afterAlignment;
    QString beforeHtml,afterHtml;
    bool paragraph=false;
    qreal width=-1;
};
// A command owns both the source-text changes and any existing source-language
// layout entries, so switching languages later cannot undo the new source
// position. Other language variants remain untouched.
class Change : public QUndoCommand {
    QPointer<Diagram> diagram;
    QList<State> states;
    QVariantMap beforeCatalog,afterCatalog;
    bool catalogChanged=false;
    void apply(bool after) {
        if(!diagram)return;
        if(catalogChanged)diagram->project()->setLanguageData(after?afterCatalog:beforeCatalog);
        for(const auto &state:states)if(auto *text=state.text.data()) {
            if(state.paragraph) {
                if(auto *free=dynamic_cast<IndependentTextItem*>(text)) {
                    free->setHtml(after?state.afterHtml:state.beforeHtml);
                    free->setTextWidth(state.width);
                }else text->setAlignment(after?state.afterAlignment:state.beforeAlignment);
            }
            text->setPos(after?state.after:state.before);
        }
        diagram->update();
    }
public:
    Change(Diagram *d,const QList<State> &changes,bool paragraph):diagram(d),states(changes),beforeCatalog(d->project()->languageData()),afterCatalog(beforeCatalog) {
        setText(paragraph?QObject::tr("Align lines inside selected texts"):QObject::tr("Align selected text positions"));
        auto entries=afterCatalog.value("entries").toMap();
        const QString source=afterCatalog.value("sourceLanguage","en").toString();
        for(const auto &state:states)if(auto *text=state.text.data()) {
            const auto id=text->languageId16();if(!entries.contains(id))continue;
            auto record=entries.value(id).toMap();auto variants=record.value("variants").toMap();auto variant=variants.value(source).toMap();
            if(paragraph) {
                if(variant.contains("alignment"))variant["alignment"]=int(state.afterAlignment&Qt::AlignHorizontal_Mask);
                if(variant.contains("align"))variant["align"]=int(state.afterAlignment&Qt::AlignHorizontal_Mask);
            }else {
                record["baseX"]=state.after.x();record["baseY"]=state.after.y();
                if(variant.contains("x")||variant.contains("y")){variant["x"]=state.after.x();variant["y"]=state.after.y();}
            }
            if(variants.contains(source))variants[source]=variant;
            record["variants"]=variants;entries[id]=record;
        }
        if(beforeCatalog.contains("entries"))afterCatalog["entries"]=entries;
        catalogChanged=beforeCatalog!=afterCatalog;
    }
    void redo()override{apply(true);}
    void undo()override{if(diagram)diagram->showMe();apply(false);}
};
struct Plan19 {
    Result result;
    QList<State> changes;
    QGraphicsItem *anchor=nullptr;
    QRectF referenceRect;
    bool paragraph=false,valid=false;
};
inline Plan19 planPositions19(Diagram *diagram,Mode mode,Reference reference,QGraphicsItem *chosen=nullptr,QList<DiagramTextItem*> texts={}) {
    Plan19 plan;if(!sourceEditingAllowed(diagram,&plan.result.message))return plan;
    if(texts.isEmpty())texts=selectedTexts(diagram);
    texts.removeIf([diagram](auto *t){return !t||!supported(t)||t->diagram()!=diagram||!t->isVisible()||t->toPlainText().trimmed().isEmpty();});
    // Preserve each target once even when both it and its owner were selected.
    QSet<DiagramTextItem*> seen;QList<DiagramTextItem*> unique;
    for(auto *text:texts)if(!seen.contains(text)){seen.insert(text);unique<<text;}texts=unique;
    plan.result.considered=texts.size();
    if(texts.isEmpty()){plan.result.message=QObject::tr("Select texts or symbols containing visible text first.");return plan;}
    if(reference==ChosenObject) {
        if(!chosen||chosen->scene()!=diagram||!chosen->isVisible()){plan.result.message=QObject::tr("Choose a reference object on this sheet.");return plan;}
        plan.anchor=chosen;plan.referenceRect=geometry(chosen);
    }else if(reference==PageDrawingArea) {
        plan.referenceRect=pageDrawingArea19(diagram);
    }else if(reference==SelectionBounds) {
        for(auto *text:texts)plan.referenceRect=plan.referenceRect.isNull()?geometry(text):plan.referenceRect.united(geometry(text));
    }else {
        // Only the checked target list participates. Unchecked borders, symbols
        // and texts in the sheet selection cannot pull labels to a page edge.
        for(auto *text:texts) {
            if(!plan.anchor){plan.anchor=text;continue;}const auto a=geometry(plan.anchor),b=geometry(text);
            if((reference==TopmostObject&&b.top()<a.top())||(reference==BottommostObject&&b.bottom()>a.bottom())||
               (reference==LeftmostObject&&b.left()<a.left())||(reference==RightmostObject&&b.right()>a.right()))plan.anchor=text;
        }
        plan.referenceRect=geometry(plan.anchor);
    }
    plan.valid=true;
    for(auto *text:texts) {
        if(locked(text)){++plan.result.locked;continue;}
        if(text==plan.anchor)continue;
        const auto box=geometry(text);QPointF offset;
        switch(mode) {
        case Left:offset.setX(plan.referenceRect.left()-box.left());break;
        case HCenter:offset.setX(plan.referenceRect.center().x()-box.center().x());break;
        case Right:offset.setX(plan.referenceRect.right()-box.right());break;
        case Top:offset.setY(plan.referenceRect.top()-box.top());break;
        case VCenter:offset.setY(plan.referenceRect.center().y()-box.center().y());break;
        case Bottom:offset.setY(plan.referenceRect.bottom()-box.bottom());break;
        }
        if(QLineF(QPointF(),offset).length()<1e-7)continue;
        State state;state.text=text;state.before=text->pos();state.after=parentPositionForSceneOffset(text,offset);plan.changes<<state;
    }
    plan.result.changed=plan.changes.size();
    plan.result.message=QObject::tr("%1 texts to move; %2 locked texts skipped.").arg(plan.result.changed).arg(plan.result.locked);
    return plan;
}
inline Plan19 planParagraphs19(Diagram *diagram,Qt::Alignment horizontal,QList<DiagramTextItem*> texts={}) {
    Plan19 plan;plan.paragraph=true;if(!sourceEditingAllowed(diagram,&plan.result.message))return plan;
    horizontal&=Qt::AlignHorizontal_Mask;
    if(!(horizontal&(Qt::AlignLeft|Qt::AlignHCenter|Qt::AlignRight))){plan.result.message=QObject::tr("Choose left, centre or right line alignment.");return plan;}
    if(texts.isEmpty())texts=selectedTexts(diagram);
    QSet<DiagramTextItem*> seen;
    for(auto *text:texts) {
        if(!text||!supported(text)||text->diagram()!=diagram||!text->isVisible()||text->toPlainText().trimmed().isEmpty()||seen.contains(text))continue;
        seen.insert(text);++plan.result.considered;if(locked(text)){++plan.result.locked;continue;}
        State state;state.text=text;state.before=state.after=text->pos();state.paragraph=true;
        state.beforeAlignment=text->alignment();state.afterAlignment=(state.beforeAlignment&Qt::AlignVertical_Mask)|horizontal;
        bool different=false;
        if(auto *free=dynamic_cast<IndependentTextItem*>(text)) {
            state.width=free->textWidth();state.beforeHtml=free->toHtml();QScopedPointer<QTextDocument> document(free->document()->clone());
            for(auto block=document->begin();block.isValid();block=block.next())if((block.blockFormat().alignment()&Qt::AlignHorizontal_Mask)!=horizontal)different=true;
            QTextCursor cursor(document.data());cursor.select(QTextCursor::Document);QTextBlockFormat format;format.setAlignment(horizontal);cursor.mergeBlockFormat(format);state.afterHtml=document->toHtml();
        }else different=(state.beforeAlignment&Qt::AlignHorizontal_Mask)!=horizontal;
        const auto variant=text->languageVariant16();
        if((variant.contains("alignment")&&variant.value("alignment").toInt()!=int(horizontal))||
           (variant.contains("align")&&variant.value("align").toInt()!=int(horizontal)))different=true;
        if(different)plan.changes<<state;
    }
    plan.result.changed=plan.changes.size();
    plan.result.message=QObject::tr("%1 texts to format; %2 locked texts skipped. Block positions stay unchanged.").arg(plan.result.changed).arg(plan.result.locked);
    if(!plan.result.considered)plan.result.message=QObject::tr("Select texts or symbols containing visible text first.");
    plan.valid=plan.result.considered>0;
    return plan;
}
inline Result applyPlan19(Diagram *diagram,const Plan19 &plan) {
    auto result=plan.result;if(!sourceEditingAllowed(diagram,&result.message)||!plan.valid)return result;
    if(!plan.changes.isEmpty()){diagram->undoStack().push(new Change(diagram,plan.changes,plan.paragraph));result.applied=true;}
    result.message=plan.paragraph?QObject::tr("%1 texts formatted; %2 locked texts skipped. Block positions stay unchanged.").arg(result.changed).arg(result.locked):QObject::tr("%1 texts moved; %2 locked texts skipped. The reference object stays fixed.").arg(result.changed).arg(result.locked);
    return result;
}
inline Result alignPositions(Diagram *diagram,Mode mode,Reference reference,QGraphicsItem *chosen=nullptr,QList<DiagramTextItem*> texts={}) {
    const auto plan=planPositions19(diagram,mode,reference,chosen,texts);
    if(!plan.result.considered)return plan.result;
    return applyPlan19(diagram,plan);
}
inline Result alignParagraphs(Diagram *diagram,Qt::Alignment horizontal,QList<DiagramTextItem*> texts={}) {
    const auto plan=planParagraphs19(diagram,horizontal,texts);
    if(!plan.result.considered)return plan.result;
    return applyPlan19(diagram,plan);
}
// Preview objects never own or edit any project object, language metadata or
// Undo stack. Closing/cancelling simply deletes these noninteractive overlays.
class Preview19 {
    QList<QGraphicsItem*> overlays;
public:
    Preview19()=default;
    Preview19(const Preview19&)=delete;
    Preview19 &operator=(const Preview19&)=delete;
    ~Preview19(){clear();}
    void clear(){qDeleteAll(overlays);overlays.clear();}
    int count()const{return overlays.size();}
    void show(Diagram *diagram,const Plan19 &plan) {
        clear();if(!diagram)return;
        if(!plan.paragraph&&!plan.referenceRect.isEmpty()) {
            auto *box=new QGraphicsRectItem(plan.referenceRect);box->setPen(QPen(QColor(30,100,210),1.5,Qt::DashLine));box->setBrush(Qt::NoBrush);
            box->setZValue(100001);box->setAcceptedMouseButtons(Qt::NoButton);box->setData(0,"alignment19-preview");diagram->addItem(box);overlays<<box;
        }
        for(const auto &state:plan.changes)if(auto *text=state.text.data()) {
            auto *ghost=new QGraphicsTextItem;ghost->setFont(text->font());
            const auto *original=text->languageDocument16()?text->languageDocument16():text->document();
            auto *document=original->clone(ghost);ghost->setDocument(document);ghost->setTextWidth(text->textWidth());
            if(state.paragraph) {
                QTextCursor cursor(document);cursor.select(QTextCursor::Document);QTextBlockFormat format;format.setAlignment(state.afterAlignment&Qt::AlignHorizontal_Mask);cursor.mergeBlockFormat(format);
            }
            ghost->setTransform(text->sceneTransform());
            QPointF offset=state.after-state.before;if(auto *parent=text->parentItem())offset=parent->mapToScene(state.after)-parent->mapToScene(state.before);
            ghost->setPos(offset);ghost->setOpacity(0.45);ghost->setZValue(100000);ghost->setAcceptedMouseButtons(Qt::NoButton);ghost->setAcceptHoverEvents(false);ghost->setData(0,"alignment19-preview");diagram->addItem(ghost);overlays<<ghost;
            const auto rect=geometry(text).translated(offset);auto *box=new QGraphicsRectItem(rect);box->setPen(QPen(QColor(30,155,75),1,Qt::DashLine));box->setBrush(QColor(50,190,100,25));
            box->setZValue(100002);box->setAcceptedMouseButtons(Qt::NoButton);box->setData(0,"alignment19-preview");diagram->addItem(box);overlays<<box;
        }
    }
};
class Dialog : public QDialog {
    Diagram *diagram;
    QList<QPointer<DiagramTextItem>> texts;
    QList<QGraphicsItem*> references;
    QTreeWidget *list;
    QComboBox *mode,*reference,*chosen,*paragraph;
    QLabel *status,*fixedReference;
    QPushButton *positionButton,*paragraphButton;
    Preview19 preview;
    bool paragraphPreview=false;
    QList<DiagramTextItem*> checkedTexts() const {
        QList<DiagramTextItem*> result;for(int row=0;row<texts.size();++row)if(texts[row]&&list->topLevelItem(row)->checkState(0)==Qt::Checked)result<<texts[row];return result;
    }
    void refreshRows() {
        for(int row=0;row<texts.size();++row)if(auto *text=texts[row].data()) {const auto r=QetTextAlignment18::geometry(text);list->topLevelItem(row)->setText(3,QString("%1, %2").arg(r.left(),0,'f',1).arg(r.top(),0,'f',1));}
    }
    Plan19 currentPlan(bool lines)const {
        const auto targets=checkedTexts();if(targets.isEmpty()){Plan19 empty;empty.result.message=tr("Check at least one text row.");return empty;}
        if(lines)return planParagraphs19(diagram,Qt::Alignment(paragraph->currentData().toInt()),targets);
        return planPositions19(diagram,Mode(mode->currentData().toInt()),Reference(reference->currentData().toInt()),references.value(chosen->currentIndex()),targets);
    }
    void showPreview(bool lines) {
        paragraphPreview=lines;const auto plan=currentPlan(lines);preview.show(diagram,plan);
        positionButton->setEnabled(currentPlan(false).result.changed>0);paragraphButton->setEnabled(currentPlan(true).result.changed>0);
        status->setText(tr("Preview only. %1 Nothing changes until Apply.").arg(plan.result.message));
        QString label;
        if(lines)label=tr("Paragraph preview: text block positions stay fixed.");
        else if(plan.anchor)label=tr("Fixed reference: %1").arg(description(plan.anchor));
        else if(reference->currentData().toInt()==PageDrawingArea)label=tr("Reference: page drawing area inside border, excluding title block.");
        else label=tr("Reference: bounds of checked texts only.");
        fixedReference->setText(label);
        for(int row=0;row<texts.size();++row){auto *item=list->topLevelItem(row);QFont font=item->font(1);font.setBold(texts[row].data()==plan.anchor);item->setFont(1,font);item->setBackground(1,texts[row].data()==plan.anchor?QBrush(QColor(225,238,255)):QBrush());}
    }
public:
    Dialog(Diagram *d,QWidget *parent=nullptr):QDialog(parent),diagram(d) {
        setObjectName("textAlignment18Dialog");setWindowTitle(tr("Text alignment and reference"));resize(960,720);
        auto *layout=new QVBoxLayout(this);auto *info=new QLabel(tr("Normal selection of texts or their symbols is enough. Check the labels to align, then choose a fixed reference or the page drawing area. Blue outlines mark the reference; green outlines show the preview. Apply creates one Undo step; Close discards the preview."));info->setWordWrap(true);layout->addWidget(info);
        auto *selection=new QHBoxLayout;auto *all=new QPushButton(tr("Select all"));all->setObjectName("textAlignment19SelectAll");auto *none=new QPushButton(tr("Select none"));none->setObjectName("textAlignment19SelectNone");selection->addWidget(all);selection->addWidget(none);selection->addStretch();layout->addLayout(selection);
        list=new QTreeWidget;list->setObjectName("textAlignment18Texts");list->setColumnCount(4);list->setHeaderLabels({tr("Use"),tr("Text"),tr("Kind"),tr("Position")});list->header()->setSectionResizeMode(1,QHeaderView::Stretch);list->setColumnWidth(0,45);list->setColumnWidth(2,145);list->setColumnWidth(3,125);layout->addWidget(list,1);
        for(auto *text:selectedTexts(d)) {
            texts<<text;auto *row=new QTreeWidgetItem(list);row->setFlags(row->flags()|Qt::ItemIsUserCheckable);row->setCheckState(0,locked(text)?Qt::Unchecked:Qt::Checked);
            row->setText(1,text->toPlainText().replace('\n'," / "));row->setToolTip(1,text->toPlainText());row->setText(2,locked(text)?tr("Locked"):owner(text)?tr("Symbol text"):tr("Sheet text"));
        }
        auto *positions=new QGroupBox(tr("Align text block positions"));auto *form=new QFormLayout(positions);
        mode=new QComboBox;mode->setObjectName("textAlignment18Mode");for(auto pair:QList<QPair<QString,int>>{{tr("Left edges"),Left},{tr("Horizontal centres"),HCenter},{tr("Right edges"),Right},{tr("Top edges"),Top},{tr("Vertical centres"),VCenter},{tr("Bottom edges"),Bottom}})mode->addItem(pair.first,pair.second);form->addRow(tr("Alignment"),mode);
        reference=new QComboBox;reference->setObjectName("textAlignment18Reference");for(auto pair:QList<QPair<QString,int>>{{tr("Chosen object"),ChosenObject},{tr("Page drawing area"),PageDrawingArea},{tr("Topmost checked text"),TopmostObject},{tr("Bottommost checked text"),BottommostObject},{tr("Bounds of checked texts"),SelectionBounds},{tr("Leftmost checked text"),LeftmostObject},{tr("Rightmost checked text"),RightmostObject}})reference->addItem(pair.first,pair.second);form->addRow(tr("Reference"),reference);
        chosen=new QComboBox;chosen->setObjectName("textAlignment18Chosen");references=referenceObjects(d);for(auto *item:references)chosen->addItem(description(item));if(!texts.isEmpty())chosen->setCurrentIndex(references.indexOf(texts.first().data()));form->addRow(tr("Chosen object"),chosen);
        auto *useRow=new QPushButton(tr("Use selected text row as reference"));useRow->setObjectName("textAlignment18UseReference");form->addRow(useRow);
        fixedReference=new QLabel;fixedReference->setObjectName("textAlignment19FixedReference");fixedReference->setWordWrap(true);form->addRow(fixedReference);
        connect(useRow,&QPushButton::clicked,this,[this](){int row=list->indexOfTopLevelItem(list->currentItem());if(row<0||row>=texts.size()||!texts[row])return;int index=references.indexOf(texts[row].data());if(index>=0){chosen->setCurrentIndex(index);reference->setCurrentIndex(reference->findData(ChosenObject));showPreview(false);}});
        auto *buttons=new QHBoxLayout;auto *positionPreview=new QPushButton(tr("Preview text alignment"));positionPreview->setObjectName("textAlignment19Preview");positionButton=new QPushButton(tr("Apply text alignment"));positionButton->setObjectName("textAlignment18Positions");buttons->addWidget(positionPreview);buttons->addWidget(positionButton);form->addRow(buttons);layout->addWidget(positions);
        auto *lines=new QGroupBox(tr("Align lines inside each text block"));auto *lineLayout=new QHBoxLayout(lines);paragraph=new QComboBox;paragraph->setObjectName("textAlignment18Paragraph");paragraph->addItem(tr("Left"),int(Qt::AlignLeft));paragraph->addItem(tr("Centre"),int(Qt::AlignHCenter));paragraph->addItem(tr("Right"),int(Qt::AlignRight));lineLayout->addWidget(paragraph);
        auto *linePreview=new QPushButton(tr("Preview paragraphs"));linePreview->setObjectName("textAlignment19ParagraphPreview");lineLayout->addWidget(linePreview);paragraphButton=new QPushButton(tr("Apply paragraph alignment"));paragraphButton->setObjectName("textAlignment18ParagraphApply");lineLayout->addWidget(paragraphButton);layout->addWidget(lines);
        status=new QLabel;status->setObjectName("textAlignment18Status");status->setWordWrap(true);layout->addWidget(status);
        auto apply=[this](bool lines){const auto plan=currentPlan(lines);preview.clear();auto result=applyPlan19(diagram,plan);refreshRows();showPreview(lines);status->setText(result.message);};
        connect(positionButton,&QPushButton::clicked,this,[apply](){apply(false);});connect(paragraphButton,&QPushButton::clicked,this,[apply](){apply(true);});
        connect(positionPreview,&QPushButton::clicked,this,[this](){showPreview(false);});connect(linePreview,&QPushButton::clicked,this,[this](){showPreview(true);});
        connect(mode,QOverload<int>::of(&QComboBox::currentIndexChanged),this,[this](){showPreview(false);});
        connect(reference,QOverload<int>::of(&QComboBox::currentIndexChanged),this,[this](){chosen->setEnabled(reference->currentData().toInt()==ChosenObject);showPreview(false);});
        connect(chosen,QOverload<int>::of(&QComboBox::currentIndexChanged),this,[this](){showPreview(false);});
        connect(paragraph,QOverload<int>::of(&QComboBox::currentIndexChanged),this,[this](){showPreview(true);});
        connect(list,&QTreeWidget::itemChanged,this,[this](QTreeWidgetItem*,int column){if(column==0)showPreview(paragraphPreview);});
        auto select=[this](bool enabled){QSignalBlocker blocker(list);for(int row=0;row<texts.size();++row)if(texts[row])list->topLevelItem(row)->setCheckState(0,enabled&&!locked(texts[row])?Qt::Checked:Qt::Unchecked);showPreview(paragraphPreview);};
        connect(all,&QPushButton::clicked,this,[select](){select(true);});connect(none,&QPushButton::clicked,this,[select](){select(false);});
        auto *close=new QDialogButtonBox(QDialogButtonBox::Close);layout->addWidget(close);connect(close,&QDialogButtonBox::rejected,this,&QDialog::reject);refreshRows();showPreview(false);
    }
};
inline void show(Diagram *diagram,QWidget *parent=nullptr) {if(diagram){Dialog dialog(diagram,parent);dialog.exec();}}
}
#endif
