// SPDX-License-Identifier: GPL-2.0-or-later
// Synthetic raster viewport workload, not a full editor latency benchmark.
#include <QApplication>
#include <QGraphicsView>
#include <QGraphicsRectItem>
#include <QElapsedTimer>
#include <QPainter>
#include <cstdio>
#include <ctime>
#ifdef Q_OS_WIN
#include <windows.h>
#include <psapi.h>
#endif
#include "QetGraphicsItemModeler/qetgraphicshandleritem.h"

static double cpuMilliseconds() {
#ifdef Q_OS_WIN
 FILETIME created,exited,kernel,user;
 if(GetProcessTimes(GetCurrentProcess(),&created,&exited,&kernel,&user)) {
  ULARGE_INTEGER k,u; k.LowPart=kernel.dwLowDateTime;k.HighPart=kernel.dwHighDateTime;
  u.LowPart=user.dwLowDateTime;u.HighPart=user.dwHighDateTime;
  return double(k.QuadPart+u.QuadPart)/10000;
 }
 return 0;
#else
 return double(std::clock())*1000/CLOCKS_PER_SEC;
#endif
}

int main(int argc,char **argv) {
 QApplication app(argc,argv);
 const qreal size=QString::fromLocal8Bit(argv[1]).toDouble();
 const int count=QString::fromLocal8Bit(argv[2]).toInt();
 const QString operation=QString::fromLocal8Bit(argv[3]);
 QGraphicsScene scene; scene.setSceneRect(-2000,-2000,4000,4000);
 QList<QGraphicsRectItem *> objects;
 for(int i=0;i<count/4;++i) {
  auto *object=scene.addRect(0,0,32,24,QPen(Qt::black),QBrush(Qt::lightGray));
  object->setPos((i%32)*36-576,(i/32)*28-224); objects<<object;
  for(auto *handle:QetGraphicsHandlerItem::handlerForPoint({{0,0},{32,0},{32,24},{0,24}},size)) {
   handle->setParentItem(object); handle->setColor(Qt::darkGreen);
  }
 }
 QGraphicsView view(&scene); view.resize(1280,800); view.setSceneRect(scene.sceneRect());
 view.show(); QApplication::processEvents();
 QImage image(1280,800,QImage::Format_ARGB32_Premultiplied);
 auto frame=[&](int i) {
  if(operation=="zoom") {view.resetTransform();view.scale(0.75+0.005*(i%60),0.75+0.005*(i%60));}
  if(operation=="pan") view.centerOn((i%60)*2,0);
  if(operation=="manipulate") for(auto *object:objects) {
   object->setRotation(i%30); object->setRect(0,0,32+(i%8),24); object->moveBy(i%2?1:-1,0);
  }
  image.fill(Qt::white); QPainter painter(&image); view.render(&painter);
 };
 for(int i=0;i<20;++i) frame(i);
 std::puts("size,handles,operation,repeat,ms_per_frame,cpu_ms_per_frame,working_set_bytes");
 for(int repeat=0;repeat<7;++repeat) {
  const auto cpu=cpuMilliseconds(); QElapsedTimer timer; timer.start();
  for(int i=0;i<120;++i) frame(i);
  const double elapsed=timer.nsecsElapsed()/1e6/120;
  const double cpuElapsed=(cpuMilliseconds()-cpu)/120;
  unsigned long long memory=0;
#ifdef Q_OS_WIN
  PROCESS_MEMORY_COUNTERS counters{}; counters.cb=sizeof(counters);
  if(GetProcessMemoryInfo(GetCurrentProcess(),&counters,sizeof(counters))) memory=counters.WorkingSetSize;
#endif
  std::printf("%.2f,%d,%s,%d,%.6f,%.6f,%llu\n",size,count,qPrintable(operation),repeat,elapsed,cpuElapsed,memory);
 }
}
