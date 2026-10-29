// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef QET_PRINTDRAWING_H
#define QET_PRINTDRAWING_H
#include <QGraphicsScene>
#include <QPageLayout>
#include <QPainter>
#include <QPicture>

namespace QetPrint {
// Drawing coordinates are logical pixels. Keep this independent of both the
// monitor scale and the printer's device resolution (96 logical pixels/inch).
constexpr qreal drawingDpi = 96.0;
class DrawingPicture : public QPicture {
protected:
 int metric(PaintDeviceMetric m) const override {
  if (m == PdmDpiX || m == PdmDpiY || m == PdmPhysicalDpiX || m == PdmPhysicalDpiY)
   return int(drawingDpi);
  return QPicture::metric(m);
 }
};
inline QSizeF pageSizeInDrawingUnits(const QPageLayout &layout, bool fullPage) {
 const QRectF mm = fullPage ? layout.fullRect(QPageLayout::Millimeter)
                           : layout.paintRect(QPageLayout::Millimeter);
 return mm.size() * (drawingDpi / 25.4);
}
inline void render(QGraphicsScene *scene, QPainter *printerPainter,
                   const QRectF &target, const QRectF &source) {
 if (source.isEmpty() || target.isEmpty()) return;
 // Record vector commands at drawing DPI so point-size text (including border,
 // title-block and table text) keeps its size relative to the circuit. Drawing
 // directly at printer DPI would enlarge such text while leaving lines alone.
 DrawingPicture picture;
 { QPainter recorder(&picture);
   scene->render(&recorder, QRectF(QPointF(), source.size()), source, Qt::IgnoreAspectRatio);
 }
 const qreal scale = qMin(target.width()/source.width(), target.height()/source.height());
 printerPainter->save();
 printerPainter->setClipRect(target, Qt::IntersectClip);
 printerPainter->translate(target.topLeft());
 // drawPicture applies the paint-device DPI ratio itself; cancel that ratio
 // here, then apply the desired scene-to-printer scale. No raster intermediate.
 printerPainter->scale(scale*drawingDpi/printerPainter->device()->logicalDpiX(),
                       scale*drawingDpi/printerPainter->device()->logicalDpiY());
 printerPainter->drawPicture(QPointF(), picture);
 printerPainter->restore();
}
}
#endif
