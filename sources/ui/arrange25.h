// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef QET_ARRANGE25_H
#define QET_ARRANGE25_H
#include <QtWidgets>
class QETDiagramEditor;
class Diagram;
#include <functional>
namespace QetArrange25 {
void install(QETDiagramEditor *,std::function<Diagram*()>);
QStringList actionIds();
}
#endif

