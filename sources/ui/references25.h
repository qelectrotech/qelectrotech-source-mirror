// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef QET_REFERENCES25_H
#define QET_REFERENCES25_H
#include <QtWidgets>
class Diagram; class QETProject; class DiagramTextItem;
namespace QetReferences25 {
struct Link { QRectF source; Diagram *target=nullptr; QRectF destination; };
QList<Link> links(Diagram *);
bool follow(Diagram *,const QPointF &);
void show(Diagram *,QWidget *);
void settings(QWidget *);
bool assign(DiagramTextItem *,Diagram *,QGraphicsItem *object=nullptr);
void editSelected(Diagram *,QWidget *);
void importPdf(Diagram *,QWidget *,const QString &fileName=QString());
}
#endif

