// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef QET_LANGUAGEPLACEMENT16_H
#define QET_LANGUAGEPLACEMENT16_H
#include <QtWidgets>
#include <cmath>
#include <limits>
#include "textgeometry23.h"
#include "../qetgraphicsitem/element.h"
#include "../qetgraphicsitem/conductor.h"
#include "../qetgraphicsitem/terminal.h"
#include "../qetgraphicsitem/diagramtextitem.h"

namespace QetPlacement16 {
// Distances are scene/drawing units, independent of the monitor's zoom or DPI.
// width is applied by the display renderer before placement, never to the
// source text here. A line limit is a review constraint, not a clipping rule.
struct Rule {
    QString side = QStringLiteral("current");
    qreal gap = 8;
    qreal width = -1;
    int maxLines = 0; // zero means unlimited
    bool automatic = true;
    bool locked = false;
    bool allowOverlap = false;
    QStringList permittedSides;
    qreal clearance = 1.5;
    bool writtenOnly23 = false; // review can exclude blank text-frame margins
};
struct Result {
    QPointF position; // parent coordinates, suitable for text->setPos()
    QStringList issues;
    bool moved = false;
    int collisions = 0;
    bool outsideBounds = false;
    bool permittedOverlap = false;
    bool resolved = true;
    int lineCount = 0;
    int candidatesEvaluated = 0;
};
inline Rule rule(const QVariantMap &v) {
    Rule r;
    r.side = v.value("side", "current").toString();
    r.gap = qBound<qreal>(0, v.value("gap", 8).toDouble(), 500);
    r.width = v.value("width", -1).toDouble();
    r.maxLines = qMax(0, v.value("maxLines", 0).toInt());
    r.automatic = v.value("auto", true).toBool();
    r.locked = v.value("locked", false).toBool();
    r.allowOverlap = v.value("allowOverlap", false).toBool();
    r.permittedSides = v.value("permittedSides").toStringList();
    r.clearance = qBound<qreal>(0, v.value("clearance", 1.5).toDouble(), 100);
    return r;
}
inline QRectF anchorRect(QGraphicsItem *anchor) {
    if (!anchor) return {};
    // Element::drawingRect deliberately excludes its dynamic label children
    // and the extra grid padding in the element's ordinary boundingRect.
    if (auto *element = dynamic_cast<Element *>(anchor))
        return element->mapRectToScene(element->drawingRect());
    return anchor->mapRectToScene(anchor->boundingRect());
}
inline bool isAncestor(const QGraphicsItem *item, const QGraphicsItem *child) {
    for (auto *p = child ? child->parentItem() : nullptr; p; p = p->parentItem())
        if (p == item) return true;
    return false;
}
inline QPainterPath footprint(const QGraphicsTextItem *text, bool writtenOnly = false) {
    if(writtenOnly) {
        const QTextDocument *doc=text->document();QPointF offset;
        if(auto *t=dynamic_cast<const DiagramTextItem*>(text)) {if(t->languageHidden21())return {};if(auto *translated=t->languageDocument16()){doc=translated;offset=t->languageOffset22();}}
        return text->mapToScene(QetTextGeometry23::writtenPath(doc).translated(offset));
    }
    QPainterPath p;
    p.addPolygon(text->mapToScene(text->boundingRect()));
    p.closeSubpath();
    return p;
}
inline QPainterPath obstacleShape(QGraphicsItem *item, qreal clearance, bool writtenOnly = false) {
    QPainterPath local;
    if (auto *element = dynamic_cast<Element *>(item)) {
        local.addRect(element->drawingRect());
    } else if (auto *conductor = dynamic_cast<Conductor *>(item)) {
        // Do not use the conductor's hover-dependent shape or its deliberately
        // generous hit-test bounds: only its actual electrical path matters.
        QPainterPathStroker stroke;
        stroke.setWidth(1.0);
        stroke.setCapStyle(Qt::RoundCap);
        stroke.setJoinStyle(Qt::RoundJoin);
        local = stroke.createStroke(conductor->path());
    } else {
        local = item->shape();
    }
    QPainterPath scene = item->mapToScene(local);
    if(writtenOnly)if(auto *text=dynamic_cast<QGraphicsTextItem*>(item))scene=footprint(text,true);
    if (clearance > 0 && !scene.isEmpty()) {
        QPainterPathStroker stroke;
        stroke.setWidth(clearance * 2);
        stroke.setCapStyle(Qt::RoundCap);
        stroke.setJoinStyle(Qt::RoundJoin);
        scene = scene.united(stroke.createStroke(scene));
    }
    return scene;
}
struct Evaluation {
    int collisions = 0;
    bool outside = false;
    qreal overlapArea = 0;
};
inline Evaluation evaluate(QGraphicsTextItem *text, QGraphicsItem *anchor,
                           const QPainterPath &candidate,
                           const QRectF &sheetBounds, qreal clearance, bool writtenOnly = false) {
    Evaluation e;
    const QRectF box = candidate.boundingRect();
    e.outside = sheetBounds.isValid() && !sheetBounds.isEmpty()
            && !sheetBounds.contains(box);
    if (!text->scene()) return e;
    const auto items = text->scene()->items(box.adjusted(-clearance,-clearance,clearance,clearance),
                                           Qt::IntersectsItemBoundingRect);
    for (auto *item : items) {
        if (!item || item == text || item == anchor || !item->isVisible()
                || item->effectiveOpacity() <= 0 || isAncestor(item, text)
                || isAncestor(text, item)) continue;
        // Groups are containers; their visible children are checked separately.
        // Terminal markers and resize/selection handles are editor aids, not
        // obstacles in the printed circuit. Conductors remain obstacles.
        if (dynamic_cast<QGraphicsItemGroup *>(item)
                || dynamic_cast<Terminal *>(item)
                || (item->flags() & QGraphicsItem::ItemHasNoContents)
                || (item->flags() & QGraphicsItem::ItemIgnoresTransformations)) continue;
        const QPainterPath obstacle = obstacleShape(item, clearance, writtenOnly);
        if (obstacle.isEmpty() || !candidate.intersects(obstacle)) continue;
        ++e.collisions;
        const QRectF overlap = box.intersected(obstacle.boundingRect());
        e.overlapArea += qMax<qreal>(0, overlap.width()) * qMax<qreal>(0, overlap.height());
    }
    return e;
}
inline QPointF parentPositionForSceneDelta(const QGraphicsTextItem *text,
                                           const QPointF &delta) {
    if (auto *parent = text->parentItem()) {
        const QPointF sceneOrigin = text->mapToScene(QPointF());
        return text->pos() + parent->mapFromScene(sceneOrigin + delta)
                          - parent->mapFromScene(sceneOrigin);
    }
    return text->pos() + delta;
}
inline QString normalizedSide(QString side) {
    side = side.toLower();
    if (side == "above") return "top";
    if (side == "below") return "bottom";
    return side;
}
inline QPointF nominalTopLeft(const QRectF &body, const QSizeF &size,
                              const QString &side, qreal gap) {
    if (side == "left") return {body.left()-gap-size.width(), body.center().y()-size.height()/2};
    if (side == "right") return {body.right()+gap, body.center().y()-size.height()/2};
    if (side == "top") return {body.center().x()-size.width()/2, body.top()-gap-size.height()};
    return {body.center().x()-size.width()/2, body.bottom()+gap};
}
inline Result place(QGraphicsTextItem *text, QGraphicsItem *anchor,
                    const Rule &requested, const QRectF &sheetBounds,
                    bool move = true) {
    Result result;
    if (!text) { result.resolved=false; result.issues << QObject::tr("Text item is unavailable."); return result; }
    result.position = text->pos();
    Rule r = requested;
    r.gap = std::isfinite(r.gap) ? qBound<qreal>(0,r.gap,500) : 8;
    r.clearance = std::isfinite(r.clearance) ? qBound<qreal>(0,r.clearance,100) : 1.5;
    const QPainterPath initial = footprint(text,r.writtenOnly23);
    const QRectF initialBox = initial.boundingRect();
    if (initialBox.isEmpty()) return result;
    if (auto *drawingText = dynamic_cast<DiagramTextItem *>(text))
        result.lineCount = drawingText->languageLineCount16();
    else {
        text->document()->documentLayout()->documentSize();
        for (QTextBlock block=text->document()->begin(); block.isValid(); block=block.next())
            if (block.layout()) result.lineCount += qMax(1,block.layout()->lineCount());
    }
    const auto finish = [&](const Evaluation &e, const QPointF &delta) {
        result.position = parentPositionForSceneDelta(text,delta);
        result.collisions = e.collisions;
        result.outsideBounds = e.outside;
        result.permittedOverlap = e.collisions > 0 && r.allowOverlap;
        result.moved = QLineF(QPointF(),delta).length() > 0.01;
        if (e.outside) result.issues << QObject::tr("The text extends outside the usable drawing area.");
        if (e.collisions) result.issues << (r.allowOverlap
            ? QObject::tr("Allowed overlap with %1 drawing item(s).").arg(e.collisions)
            : QObject::tr("Unresolved overlap with %1 drawing item(s).").arg(e.collisions));
        if (r.maxLines > 0 && result.lineCount > r.maxLines)
            result.issues << QObject::tr("The text uses %1 lines; the configured maximum is %2. It has not been clipped.").arg(result.lineCount).arg(r.maxLines);
        result.resolved = !e.outside && (e.collisions == 0 || r.allowOverlap)
                       && (r.maxLines <= 0 || result.lineCount <= r.maxLines);
        if (move && result.moved) text->setPos(result.position);
    };
    const Evaluation current = evaluate(text,anchor,initial,sheetBounds,r.clearance,r.writtenOnly23);
    if (r.locked || !r.automatic || (r.allowOverlap && normalizedSide(r.side)=="current")) {
        finish(current,{});
        return result;
    }
    struct Candidate { QPointF delta; int priority=0; bool fallback=false; };
    QVector<Candidate> candidates;
    const QString preferred = normalizedSide(r.side);
    if (preferred=="current") candidates << Candidate{{},0};
    QRectF body=anchorRect(anchor);
    QStringList sides;
    if (QStringList{"left","right","top","bottom"}.contains(preferred)) sides << preferred;
    for (auto side:r.permittedSides) {
        side=normalizedSide(side);
        if (QStringList{"left","right","top","bottom"}.contains(side) && !sides.contains(side)) sides<<side;
    }
    if (r.permittedSides.isEmpty()) for (const auto &side:QStringList{"left","right","bottom","top"})
        if (!sides.contains(side)) sides<<side;
    const qreal step=qMax<qreal>(4,qMin<qreal>(12,r.gap>0?r.gap:8));
    if (anchor && !body.isEmpty()) {
        for (int rank=0;rank<sides.size();++rank) {
            const QString side=sides[rank];
            for (int outward=0;outward<3;++outward) for (int shift: {0,-1,1,-2,2,-3,3}) {
                QPointF pos=nominalTopLeft(body,initialBox.size(),side,r.gap+outward*step);
                if (side=="left" || side=="right") pos.ry()+=shift*step;
                else pos.rx()+=shift*step;
                candidates << Candidate{pos-initialBox.topLeft(),rank+1};
            }
        }
    } else {
        // Free text has no device anchor. Search only a small neighbourhood of
        // its current position; do not migrate it across the sheet.
        for (int y=-3;y<=3;++y) for (int x=-3;x<=3;++x)
            candidates << Candidate{QPointF(x*step,y*step),1};
    }
    // The unchanged position is also a fallback when every permitted anchored
    // position is obstructed; moving a label should not make its conflicts worse.
    candidates << Candidate{{},int(sides.size())+2,true};
    qreal bestScore=std::numeric_limits<qreal>::max();
    Evaluation best=current;
    QPointF bestDelta;
    qreal bestFreeScore=std::numeric_limits<qreal>::max();
    Evaluation bestFree;QPointF bestFreeDelta;bool freeFound=false;
    for (const auto &candidate:candidates) {
        QPainterPath path=initial.translated(candidate.delta);
        const Evaluation e=evaluate(text,anchor,path,sheetBounds,r.clearance,r.writtenOnly23);
        ++result.candidatesEvaluated;
        const qreal overlapPenalty=r.allowOverlap?0:e.collisions*100000.0+e.overlapArea;
        const qreal score=(e.outside?10000000.0:0)+overlapPenalty
                         + QLineF(QPointF(),candidate.delta).length()
                         + candidate.priority*8;
        if (score<bestScore) {bestScore=score;best=e;bestDelta=candidate.delta;}
        if (!candidate.fallback && !e.outside && (e.collisions==0 || r.allowOverlap) && score<bestFreeScore) {
            freeFound=true;bestFreeScore=score;bestFree=e;bestFreeDelta=candidate.delta;
        }
    }
    if (freeFound) { best=bestFree;bestDelta=bestFreeDelta; }
    finish(best,bestDelta);
    return result;
}
inline Result inspect(QGraphicsTextItem *text,QGraphicsItem *anchor,
                      Rule rule,const QRectF &sheetBounds) {
    rule.automatic=false;
    return place(text,anchor,rule,sheetBounds,false);
}
inline QStringList check(QGraphicsTextItem *text,QGraphicsItem *anchor,
                         const Rule &rule,const QRectF &sheetBounds) {
    return inspect(text,anchor,rule,sheetBounds).issues;
}
}
#endif
