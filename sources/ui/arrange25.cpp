// SPDX-License-Identifier: GPL-2.0-or-later
#include "arrange25.h"
#include "textalignment18.h"
#include "customcommands19.h"
#include "../qetdiagrameditor.h"
#include "../diagramview.h"
#include "../qetgraphicsitem/qetshapeitem.h"
#include "../QPropertyUndoCommand/qpropertyundocommand.h"

namespace QetArrange25 {
QStringList names(){return {"left","hcenter","right","top","vcenter","bottom","distribute_h","distribute_v","page_h","page_v","text_left","text_center","text_right","match_width","match_height"};}
QStringList actionIds(){QStringList ids;for(auto n:names())ids<<"diagrameditor.arrange_"+n;return ids;}
class Outline : public QGraphicsObject {
    QRectF rectangle;
public:
    Outline(){setObjectName("arrangeReferenceOutline25");setZValue(100000);setAcceptedMouseButtons(Qt::NoButton);}
    QRectF boundingRect()const override{return rectangle.adjusted(-1,-1,1,1);}
    void setRect(const QRectF &r){if(r==rectangle)return;prepareGeometryChange();rectangle=r;update();}
    void paint(QPainter *p,const QStyleOptionGraphicsItem *,QWidget *w) override {if(w){p->setPen(QPen(QColor("#008fd5"),1.5,Qt::DashLine));p->setBrush(Qt::NoBrush);p->drawRect(rectangle);}}
};
class Controller : public QObject {
    QETDiagramEditor *editor;
    std::function<Diagram*()> current;
    QPointer<Diagram> watched;
    QPointer<QGraphicsObject> reference;
    QPointer<Outline> outline;
    bool picking=false;
    QList<QAction*> actions;
    QList<QComboBox*> references,modes;
    int referenceMode=0,moveMode=0;
    Diagram *diagram() const {return current();}
    QList<QGraphicsObject*> targets() const {
        QList<QGraphicsObject*> result;auto *d=diagram();if(!d)return result;
        if(moveMode==0){for(auto *t:QetTextAlignment18::selectedTexts(d))if(!QetTextAlignment18::locked(t))result<<t;}
        else for(auto *i:d->selectedItems()){
            auto *o=dynamic_cast<QGraphicsObject*>(i);if(!o||!(i->flags()&QGraphicsItem::ItemIsMovable))continue;
            if(auto *g=dynamic_cast<QetGraphicsItem*>(i))if(!g->isMovable())continue;
            if(auto *e=dynamic_cast<Element*>(i))if(!e->isMovable())continue;
            bool ancestor=false;for(auto *p=i->parentItem();p;p=p->parentItem())if(p->isSelected()){ancestor=true;break;}
            if(!ancestor)result<<o;
        }
        return result;
    }
    QRectF box(QGraphicsItem *i)const{
        if(auto *t=dynamic_cast<DiagramTextItem*>(i))return t->sceneBoundingRect();
        return QetTextAlignment18::geometry(i);
    }
    bool sizeEligible(QGraphicsObject *i,bool height)const{
        const auto t=i->sceneTransform();if(qAbs(t.m12())>0.0001||qAbs(t.m21())>0.0001)return false;
        if(auto *shape=dynamic_cast<QetShapeItem*>(i))return shape->shapeType()==QetShapeItem::Rectangle||shape->shapeType()==QetShapeItem::Ellipse;
        return !height&&dynamic_cast<DiagramTextItem*>(i)&&i->metaObject()->indexOfProperty("textWidth")>=0;
    }
    QRectF sizeBox(QGraphicsItem *i)const{if(auto *shape=dynamic_cast<QetShapeItem*>(i))return shape->mapRectToScene(shape->rect());return box(i);}
    void matchSize(bool height){
        auto *d=diagram();if(!d||d->isReadOnly())return;auto candidates=targets();QList<QGraphicsObject*> items;
        for(auto *i:candidates)if(sizeEligible(i,height))items<<i;
        if(items.isEmpty()){editor->statusBar()->showMessage(tr("Match width supports unrotated text boxes, rectangles and ellipses. Match height supports rectangles and ellipses; text height remains automatic."),9000);return;}
        for(auto *i:items)if(dynamic_cast<DiagramTextItem*>(i)&&!QetTextAlignment18::sourceEditingAllowed(d)){editor->statusBar()->showMessage(tr("Switch to the source language before resizing text boxes."),7000);return;}
        qreal target=0;
        if(referenceMode==1){if(!reference||reference->scene()!=d){editor->statusBar()->showMessage(tr("Pick a reference object first."),7000);return;}auto b=sizeBox(reference);target=height?b.height():b.width();}
        else if(referenceMode==2){auto b=QetTextAlignment18::pageDrawingArea19(d);target=height?b.height():b.width();}
        else for(auto *i:items){auto b=sizeBox(i);target=qMax(target,height?b.height():b.width());}
        auto *command=new QUndoCommand(height?tr("Match selected heights"):tr("Match selected widths"));auto before=d->project()->languageData(),after=before;auto entries=after.value("entries").toMap();auto source=after.value("sourceLanguage","en").toString();
        for(auto *i:items){if(referenceMode==1&&i==reference)continue;auto t=i->sceneTransform();qreal scale=qAbs(height?t.m22():t.m11());if(scale<0.001)continue;qreal extent=qBound(1.0,target/scale,100000.0);
            if(auto *shape=dynamic_cast<QetShapeItem*>(i)){auto old=shape->rect(),next=old.normalized();if(height)next.setHeight(extent);else next.setWidth(extent);if(next!=old)new QPropertyUndoCommand(shape,"rect",old,next,command);}
            else if(auto *text=dynamic_cast<DiagramTextItem*>(i)){auto old=i->property("textWidth");if(qAbs(old.toDouble()-extent)<0.001)continue;new QPropertyUndoCommand(i,"textWidth",old,extent,command);auto id=text->languageId16();if(entries.contains(id)){auto entry=entries[id].toMap();auto variants=entry.value("variants").toMap();auto variant=variants.value(source).toMap();if(variant.contains("width")){variant["width"]=extent;variants[source]=variant;entry["variants"]=variants;entries[id]=entry;}}}
        }
        if(before.contains("entries"))after["entries"]=entries;if(after!=before)new QPropertyUndoCommand(d->project(),"languageData",before,after,command);
        if(command->childCount())d->undoStack().push(command);else delete command;refresh();
    }
    void refresh(){
        auto *d=diagram();
        if(watched!=d){
            if(watched)watched->removeEventFilter(this);
            // Either scene destruction or scene clearing can delete the outline.
            // Its guarded pointer must expire independently of the watched scene.
            delete outline.data();
            outline=nullptr;watched=d;reference=nullptr;picking=false;
            if(d)d->installEventFilter(this);
        }
        if(d&&!outline){outline=new Outline;d->addItem(outline);}
        bool editable=d&&!d->isReadOnly();auto items=targets();
        for(int i=0;i<actions.size();++i){actions[i]->setEnabled(editable&&(i==6||i==7?items.size()>=3:!items.isEmpty()));actions[i]->setToolTip(actions[i]->text()+(actions[i]->shortcut().isEmpty()?QString():" ("+actions[i]->shortcut().toString(QKeySequence::NativeText)+")"));}
        for(int i=13;i<actions.size();++i){bool supported=false;for(auto *item:items)if(sizeEligible(item,i==14)){supported=true;break;}actions[i]->setEnabled(editable&&supported);}
        if(outline){bool visible=reference&&reference->scene()==d&&referenceMode==1;outline->setVisible(visible);if(visible)outline->setRect(box(reference).adjusted(-3,-3,3,3));}
    }
    void apply(int op){
        auto *d=diagram();if(!d||d->isReadOnly())return;
        if(op>=13){matchSize(op==14);return;}
        if(op>=10){auto result=QetTextAlignment18::alignParagraphs(d,op==10?Qt::AlignLeft:op==11?Qt::AlignHCenter:Qt::AlignRight);editor->statusBar()->showMessage(result.message,7000);return;}
        auto items=targets();if(items.isEmpty())return;
        if(moveMode==0&&!QetTextAlignment18::sourceEditingAllowed(d)){editor->statusBar()->showMessage(tr("Switch to the source language before arranging text."),7000);return;}
        QRectF bounds;for(auto *i:items)bounds=bounds.isNull()?box(i):bounds.united(box(i));
        QRectF anchor=bounds;
        if(op==8||op==9||referenceMode==2)anchor=QetTextAlignment18::pageDrawingArea19(d);
        else if(referenceMode==1){
            if(!reference||reference->scene()!=d){editor->statusBar()->showMessage(tr("Use Pick reference, then click a selected object."),7000);return;}
            anchor=box(reference);
        }
        QList<QPointF> offsets;for(int i=0;i<items.size();++i)offsets<<QPointF();
        if(op==6||op==7){
            bool horizontal=op==6;
            std::sort(items.begin(),items.end(),[&](auto *a,auto *b){return horizontal?box(a).center().x()<box(b).center().x():box(a).center().y()<box(b).center().y();});
            if(items.size()<3)return;
            qreal total=0;for(auto *i:items)total+=horizontal?box(i).width():box(i).height();
            qreal first=horizontal?box(items.first()).left():box(items.first()).top();
            qreal last=horizontal?box(items.last()).right():box(items.last()).bottom();
            if(referenceMode==2){first=horizontal?anchor.left():anchor.top();last=horizontal?anchor.right():anchor.bottom();}
            qreal gap=(last-first-total)/(items.size()-1),position=first;
            for(int n=0;n<items.size();++n){auto b=box(items[n]);offsets[n]=horizontal?QPointF(position-b.left(),0):QPointF(0,position-b.top());position+=(horizontal?b.width():b.height())+gap;}
            if(referenceMode==1&&items.contains(reference)){auto fixed=offsets[items.indexOf(reference)];for(auto &offset:offsets)offset-=fixed;}
        }else for(int n=0;n<items.size();++n){
            if(op<6&&referenceMode==1&&items[n]==reference)continue;
            auto b=box(items[n]);
            switch(op){
                case 0:offsets[n].setX(anchor.left()-b.left());break;
                case 1:offsets[n].setX(anchor.center().x()-b.center().x());break;
                case 2:offsets[n].setX(anchor.right()-b.right());break;
                case 3:offsets[n].setY(anchor.top()-b.top());break;
                case 4:offsets[n].setY(anchor.center().y()-b.center().y());break;
                case 5:offsets[n].setY(anchor.bottom()-b.bottom());break;
                case 8:offsets[n].setX(anchor.center().x()-bounds.center().x());break;
                case 9:offsets[n].setY(anchor.center().y()-bounds.center().y());break;
            }
        }
        if(moveMode==0){
            QList<QetTextAlignment18::State> states;
            for(int n=0;n<items.size();++n)if(!offsets[n].isNull()){auto *t=dynamic_cast<DiagramTextItem*>(items[n]);QetTextAlignment18::State state;state.text=t;state.before=t->pos();state.after=QetTextAlignment18::parentPositionForSceneOffset(t,offsets[n]);states<<state;}
            if(!states.isEmpty())d->undoStack().push(new QetTextAlignment18::Change(d,states,false));
        }else {
            auto *command=new QUndoCommand(tr("Arrange selected objects"));
            for(int n=0;n<items.size();++n)if(!offsets[n].isNull()){
                auto *i=items[n];auto offset=offsets[n];
                if(i->parentItem())offset=i->parentItem()->mapFromScene(offset)-i->parentItem()->mapFromScene(QPointF());
                new QPropertyUndoCommand(i,"pos",i->pos(),i->pos()+offset,command);
            }
            if(command->childCount())d->undoStack().push(command);else delete command;
        }
        refresh();
    }
    QIcon icon(int op){
        QPixmap image(24,24);image.fill(Qt::transparent);QPainter p(&image);p.setRenderHint(QPainter::Antialiasing);p.setPen(QPen(QColor("#176899"),1.5));p.setBrush(QColor("#76b8d4"));
        if(op<6){bool h=op<3;int a=op%3;for(int n=0;n<3;++n){int w=5+n*3,start=a==0?4:a==1?12-w/2:20-w;if(h)p.drawRect(start,3+n*7,w,4);else p.drawRect(3+n*7,start,4,w);}int v=a==0?2:a==1?12:22;if(h)p.drawLine(v,1,v,23);else p.drawLine(1,v,23,v);}
        else if(op<10){p.drawRect(2,2,20,20);p.drawLine(5,12,19,12);p.drawLine(12,5,12,19);if(op%2)p.drawRect(9,5,6,14);else p.drawRect(5,9,14,6);}
        else if(op>=13){p.drawRect(3,4,7,15);p.drawRect(14,4,7,15);if(op==13){p.drawLine(2,2,22,2);p.drawLine(2,22,22,22);}else {p.drawLine(1,2,1,22);p.drawLine(23,2,23,22);}}
        else {for(int n=0;n<4;++n){int w=n%2?10:18;int x=op==10?3:op==11?(24-w)/2:21-w;p.drawLine(x,4+n*5,x+w,4+n*5);}}
        return QIcon(image);
    }
public:
    explicit Controller(QETDiagramEditor *e,std::function<Diagram*()> get):QObject(e),editor(e),current(get){
        setObjectName("arrangeController25");
        QStringList labels{tr("Align left edges"),tr("Align horizontal centres"),tr("Align right edges"),tr("Align top edges"),tr("Align vertical centres"),tr("Align bottom edges"),tr("Equal horizontal gaps"),tr("Equal vertical gaps"),tr("Centre group horizontally on page"),tr("Centre group vertically on page"),tr("Text inside boxes: left"),tr("Text inside boxes: centre"),tr("Text inside boxes: right"),tr("Match width (text boxes / shapes)"),tr("Match height (shapes)")};
        auto ids=actionIds();
        for(int n=0;n<labels.size();++n){auto *a=new QAction(icon(n),labels[n],e);QetCustomCommands19::registerCommand(a,ids[n],tr("Arrange"),icon(n),labels[n],e,"sheet-edit");actions<<a;connect(a,&QAction::triggered,this,[this,n]{apply(n);});}
        auto *toolbar=new QToolBar(tr("Arrange"),e);toolbar->setObjectName("arrange25");e->addToolBarBreak(Qt::TopToolBarArea);e->addToolBar(toolbar);
        auto *controlsAction=toolbar->addWidget(controls(toolbar));controlsAction->setObjectName("arrangeControls25");toolbar->addActions(actions);
        if(auto *dock=e->findChild<QDockWidget*>("diagram_properties_editor_dock_widget")){
            if(auto *layout=dock->findChild<QVBoxLayout*>("m_main_vlayout")){auto *group=new QGroupBox(tr("Arrange"),dock);auto *v=new QVBoxLayout(group);v->addWidget(controls(group));auto *grid=new QGridLayout;for(int n=0;n<actions.size();++n){auto *b=new QToolButton;b->setDefaultAction(actions[n]);grid->addWidget(b,n/6,n%6);}v->addLayout(grid);layout->insertWidget(0,group);}
        }
        auto *timer=new QTimer(this);connect(timer,&QTimer::timeout,this,[this]{refresh();});timer->start(200);
    }
    ~Controller(){delete outline.data();}
    QWidget *controls(QWidget *parent){
        auto *w=new QWidget(parent);auto *layout=new QHBoxLayout(w);layout->setContentsMargins(0,0,0,0);
        auto *ref=new QComboBox;ref->setObjectName("arrangeReference25");ref->setToolTip(tr("For matching sizes, Selection bounds uses the widest or tallest eligible item."));ref->addItems({tr("Selection bounds"),tr("Chosen reference"),tr("Page drawing area")});references<<ref;layout->addWidget(ref);
        auto *pick=new QToolButton;pick->setText(tr("Pick reference"));layout->addWidget(pick);connect(pick,&QToolButton::clicked,this,[this]{picking=true;editor->statusBar()->showMessage(tr("Click one selected object to keep it fixed."));});
        auto *mode=new QComboBox;mode->setObjectName("arrangeMove25");mode->addItems({tr("Texts"),tr("Whole objects")});mode->setToolTip(tr("Shift-click adds text from different symbols to the selection. Ctrl+Shift-click removes a selected text. Texts moves labels while their symbols stay fixed."));modes<<mode;layout->addWidget(mode);
        connect(ref,QOverload<int>::of(&QComboBox::activated),this,[this](int n){referenceMode=n;for(auto *c:references)c->setCurrentIndex(n);refresh();});
        connect(mode,QOverload<int>::of(&QComboBox::activated),this,[this](int n){moveMode=n;for(auto *c:modes)c->setCurrentIndex(n);refresh();});
        return w;
    }
    bool eventFilter(QObject *obj,QEvent *event)override {
        if(obj==watched&&picking&&event->type()==QEvent::GraphicsSceneMousePress){
            auto *e=static_cast<QGraphicsSceneMouseEvent*>(event);if(e->button()!=Qt::LeftButton){picking=false;return true;}
            for(auto *item:watched->items(e->scenePos()))if(item->isSelected())if(auto *o=dynamic_cast<QGraphicsObject*>(item)){reference=o;referenceMode=1;for(auto *c:references)c->setCurrentIndex(1);picking=false;refresh();return true;}
            editor->statusBar()->showMessage(tr("Choose an object that is already selected."),5000);return true;
        }
        return QObject::eventFilter(obj,event);
    }
};
void install(QETDiagramEditor *editor,std::function<Diagram*()> current){new Controller(editor,current);}
}

