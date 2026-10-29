// SPDX-License-Identifier: GPL-2.0-or-later
#include "labelclearance24.h"
#include "languageplacement16.h"
#include "languagepositions21.h"
#include "../qetproject.h"
#include "../diagram.h"
#include "../qetgraphicsitem/dynamicelementtextitem.h"
#include "../qetgraphicsitem/independenttextitem.h"
#include "../qetgraphicsitem/conductortextitem.h"
#include "../qetgraphicsitem/elementtextitemgroup.h"
#include <algorithm>
#include "../qetgraphicsitem/terminal.h"
#include "../qetgraphicsitem/qetshapeitem.h"
#include <cmath>

namespace QetLabelClearance24 {
static constexpr auto propertyKey = "labelClearance24";
static constexpr auto defaultKey = "diagrameditor/labelClearance24";
static constexpr auto busyKey = "labelClearanceBusy24";
QVariantMap Settings::toMap() const {
    return {{"version",1},{"distance",distance},{"maxMove",maxMove},{"automatic",automatic},
        {"labels",labels},{"terminals",terminals},{"descriptions",descriptions},{"pins",pins},
        {"keepSide",keepSide},{"keepStacks",keepStacks},{"exact",exact},{"direction",direction},{"wires",wires},{"graphicLines",graphicLines},{"symbolBodies",symbolBodies},{"pinObstacles",pinObstacles},{"graphicDistance",graphicDistance},{"bodyDistance",bodyDistance},{"pinDistance",pinDistance}};
}
Settings Settings::fromMap(const QVariantMap &v) {
    Settings s;
    auto number=[&](const char *key,qreal fallback,qreal min,qreal max){bool ok=false;auto n=v.value(key,fallback).toDouble(&ok);return ok&&std::isfinite(n)?qBound(min,n,max):fallback;};
    s.distance=number("distance",4,0,100);s.maxMove=number("maxMove",30,1,300);
    s.automatic=v.value("automatic",false).toBool();s.labels=v.value("labels",true).toBool();
    s.terminals=v.value("terminals",true).toBool();s.descriptions=v.value("descriptions",true).toBool();
    s.pins=v.value("pins",true).toBool();s.keepSide=v.value("keepSide",true).toBool();s.keepStacks=v.value("keepStacks",true).toBool();
    s.exact=v.value("exact",false).toBool();s.direction=qBound(0,v.value("direction",0).toInt(),2);s.wires=v.value("wires",true).toBool();s.graphicLines=v.value("graphicLines",false).toBool();s.symbolBodies=v.value("symbolBodies",false).toBool();s.pinObstacles=v.value("pinObstacles",false).toBool();s.graphicDistance=number("graphicDistance",4,0,100);s.bodyDistance=number("bodyDistance",4,0,100);s.pinDistance=number("pinDistance",4,0,100);return s;
}
Settings settings(QETProject *p) {return Settings::fromMap(p?p->property(propertyKey).toMap():QVariantMap());}
static Element *owner(QGraphicsItem *item) {
    for(auto *p=item?item->parentItem():nullptr;p;p=p->parentItem())if(auto *e=dynamic_cast<Element*>(p))return e;return nullptr;
}
static bool selected(QGraphicsItem *item) {for(auto *p=item;p;p=p->parentItem())if(p->isSelected())return true;return false;}
enum Kind { Label, TerminalLabel, Description, Pin };
static Kind kind(DiagramTextItem *t) {
    if(auto *e=owner(t))if(e->linkType()==Element::Terminale)return TerminalLabel;
    const auto value=t->toPlainText().trimmed();
    if(auto *d=dynamic_cast<DynamicElementTextItem*>(t))if(d->textFrom()==DynamicElementTextItem::ElementInfo&&d->infoName()=="label")return Label;
    static const QRegularExpression pin("^(?:[0-9]+(?:[.:/-][0-9]+)*|[A-Z][0-9]+|[UVW][12]|PE|N)$");
    if(pin.match(value).hasMatch())return Pin;
    if(value.startsWith('-')||value.startsWith('='))return Label;
    return Description;
}
static bool eligible(DiagramTextItem *t,const Settings &s) {
    if(!t || dynamic_cast<ConductorTextItem*>(t) || (!dynamic_cast<DynamicElementTextItem*>(t)&&!dynamic_cast<IndependentTextItem*>(t)))return false;
    if(!t->isVisible()||t->effectiveOpacity()<=0||t->languageHidden21()||t->toPlainText().trimmed().isEmpty())return false;
    auto k=kind(t);return k==Label?s.labels:k==TerminalLabel?s.terminals:k==Pin?s.pins:s.descriptions;
}
QPainterPath textPath(DiagramTextItem *t) {
    auto shape=QetPlacement16::footprint(t,true);
    for(auto *child:t->childItems())if(auto *text=dynamic_cast<QGraphicsTextItem*>(child))if(text->isVisible())shape=shape.united(QetPlacement16::footprint(text,true));
    return shape;
}
QPainterPath wirePath(QGraphicsItem *item,qreal clearance) {
    auto *wire=dynamic_cast<Conductor*>(item);if(!wire)return {};
    QPainterPathStroker stroke;stroke.setWidth(qMax<qreal>(.01,wire->properties().cond_size)+clearance*2);
    stroke.setCapStyle(Qt::SquareCap);stroke.setJoinStyle(Qt::RoundJoin);
    return wire->mapToScene(stroke.createStroke(wire->path()));
}
static bool locked(DiagramTextItem *text,const QVariantMap &data) {
    auto id=QetLanguages16::effectiveId21(data,text->languageId16());
    return QetLanguages16::variant(data,id).value("locked").toBool() || text->property("clearanceLocked24").toBool();
}
static QString context(DiagramTextItem *t) {
    auto *e=owner(t);return e?e->elementInformations().value("label").toString()+": "+t->displayedText16():t->displayedText16();
}

static bool pinText26(DiagramTextItem *t){static QRegularExpression re("^(?:[0-9]+(?:[.:/-][0-9]+)*|[A-Z][0-9]+|[UVW][12]|PE|N)$");return owner(t)&&re.match(t->toPlainText().trimmed()).hasMatch();}
static Terminal *pinFor26(DiagramTextItem *t){auto *e=owner(t);if(!e||!pinText26(t))return nullptr;Terminal *best=nullptr;qreal score=1e30;auto center=textPath(t).boundingRect().center();for(auto *pin:e->terminals()){auto n=QLineF(center,pin->scenePos()).length();if(n<score){score=n;best=pin;}}return best;}
static int pinAxis26(DiagramTextItem *t){if(auto *pin=pinFor26(t)){auto direction=pin->orientation();return direction==Qet::North||direction==Qet::South?1:2;}return 0;}
static bool samePinRow26(DiagramTextItem *a,DiagramTextItem *b){
 auto *pa=pinFor26(a),*pb=pinFor26(b);if(!pa||!pb||pa==pb||pinAxis26(a)!=pinAxis26(b))return false;
 auto x=textPath(a).boundingRect(),y=textPath(b).boundingRect();if(qAbs(a->sceneTransform().m11()-b->sceneTransform().m11())>.001||qAbs(a->sceneTransform().m12()-b->sceneTransform().m12())>.001)return false;
 if(QLineF(pa->scenePos(),pb->scenePos()).length()>100)return false;
 auto da=x.center()-pa->scenePos(),db=y.center()-pb->scenePos();
 return pinAxis26(a)==1?(qAbs(pa->scenePos().y()-pb->scenePos().y())<2&&qAbs(da.x()-db.x())<2&&qAbs(da.y()-db.y())<2):(qAbs(pa->scenePos().x()-pb->scenePos().x())<2&&qAbs(da.y()-db.y())<2&&qAbs(da.x()-db.x())<2);
}
struct Obstacle { QGraphicsItem *item; QPainterPath shape; QRectF box; bool wire; };
struct Group { QList<DiagramTextItem*> texts; QPainterPath shape; Element *anchor=nullptr; };
// Keep actual Qt text groups and close, aligned word stacks together. We never
// infer a stack between different symbols or between descriptions and pin IDs.
static bool sameStack(DiagramTextItem *a,DiagramTextItem *b) {
    if(a->parentItem()!=b->parentItem()||!owner(a)||kind(a)!=kind(b))return false;
    if(dynamic_cast<ElementTextItemGroup*>(a->parentItem()))return true;
    if(kind(a)!=Description||!qFuzzyCompare(a->rotation()+1,b->rotation()+1))return false;
    const auto x=textPath(a).boundingRect(),y=textPath(b).boundingRect();
    const auto verticalGap=qMax(x.top(),y.top())-qMin(x.bottom(),y.bottom());
    if(verticalGap < -1 || verticalGap>qMax(x.height(),y.height())*.7+2)return false;
    return qAbs(x.left()-y.left())<3.5||qAbs(x.right()-y.right())<3.5||qAbs(x.center().x()-y.center().x())<3.5;
}
Plan plan(const QList<Diagram *> &pages,const Settings &s,bool selectedOnly,const QHash<Diagram*,QRectF> &regions) {
    Plan result;QElapsedTimer timer;timer.start();
    for(auto *page:pages) {
        if(!page||page->isReadOnly()||!page->project())continue;
        const auto data=page->project()->languageData();
        QList<DiagramTextItem*> texts;QList<Element*> elements;QList<Obstacle> obstacles;
        const auto region=regions.value(page);const bool local=regions.contains(page);
        // One scene traversal per affected folio, not one per candidate position.
        for(auto *item:page->items()) {
            if(!item->isVisible()||item->effectiveOpacity()<=0)continue;
            if(auto *t=dynamic_cast<DiagramTextItem*>(item)) {
                if(eligible(t,s))texts<<t;
                else if((!selectedOnly||selected(t))&&!t->toPlainText().trimmed().isEmpty())result.issues<<Issue{t,page,context(t)+": "+QObject::tr("Excluded: wire text, hidden text or unchecked text category.")};
                auto p=textPath(t);if(!p.isEmpty())obstacles<<Obstacle{t,p,p.boundingRect(),false};
            } else if(auto *plain=dynamic_cast<QGraphicsTextItem*>(item)) {
                auto p=QetPlacement16::footprint(plain,true);if(!p.isEmpty())obstacles<<Obstacle{plain,p,p.boundingRect(),false};
            } else if(auto *w=dynamic_cast<Conductor*>(item)) {
                auto p=wirePath(w,s.wires?s.distance:0);obstacles<<Obstacle{w,p,p.boundingRect(),s.wires};
            } else if(auto *pin=dynamic_cast<Terminal*>(item)) {
                if(s.pinObstacles){QPainterPath p;p.addEllipse(pin->scenePos(),s.pinDistance+1,s.pinDistance+1);obstacles<<Obstacle{pin,p,p.boundingRect(),true};}
            } else if(auto *shape=dynamic_cast<QetShapeItem*>(item)) {
                QPainterPathStroker pen;pen.setWidth(qMax(.01,shape->pen().widthF()));auto geometry=shape->geometryPath26();auto strokeShape=pen.createStroke(geometry);if(shape->brush().style()!=Qt::NoBrush)strokeShape=strokeShape.united(geometry);auto p=shape->mapToScene(strokeShape);if(s.graphicLines&&s.graphicDistance>0){QPainterPathStroker stroke;stroke.setWidth(2*s.graphicDistance);p=p.united(stroke.createStroke(p));}obstacles<<Obstacle{shape,p,p.boundingRect(),s.graphicLines};
            } else if(auto *e=dynamic_cast<Element*>(item)) {
                elements<<e;QPainterPath p;p.addRect(e->drawingRect());p=e->mapToScene(p);if(s.symbolBodies&&s.bodyDistance>0){QPainterPathStroker stroke;stroke.setWidth(2*s.bodyDistance);p=p.united(stroke.createStroke(p));}obstacles<<Obstacle{e,p,p.boundingRect(),s.symbolBodies};
            }
        }
        std::sort(texts.begin(),texts.end(),[](auto *a,auto *b){return a->languageId16()<b->languageId16();});
        QSet<DiagramTextItem*> used;
        for(auto *seed:texts) {
            if(used.contains(seed)||(selectedOnly&&!selected(seed)))continue;
            Group g;g.texts<<seed;used.insert(seed);g.anchor=owner(seed);
            if(s.keepStacks)for(int index=0;index<g.texts.size();++index)for(auto *other:texts)if(!used.contains(other)&&(!selectedOnly||selected(other))&&(sameStack(g.texts[index],other)||samePinRow26(g.texts[index],other))){used.insert(other);g.texts<<other;}
            bool anySelected=false,anyLocal=false,isLocked=false;
            for(auto *t:g.texts){auto p=textPath(t);g.shape=g.shape.united(p);anySelected|=selected(t);anyLocal|=p.boundingRect().intersects(region);isLocked|=locked(t,data);}
            if((selectedOnly&&!anySelected)||(local&&!anyLocal))continue;
            if(g.shape.isEmpty())continue;
            result.considered+=g.texts.size();
            QList<Obstacle*> nearby;
            auto range=g.shape.boundingRect().adjusted(-s.maxMove-s.distance,-s.maxMove-s.distance,s.maxMove+s.distance,s.maxMove+s.distance);
            for(auto &o:obstacles){bool own=false;for(auto *member:g.texts)if(o.item==member||QetPlacement16::isAncestor(member,o.item)){own=true;break;}if(o.box.intersects(range)&&!own&&(o.item!=g.anchor||s.symbolBodies))nearby<<&o;}
            auto hitsWire=[&](const QPainterPath &p){for(auto *o:nearby)if(o->wire&&p.boundingRect().intersects(o->box)&&p.intersects(o->shape))return true;return false;};
            if(!s.exact&&!hitsWire(g.shape)){result.alreadyClear+=g.texts.size();for(auto *t:g.texts)result.issues<<Issue{t,page,context(t)+": "+QObject::tr("Already clear; position retained.")};continue;}
            if(nearby.isEmpty()){result.alreadyClear+=g.texts.size();continue;}
            auto addIssue=[&](QString message){result.issues<<Issue{seed,page,context(seed)+": "+message};};
            if(isLocked){result.locked+=g.texts.size();addIssue(QObject::tr("Spacing conflict; position is locked."));continue;}
            const auto original=g.shape.boundingRect();const auto body=QetPlacement16::anchorRect(g.anchor);int side=0;
            if(s.keepSide&&g.anchor){if(original.right()<=body.left()+.5)side=1;else if(original.left()>=body.right()-.5)side=2;else if(original.bottom()<=body.top()+.5)side=3;else if(original.top()>=body.bottom()-.5)side=4;}
            QSet<QGraphicsItem*> oldIntersections;
            for(auto *o:nearby)if(!o->wire&&g.shape.intersects(o->shape))oldIntersections.insert(o->item);
            int axis=s.direction?s.direction:pinAxis26(seed);
            Obstacle *nearest=nullptr;qreal nearestDistance=1e30;for(auto *o:nearby)if(o->wire){const auto b=o->box;const auto c=original.center();qreal dx=qMax(qMax(b.left()-c.x(),c.x()-b.right()),0.0),dy=qMax(qMax(b.top()-c.y(),c.y()-b.bottom()),0.0);qreal dist=dx*dx+dy*dy;if(dist<nearestDistance){nearestDistance=dist;nearest=o;}}
            if(!axis&&nearest)axis=nearest->box.height()>nearest->box.width()?1:2;
            if(!axis)axis=1;
            auto valid=[&](const QPointF &delta){
                if((axis==1&&qAbs(delta.y())>.001)||(axis==2&&qAbs(delta.x())>.001))return false;
                for(auto *t:g.texts)if(auto *pin=pinFor26(t)){
                    auto old=textPath(t).boundingRect().center();auto now=old+delta;auto anchor=pin->scenePos();
                    if((old.x()-anchor.x())*(now.x()-anchor.x())<0||(old.y()-anchor.y())*(now.y()-anchor.y())<0)return false;
                    const auto newDist=QLineF(now,anchor).length();for(auto *other:owner(t)->terminals())if(other!=pin&&QLineF(now,other->scenePos()).length()+.01<newDist)return false;
                }
                auto p=g.shape.translated(delta);auto b=p.boundingRect();auto bounds=page->border_and_titleblock.insideBorderRect();if(bounds.isValid()&&!bounds.contains(b))return false;
                if((side==1&&b.right()>body.left()+.5)||(side==2&&b.left()<body.right()-.5)||(side==3&&b.bottom()>body.top()+.5)||(side==4&&b.top()<body.bottom()-.5))return false;
                if(hitsWire(p))return false;
                for(auto *o:nearby)if(!o->wire&&b.intersects(o->box)&&p.intersects(o->shape)&&!oldIntersections.contains(o->item))return false;
                return true;};
            // Axis candidates include exact obstacle edges and a bounded search
            // on that same axis; sorting favours the smallest displacement.
            QList<QPointF> candidates;const qreal epsilon=.08;
            for(auto *o:nearby)if(o->wire){candidates<<QPointF(o->box.left()-original.right()-epsilon,0)<<QPointF(o->box.right()-original.left()+epsilon,0)<<QPointF(0,o->box.top()-original.bottom()-epsilon)<<QPointF(0,o->box.bottom()-original.top()+epsilon);}
            if(s.exact){candidates.clear();if(nearest){auto b=nearest->box;if(axis==1){if(original.center().x()<b.center().x())candidates<<QPointF(b.left()-original.right()-epsilon,0);else candidates<<QPointF(b.right()-original.left()+epsilon,0);}else {if(original.center().y()<b.center().y())candidates<<QPointF(0,b.top()-original.bottom()-epsilon);else candidates<<QPointF(0,b.bottom()-original.top()+epsilon);}}}
            else {const qreal step=qMax<qreal>(.5,s.maxMove/60.0);for(qreal radius=step;radius<=s.maxMove+.001;radius+=step){if(axis==1)candidates<<QPointF(radius,0)<<QPointF(-radius,0);else candidates<<QPointF(0,radius)<<QPointF(0,-radius);}}
            std::stable_sort(candidates.begin(),candidates.end(),[](const auto &a,const auto &b){return QPointF::dotProduct(a,a)<QPointF::dotProduct(b,b);});
            QPointF delta;bool found=false;
            for(const auto &c:candidates){if(QLineF(QPointF(),c).length()>s.maxMove+.001)continue;if(valid(c)){delta=c;found=true;break;}}
            if(!found){result.unresolved+=g.texts.size();addIssue(QObject::tr("Unresolved: no safe position on the allowed axis within %1 du while preserving pin association and aligned groups; kept in place.").arg(s.maxMove));continue;}
            if(QLineF(QPointF(),delta).length()<.01){result.alreadyClear+=g.texts.size();for(auto *t:g.texts)result.issues<<Issue{t,page,context(t)+": "+QObject::tr("Already at requested spacing.")};continue;}
            for(auto *t:g.texts){result.issues<<Issue{t,page,context(t)+": "+QObject::tr("Proposed move: X %1, Y %2 du").arg(delta.x(),0,'f',2).arg(delta.y(),0,'f',2)};result.changes<<Change{t,t->pos(),QetPlacement16::parentPositionForSceneDelta(t,delta)};for(auto &o:obstacles)if(o.item==t){o.shape.translate(delta);o.box=o.shape.boundingRect();}}
        }
        // Fixed text painted into a symbol definition cannot be moved as an
        // independent label. Report it honestly, without changing the shared
        // symbol definition or the electrical terminals.
        if(s.pins&&!local)for(auto *e:elements){if(selectedOnly&&!selected(e))continue;const auto definition=e->location().xml();bool fixed=false;auto graphic=definition.firstChildElement("description");for(auto node=graphic.firstChildElement();!node.isNull();node=node.nextSiblingElement())if(node.tagName()=="text"&&!node.attribute("text").trimmed().isEmpty()){fixed=true;break;}if(fixed){++result.embeddedSymbols;result.issues<<Issue{{},page,(e->elementInformations().value("label").toString().isEmpty()?e->name():e->elementInformations().value("label").toString())+": "+QObject::tr("Contains fixed symbol text; edit the symbol to reposition that text. Movable labels were checked.")};}}
    }
    result.elapsedMs=timer.elapsed();return result;
}

class Busy {
    QETProject *p;bool was;
public:explicit Busy(QETProject *project):p(project),was(p->property(busyKey).toBool()){p->setProperty(busyKey,true);}~Busy(){p->setProperty(busyKey,was);}
};
class Controller : public QObject {
public:
    QETProject *project;QTimer timer;QHash<Diagram*,QRectF> pending;QHash<Diagram*,QMetaObject::Connection> deaths;
    QVector<const QUndoCommand*> history;int lastIndex=0;bool paused=false;QVariantMap lastLanguage;
    explicit Controller(QETProject *p):QObject(p),project(p){setObjectName("labelClearanceController24");timer.setSingleShot(true);timer.setInterval(180);
        auto *stack=p->undoStack();lastIndex=stack->index();for(int i=0;i<stack->count();++i)history<<stack->command(i);
        connect(&timer,&QTimer::timeout,this,[this]{run();});
        connect(stack,&QUndoStack::indexChanged,this,[this,stack](int index){
            // Mutations emitted inside undo/redo must never trigger a new command
            // which would erase the redo branch or fight the user's Undo.
            bool navigation=index<lastIndex||(index>lastIndex&&index<=history.size()&&history.value(index-1)==stack->command(index-1));
            lastIndex=index;history.clear();for(int i=0;i<stack->count();++i)history<<stack->command(i);
            if(navigation){pending.clear();timer.stop();}
        });
        connect(p,&QETProject::languageDataChanged,this,[this]{
            auto current=project->languageData();
            // Keep a cheap shared snapshot while inactive; comparing the entire
            // catalogue is only useful when an automatic pass could be queued.
            if(project->property(busyKey).toBool()||!project->property(propertyKey).toMap().value("automatic").toBool()||project->state()!=QETProject::Ok){lastLanguage=current;return;}
            bool changed=current!=lastLanguage;lastLanguage=current;if(!changed)return;
            for(auto *page:project->builtFolios())queue(page,page->itemsBoundingRect());
        });
    }
    void queue(Diagram *page,QRectF area){if(paused||project->property(busyKey).toBool()||!settings(project).automatic||project->state()!=QETProject::Ok||!page->isLoaded()||page->isReadOnly()||area.isEmpty())return;
        pending[page]=pending.contains(page)?pending[page].united(area):area;
        if(!deaths.contains(page))deaths[page]=connect(page,&QObject::destroyed,this,[this,page]{pending.remove(page);deaths.remove(page);});timer.start();}
    void run(){if(paused||project->property(busyKey).toBool()||pending.isEmpty())return;if(QApplication::mouseButtons()!=Qt::NoButton||QApplication::activeModalWidget()){timer.start();return;}
        timer.stop();auto areas=pending;pending.clear();const auto options=settings(project);if(!options.automatic)return;
        auto pages=areas.keys();auto result=plan(pages,options,false,areas);project->setProperty("labelClearanceLastConsidered24",result.considered);
        project->setProperty("labelClearanceLastUnresolved24",result.unresolved+result.locked);
        if(!result.changes.isEmpty())apply(project,result,options,QObject::tr("Automatically space labels from conductors"));
    }
};
static Controller *controller(QETProject *p) {if(auto *c=p->findChild<QObject*>("labelClearanceController24",Qt::FindDirectChildrenOnly))return static_cast<Controller*>(c);return new Controller(p);}
void initialize(QETProject *p) {p->setProperty(propertyKey,Settings::fromMap(QSettings().value(defaultKey).toMap()).toMap());controller(p);}
void setSettings(QETProject *p,const Settings &s){if(p)p->setProperty(propertyKey,Settings::fromMap(s.toMap()).toMap());}
void notifyArea(Diagram *page,const QRectF &area){if(!page||!page->project())return;auto *p=page->project();if(p->property(busyKey).toBool()||!p->property(propertyKey).toMap().value("automatic").toBool()||p->state()!=QETProject::Ok||!page->isLoaded())return;auto s=settings(p);controller(p)->queue(page,area.adjusted(-s.distance-s.maxMove,-s.distance-s.maxMove,s.distance+s.maxMove,s.distance+s.maxMove));}
void notify(QGraphicsItem *item){if(!item)return;auto *page=qobject_cast<Diagram*>(item->scene());if(!page||!page->project()||!page->project()->property(propertyKey).toMap().value("automatic").toBool()||page->project()->property(busyKey).toBool())return;notifyArea(page,item->sceneBoundingRect());}
void flush(QETProject *p){if(p)controller(p)->run();}

class Command : public QUndoCommand {
    QPointer<QETProject> project;Plan plan;QVariantMap beforeSettings,afterSettings,beforeLanguage,afterLanguage;
    void change(bool after){if(!project)return;Busy busy(project);auto *c=controller(project);c->pending.clear();c->timer.stop();project->setProperty(propertyKey,after?afterSettings:beforeSettings);
        if(beforeLanguage!=afterLanguage)project->setLanguageData(after?afterLanguage:beforeLanguage);
        QSet<ElementTextItemGroup*> groups;for(const auto &move:plan.changes)if(move.text)if(auto *g=dynamic_cast<ElementTextItemGroup*>(move.text->parentItem()))groups.insert(g);
        for(auto *g:groups)g->blockAlignmentUpdate(true);
        for(const auto &move:plan.changes)if(move.text)move.text->setPos(after?move.after:move.before);
        for(auto *g:groups)g->blockAlignmentUpdate(false);
        for(const auto &move:plan.changes)if(move.text)move.text->refreshLanguageSiblings23();
    }
public:
    Command(QETProject *p,const Plan &value,const Settings &s,const QString &caption):project(p),plan(value),beforeSettings(p->property(propertyKey).toMap()),afterSettings(s.toMap()),beforeLanguage(p->languageData()),afterLanguage(beforeLanguage){
        setText(caption.isEmpty()?QObject::tr("Space labels from conductors"):caption);
        auto entries=afterLanguage.value("entries").toMap();const auto lang=QetLanguages16::activeLanguage(afterLanguage);
        for(const auto &move:plan.changes)if(move.text){auto id=move.text->languageId16();auto e=entries.value(id).toMap();if(e.isEmpty())continue;
            if(lang==QetLanguages16::sourceLanguage(afterLanguage)||!e.value("enabled").toBool()){e["baseX"]=move.after.x();e["baseY"]=move.after.y();entries[id]=e;}
            else {id=QetLanguages16::effectiveId21(afterLanguage,id);e=entries.value(id).toMap();auto variants=e.value("variants").toMap();auto v=variants.value(lang).toMap();v["x"]=move.after.x();v["y"]=move.after.y();variants[lang]=v;e["variants"]=variants;entries[id]=e;}
        }
        if(!beforeLanguage.isEmpty())afterLanguage["entries"]=entries;
    }
    void redo()override{change(true);}void undo()override{change(false);}
};
void apply(QETProject *p,const Plan &value,const Settings &s,const QString &caption){if(!p)return;if(value.changes.isEmpty()&&p->property(propertyKey).toMap()==s.toMap())return;p->undoStack()->push(new Command(p,value,s,caption));}

class Dialog : public QDialog {
    Diagram *page;QETProject *project;QComboBox *scope;QDoubleSpinBox *distance,*maxMove;QCheckBox *automatic,*labels,*terminals,*descriptions,*pins,*keepSide,*keepStacks;
    QComboBox *mode26,*axis26;QCheckBox *wires26,*graphics26,*bodies26,*pinObstacles26;QDoubleSpinBox *graphicGap26,*bodyGap26,*pinGap26;
    QLabel *summary;QTreeWidget *issues;Plan preview;QList<QGraphicsItem*> overlays;bool previewValid=false;
    Settings options()const {Settings s;s.distance=distance->value();s.maxMove=maxMove->value();s.automatic=automatic->isChecked();s.labels=labels->isChecked();s.terminals=terminals->isChecked();s.descriptions=descriptions->isChecked();s.pins=pins->isChecked();s.keepSide=keepSide->isChecked();s.keepStacks=keepStacks->isChecked();s.exact=mode26->currentIndex()==1;s.direction=axis26->currentIndex();s.wires=wires26->isChecked();s.graphicLines=graphics26->isChecked();s.symbolBodies=bodies26->isChecked();s.pinObstacles=pinObstacles26->isChecked();s.graphicDistance=graphicGap26->value();s.bodyDistance=bodyGap26->value();s.pinDistance=pinGap26->value();return s;}
    void clear(){qDeleteAll(overlays);overlays.clear();previewValid=false;}
    void load(const Settings &s){distance->setValue(s.distance);maxMove->setValue(s.maxMove);automatic->setChecked(s.automatic);labels->setChecked(s.labels);terminals->setChecked(s.terminals);descriptions->setChecked(s.descriptions);pins->setChecked(s.pins);keepSide->setChecked(s.keepSide);keepStacks->setChecked(s.keepStacks);mode26->setCurrentIndex(s.exact?1:0);axis26->setCurrentIndex(s.direction);wires26->setChecked(s.wires);graphics26->setChecked(s.graphicLines);bodies26->setChecked(s.symbolBodies);pinObstacles26->setChecked(s.pinObstacles);graphicGap26->setValue(s.graphicDistance);bodyGap26->setValue(s.bodyDistance);pinGap26->setValue(s.pinDistance);clear();}
    void inspect(){clear();Busy busy(project);QApplication::setOverrideCursor(Qt::WaitCursor);auto pages=scope->currentData().toInt()==Project?project->diagrams():QList<Diagram*>{page};preview=plan(pages,options(),scope->currentData().toInt()==Selection);QApplication::restoreOverrideCursor();previewValid=true;
        for(const auto &move:preview.changes)if(move.text&&move.text->diagram()==page){auto p=textPath(move.text);QPointF delta=move.after-move.before;if(auto *parent=move.text->parentItem())delta=parent->mapToScene(move.after)-parent->mapToScene(move.before);auto *outline=new QGraphicsPathItem(p.translated(delta));outline->setPen(QPen(QColor(25,150,65),.7,Qt::DashLine));outline->setBrush(QColor(25,150,65,22));outline->setZValue(100000);outline->setAcceptedMouseButtons(Qt::NoButton);page->addItem(outline);overlays<<outline;}
        issues->clear();QTreeWidgetItem *fixedInfo=nullptr;
        for(int i=0;i<preview.issues.size();++i){const auto &issue=preview.issues[i];QTreeWidgetItem *row;
            if(!issue.text){if(!fixedInfo){fixedInfo=new QTreeWidgetItem(issues,{tr("Information"),tr("Fixed text inside %1 symbols — expand for details").arg(preview.embeddedSymbols)});fixedInfo->setData(0,Qt::UserRole,-1);}row=new QTreeWidgetItem(fixedInfo,{issue.sheet?issue.sheet->title():QString(),issue.message});}
            else row=new QTreeWidgetItem(issues,{issue.sheet?issue.sheet->title():QString(),issue.message});
            row->setData(0,Qt::UserRole,i);
        }
        summary->setText(tr("%1 labels checked · %2 to move · %3 already clear · %4 locked · %5 unresolved · %6 symbols with fixed text\nPreview only; %7 ms. Green outlines show proposed positions on this sheet.").arg(preview.considered).arg(preview.changes.size()).arg(preview.alreadyClear).arg(preview.locked).arg(preview.unresolved).arg(preview.embeddedSymbols).arg(preview.elapsedMs));
    }
public:
    Dialog(Diagram *d,QWidget *parent):QDialog(parent),page(d),project(d->project()){
        setObjectName("labelClearance24Dialog");setWindowTitle(tr("Label spacing from conductors"));resize(850,650);controller(project)->paused=true;controller(project)->timer.stop();controller(project)->pending.clear();
        auto *outer26=new QVBoxLayout(this);auto *scroll26=new QScrollArea;scroll26->setWidgetResizable(true);auto *body26=new QWidget;auto *l=new QVBoxLayout(body26);scroll26->setWidget(body26);outer26->addWidget(scroll26);auto *note=new QLabel(tr("Distances use drawing units (normal grid step = 10). Minimum clearance moves conflicts; Exact spacing targets the nearest expanded obstacle edge (0.08 du tolerance), preserving aligned groups. Pin labels stay on their original side and keep their nearest pin. Unsafe moves remain unresolved. Green outlines show proposed positions; the list includes checked and excluded texts. Apply creates one Undo step."));note->setWordWrap(true);l->addWidget(note);
        auto *form=new QFormLayout;scope=new QComboBox;scope->setObjectName("clearanceScope24");scope->addItem(tr("Selected texts or symbols"),Selection);scope->addItem(tr("Current sheet"),Sheet);scope->addItem(tr("Whole project"),Project);scope->setCurrentIndex(d->selectedItems().isEmpty()?1:0);form->addRow(tr("Apply to"),scope);
        distance=new QDoubleSpinBox;distance->setObjectName("clearanceDistance24");distance->setRange(0,100);distance->setDecimals(2);distance->setSuffix(tr(" du"));form->addRow(tr("Minimum text-to-wire distance"),distance);
        maxMove=new QDoubleSpinBox;maxMove->setObjectName("clearanceMaxMove24");maxMove->setRange(1,300);maxMove->setSuffix(tr(" du"));form->addRow(tr("Maximum movement"),maxMove);
        mode26=new QComboBox;mode26->addItems({tr("Minimum clearance"),tr("Exact spacing to nearest obstacle edge")});form->addRow(tr("Spacing mode"),mode26);
        axis26=new QComboBox;axis26->addItems({tr("Automatic: perpendicular to pin / nearest obstacle"),tr("Horizontal only"),tr("Vertical only")});form->addRow(tr("Movement direction"),axis26);
        wires26=new QCheckBox(tr("Conductors"));graphics26=new QCheckBox(tr("Graphic lines / shapes"));bodies26=new QCheckBox(tr("Symbol / terminal-block outlines"));pinObstacles26=new QCheckBox(tr("Pin connection points"));
        auto addGap=[&](QCheckBox *check){auto *spin=new QDoubleSpinBox;spin->setRange(0,100);spin->setSuffix(" du");form->addRow(check,spin);return spin;};form->addRow(wires26);graphicGap26=addGap(graphics26);bodyGap26=addGap(bodies26);pinGap26=addGap(pinObstacles26);l->addLayout(form);
        for(auto *box:{wires26,graphics26,bodies26,pinObstacles26})connect(box,&QCheckBox::toggled,this,[this]{clear();});for(auto *spin:{graphicGap26,bodyGap26,pinGap26})connect(spin,qOverload<double>(&QDoubleSpinBox::valueChanged),this,[this]{clear();});for(auto *combo:{mode26,axis26})connect(combo,qOverload<int>(&QComboBox::currentIndexChanged),this,[this]{clear();});
        auto *types=new QHBoxLayout;labels=new QCheckBox(tr("Contact / device labels"));terminals=new QCheckBox(tr("Terminal labels"));descriptions=new QCheckBox(tr("Descriptions"));pins=new QCheckBox(tr("Movable pin numbers"));for(auto *box:{labels,terminals,descriptions,pins})types->addWidget(box);l->addLayout(types);
        keepSide=new QCheckBox(tr("Keep the current side of the symbol"));keepStacks=new QCheckBox(tr("Keep aligned text stacks and pin rows together"));automatic=new QCheckBox(tr("Keep spacing automatically after edits and language changes"));automatic->setObjectName("clearanceAutomatic24");for(auto *box:{keepSide,keepStacks,automatic})l->addWidget(box);
        auto *autoNote=new QLabel(tr("Automatic correction checks the affected area after editing stops and creates a separate Undo step. Opening a project preserves its saved layout. Locked positions are respected. Fixed text inside a symbol definition is listed for review."));autoNote->setWordWrap(true);l->addWidget(autoNote);
        auto *defaults=new QHBoxLayout;auto *saveDefault=new QPushButton(tr("Use as default for new projects"));auto *loadDefault=new QPushButton(tr("Load default"));saveDefault->setObjectName("clearanceSaveDefault24");defaults->addWidget(saveDefault);defaults->addWidget(loadDefault);defaults->addStretch();l->addLayout(defaults);
        summary=new QLabel;summary->setWordWrap(true);summary->setMinimumHeight(summary->fontMetrics().height()*3);summary->setSizePolicy(QSizePolicy::Preferred,QSizePolicy::Minimum);l->addWidget(summary);issues=new QTreeWidget;issues->setHeaderLabels({tr("Sheet"),tr("Checked / excluded text and result (double-click to locate)")});issues->header()->setSectionResizeMode(1,QHeaderView::Stretch);l->addWidget(issues,1);
        auto *buttons=new QDialogButtonBox(QDialogButtonBox::Apply|QDialogButtonBox::Close);auto *check=buttons->addButton(tr("Preview / check spacing"),QDialogButtonBox::ActionRole);check->setObjectName("clearancePreview24");outer26->addWidget(buttons);
        load(settings(project));summary->setText(tr("Preview to see how many labels need correction. Defaults affect new projects; Apply saves these settings with this project."));
        connect(check,&QPushButton::clicked,this,[this]{inspect();});
        connect(buttons->button(QDialogButtonBox::Apply),&QPushButton::clicked,this,[this]{if(!previewValid)inspect();clear();apply(project,preview,options());summary->setText(tr("Applied: %1 labels moved. %2 locked or unresolved labels remain. One Undo reverses this operation.").arg(preview.changes.size()).arg(preview.locked+preview.unresolved));});
        connect(buttons,&QDialogButtonBox::rejected,this,&QDialog::reject);
        connect(saveDefault,&QPushButton::clicked,this,[this]{QSettings().setValue(defaultKey,options().toMap());summary->setText(tr("Default saved for new projects. Apply to use these settings in this project."));});connect(loadDefault,&QPushButton::clicked,this,[this]{load(Settings::fromMap(QSettings().value(defaultKey).toMap()));});
        for(auto *box:{labels,terminals,descriptions,pins,keepSide,keepStacks,automatic})connect(box,&QCheckBox::toggled,this,[this]{clear();});for(auto *spin:{distance,maxMove})connect(spin,qOverload<double>(&QDoubleSpinBox::valueChanged),this,[this]{clear();});connect(scope,qOverload<int>(&QComboBox::currentIndexChanged),this,[this]{clear();});
        connect(issues,&QTreeWidget::itemDoubleClicked,this,[this](QTreeWidgetItem *row,int){int i=row->data(0,Qt::UserRole).toInt();if(i<0||i>=preview.issues.size())return;const auto &issue=preview.issues[i];if(issue.text&&issue.sheet==page){page->clearSelection();issue.text->setSelected(true);for(auto *view:page->views())view->centerOn(issue.text);}});
    }
    ~Dialog()override{clear();controller(project)->paused=false;}
};
void show(Diagram *d,QWidget *parent){if(!d||!d->project()||d->isReadOnly())return;Dialog dialog(d,parent);dialog.exec();}
}
