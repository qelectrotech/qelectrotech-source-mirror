// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef QET_LABELCLEARANCE24_H
#define QET_LABELCLEARANCE24_H
#include <QtWidgets>
class QETProject;
class Diagram;
class DiagramTextItem;
namespace QetLabelClearance24 {
struct Settings {
    qreal distance = 4.0;
    qreal maxMove = 30.0;
    bool automatic = false;
    bool labels = true, terminals = true, descriptions = true, pins = true;
    bool keepSide = true, keepStacks = true;
    bool exact = false, wires = true, graphicLines = false, symbolBodies = false, pinObstacles = false;
    int direction = 0; // 0 automatic perpendicular, 1 horizontal, 2 vertical
    qreal graphicDistance=4, bodyDistance=4, pinDistance=4;
    QVariantMap toMap() const;
    static Settings fromMap(const QVariantMap &);
};
enum Scope { Selection, Sheet, Project };
struct Change { QPointer<DiagramTextItem> text; QPointF before, after; };
struct Issue { QPointer<DiagramTextItem> text; QPointer<Diagram> sheet; QString message; };
struct Plan {
    QList<Change> changes;
    QList<Issue> issues;
    int considered = 0, alreadyClear = 0, locked = 0, unresolved = 0, embeddedSymbols = 0;
    qint64 elapsedMs = 0;
};
Settings settings(QETProject *);
void initialize(QETProject *);
void setSettings(QETProject *, const Settings &);
void notify(QGraphicsItem *);
void notifyArea(Diagram *, const QRectF &);
Plan plan(const QList<Diagram *> &, const Settings &, bool selectedOnly = false,
          const QHash<Diagram *, QRectF> &regions = {});
void apply(QETProject *, const Plan &, const Settings &, const QString &caption = {});
void show(Diagram *, QWidget *parent = nullptr);
QPainterPath textPath(DiagramTextItem *);
QPainterPath wirePath(QGraphicsItem *, qreal clearance);
// Explicitly drain the coalesced local work; also used by regression tests.
void flush(QETProject *);
}
#endif
