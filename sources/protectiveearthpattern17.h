/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef PROTECTIVE_EARTH_PATTERN17_H
#define PROTECTIVE_EARTH_PATTERN17_H

#include "protectiveearthpattern.h"
#include "qetproject.h"
#include "diagram.h"
#include "qetgraphicsitem/conductor.h"
#include "qetgraphicsitem/conductortextitem.h"
#include "qetgraphicsitem/diagramtextitem.h"
#include "qetgraphicsitem/element.h"
#include "qetgraphicsitem/terminal.h"
#include <QLineF>
#include <QTextDocument>
#include <limits>
#include <cmath>
#include <algorithm>

namespace ProtectiveEarthPattern {

// A separate visual label is evidence to REVIEW, not electrical metadata.
// Never automatically check these candidates or classify by colour/dash style.
struct Target17 {
    Conductor *wire = nullptr;
    bool confirmed = false;
    QString reason;
};
struct Scan17 {
    QList<Target17> targets;
    int scannedSheets = 0;
    int scannedWires = 0;
};

inline qreal pointSegmentDistance17(const QPointF &point, const QPointF &a,
                                    const QPointF &b)
{
    const QPointF delta = b-a;
    const qreal lengthSquared = QPointF::dotProduct(delta,delta);
    const qreal t = lengthSquared > 0
            ? qBound(qreal(0),QPointF::dotProduct(point-a,delta)/lengthSquared,qreal(1)) : 0;
    return QLineF(point,a+delta*t).length();
}
inline qreal pointRectDistance17(const QPointF &p, const QRectF &r)
{
    return QLineF(p,QPointF(qBound(r.left(),p.x(),r.right()),
                           qBound(r.top(),p.y(),r.bottom()))).length();
}
inline qreal labelPathDistance17(const QRectF &rect, const QPainterPath &path)
{
    qreal best = std::numeric_limits<qreal>::max();
    const QPointF corners[] = {rect.topLeft(),rect.topRight(),rect.bottomRight(),rect.bottomLeft()};
    for (const QPolygonF &polygon : path.toSubpathPolygons()) {
        for (int i=1;i<polygon.size();++i) {
            const QPointF a=polygon[i-1],b=polygon[i];
            if (rect.contains(a)||rect.contains(b)) return 0;
            for (int edge=0;edge<4;++edge)
                if (QLineF(a,b).intersects(QLineF(corners[edge],corners[(edge+1)%4]),nullptr)
                        ==QLineF::BoundedIntersection) return 0;
            best=qMin(best,qMin(pointRectDistance17(a,rect),pointRectDistance17(b,rect)));
            for (const QPointF &corner : corners)
                best=qMin(best,pointSegmentDistance17(corner,a,b));
        }
    }
    return best;
}

inline Scan17 scanProject17(QETProject *project, Conductor *source=nullptr,
                            const ConductorProperties *draft=nullptr)
{
    Scan17 result;
    if (!project) return result;
    // The actual application API builds lazy .qet/.qetz folios here.
    const auto diagrams=project->diagrams();
    QList<Conductor *> ordered;
    QSet<Conductor *> explicitPe,visualPe;
    QHash<Conductor *,QString> directReasons;
    for (Diagram *diagram : diagrams) {
        ++result.scannedSheets;
        const auto wires=diagram->conductors();
        result.scannedWires+=wires.size();
        for (Conductor *wire : wires) {
            ordered << wire;
            QStringList names;
            if (wire->terminal1) names<<wire->terminal1->name();
            if (wire->terminal2) names<<wire->terminal2->name();
            const bool pending=wire==source&&draft&&isExplicitPe(*draft,names);
            if (pending||isExplicitPe(wire->properties(),names)) {
                explicitPe.insert(wire);
                directReasons[wire]=pending&&!isExplicitPe(wire->properties(),names)
                        ? QObject::tr("PE in current source properties (pattern only is saved)")
                        : QObject::tr("PE label, property or terminal");
            }
        }
        for (QGraphicsItem *item : diagram->items()) {
            auto *label=dynamic_cast<DiagramTextItem *>(item);
            if (!label||!label->isVisible()||dynamic_cast<ConductorTextItem *>(label)||!isPeName(label->toPlainText())) continue;
            Element *owner=nullptr;
            for (QGraphicsItem *p=label->parentItem();p;p=p->parentItem())
                if ((owner=dynamic_cast<Element *>(p))) break;
            // QTextDocument's outer margin is not printed ink. Removing it
            // avoids mistaking a close parallel N wire for the PE-labelled one.
            const qreal margin=label->document()->documentMargin();
            QRectF localBounds=label->boundingRect().adjusted(margin,margin,-margin,-margin);
            if(localBounds.isEmpty())localBounds=label->boundingRect();
            const QRectF bounds=label->mapRectToScene(localBounds);
            if (owner) {
                // A PE word somewhere inside a motor does not identify every
                // terminal. Offer only the unique nearby terminal for review.
                Terminal *nearest=nullptr;
                qreal best=std::numeric_limits<qreal>::max(),second=best;
                for (Terminal *terminal : owner->terminals()) {
                    const qreal distance=pointRectDistance17(terminal->dockConductor(),bounds);
                    if (distance<best) {second=best;best=distance;nearest=terminal;}
                    else second=qMin(second,distance);
                }
                if (nearest&&best<=12&&second-best>1)
                    for (Conductor *wire : nearest->conductors()) {
                        visualPe.insert(wire);
                        if (!explicitPe.contains(wire)) directReasons[wire]=QObject::tr("Nearby PE terminal text — confirm");
                    }
            } else {
                Conductor *nearest=nullptr;
                qreal best=std::numeric_limits<qreal>::max(),second=best;
                for (Conductor *wire : wires) {
                    const qreal distance=labelPathDistance17(bounds,wire->mapToScene(wire->path()));
                    if (distance<best) {second=best;best=distance;nearest=wire;}
                    else second=qMin(second,distance);
                }
                // Ambiguous labels between two close wires are not offered.
                if (nearest&&best<=12&&second-best>1) {
                    visualPe.insert(nearest);
                    if (!explicitPe.contains(nearest)) directReasons[nearest]=QObject::tr("Nearby separate PE text — confirm");
                }
            }
        }
    }
    QSet<Conductor *> visited;
    for (Conductor *wire : ordered) {
        if (visited.contains(wire)) continue;
        QSet<Conductor *> network=wire->relatedPotentialConductors();
        network.insert(wire);
        visited.unite(network);
        bool confirmed=false,visual=false;
        for (Conductor *member : network) {
            confirmed|=explicitPe.contains(member);
            visual|=visualPe.contains(member);
        }
        if (!confirmed&&!visual) continue;
        for (Conductor *member : network) {
            if (!member||!member->diagram()||member->diagram()->isReadOnly()) continue;
            QString reason;
            if (explicitPe.contains(member)) reason=directReasons.value(member);
            else if (confirmed) reason=QObject::tr("Connected PE network");
            else if (visualPe.contains(member)) reason=directReasons.value(member);
            else reason=QObject::tr("Connected to nearby PE text — confirm");
            result.targets.append({member,confirmed,reason});
        }
    }
    // Stable sheet/file order, independent of QSet iteration.
    QHash<Conductor *,int> rank;
    for(int i=0;i<ordered.size();++i) rank[ordered[i]]=i;
    std::sort(result.targets.begin(),result.targets.end(),[&](const Target17 &a,const Target17 &b){return rank.value(a.wire)<rank.value(b.wire);});
    return result;
}
}
#endif
